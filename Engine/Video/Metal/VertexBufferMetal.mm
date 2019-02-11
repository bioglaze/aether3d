#include <vector>
#include "VertexBuffer.hpp"
#include "GfxDevice.hpp"
#include "System.hpp"

extern id <MTLCommandQueue> commandQueue;
static int vertexBufferMemoryUsage = 0;

void ae3d::VertexBuffer::Bind() const
{
}

void ae3d::VertexBuffer::SetDebugName( const char* name )
{
    vertexBuffer.label = [NSString stringWithUTF8String:name];
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTC* vertices, int vertexCount, Storage storage )
{
    if (faceCount == 0)
    {
        return;
    }
    
    vertexFormat = VertexFormat::PTC;
    
    if (storage == Storage::GPU)
    {
        vertexBuffer = [GfxDevice::GetMetalDevice() newBufferWithLength:sizeof( VertexPTC ) * vertexCount
                       options:MTLResourceStorageModePrivate];
        
        id<MTLBuffer> blitBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:vertices
                                       length:sizeof( VertexPTC ) * vertexCount
                                      options:MTLResourceCPUCacheModeDefaultCache];
        blitBuffer.label = @"BlitBuffer";
        
        id <MTLCommandBuffer> cmd_buffer =     [commandQueue commandBuffer];
        cmd_buffer.label = @"BlitCommandBuffer";
        id <MTLBlitCommandEncoder> blit_encoder = [cmd_buffer blitCommandEncoder];
        [blit_encoder copyFromBuffer:blitBuffer
                        sourceOffset:0
                            toBuffer:vertexBuffer
                   destinationOffset:0
                                size:sizeof( VertexPTC ) * vertexCount];
        [blit_encoder endEncoding];
        [cmd_buffer commit];
        [cmd_buffer waitUntilCompleted];
    }
    else
    {
        vertexBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:vertices
                                       length:sizeof( VertexPTC ) * vertexCount
                                      options:MTLResourceCPUCacheModeDefaultCache];
    }
    
    vertexBuffer.label = @"Vertex buffer PTC";
    
    std::vector< float > positions( vertexCount * 3 );
    
    for (int v = 0; v < vertexCount; ++v)
    {
        positions[ v * 3 + 0 ] = vertices[ v ].position.x;
        positions[ v * 3 + 1 ] = vertices[ v ].position.y;
        positions[ v * 3 + 2 ] = vertices[ v ].position.z;
    }
    
    if (positionCount != (unsigned)vertexCount)
    {
        positionBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:positions.data()
                         length:3 * 4 * vertexCount
                        options:MTLResourceCPUCacheModeDefaultCache];
        positionBuffer.label = @"Position buffer";
    }
    else
    {
        memcpy( [positionBuffer contents], positions.data(), 3 * 4 * vertexCount );
    }
    
    std::vector< float > texcoords( vertexCount * 2 );
    
    for (int v = 0; v < vertexCount; ++v)
    {
        texcoords[ v * 2 + 0 ] = vertices[ v ].u;
        texcoords[ v * 2 + 1 ] = vertices[ v ].v;
    }
    
    if (positionCount != (unsigned)vertexCount)
    {
        texcoordBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:texcoords.data()
                         length:2 * 4 * vertexCount
                        options:MTLResourceCPUCacheModeDefaultCache];
        texcoordBuffer.label = @"Texcoord buffer";
    }
    else
    {
        memcpy( [texcoordBuffer contents], texcoords.data(), 2 * 4 * vertexCount );
    }
    
    std::vector< float > colors( vertexCount * 4 );
    
    for (int v = 0; v < vertexCount; ++v)
    {
        colors[ v * 4 + 0 ] = vertices[ v ].color.x;
        colors[ v * 4 + 1 ] = vertices[ v ].color.y;
        colors[ v * 4 + 2 ] = vertices[ v ].color.z;
        colors[ v * 4 + 3 ] = vertices[ v ].color.w;
    }
    
    if (positionCount != (unsigned)vertexCount)
    {
        colorBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:colors.data()
                      length:4 * 4 * vertexCount
                     options:MTLResourceCPUCacheModeDefaultCache];
        colorBuffer.label = @"Color buffer";
    }
    else
    {
        memcpy( [colorBuffer contents], colors.data(), 4 * 4 * vertexCount );
    }
    
    std::vector< float > normals( vertexCount * 3 );
    
    for (std::size_t v = 0; v < normals.size(); ++v)
    {
        normals[ v ] = 1;
    }
    
    if (positionCount != (unsigned)vertexCount)
    {
        normalBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:normals.data()
                       length:3 * 4 * vertexCount
                      options:MTLResourceCPUCacheModeDefaultCache];
        normalBuffer.label = @"Normal buffer";
    }
    else
    {
        memcpy( [normalBuffer contents], normals.data(), 3 * 4 * vertexCount );
    }

    std::vector< float > tangents( vertexCount * 4 );
    
    for (std::size_t v = 0; v < tangents.size(); ++v)
    {
        tangents[ v ] = 1;
    }
    
    if (positionCount != (unsigned)vertexCount)
    {
        tangentBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:tangents.data()
                        length:4 * 4 * vertexCount
                       options:MTLResourceCPUCacheModeDefaultCache];
        tangentBuffer.label = @"Tangent buffer";
    }
    else
    {
        memcpy( [tangentBuffer contents], tangents.data(), 4 * 4 * vertexCount );
    }

    std::vector< int > bones( vertexCount * 4 );
    
    for (std::size_t v = 0; v < bones.size(); ++v)
    {
        bones[ v ] = 0;
    }

    boneBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:bones.data()
                     length:4 * 4 * vertexCount
                    options:MTLResourceCPUCacheModeDefaultCache];
    boneBuffer.label = @"Bone buffer";
    
    std::vector< float > weights( vertexCount * 4 );
    
    for (std::size_t v = 0; v < weights.size(); ++v)
    {
        weights[ v ] = 1;
    }
    
    weightBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:weights.data()
                       length:4 * 4 * vertexCount
                      options:MTLResourceCPUCacheModeDefaultCache];
    weightBuffer.label = @"Weight buffer";
    
    indexBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:faces
                      length:sizeof( Face ) * faceCount
                     options:MTLResourceCPUCacheModeDefaultCache];
    indexBuffer.label = @"Index buffer";
    
    elementCount = faceCount * 3;
    positionCount = vertexCount;

    vertexBufferMemoryUsage += [vertexBuffer allocatedSize];
    vertexBufferMemoryUsage += [indexBuffer allocatedSize];
    vertexBufferMemoryUsage += [weightBuffer allocatedSize];
    vertexBufferMemoryUsage += [boneBuffer allocatedSize];
    vertexBufferMemoryUsage += [tangentBuffer allocatedSize];
    vertexBufferMemoryUsage += [normalBuffer allocatedSize];
    vertexBufferMemoryUsage += [texcoordBuffer allocatedSize];
    vertexBufferMemoryUsage += [colorBuffer allocatedSize];
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTN* vertices, int vertexCount )
{
    if (faceCount == 0)
    {
        return;
    }
    
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
    
    std::vector< int > bones( vertexCount * 4 );
    
    for (std::size_t v = 0; v < bones.size(); ++v)
    {
        bones[ v ] = 0;
    }
    
    boneBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:bones.data()
                     length:4 * 4 * vertexCount
                    options:MTLResourceCPUCacheModeDefaultCache];
    boneBuffer.label = @"Bone buffer";
    
    std::vector< float > weights( vertexCount * 4 );
    
    for (std::size_t v = 0; v < weights.size(); ++v)
    {
        weights[ v ] = 1;
    }
    
    weightBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:weights.data()
                       length:4 * 4 * vertexCount
                      options:MTLResourceCPUCacheModeDefaultCache];
    weightBuffer.label = @"Weight buffer";
    
    indexBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:faces
                      length:sizeof( Face ) * faceCount
                     options:MTLResourceCPUCacheModeDefaultCache];
    indexBuffer.label = @"Index buffer";
    
    elementCount = faceCount * 3;
    
    vertexBufferMemoryUsage += [vertexBuffer allocatedSize];
    vertexBufferMemoryUsage += [indexBuffer allocatedSize];
    vertexBufferMemoryUsage += [weightBuffer allocatedSize];
    vertexBufferMemoryUsage += [boneBuffer allocatedSize];
    vertexBufferMemoryUsage += [tangentBuffer allocatedSize];
    vertexBufferMemoryUsage += [normalBuffer allocatedSize];
    vertexBufferMemoryUsage += [texcoordBuffer allocatedSize];
    vertexBufferMemoryUsage += [colorBuffer allocatedSize];
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTNTC* vertices, int vertexCount )
{
    if (faceCount == 0)
    {
        return;
    }

    vertexFormat = VertexFormat::PTNTC;
    vertexBuffer = [GfxDevice::GetMetalDevice() newBufferWithLength:sizeof( VertexPTNTC ) * vertexCount
                      options:MTLResourceStorageModePrivate];
    vertexBuffer.label = @"Vertex buffer PTNTC";

    id<MTLBuffer> blitBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:vertices
                      length:sizeof( VertexPTNTC ) * vertexCount
                      options:MTLResourceCPUCacheModeDefaultCache];
    blitBuffer.label = @"BlitBuffer";

    id <MTLCommandBuffer> cmd_buffer = 	[commandQueue commandBuffer];
    cmd_buffer.label = @"BlitCommandBuffer";
    id <MTLBlitCommandEncoder> blit_encoder = [cmd_buffer blitCommandEncoder];
    [blit_encoder copyFromBuffer:blitBuffer
                    sourceOffset:0
                        toBuffer:vertexBuffer
               destinationOffset:0
                            size:sizeof( VertexPTNTC ) * vertexCount];
    [blit_encoder endEncoding];
    [cmd_buffer commit];
    [cmd_buffer waitUntilCompleted];
    
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
    
    std::vector< int > bones( vertexCount * 4 );
    
    for (std::size_t v = 0; v < bones.size(); ++v)
    {
        bones[ v ] = 0;
    }
        
    boneBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:bones.data()
                     length:4 * 4 * vertexCount
                    options:MTLResourceCPUCacheModeDefaultCache];
    boneBuffer.label = @"Bone buffer";
    
    std::vector< float > weights( vertexCount * 4 );
    
    for (std::size_t v = 0; v < weights.size(); ++v)
    {
        weights[ v ] = 1;
    }
    
    weightBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:weights.data()
                       length:4 * 4 * vertexCount
                      options:MTLResourceCPUCacheModeDefaultCache];
    weightBuffer.label = @"Weight buffer";
    
    indexBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:faces
                      length:sizeof( Face ) * faceCount
                     options:MTLResourceCPUCacheModeDefaultCache];
    indexBuffer.label = @"Index buffer";
    
    elementCount = faceCount * 3;
    
    vertexBufferMemoryUsage += [vertexBuffer allocatedSize];
    vertexBufferMemoryUsage += [indexBuffer allocatedSize];
    vertexBufferMemoryUsage += [weightBuffer allocatedSize];
    vertexBufferMemoryUsage += [boneBuffer allocatedSize];
    vertexBufferMemoryUsage += [tangentBuffer allocatedSize];
    vertexBufferMemoryUsage += [normalBuffer allocatedSize];
    vertexBufferMemoryUsage += [texcoordBuffer allocatedSize];
    vertexBufferMemoryUsage += [colorBuffer allocatedSize];
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTNTC_Skinned* vertices, int vertexCount )
{
    if (faceCount == 0)
    {
        return;
    }

    vertexFormat = VertexFormat::PTNTC_Skinned;
    vertexBuffer = [GfxDevice::GetMetalDevice() newBufferWithLength:sizeof( VertexPTNTC_Skinned ) * vertexCount
                      options:MTLResourceStorageModePrivate];
    vertexBuffer.label = @"Vertex buffer PTNTC_Skinned";
    
    id<MTLBuffer> blitBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:vertices
                                   length:sizeof( VertexPTNTC_Skinned ) * vertexCount
                                  options:MTLResourceCPUCacheModeDefaultCache];
    blitBuffer.label = @"BlitBuffer";
    
    id <MTLCommandBuffer> cmd_buffer = [commandQueue commandBuffer];
    cmd_buffer.label = @"BlitCommandBuffer";
    id <MTLBlitCommandEncoder> blit_encoder = [cmd_buffer blitCommandEncoder];
    [blit_encoder copyFromBuffer:blitBuffer
                    sourceOffset:0
                        toBuffer:vertexBuffer
               destinationOffset:0
                            size:sizeof( VertexPTNTC_Skinned ) * vertexCount];
    [blit_encoder endEncoding];
    [cmd_buffer commit];
    [cmd_buffer waitUntilCompleted];

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
    
    std::vector< float > weights( vertexCount * 4 );
    
    for (int v = 0; v < vertexCount; ++v)
    {
        weights[ v * 4 + 0 ] = vertices[ v ].weights.x;
        weights[ v * 4 + 1 ] = vertices[ v ].weights.y;
        weights[ v * 4 + 2 ] = vertices[ v ].weights.z;
        weights[ v * 4 + 3 ] = vertices[ v ].weights.w;
    }
    
    weightBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:weights.data()
                       length:4 * 4 * vertexCount
                      options:MTLResourceCPUCacheModeDefaultCache];
    weightBuffer.label = @"Weight buffer";
    
    std::vector< int > bones( vertexCount * 4 );
    
    for (int v = 0; v < vertexCount; ++v)
    {
        bones[ v * 4 + 0 ] = vertices[ v ].bones[ 0 ];
        bones[ v * 4 + 1 ] = vertices[ v ].bones[ 1 ];
        bones[ v * 4 + 2 ] = vertices[ v ].bones[ 2 ];
        bones[ v * 4 + 3 ] = vertices[ v ].bones[ 3 ];
    }
    
    boneBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:bones.data()
                     length:4 * 4 * vertexCount
                    options:MTLResourceCPUCacheModeDefaultCache];
    boneBuffer.label = @"Bone buffer";
    
    indexBuffer = [GfxDevice::GetMetalDevice() newBufferWithBytes:faces
                      length:sizeof( Face ) * faceCount
                     options:MTLResourceCPUCacheModeDefaultCache];
    indexBuffer.label = @"Index buffer";
    
    elementCount = faceCount * 3;
    
    vertexBufferMemoryUsage += [vertexBuffer allocatedSize];
    vertexBufferMemoryUsage += [indexBuffer allocatedSize];
    vertexBufferMemoryUsage += [weightBuffer allocatedSize];
    vertexBufferMemoryUsage += [boneBuffer allocatedSize];
    vertexBufferMemoryUsage += [tangentBuffer allocatedSize];
    vertexBufferMemoryUsage += [normalBuffer allocatedSize];
    vertexBufferMemoryUsage += [texcoordBuffer allocatedSize];
    vertexBufferMemoryUsage += [colorBuffer allocatedSize];
}
