#include "VertexBuffer.hpp"
#include <vector>
#include <GL/glxw.h>
#include "GfxDevice.hpp"
#include "Vec3.hpp"

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
    const int posChannel = 0;
    glEnableVertexAttribArray( posChannel );
    glVertexAttribPointer( posChannel, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPTC), nullptr );
    
    // TexCoord.
    const int uvChannel = 1;
    glEnableVertexAttribArray( uvChannel );
    glVertexAttribPointer( uvChannel, 2, GL_FLOAT, GL_FALSE, sizeof(VertexPTC), (GLvoid*)offsetof( struct VertexPTC, u ) );

    // Color.
    const int colorChannel = 2;
    glEnableVertexAttribArray( colorChannel );
    glVertexAttribPointer( colorChannel, 4, GL_FLOAT, GL_FALSE, sizeof(VertexPTC), (GLvoid*)offsetof( struct VertexPTC, color ) );
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
    const int posChannel = 0;
    glEnableVertexAttribArray( posChannel );
    glVertexAttribPointer( posChannel, 3, GL_FLOAT, GL_FALSE, sizeof( VertexPTNTC ), nullptr );
    
    // TexCoord.
    const int uvChannel = 1;
    glEnableVertexAttribArray( uvChannel );
    glVertexAttribPointer( uvChannel, 2, GL_FLOAT, GL_FALSE, sizeof( VertexPTNTC ), (GLvoid*)offsetof( struct VertexPTNTC, u ) );

    // Normal.
    const int normalChannel = 2;
    glEnableVertexAttribArray( normalChannel );
    glVertexAttribPointer( normalChannel, 3, GL_FLOAT, GL_FALSE, sizeof( VertexPTNTC ), (GLvoid*)offsetof( struct VertexPTNTC, normal ) );

    // Tangent.
    const int tangentChannel = 3;
    glEnableVertexAttribArray( tangentChannel );
    glVertexAttribPointer( tangentChannel, 4, GL_FLOAT, GL_FALSE, sizeof( VertexPTNTC ), (GLvoid*)offsetof( struct VertexPTNTC, tangent ) );

    // Color.
    const int colorChannel = 4;
    glEnableVertexAttribArray( colorChannel );
    glVertexAttribPointer( colorChannel, 4, GL_FLOAT, GL_FALSE, sizeof( VertexPTNTC ), (GLvoid*)offsetof(struct VertexPTNTC, color) );
}

void ae3d::VertexBuffer::Bind() const
{
    glBindVertexArray( vaoId );
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
