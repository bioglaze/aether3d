#include "Font.hpp"
#include <sstream>
#include "Array.hpp"
#include "FileSystem.hpp"
#include "System.hpp"
#include "Texture2D.hpp"
#include "VertexBuffer.hpp"
#include "Vec3.hpp"

namespace BlockType
{
    enum Enum
    {
        Info = 1,
        Common,
        Pages,
        Chars,
        Kerning
    };
}

struct CharacterBlock
{
    unsigned id;
    unsigned short x;
    unsigned short y;
    unsigned short width;
    unsigned short height;
    short xOffset;
    short yOffset;
    short xAdvance;
    unsigned char page;
    unsigned char channel;
};

static_assert( sizeof( CharacterBlock ) == 20, "" );

struct CommonBlock
{
    unsigned short lineHeight;
    unsigned short base;
    unsigned short scaleW;
    unsigned short scaleH;
    unsigned short pages;
    unsigned char  bitField;
    unsigned char  alpha;
    unsigned char  red;
    unsigned char  green;
    unsigned char  blue;
};

ae3d::Font::Font()
{
    texture = Texture2D::GetDefaultTexture();
    padding[ 0 ] = 1;
    padding[ 1 ] = 2;
    padding[ 2 ] = 3;
    padding[ 3 ] = 4;
    spacing[ 0 ] = 10;
    spacing[ 1 ] = 20;
}

void ae3d::Font::CreateVertexBuffer( const char* text, const Vec4& color, VertexBuffer& outVertexBuffer ) const
{
    const std::string textStr( text );
    
    ae3d::VertexBuffer::VertexPTC* vertices = new ae3d::VertexBuffer::VertexPTC[ textStr.size() * 6 ];
    ae3d::VertexBuffer::Face* faces = new ae3d::VertexBuffer::Face[ textStr.size() * 2 ];

    float accumX = 0;
    float y = 0;
    
    for (unsigned short c = 0; c < static_cast<unsigned short>( textStr.size() ); ++c)
    {
        if (static_cast<int>(text[ c ]) < 0)
        {
            continue;
        }
        
        const Character& ch = chars[ static_cast<int>(text[ c ]) ];

        if (text[ c ] == '\n')
        {
            const Character& charA = chars[ static_cast<int>( 'a' )];
            accumX = 0;
            y += charA.height + charA.yOffset;
        }
        else
        {
            accumX += ch.xAdvance;
        }
        
        const float scale = 1;
        float x = 0;
        const float z = -0.6f;

        float offx = x + ch.xOffset * scale + accumX * scale;
        float offy = y + ch.yOffset * scale;
        
        auto tex = texture;

        float u0 = ch.x / tex->GetWidth();
        float u1 = (ch.x + ch.width) / tex->GetWidth();
        
        float v0 = (ch.y + ch.height) / tex->GetHeight();
        float v1 = (ch.y) / tex->GetHeight();

        // Upper triangle.
        faces[ c * 2 + 0 ].a = c * 6 + 0;
        faces[ c * 2 + 0 ].b = c * 6 + 1;
        faces[ c * 2 + 0 ].c = c * 6 + 2;
        
        vertices[ c * 6 + 0 ] = VertexBuffer::VertexPTC( ae3d::Vec3( offx, offy, z ), u0, v1 );
        vertices[ c * 6 + 1 ] = VertexBuffer::VertexPTC( ae3d::Vec3( offx + ch.width * scale, offy, z ), u1, v1 );
        vertices[ c * 6 + 2 ] = VertexBuffer::VertexPTC( ae3d::Vec3( offx, offy + ch.height * scale, z ), u0, v0 );
        
        // Lower triangle.
        faces[ c * 2 + 1 ].a = c * 6 + 3;
        faces[ c * 2 + 1 ].b = c * 6 + 4;
        faces[ c * 2 + 1 ].c = c * 6 + 5;
        
        vertices[ c * 6 + 3 ] = VertexBuffer::VertexPTC( ae3d::Vec3( offx + ch.width * scale, offy, z ), u1, v1 );
        vertices[ c * 6 + 4 ] = VertexBuffer::VertexPTC( ae3d::Vec3( offx + ch.width * scale, offy + ch.height * scale, z ), u1, v0 );
        vertices[ c * 6 + 5 ] = VertexBuffer::VertexPTC( ae3d::Vec3( offx, offy + ch.height * scale, z ), u0, v0 );
        
        vertices[ c * 6 + 0 ].color = color;
        vertices[ c * 6 + 1 ].color = color;
        vertices[ c * 6 + 2 ].color = color;

        vertices[ c * 6 + 3 ].color = color;
        vertices[ c * 6 + 4 ].color = color;
        vertices[ c * 6 + 5 ].color = color;
    }
    
    outVertexBuffer.Generate( faces, int( textStr.size() * 2 ), vertices, int( textStr.size() * 6 ), VertexBuffer::Storage::GPU );
    outVertexBuffer.SetDebugName( text );

    delete[] vertices;
    delete[] faces;
}

