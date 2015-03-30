#include "VertexBuffer.hpp"
#include <vector>
#include <GL/glxw.h>
#include "GfxDevice.hpp"
#include "Vec3.hpp"
//#include <cstddef> for offset of on VS2013

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTC* vertices, int vertexCount )
{
    elementCount = faceCount * 3;
    
    if (id == 0)
    {
        id = GfxDevice::CreateVaoId();
    }
    
    glGenVertexArrays(1, &id);
    glBindVertexArray(id);
    
    GLuint vboId = GfxDevice::CreateBufferId();
    glBindBuffer( GL_ARRAY_BUFFER, vboId );
    glBufferData( GL_ARRAY_BUFFER, vertexCount * sizeof(VertexPTC), vertices, GL_STATIC_DRAW );
    
    GLuint iboId = GfxDevice::CreateBufferId();
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, iboId );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, faceCount * sizeof( Face ), faces, GL_STATIC_DRAW );
    
    // Vertex
    const int posChannel = 0;
    glEnableVertexAttribArray( posChannel );
    glVertexAttribPointer( posChannel, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPTC), nullptr );
    
    // TexCoord.
    const int uvChannel = 1;
    glEnableVertexAttribArray( uvChannel );
    //glVertexAttribPointer(uvChannel, 2, GL_FLOAT, GL_FALSE, sizeof(VertexPTC), (const GLvoid*)(3 * 4)); PVS-Studio warning
    glVertexAttribPointer( uvChannel, 2, GL_FLOAT, GL_FALSE, sizeof(VertexPTC), (GLvoid*)offsetof(struct VertexPTC, u) ); // No warning.

    // Color.
    const int colorChannel = 2;
    glEnableVertexAttribArray( colorChannel );
    //glVertexAttribPointer(colorChannel, 2, GL_FLOAT, GL_FALSE, sizeof(VertexPTC), (const GLvoid*)(3 * 4)); PVS-Studio warning
    glVertexAttribPointer( colorChannel, 4, GL_FLOAT, GL_FALSE, sizeof(VertexPTC), (GLvoid*)offsetof(struct VertexPTC, color) );
}

void ae3d::VertexBuffer::Bind() const
{
    glBindVertexArray( id );
}

void ae3d::VertexBuffer::Draw() const
{
    glDrawElements( GL_TRIANGLES, elementCount, GL_UNSIGNED_SHORT, nullptr );
}

void ae3d::VertexBuffer::DrawRange( int start, int end ) const
{
    glDrawRangeElements(GL_TRIANGLES, start, end, (end - start) * 3, GL_UNSIGNED_SHORT, (const GLvoid*)(start * sizeof(Face)));
}
