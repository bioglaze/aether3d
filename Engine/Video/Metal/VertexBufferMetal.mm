#include <vector>
#include "VertexBuffer.hpp"
#include "GfxDevice.hpp"
#include "System.hpp"

void ae3d::VertexBuffer::Bind() const
{
}

void ae3d::VertexBuffer::SetDebugName( const char* name )
{
    vertexBuffer.label = [NSString stringWithUTF8String:name];
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTC* vertices, int vertexCount )
{
    if (faceCount > 0)
    {
        vertexFormat = VertexFormat::PTC;
        vertexBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:vertices
                                                 length:sizeof( VertexPTC ) * vertexCount
                                                options:MTLResourceCPUCacheModeDefaultCache];
        vertexBuffer.label = @"Vertex buffer PTC";

        std::vector< float > positions( vertexCount * 3 );
        
        for (int v = 0; v < vertexCount; ++v)
        {
            positions[ v * 3 + 0 ] = vertices[ v ].position.x;
            positions[ v * 3 + 1 ] = vertices[ v ].position.y;
            positions[ v * 3 + 2 ] = vertices[ v ].position.z;
        }
        
        positionBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:positions.data()
                           length:3 * 4 * vertexCount
                          options:MTLResourceCPUCacheModeDefaultCache];
        positionBuffer.label = @"Position buffer";

        std::vector< float > texcoords( vertexCount * 2 );
        
        for (int v = 0; v < vertexCount; ++v)
        {
            texcoords[ v * 2 + 0 ] = vertices[ v ].u;
            texcoords[ v * 2 + 1 ] = vertices[ v ].v;
        }
        
        texcoordBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:texcoords.data()
                             length:2 * 4 * vertexCount
                            options:MTLResourceCPUCacheModeDefaultCache];
        texcoordBuffer.label = @"Texcoord buffer";

        std::vector< float > colors( vertexCount * 4 );
        
        for (int v = 0; v < vertexCount; ++v)
        {
            colors[ v * 4 + 0 ] = vertices[ v ].color.x;
            colors[ v * 4 + 1 ] = vertices[ v ].color.y;
            colors[ v * 4 + 2 ] = vertices[ v ].color.z;
            colors[ v * 4 + 3 ] = vertices[ v ].color.w;
        }
        
        colorBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:colors.data()
                             length:4 * 4 * vertexCount
                            options:MTLResourceCPUCacheModeDefaultCache];
        colorBuffer.label = @"Color buffer";

        std::vector< float > normals( vertexCount * 3 );
        
        for (std::size_t v = 0; v < normals.size(); ++v)
        {
            normals[ v ] = 1;
        }
        
        normalBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:colors.data()
                          length:3 * 4 * vertexCount
                         options:MTLResourceCPUCacheModeDefaultCache];
        normalBuffer.label = @"Normal buffer";
        
        std::vector< float > tangents( vertexCount * 4 );
        
        for (std::size_t v = 0; v < tangents.size(); ++v)
        {
            tangents[ v ] = 1;
        }
        
        tangentBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:tangents.data()
                            length:4 * 4 * vertexCount
                           options:MTLResourceCPUCacheModeDefaultCache];
        tangentBuffer.label = @"Tangent buffer";

        indexBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:faces
                           length:sizeof( Face ) * faceCount
                          options:MTLResourceCPUCacheModeDefaultCache];
        indexBuffer.label = @"Index buffer";

        elementCount = faceCount * 3;
    }
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTN* vertices, int vertexCount )
{
    if (faceCount > 0)
    {
        vertexFormat = VertexFormat::PTN;
        vertexBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:vertices
                           length:sizeof( VertexPTN ) * vertexCount
                          options:MTLResourceCPUCacheModeDefaultCache];
        vertexBuffer.label = @"Vertex buffer PTN";

        std::vector< float > positions( vertexCount * 3 );
        
        for (int v = 0; v < vertexCount; ++v)
        {
            positions[ v * 3 + 0 ] = vertices[ v ].position.x;
            positions[ v * 3 + 1 ] = vertices[ v ].position.y;
            positions[ v * 3 + 2 ] = vertices[ v ].position.z;
        }
        
        positionBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:positions.data()
                             length:3 * 4 * vertexCount
                            options:MTLResourceCPUCacheModeDefaultCache];
        positionBuffer.label = @"Position buffer";
        
        std::vector< float > texcoords( vertexCount * 2 );
        
        for (int v = 0; v < vertexCount; ++v)
        {
            texcoords[ v * 2 + 0 ] = vertices[ v ].u;
            texcoords[ v * 2 + 1 ] = vertices[ v ].v;
        }
        
        texcoordBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:texcoords.data()
                             length:2 * 4 * vertexCount
                            options:MTLResourceCPUCacheModeDefaultCache];
        texcoordBuffer.label = @"Texcoord buffer";
        
        std::vector< float > normals( vertexCount * 3 );
        
        for (int v = 0; v < vertexCount; ++v)
        {
            normals[ v * 3 + 0 ] = vertices[ v ].normal.x;
            normals[ v * 3 + 1 ] = vertices[ v ].normal.y;
            normals[ v * 3 + 2 ] = vertices[ v ].normal.z;
        }
        
        normalBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:normals.data()
                          length:3 * 4 * vertexCount
                         options:MTLResourceCPUCacheModeDefaultCache];
        normalBuffer.label = @"Normal buffer";

        std::vector< float > colors( vertexCount * 4 );
        
        for (std::size_t v = 0; v < colors.size(); ++v)
        {
            colors[ v ] = 1;
        }
        
        colorBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:colors.data()
                          length:4 * 4 * vertexCount
                         options:MTLResourceCPUCacheModeDefaultCache];
        colorBuffer.label = @"Color buffer";
        
        std::vector< float > tangents( vertexCount * 4 );
        
        for (std::size_t v = 0; v < tangents.size(); ++v)
        {
            tangents[ v ] = 1;
        }
        
        tangentBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:tangents.data()
                            length:4 * 4 * vertexCount
                           options:MTLResourceCPUCacheModeDefaultCache];
        tangentBuffer.label = @"Tangent buffer";

        indexBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:faces
                          length:sizeof( Face ) * faceCount
                         options:MTLResourceCPUCacheModeDefaultCache];
        indexBuffer.label = @"Index buffer";
        
        elementCount = faceCount * 3;
    }
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTNTC* vertices, int vertexCount )
{
    if (faceCount > 0)
    {
        vertexFormat = VertexFormat::PTNTC;
        vertexBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:vertices
                           length:sizeof( VertexPTNTC ) * vertexCount
                          options:MTLResourceCPUCacheModeDefaultCache];
        vertexBuffer.label = @"Vertex buffer PTNTC";

        std::vector< float > positions( vertexCount * 3 );
        
        for (int v = 0; v < vertexCount; ++v)
        {
            positions[ v * 3 + 0 ] = vertices[ v ].position.x;
            positions[ v * 3 + 1 ] = vertices[ v ].position.y;
            positions[ v * 3 + 2 ] = vertices[ v ].position.z;
        }
        
        positionBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:positions.data()
                             length:3 * 4 * vertexCount
                            options:MTLResourceCPUCacheModeDefaultCache];
        positionBuffer.label = @"Position buffer";
        
        std::vector< float > texcoords( vertexCount * 2 );
        
        for (int v = 0; v < vertexCount; ++v)
        {
            texcoords[ v * 2 + 0 ] = vertices[ v ].u;
            texcoords[ v * 2 + 1 ] = vertices[ v ].v;
        }
        
        texcoordBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:texcoords.data()
                             length:2 * 4 * vertexCount
                            options:MTLResourceCPUCacheModeDefaultCache];
        texcoordBuffer.label = @"Texcoord buffer";
        
        std::vector< float > normals( vertexCount * 3 );
        
        for (int v = 0; v < vertexCount; ++v)
        {
            normals[ v * 3 + 0 ] = vertices[ v ].normal.x;
            normals[ v * 3 + 1 ] = vertices[ v ].normal.y;
            normals[ v * 3 + 2 ] = vertices[ v ].normal.z;
        }
        
        normalBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:normals.data()
                           length:3 * 4 * vertexCount
                          options:MTLResourceCPUCacheModeDefaultCache];
        normalBuffer.label = @"Normal buffer";

        std::vector< float > colors( vertexCount * 4 );
        
        for (int v = 0; v < vertexCount; ++v)
        {
            colors[ v * 4 + 0 ] = vertices[ v ].color.x;
            colors[ v * 4 + 1 ] = vertices[ v ].color.y;
            colors[ v * 4 + 2 ] = vertices[ v ].color.z;
            colors[ v * 4 + 3 ] = vertices[ v ].color.w;
        }
        
        colorBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:colors.data()
                          length:4 * 4 * vertexCount
                         options:MTLResourceCPUCacheModeDefaultCache];
        colorBuffer.label = @"Color buffer";

        std::vector< float > tangents( vertexCount * 4 );
        
        for (int v = 0; v < vertexCount; ++v)
        {
            tangents[ v * 4 + 0 ] = vertices[ v ].tangent.x;
            tangents[ v * 4 + 1 ] = vertices[ v ].tangent.y;
            tangents[ v * 4 + 2 ] = vertices[ v ].tangent.z;
            tangents[ v * 4 + 3 ] = vertices[ v ].tangent.w;
        }
        
        tangentBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:tangents.data()
                          length:4 * 4 * vertexCount
                         options:MTLResourceCPUCacheModeDefaultCache];
        tangentBuffer.label = @"Tangent buffer";

        indexBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:faces
                          length:sizeof( Face ) * faceCount
                         options:MTLResourceCPUCacheModeDefaultCache];
        indexBuffer.label = @"Index buffer";
        
        elementCount = faceCount * 3;
    }
}