void ae3d::Font::LoadBMFont( Texture2D* fontTex, const FileSystem::FileContentsData& metaData )
{
    if (fontTex != nullptr)
    {
        texture = fontTex;
    }

    std::stringstream metaStream( std::string( std::begin( metaData.data ), std::end( metaData.data ) ) );
    std::string token;
    // Determines the encoding (text or binary).
    metaStream >> token;
    
    if (token == "info")
    {
        LoadBMFontMetaText( metaData );
    }
    else
    {
        LoadBMFontMetaBinary( metaData );
    }
}

void ae3d::Font::LoadBMFontMetaText( const FileSystem::FileContentsData& metaData )
{
    std::stringstream metaStream( std::string( metaData.data.begin(), metaData.data.end() ) );

    std::string line;
    std::getline( metaStream, line );
    
    std::string token;
    
    // First line (info)
    std::stringstream infoStream( line );
    while (!infoStream.eof())
    {
        infoStream >> token;
        
        if (token.find( "padding" ) != std::string::npos)
        {
            const char a[ 2 ] = { token[  8 ], 0 };
            const char b[ 2 ] = { token[ 10 ], 0 };
            const char c[ 2 ] = { token[ 12 ], 0 };
            const char d[ 2 ] = { token[ 14 ], 0 };
            
            padding[ 0 ] = std::atoi( &a[ 0 ] );
            padding[ 1 ] = std::atoi( &b[ 0 ] );
            padding[ 2 ] = std::atoi( &c[ 0 ] );
            padding[ 3 ] = std::atoi( &d[ 0 ] );
        }
        else if (token.find( "spacing" ) != std::string::npos)
        {
            const char a[ 2 ] = { token[  8 ], 0 };
            const char b[ 2 ] = { token[ 10 ], 0 };
            
            spacing[ 0 ] = std::atoi( &a[ 0 ] );
            spacing[ 1 ] = std::atoi( &b[ 0 ] );
        }
    }
    
    std::getline( metaStream, line );
    
    // Second line (common)
    std::stringstream commonStream( line );

    while (!commonStream.eof())
    {
        commonStream >> token;
        
        if (token.find( "lineHeight" ) != std::string::npos)
        {
            const char a[ 3 ] = { token[ 11 ], token[ 12 ], 0 };
            
            lineHeight = std::atoi( a );
        }
        else if (token.find( "base" ) != std::string::npos)
        {
            const char a[ 3 ] = { token[ 5 ], token[ 6 ], 0 };
            
            base = std::atoi( a );
        }
    }
    
    // Third line (page)
    std::getline( metaStream, line );
    // Fourth line (chars)
    std::getline( metaStream, line );
    
    // Character tags.
    while (!metaStream.eof())
    {
        std::getline( metaStream, line );
        std::stringstream charStream( line );
        
        charStream.seekg( line.find( "id=" ) + 3 );
        int id;
        charStream >> id;

        if (id > 255)
        {
            continue;
        }

        charStream.seekg( line.find( "x=" ) + 2 );
        charStream >> chars[ id ].x;
        
        charStream.seekg( line.find( "y=" ) + 2 );
        charStream >> chars[ id ].y;
        
        charStream.seekg( line.find( "width=" ) + 6 );
        charStream >> chars[ id ].width;
        
        charStream.seekg( line.find( "height=" ) + 7 );
        charStream >> chars[ id ].height;
        
        charStream.seekg( line.find( "xoffset=" ) + 8 );
        charStream >> chars[ id ].xOffset;
        
        charStream.seekg( line.find( "yoffset=" ) + 8 );
        charStream >> chars[ id ].yOffset;
        
        charStream.seekg( line.find( "xadvance=" ) + 9 );
        charStream >> chars[ id ].xAdvance;
    }
}

