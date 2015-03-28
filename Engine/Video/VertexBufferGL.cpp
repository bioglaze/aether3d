#include "VertexBuffer.hpp"
#include <vector>
#include <GL/glxw.h>
#include "GfxDevice.hpp"
#include "Vec3.hpp"
//#include <cstddef> for offset of on VS2013

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPT* vertices, int vertexCount )
{
    elementCount = faceCount * 3;
    
    if (id == 0)
    {
        id = GfxDevice::CreateVaoId();
    }
    
    glGenVertexArrays(1, &id);
    glBindVertexArray(id);
    
    GLuint vboId = GfxDevice::CreateVboId();
    glBindBuffer( GL_ARRAY_BUFFER, vboId );
    glBufferData( GL_ARRAY_BUFFER, vertexCount * sizeof(VertexPT), vertices, GL_STATIC_DRAW );
    
    GLuint iboId = GfxDevice::CreateIboId();
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, iboId );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, faceCount * sizeof( Face ), faces, GL_STATIC_DRAW );
    
    // Vertex
    const int posChannel = 0;
    glEnableVertexAttribArray( posChannel );
    glVertexAttribPointer( posChannel, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPT), nullptr );
    
    // TexCoord.
    const int uvChannel = 1;
    glEnableVertexAttribArray(uvChannel);
    //glVertexAttribPointer(uvChannel, 2, GL_FLOAT, GL_FALSE, sizeof(VertexPT), (const GLvoid*)(3 * 4)); PVS-Studio warning
    glVertexAttribPointer( uvChannel, 2, GL_FLOAT, GL_FALSE, sizeof(VertexPT), (GLvoid*)offsetof(struct VertexPT, u) ); // No warning.
}

void ae3d::VertexBuffer::Draw() const
{
    glBindVertexArray( id );
    glDrawElements( GL_TRIANGLES, elementCount, GL_UNSIGNED_SHORT, 0 );
}
