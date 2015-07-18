#include "VertexBuffer.hpp"
#include <vector>
#include <GL/glxw.h>
#include "GfxDevice.hpp"
#include "Vec3.hpp"

namespace Global
{
    GLuint activeVao = -1;
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
    glVertexAttribPointer( posChannel, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPTC), nullptr );
    
    // TexCoord.
    glEnableVertexAttribArray( uvChannel );
    glVertexAttribPointer( uvChannel, 2, GL_FLOAT, GL_FALSE, sizeof(VertexPTC), (GLvoid*)offsetof( struct VertexPTC, u ) );

    // Color.
    glEnableVertexAttribArray( colorChannel );
    glVertexAttribPointer( colorChannel, 4, GL_FLOAT, GL_FALSE, sizeof(VertexPTC), (GLvoid*)offsetof( struct VertexPTC, color ) );
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
    glBufferData( GL_ARRAY_BUFFER, vertexCount * sizeof(VertexPTN), vertices, GL_STATIC_DRAW );
    
    if (iboId == 0)
    {
        iboId = GfxDevice::CreateBufferId();
    }
    
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, iboId );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, faceCount * sizeof( Face ), faces, GL_STATIC_DRAW );
    
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
    glBufferData( GL_ARRAY_BUFFER, vertexCount * sizeof( VertexPTNTC ), vertices, GL_STATIC_DRAW );
    
    if (iboId == 0)
    {
        iboId = GfxDevice::CreateBufferId();
    }

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, iboId );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, faceCount * sizeof( Face ), faces, GL_STATIC_DRAW );
    
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
    glVertexAttribPointer( colorChannel, 4, GL_FLOAT, GL_FALSE, sizeof( VertexPTNTC ), (GLvoid*)offsetof(struct VertexPTNTC, color) );
}

void ae3d::VertexBuffer::Bind() const
{
    if (Global::activeVao != vaoId)
    {
        glBindVertexArray( vaoId );
        Global::activeVao = vaoId;
        GfxDevice::IncVertexBufferBinds();
    }
}

void ae3d::VertexBuffer::Draw() const
{
    GfxDevice::IncDrawCalls();
    glDrawElements( GL_TRIANGLES, elementCount, GL_UNSIGNED_SHORT, nullptr );
}

void ae3d::VertexBuffer::DrawRange( int start, int end ) const
{
    GfxDevice::IncDrawCalls();
    glDrawRangeElements( GL_TRIANGLES, start, end, (end - start) * 3, GL_UNSIGNED_SHORT, (const GLvoid*)(start * sizeof( Face )) );
}
