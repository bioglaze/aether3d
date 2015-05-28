#include "VertexBuffer.hpp"
#include "GfxDevice.hpp"

void ae3d::VertexBuffer::Bind() const
{
}

void ae3d::VertexBuffer::Draw() const
{
    GfxDevice::DrawVertexBuffer( vertexBuffer, indexBuffer, elementCount, 0 );
}

void ae3d::VertexBuffer::DrawRange( int start, int end ) const
{
    GfxDevice::DrawVertexBuffer( vertexBuffer, indexBuffer, (end - start) * 3, start * 2 * 3 );
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTC* vertices, int vertexCount )
{
    if (faceCount > 0)
    {
        vertexBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:vertices
                                                 length:sizeof( VertexPTC ) * vertexCount
                                                options:MTLResourceOptionCPUCacheModeDefault];
        vertexBuffer.label = @"Vertex buffer";

        indexBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:faces
                           length:sizeof( Face ) * faceCount
                          options:MTLResourceOptionCPUCacheModeDefault];
        indexBuffer.label = @"Index buffer";

        elementCount = faceCount * 3;
    }
}