void ae3d::Font::LoadBMFontMetaBinary(const FileSystem::FileContentsData& metaData)
{
    std::stringstream ifs( std::string( metaData.data.begin(), metaData.data.end() ) );
    unsigned char header[ 4 ];
    ifs.read( (char*)&header[ 0 ], 4 );
    const bool validHeaderHead = header[ 0 ] == 66 && header[ 1 ] == 77 && header[ 2 ] == 70;
    
    if (!validHeaderHead)
    {
        System::Print( "%s does not contain a valid BMFont header!\n", metaData.path.c_str() );
        return;
    }
    
    const bool validHeaderTail = header[ 3 ] == 3;
    
    if (!validHeaderTail)
    {
        System::Print( "%s contains a BMFont header but the version is invalid! Valid is version 3.\n", metaData.path.c_str() );
        return;
    }
    
    while (!ifs.eof())
    {
        unsigned char blockId;
        ifs.read( (char*)&blockId, 1 );
        int blockType = (BlockType::Enum)blockId;
        
        int blockSize;
        ifs.read( (char*)&blockSize, 4 );
        
        Array< unsigned char > blockData;
        blockData.Allocate( blockSize );
        
        if (blockType == BlockType::Info)
        {
            ifs.read( (char*)&blockData[ 0 ], blockSize );
            
            padding[ 0 ] = blockData[  7 ];
            padding[ 1 ] = blockData[  8 ];
            padding[ 2 ] = blockData[  9 ];
            padding[ 3 ] = blockData[ 10 ];
            
            spacing[ 0 ] = blockData[ 11 ];
            spacing[ 1 ] = blockData[ 12 ];
        }
        else if (blockType == BlockType::Common)
        {
            CommonBlock block;
            ifs.read( (char*)&block, blockSize );
            
            lineHeight = block.lineHeight;
            base = block.base;
        }
        else if (blockType == BlockType::Pages)
        {
            ifs.read( (char*)&blockData[ 0 ], blockSize );
        }
        else if (blockType == BlockType::Chars)
        {
            Array< CharacterBlock > blocks;
            blocks.Allocate( blockSize / sizeof( CharacterBlock ) );
            
            ifs.read( (char*)&blocks[ 0 ].id, blockSize );
            System::Assert( blocks.GetLength() <= 256, "Too many glyphs." );
            
            for (int c = 0; c < blocks.GetLength(); ++c)
            {
                chars[ blocks[ c ].id ].x = blocks[ c ].x;
                chars[ blocks[ c ].id ].y = blocks[ c ].y;
                chars[ blocks[ c ].id ].width = blocks[ c ].width;
                chars[ blocks[ c ].id ].height = blocks[ c ].height;
                chars[ blocks[ c ].id ].xOffset = blocks[ c ].xOffset;
                chars[ blocks[ c ].id ].yOffset = blocks[ c ].yOffset;
                chars[ blocks[ c ].id ].xAdvance = blocks[ c ].xAdvance;
            }
            
            return;
        }
        else if (blockType == BlockType::Kerning)
        {
            ifs.read( (char*)&blockData[ 0 ], blockSize );
        }
        else
        {
            System::Assert( false, "Fonter: Corrupted binary meta data file or a bug in reader!" );
        }
    }
}

int ae3d::Font::TextWidth( const char* text ) const
{
    if (!text)
    {
        return 0;
    }

    return (int)(std::string( text ).length() * chars[ (int)'a' ].width);
}
