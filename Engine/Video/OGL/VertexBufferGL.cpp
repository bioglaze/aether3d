#include "VertexBuffer.hpp"
#include <string>
#include <vector>
#include <GL/glxw.h>
#include "GfxDevice.hpp"
#include "Statistics.hpp"
#include "Vec3.hpp"

namespace Global
{
    GLuint activeVao = 0;
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTC* vertices, int vertexCount )
{
    vertexFormat = VertexFormat::PTC;
    elementCount = faceCount * 3;
    
    if (vaoId == 0)
    {
        vaoId = GfxDevice::CreateVaoId();
    }
    
    glBindVertexArray( vaoId );
    
    if (vboId == 0)
    {
        vboId = GfxDevice::CreateBufferId();
    }

    glBindBuffer( GL_ARRAY_BUFFER, vboId );
    glBufferData( GL_ARRAY_BUFFER, vertexCount * sizeof(VertexPTC), vertices, GL_STATIC_DRAW );

    if (iboId == 0)
    {
        iboId = GfxDevice::CreateBufferId();
    }

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, iboId );    
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, faceCount * sizeof( Face ), faces, GL_STATIC_DRAW );

    // Position.
    glEnableVertexAttribArray( posChannel );
    glVertexAttribPointer( posChannel, 3, GL_FLOAT, GL_FALSE, sizeof( VertexPTC ), nullptr );

    // TexCoord.
    glEnableVertexAttribArray( uvChannel );
    glVertexAttribPointer( uvChannel, 2, GL_FLOAT, GL_FALSE, sizeof( VertexPTC ), (GLvoid*)offsetof( struct VertexPTC, u ) );

    // Color.
    glEnableVertexAttribArray( colorChannel );
    glVertexAttribPointer( colorChannel, 4, GL_FLOAT, GL_FALSE, sizeof( VertexPTC ), (GLvoid*)offsetof( struct VertexPTC, color ) );
}

void ae3d::VertexBuffer::SetDebugName( const char* name )
{
    if (GfxDevice::HasExtension( "GL_KHR_debug" ))
    {
        glObjectLabel( GL_BUFFER, vboId, -1, name );
    }
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTN* vertices, int vertexCount )
{
    vertexFormat = VertexFormat::PTN;
    elementCount = faceCount * 3;
    
    if (vaoId == 0)
    {
        vaoId = GfxDevice::CreateVaoId();
    }
    
    glBindVertexArray( vaoId );
    
    if (vboId == 0)
    {
        vboId = GfxDevice::CreateBufferId();
    }
    
    glBindBuffer( GL_ARRAY_BUFFER, vboId );

    if (GfxDevice::HasExtension( "GL_KHR_debug" ))
    {
        glObjectLabel( GL_BUFFER, vboId, -1, "vbo" );
    }

    if (GfxDevice::HasExtension( "GL_ARB_buffer_storage" ))
    {
        const GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        glBufferStorage( GL_ARRAY_BUFFER, vertexCount * sizeof( VertexPTN ), vertices, flags );
    }
    else
    {
        glBufferData( GL_ARRAY_BUFFER, vertexCount * sizeof( VertexPTN ), vertices, GL_STATIC_DRAW );
    }

    if (iboId == 0)
    {
        iboId = GfxDevice::CreateBufferId();
    }
    
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, iboId );

    if (GfxDevice::HasExtension( "GL_KHR_debug" ))
    {
        glObjectLabel( GL_BUFFER, iboId, -1, "ibo" );
    }

    if (GfxDevice::HasExtension( "GL_ARB_buffer_storage" ))
    {
        const GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        glBufferStorage( GL_ELEMENT_ARRAY_BUFFER, faceCount * sizeof( Face ), faces, flags );
    }
    else
    {
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, faceCount * sizeof( Face ), faces, GL_STATIC_DRAW );
    }

    // Position.
    glEnableVertexAttribArray( posChannel );
    glVertexAttribPointer( posChannel, 3, GL_FLOAT, GL_FALSE, sizeof( VertexPTN ), nullptr );

    // TexCoord.
    glEnableVertexAttribArray( uvChannel );
    glVertexAttribPointer( uvChannel, 2, GL_FLOAT, GL_FALSE, sizeof( VertexPTN ), (GLvoid*)offsetof( struct VertexPTN, u ) );

    // Normal.
    glEnableVertexAttribArray( normalChannel );
    glVertexAttribPointer( normalChannel, 3, GL_FLOAT, GL_FALSE, sizeof( VertexPTN ), (GLvoid*)offsetof( struct VertexPTN, normal ) );
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTNTC* vertices, int vertexCount )
{
    vertexFormat = VertexFormat::PTNTC;
    elementCount = faceCount * 3;
    
    if (vaoId == 0)
    {
        vaoId = GfxDevice::CreateVaoId();
    }
    
    glBindVertexArray( vaoId );
    
    if (vboId == 0)
    {
        vboId = GfxDevice::CreateBufferId();
    }

    glBindBuffer( GL_ARRAY_BUFFER, vboId );
    
    if (GfxDevice::HasExtension( "GL_ARB_buffer_storage" ))
    {
        const GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        glBufferStorage( GL_ARRAY_BUFFER, vertexCount * sizeof( VertexPTNTC ), vertices, flags );
    }
    else
    {
        glBufferData( GL_ARRAY_BUFFER, vertexCount * sizeof( VertexPTNTC ), vertices, GL_STATIC_DRAW );
    }

    if (iboId == 0)
    {
        iboId = GfxDevice::CreateBufferId();
    }

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, iboId );

    if (GfxDevice::HasExtension( "GL_ARB_buffer_storage" ))
    {
        const GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        glBufferStorage( GL_ELEMENT_ARRAY_BUFFER, faceCount * sizeof( Face ), faces, flags );
    }
    else
    {
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, faceCount * sizeof( Face ), faces, GL_STATIC_DRAW );
    }

    // Position.
    glEnableVertexAttribArray( posChannel );
    glVertexAttribPointer( posChannel, 3, GL_FLOAT, GL_FALSE, sizeof( VertexPTNTC ), nullptr );

    // TexCoord.
    glEnableVertexAttribArray( uvChannel );
    glVertexAttribPointer( uvChannel, 2, GL_FLOAT, GL_FALSE, sizeof( VertexPTNTC ), (GLvoid*)offsetof( struct VertexPTNTC, u ) );

    // Normal.
    glEnableVertexAttribArray( normalChannel );
    glVertexAttribPointer( normalChannel, 3, GL_FLOAT, GL_FALSE, sizeof( VertexPTNTC ), (GLvoid*)offsetof( struct VertexPTNTC, normal ) );

    // Tangent.
    glEnableVertexAttribArray( tangentChannel );
    glVertexAttribPointer( tangentChannel, 4, GL_FLOAT, GL_FALSE, sizeof( VertexPTNTC ), (GLvoid*)offsetof( struct VertexPTNTC, tangent ) );

    // Color.
    glEnableVertexAttribArray( colorChannel );
    glVertexAttribPointer( colorChannel, 4, GL_FLOAT, GL_FALSE, sizeof( VertexPTNTC ), (GLvoid*)offsetof( struct VertexPTNTC, color ) );
}

void ae3d::VertexBuffer::Bind() const
{
    if (Global::activeVao != vaoId)
    {
        glBindVertexArray( vaoId );
        Global::activeVao = vaoId;
        Statistics::IncVertexBufferBinds();
    }
}
