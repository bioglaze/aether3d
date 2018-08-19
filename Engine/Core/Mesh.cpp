#include "Mesh.hpp"
#include <vector>
#include <cstdint>
#include <sstream>
#include <string>
#include "FileSystem.hpp"
#include "FileWatcher.hpp"
#include "Matrix.hpp"
#include "VertexBuffer.hpp"
#include "SubMesh.hpp"
#include "System.hpp"

using namespace ae3d;

extern ae3d::FileWatcher fileWatcher;

struct ae3d::Mesh::Impl
{
    Impl() noexcept
    {
        static_assert( sizeof( ae3d::Mesh::Impl ) <= ae3d::Mesh::StorageSize, "Impl too big!");
        static_assert( ae3d::Mesh::StorageAlign % alignof( ae3d::Mesh::Impl ) == 0, "Impl misaligned!");
    }
    
    Vec3 aabbMin;
    Vec3 aabbMax;
    std::vector< SubMesh > subMeshes;
    std::string path;
};

struct MeshCacheEntry
{
    std::string path;
    Vec3 aabbMin;
    Vec3 aabbMax;
    std::vector< SubMesh > subMeshes;    
};

namespace
{

std::vector< MeshCacheEntry > gMeshCache;
std::vector< Mesh* > gMeshInstances;

struct membuf : std::streambuf
{
    membuf( char const* base, size_t size )
    {
        char* p( const_cast<char*>(base) );
        this->setg( p, p, p + size );
    }
};

struct imemstream : virtual membuf, std::istream
{
    imemstream( char const* base, size_t size )
        : membuf( base, size )
        , std::istream( static_cast<std::streambuf*>(this) ) {
    }
};
}

void AddUniqueInstance( Mesh* mesh )
{
    bool found = false;
    
    for (auto instance : gMeshInstances)
    {
        if (instance == mesh)
        {
            found = true;
        }
    }
    
    if (!found)
    {
        gMeshInstances.push_back( mesh );
    }
}

void MeshReload( const std::string& path )
{
    // Invalidates cache
    for (std::size_t i = 0; i < gMeshCache.size(); ++i)
    {
        if (gMeshCache[ i ].path == path)
        {
            gMeshCache.erase( std::begin( gMeshCache ) + i );
            break;
        }
    }
    
    for (auto instance : gMeshInstances)
    {
        if (instance->GetPath() == path)
        {
            instance->Load( FileSystem::FileContents( path.c_str() ) );
        }
    }
}

ae3d::Mesh::Mesh()
{
    new(&_storage)Impl();
}

ae3d::Mesh::~Mesh()
{
    reinterpret_cast< Impl* >(&_storage)->~Impl();
}

ae3d::Mesh::Mesh( const Mesh& other )
{
    new(&_storage)Impl();
    reinterpret_cast<Impl&>(_storage) = reinterpret_cast<Impl const&>(other._storage);
}

ae3d::Mesh& ae3d::Mesh::operator=( const Mesh& other )
{
    if (this == &other)
    {
        return *this;
    }

    new(&_storage)Impl();
    reinterpret_cast<Impl&>(_storage) = reinterpret_cast<Impl const&>(other._storage);
    return *this;
}

const char* ae3d::Mesh::GetPath() const
{
    return m().path.c_str();
}

const Vec3& ae3d::Mesh::GetAABBMin() const
{
    return m().aabbMin;
}

const Vec3& ae3d::Mesh::GetAABBMax() const
{
    return m().aabbMax;
}

const Vec3& ae3d::Mesh::GetSubMeshAABBMin( unsigned subMeshIndex ) const
{
    return m().subMeshes[ subMeshIndex < m().subMeshes.size() ? subMeshIndex : 0 ].aabbMin;
}

const Vec3& ae3d::Mesh::GetSubMeshAABBMax( unsigned subMeshIndex ) const
{
    return m().subMeshes[ subMeshIndex < m().subMeshes.size() ? subMeshIndex : 0 ].aabbMax;
}

const char* ae3d::Mesh::GetSubMeshName( unsigned index ) const
{
    return m().subMeshes[ index < m().subMeshes.size() ? index : 0 ].name.c_str();
}

std::vector< ae3d::SubMesh >& ae3d::Mesh::GetSubMeshes()
{
    return m().subMeshes;
}

std::vector< Vec3 > ae3d::Mesh::GetSubMeshFlattenedTriangles( unsigned subMeshIndex ) const
{
    if (subMeshIndex >= m().subMeshes.size())
    {
        System::Print( "Invalid submesh index in GetSubMeshFlattenedTriangles\n" );
        std::vector< Vec3 > outTriangles;
        return outTriangles;
    }
    
    auto& subMesh = m().subMeshes[ subMeshIndex ];
    const std::size_t faceCount = subMesh.vertexBuffer.GetFaceCount();
    std::vector< Vec3 > outTriangles( faceCount * 3 );
    
    if (!subMesh.verticesPTNTC.empty())
    {
        for (std::size_t faceIndex = 0; faceIndex < faceCount / 3; ++faceIndex)
        {
            const auto& face = subMesh.indices[ faceIndex ];
            outTriangles[ faceIndex * 3 + 0 ] = subMesh.verticesPTNTC.at( face.a ).position;
            outTriangles[ faceIndex * 3 + 1 ] = subMesh.verticesPTNTC.at( face.b ).position;
            outTriangles[ faceIndex * 3 + 2 ] = subMesh.verticesPTNTC.at( face.c ).position;
        }
    }
    else if (!subMesh.verticesPTN.empty())
    {
        for (std::size_t faceIndex = 0; faceIndex < faceCount; ++faceIndex)
        {
            const auto& face = subMesh.indices[ faceIndex ];
            outTriangles[ faceIndex * 3 + 0 ] = subMesh.verticesPTN.at( face.a ).position;
            outTriangles[ faceIndex * 3 + 1 ] = subMesh.verticesPTN.at( face.b ).position;
            outTriangles[ faceIndex * 3 + 2 ] = subMesh.verticesPTN.at( face.c ).position;
        }
    }
    else
    {
        System::Print("Empty vertex data in subMesh!\n");
    }
    
    return outTriangles;
}

unsigned ae3d::Mesh::GetSubMeshCount() const
{
    return (unsigned)m().subMeshes.size();
}

ae3d::Mesh::LoadResult ae3d::Mesh::Load( const FileSystem::FileContentsData& meshData )
{
    for (const auto& entry : gMeshCache)
    {
        if (entry.path == meshData.path)
        {
            m().aabbMin = entry.aabbMin;
            m().aabbMax = entry.aabbMax;
            m().subMeshes = entry.subMeshes;
            m().path = entry.path;

            AddUniqueInstance( this );
            
            return LoadResult::Success;
        }
    }
    
    if (!meshData.isLoaded)
    {
        const float s = 1;
        
        const VertexBuffer::VertexPTC vertices[ 8 ] =
        {
            { Vec3( -s, -s, s ), 0, 0 },
            { Vec3( s, -s, s ), 0, 0 },
            { Vec3( s, -s, -s ), 0, 0 },
            { Vec3( -s, -s, -s ), 0, 0 },
            { Vec3( -s, s, s ), 0, 0 },
            { Vec3( s, s, s ), 0, 0 },
            { Vec3( s, s, -s ), 0, 0 },
            { Vec3( -s, s, -s ), 0, 0 }
        };
        
        const VertexBuffer::Face indices[ 12 ] =
        {
            { 0, 4, 1 },
            { 4, 5, 1 },
            { 1, 5, 2 },
            { 2, 5, 6 },
            { 2, 6, 3 },
            { 3, 6, 7 },
            { 3, 7, 0 },
            { 0, 7, 4 },
            { 4, 7, 5 },
            { 5, 7, 6 },
            { 3, 0, 2 },
            { 2, 0, 1 }
        };
        
        m().subMeshes.resize( 1 );
        auto& firstSubMesh = m().subMeshes[ 0 ];
        firstSubMesh.vertexBuffer.Generate( indices, 12, vertices, 8, VertexBuffer::Storage::GPU );
        firstSubMesh.vertexBuffer.SetDebugName( "default mesh" );
        firstSubMesh.aabbMin = {-s, -s, -s};
        firstSubMesh.aabbMax = { s,  s, s };
        return LoadResult::FileNotFound;
    }
    
    uint8_t magic[ 2 ];

    imemstream is( (const char*)meshData.data.data(), meshData.data.size() );
    is.read( (char*)&magic[ 0 ], sizeof( magic ) );

    if (magic[ 0 ] != 'a' || magic[ 1 ] != '9')
    {
        System::Print( "%s is corrupted or old format: Wrong magic number!\n", meshData.path.c_str() );
        return LoadResult::Corrupted;
    }

    is.read( (char*)&m().aabbMin, sizeof( m().aabbMin ) );
    is.read( (char*)&m().aabbMax, sizeof( m().aabbMax ) );

    const auto& aabbMin = m().aabbMin;
    const auto& aabbMax = m().aabbMax;

    if (aabbMin.x > aabbMax.x || aabbMin.y > aabbMax.y || aabbMin.z > aabbMax.z)
    {
        return LoadResult::Corrupted;
    }
    
    uint16_t meshCount;
    is.read( (char*)&meshCount, sizeof( meshCount ) );

    m().subMeshes.clear();
    m().subMeshes.resize( meshCount );

    for (auto& subMesh : m().subMeshes)
    {
        is.read( (char*)&subMesh.aabbMin, sizeof( subMesh.aabbMin ) );
        is.read( (char*)&subMesh.aabbMax, sizeof( subMesh.aabbMax ) );

        uint16_t nameLength = 0;
        is.read( (char*)&nameLength, sizeof( nameLength ) );

        std::vector< char > meshName( nameLength + 1 );
        is.read( &meshName[ 0 ], nameLength );
        subMesh.name = std::string( meshName.data(), meshName.size() - 1 );

        uint16_t vertexCount = 0;
        is.read( (char*)&vertexCount, sizeof( vertexCount ) );

        uint8_t vertexFormat = 0;
        is.read( (char*)&vertexFormat, sizeof( vertexFormat ) );
        
        if (vertexFormat == 0) // PTNTC
        {
            try { subMesh.verticesPTNTC.resize( vertexCount ); }
            catch (std::bad_alloc&)
            {
                return LoadResult::OutOfMemory;
            }
        
            is.read( (char*)&subMesh.verticesPTNTC[ 0 ].position.x, vertexCount * sizeof( VertexBuffer::VertexPTNTC ) );
        }
        else if (vertexFormat == 1) // PTN
        {
            try { subMesh.verticesPTN.resize( vertexCount ); }
            catch (std::bad_alloc&)
            {
                return LoadResult::OutOfMemory;
            }
            
            is.read( (char*)&subMesh.verticesPTN[ 0 ].position.x, vertexCount * sizeof( VertexBuffer::VertexPTN ) );
        }
        else if (vertexFormat == 2) // PTNTC_Skinned
        {
            try { subMesh.verticesPTNTC_Skinned.resize( vertexCount ); }
            catch (std::bad_alloc&)
            {
                return LoadResult::OutOfMemory;
            }
            
            is.read( (char*)&subMesh.verticesPTNTC_Skinned[ 0 ].position.x, vertexCount * sizeof( VertexBuffer::VertexPTNTC_Skinned ) );
        }
        else
        {
            System::Print( "Mesh %s submesh %s has invalid vertex format %d. Only 0 and 1 are valid!\n", meshData.path.c_str(), subMesh.name.c_str(), vertexFormat );
            return LoadResult::Corrupted;
        }

        uint16_t faceCount = 0;
        is.read( (char*)&faceCount, sizeof( faceCount ) );

        try { subMesh.indices.resize( faceCount ); }
        catch (std::bad_alloc&)
        {
            return LoadResult::OutOfMemory;
        }

        is.read( (char*)&subMesh.indices[ 0 ], faceCount * sizeof( VertexBuffer::Face ) );

        if (vertexFormat == 0)
        {
            subMesh.vertexBuffer.Generate( subMesh.indices.data(), static_cast< int >( subMesh.indices.size() ), subMesh.verticesPTNTC.data(), static_cast< int >( subMesh.verticesPTNTC.size() ) );
        }
        else if (vertexFormat == 1)
        {
            subMesh.vertexBuffer.Generate( subMesh.indices.data(), static_cast< int >( subMesh.indices.size() ), subMesh.verticesPTN.data(), static_cast< int >( subMesh.verticesPTN.size() ) );
        }
        else if (vertexFormat == 2)
        {
            subMesh.vertexBuffer.Generate( subMesh.indices.data(), static_cast< int >( subMesh.indices.size() ), subMesh.verticesPTNTC_Skinned.data(), static_cast< int >( subMesh.verticesPTNTC_Skinned.size() ) );
        }
        else
        {
            ae3d::System::Assert( false, "unhandled vertex format" );
        }

        if (vertexFormat == 2)
        {
            uint16_t jointCount = 0;
            is.read( (char*)&jointCount, sizeof( jointCount ) );

            subMesh.joints.resize( jointCount );
            
            for (size_t j = 0; j < subMesh.joints.size(); ++j)
            {
                is.read( (char*)&subMesh.joints[ j ].globalBindposeInverse, sizeof( ae3d::Matrix44 ) );
                is.read( (char*)&subMesh.joints[ j ].parentIndex, 4 );
                int jointNameLength;
                is.read( (char*)&jointNameLength, sizeof( int ) );
                std::vector< char > jointName( jointNameLength + 1 );
                is.read( &jointName[ 0 ], jointNameLength );
                subMesh.joints[ j ].name = std::string( jointName.data(), jointName.size() - 1 );
                int animLength;
                is.read( (char*)&animLength, sizeof( int ) );
                subMesh.joints[ j ].animTransforms.resize( animLength );
                is.read( (char*)subMesh.joints[ j ].animTransforms.data(), subMesh.joints[ j ].animTransforms.size() * sizeof( ae3d::Matrix44 ) );
            }
        }
        
        const std::size_t pos = meshData.path.find_last_of( '/' );
        std::string shortPath = meshData.path;
        
        if (pos != std::string::npos)
        {
            shortPath = meshData.path.substr( pos );
        }

        std::string subMeshDebugName = shortPath + std::string( ":" ) + subMesh.name;
        subMesh.vertexBuffer.SetDebugName( subMeshDebugName.c_str() );
    }
    
    uint8_t terminator = 0;
    is.read( (char*)&terminator, sizeof( terminator ) );

    if (terminator != 100)
    {
        return LoadResult::Corrupted;
    }

    MeshCacheEntry cacheEntry;
    cacheEntry.path = meshData.path;
    cacheEntry.aabbMin = m().aabbMin;
    cacheEntry.aabbMax = m().aabbMax;
    cacheEntry.subMeshes = m().subMeshes;
    gMeshCache.push_back( cacheEntry );

    AddUniqueInstance( this );

    fileWatcher.AddFile( meshData.path, MeshReload );
    
    m().path = meshData.path;
    
    return LoadResult::Success;
}
