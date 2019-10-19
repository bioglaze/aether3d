// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "VertexBuffer.hpp"
#include <vector>
#include <cstring>
#include <cstdint>
#include "Array.hpp"
#include "Macros.hpp"
#include "Statistics.hpp"
#include "System.hpp"
#include "VulkanUtils.hpp"

namespace GfxDeviceGlobal
{
    extern VkDevice device;
    extern VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
    extern Array< VkBuffer > pendingFreeVBs;
    extern VkCommandPool cmdPool;
    extern VkQueue graphicsQueue;
}

namespace VertexBufferGlobal
{
    std::vector< VkBuffer > buffersToReleaseAtExit;
    std::vector< VkDeviceMemory > memoryToReleaseAtExit;
}

void ae3d::VertexBuffer::DestroyBuffers()
{
    for (std::size_t bufferIndex = 0; bufferIndex < VertexBufferGlobal::buffersToReleaseAtExit.size(); ++bufferIndex)
    {
        vkDestroyBuffer( GfxDeviceGlobal::device, VertexBufferGlobal::buffersToReleaseAtExit[ bufferIndex ], nullptr );
    }

    for (std::size_t memoryIndex = 0; memoryIndex < VertexBufferGlobal::memoryToReleaseAtExit.size(); ++memoryIndex)
    {
        vkFreeMemory( GfxDeviceGlobal::device, VertexBufferGlobal::memoryToReleaseAtExit[ memoryIndex ], nullptr );
    }
}

void ae3d::VertexBuffer::SetDebugName( const char* name )
{
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)vertexBuffer, VK_OBJECT_TYPE_BUFFER, name );
}

void CopyBuffer( VkBuffer source, VkBuffer& destination, int bufferSize )
{
    VkCommandBufferAllocateInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufInfo.commandPool = GfxDeviceGlobal::cmdPool;
    cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufInfo.commandBufferCount = 1;

    VkCommandBuffer copyCommandBuffer;
    VkResult err = vkAllocateCommandBuffers( GfxDeviceGlobal::device, &cmdBufInfo, &copyCommandBuffer );
    AE3D_CHECK_VULKAN( err, "copy command buffer" );

    VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufferBeginInfo.pNext = nullptr;

    VkBufferCopy copyRegion = {};
    copyRegion.size = bufferSize;

    err = vkBeginCommandBuffer( copyCommandBuffer, &cmdBufferBeginInfo );
    AE3D_CHECK_VULKAN( err, "begin staging copy" );

    vkCmdCopyBuffer( copyCommandBuffer, source, destination, 1, &copyRegion );

    err = vkEndCommandBuffer( copyCommandBuffer );
    AE3D_CHECK_VULKAN( err, "end staging copy" );

    VkSubmitInfo copySubmitInfo = {};
    copySubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    copySubmitInfo.commandBufferCount = 1;
    copySubmitInfo.pCommandBuffers = &copyCommandBuffer;

    err = vkQueueSubmit( GfxDeviceGlobal::graphicsQueue, 1, &copySubmitInfo, VK_NULL_HANDLE );
    AE3D_CHECK_VULKAN( err, "submit staging VB copy" );

    err = vkQueueWaitIdle( GfxDeviceGlobal::graphicsQueue );
    AE3D_CHECK_VULKAN( err, "wait after staging VB copy" );

    vkFreeCommandBuffers( GfxDeviceGlobal::device, cmdBufInfo.commandPool, 1, &copyCommandBuffer );
}

void CreateBuffer( VkBuffer& buffer, int bufferSize, VkDeviceMemory& memory, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryFlags, const char* debugName )
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = usageFlags;
    VkResult err = vkCreateBuffer( GfxDeviceGlobal::device, &bufferInfo, nullptr, &buffer );
    AE3D_CHECK_VULKAN( err, "vkCreateBuffer" );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)buffer, VK_OBJECT_TYPE_BUFFER, debugName );

    VkMemoryRequirements memReqs;
    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

    vkGetBufferMemoryRequirements( GfxDeviceGlobal::device, buffer, &memReqs );
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = ae3d::GetMemoryType( memReqs.memoryTypeBits, memoryFlags );
    err = vkAllocateMemory( GfxDeviceGlobal::device, &memAlloc, nullptr, &memory );
    AE3D_CHECK_VULKAN( err, "vkAllocateMemory" );
    Statistics::IncAllocCalls();
    Statistics::IncTotalAllocCalls();

    err = vkBindBufferMemory( GfxDeviceGlobal::device, buffer, memory, 0 );
    AE3D_CHECK_VULKAN( err, "vkBindBufferMemory" );
}

void MarkForFreeing( VkBuffer vertexBuffer, VkDeviceMemory vertexMem, VkBuffer indexBuffer, VkDeviceMemory indexMem )
{
    for (std::size_t memoryIndex = 0; memoryIndex < VertexBufferGlobal::memoryToReleaseAtExit.size(); ++memoryIndex)
    {
        if (VertexBufferGlobal::memoryToReleaseAtExit[ memoryIndex ] == vertexMem)
        {
            VertexBufferGlobal::memoryToReleaseAtExit.erase( std::begin( VertexBufferGlobal::memoryToReleaseAtExit ) + memoryIndex );
        }
    }

    for (std::size_t memoryIndex = 0; memoryIndex < VertexBufferGlobal::memoryToReleaseAtExit.size(); ++memoryIndex)
    {
        if (VertexBufferGlobal::memoryToReleaseAtExit[ memoryIndex ] == indexMem)
        {
            VertexBufferGlobal::memoryToReleaseAtExit.erase( std::begin( VertexBufferGlobal::memoryToReleaseAtExit ) + memoryIndex );
        }
    }

    for (std::size_t bufferIndex = 0; bufferIndex < VertexBufferGlobal::buffersToReleaseAtExit.size(); ++bufferIndex)
    {
        if (VertexBufferGlobal::buffersToReleaseAtExit[ bufferIndex ] == vertexBuffer)
        {
            VertexBufferGlobal::buffersToReleaseAtExit.erase( std::begin( VertexBufferGlobal::buffersToReleaseAtExit ) + bufferIndex );
        }
    }

    for (std::size_t bufferIndex = 0; bufferIndex < VertexBufferGlobal::buffersToReleaseAtExit.size(); ++bufferIndex)
    {
        if (VertexBufferGlobal::buffersToReleaseAtExit[ bufferIndex ] == indexBuffer)
        {
            VertexBufferGlobal::buffersToReleaseAtExit.erase( std::begin( VertexBufferGlobal::buffersToReleaseAtExit ) + bufferIndex );
        }
    }

    vkFreeMemory( GfxDeviceGlobal::device, vertexMem, nullptr );
    vkFreeMemory( GfxDeviceGlobal::device, indexMem, nullptr );
    GfxDeviceGlobal::pendingFreeVBs.Add( vertexBuffer );
    GfxDeviceGlobal::pendingFreeVBs.Add( indexBuffer );
}

void ae3d::VertexBuffer::GenerateVertexBuffer( const void* vertexData, int vertexBufferSize, int vertexStride, const void* indexData, int indexBufferSize )
{
    System::Assert( GfxDeviceGlobal::device != VK_NULL_HANDLE, "device not initialized" );
    System::Assert( vertexData != nullptr, "vertexData not initialized" );
    System::Assert( indexData != nullptr, "indexData not initialized" );

    if (vertexBuffer != VK_NULL_HANDLE)
    {
        MarkForFreeing( vertexBuffer, vertexMem, indexBuffer, indexMem );
    }

    // Vertex buffer
    const bool shouldCreateVertexBuffer = stagingBuffers.vertices.size != vertexBufferSize;

    if (shouldCreateVertexBuffer)
    {
        stagingBuffers.vertices.size = vertexBufferSize;
        CreateBuffer( stagingBuffers.vertices.buffer, vertexBufferSize, stagingBuffers.vertices.memory, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "staging vertex buffer" );
        VertexBufferGlobal::buffersToReleaseAtExit.push_back( stagingBuffers.vertices.buffer );
        VertexBufferGlobal::memoryToReleaseAtExit.push_back( stagingBuffers.vertices.memory );
    }
    
    void* bufferData = nullptr;
    VkResult err = vkMapMemory( GfxDeviceGlobal::device, stagingBuffers.vertices.memory, 0, vertexBufferSize, 0, &bufferData );
    AE3D_CHECK_VULKAN( err, "map vertex memory" );

    std::memcpy( bufferData, vertexData, vertexBufferSize );
    vkUnmapMemory( GfxDeviceGlobal::device, stagingBuffers.vertices.memory );

    {
        CreateBuffer( vertexBuffer, vertexBufferSize, vertexMem, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "vertex buffer" );
        VertexBufferGlobal::buffersToReleaseAtExit.push_back( vertexBuffer );
        VertexBufferGlobal::memoryToReleaseAtExit.push_back( vertexMem );
    }
    
    CopyBuffer( stagingBuffers.vertices.buffer, vertexBuffer, vertexBufferSize );

    // Index buffer
    const bool shouldCreateIndexBuffer = stagingBuffers.indices.size != indexBufferSize;
    
    if (shouldCreateIndexBuffer)
    {
        stagingBuffers.indices.size = indexBufferSize;
        CreateBuffer( stagingBuffers.indices.buffer, indexBufferSize, stagingBuffers.indices.memory, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "staging index buffer" );
        VertexBufferGlobal::buffersToReleaseAtExit.push_back( stagingBuffers.indices.buffer );
        VertexBufferGlobal::memoryToReleaseAtExit.push_back( stagingBuffers.indices.memory );
    }
    
    err = vkMapMemory( GfxDeviceGlobal::device, stagingBuffers.indices.memory, 0, indexBufferSize, 0, &bufferData );
    AE3D_CHECK_VULKAN( err, "map staging index memory" );

    std::memcpy( bufferData, indexData, indexBufferSize );
    vkUnmapMemory( GfxDeviceGlobal::device, stagingBuffers.indices.memory );

    {
        CreateBuffer( indexBuffer, indexBufferSize, indexMem, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "index buffer" );
        VertexBufferGlobal::buffersToReleaseAtExit.push_back( indexBuffer );
        VertexBufferGlobal::memoryToReleaseAtExit.push_back( indexMem );
    }
    
    CopyBuffer( stagingBuffers.indices.buffer, indexBuffer, indexBufferSize );

    CreateInputState( vertexStride );
}

void ae3d::VertexBuffer::CreateInputState( int vertexStride )
{
    bindingDescriptions.binding = VERTEX_BUFFER_BIND_ID;
    bindingDescriptions.stride = vertexStride;
    bindingDescriptions.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::uint32_t attributeCount = 0;

    if (vertexFormat == VertexFormat::PTC)
    {
		attributeCount = 3;

        // Location 0 : Position
        attributeDescriptions[ 0 ].binding = VERTEX_BUFFER_BIND_ID;
        attributeDescriptions[ 0 ].location = posChannel;
        attributeDescriptions[ 0 ].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[ 0 ].offset = 0;

        // Location 1 : TexCoord
        attributeDescriptions[ 1 ].binding = VERTEX_BUFFER_BIND_ID;
        attributeDescriptions[ 1 ].location = uvChannel;
        attributeDescriptions[ 1 ].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[ 1 ].offset = sizeof( float ) * 3;

        // Location 2 : Color
        attributeDescriptions[ 2 ].binding = VERTEX_BUFFER_BIND_ID;
        attributeDescriptions[ 2 ].location = colorChannel;
        attributeDescriptions[ 2 ].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[ 2 ].offset = sizeof( float ) * 5;
    }
    else if (vertexFormat == VertexFormat::PTN)
    {
		attributeCount = 3;

        // Location 0 : Position
        attributeDescriptions[ 0 ].binding = VERTEX_BUFFER_BIND_ID;
        attributeDescriptions[ 0 ].location = posChannel;
        attributeDescriptions[ 0 ].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[ 0 ].offset = 0;

        // Location 1 : TexCoord
        attributeDescriptions[ 1 ].binding = VERTEX_BUFFER_BIND_ID;
        attributeDescriptions[ 1 ].location = uvChannel;
        attributeDescriptions[ 1 ].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[ 1 ].offset = sizeof( float ) * 3;

        // Location 2 : Normal
        attributeDescriptions[ 2 ].binding = VERTEX_BUFFER_BIND_ID;
        attributeDescriptions[ 2 ].location = normalChannel;
        attributeDescriptions[ 2 ].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[ 2 ].offset = sizeof( float ) * 5;
    }
    else if (vertexFormat == VertexFormat::PTNTC)
    {
		attributeCount = 5;

        // Location 0 : Position
        attributeDescriptions[ 0 ].binding = VERTEX_BUFFER_BIND_ID;
        attributeDescriptions[ 0 ].location = posChannel;
        attributeDescriptions[ 0 ].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[ 0 ].offset = 0;

        // Location 1 : TexCoord
        attributeDescriptions[ 1 ].binding = VERTEX_BUFFER_BIND_ID;
        attributeDescriptions[ 1 ].location = uvChannel;
        attributeDescriptions[ 1 ].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[ 1 ].offset = sizeof( float ) * 3;

        // Location 2 : Normal
        attributeDescriptions[ 2 ].binding = VERTEX_BUFFER_BIND_ID;
        attributeDescriptions[ 2 ].location = normalChannel;
        attributeDescriptions[ 2 ].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[ 2 ].offset = sizeof( float ) * 5;

        // Location 3 : Tangent
        attributeDescriptions[ 3 ].binding = VERTEX_BUFFER_BIND_ID;
        attributeDescriptions[ 3 ].location = tangentChannel;
        attributeDescriptions[ 3 ].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[ 3 ].offset = sizeof( float ) * 8;

        // Location 4 : Color
        attributeDescriptions[ 4 ].binding = VERTEX_BUFFER_BIND_ID;
        attributeDescriptions[ 4 ].location = colorChannel;
        attributeDescriptions[ 4 ].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[ 4 ].offset = sizeof( float ) * 12;
    }
    else if (vertexFormat == VertexFormat::PTNTC_Skinned)
    {
		attributeCount = 7;

        // Location 0 : Position
        attributeDescriptions[ 0 ].binding = VERTEX_BUFFER_BIND_ID;
        attributeDescriptions[ 0 ].location = posChannel;
        attributeDescriptions[ 0 ].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[ 0 ].offset = 0;

        // Location 1 : TexCoord
        attributeDescriptions[ 1 ].binding = VERTEX_BUFFER_BIND_ID;
        attributeDescriptions[ 1 ].location = uvChannel;
        attributeDescriptions[ 1 ].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[ 1 ].offset = sizeof( float ) * 3;

        // Location 2 : Normal
        attributeDescriptions[ 2 ].binding = VERTEX_BUFFER_BIND_ID;
        attributeDescriptions[ 2 ].location = normalChannel;
        attributeDescriptions[ 2 ].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[ 2 ].offset = sizeof( float ) * 5;

        // Location 3 : Tangent
        attributeDescriptions[ 3 ].binding = VERTEX_BUFFER_BIND_ID;
        attributeDescriptions[ 3 ].location = tangentChannel;
        attributeDescriptions[ 3 ].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[ 3 ].offset = sizeof( float ) * 8;

        // Location 4 : Color
        attributeDescriptions[ 4 ].binding = VERTEX_BUFFER_BIND_ID;
        attributeDescriptions[ 4 ].location = colorChannel;
        attributeDescriptions[ 4 ].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[ 4 ].offset = sizeof( float ) * 12;

        // Location 5 : Weights
        attributeDescriptions[ 5 ].binding = VERTEX_BUFFER_BIND_ID;
        attributeDescriptions[ 5 ].location = 5;
        attributeDescriptions[ 5 ].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[ 5 ].offset = sizeof( float ) * 16;

        // Location 6 : Bones
        attributeDescriptions[ 6 ].binding = VERTEX_BUFFER_BIND_ID;
        attributeDescriptions[ 6 ].location = 6;
        attributeDescriptions[ 6 ].format = VK_FORMAT_R32G32B32A32_UINT;
        attributeDescriptions[ 6 ].offset = sizeof( float ) * 20;
    }
    else
    {
        System::Assert( false, "unhandled vertex format" );
    }

    inputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    inputStateCreateInfo.pNext = nullptr;
    inputStateCreateInfo.vertexBindingDescriptionCount = 1;
    inputStateCreateInfo.pVertexBindingDescriptions = &bindingDescriptions;
    inputStateCreateInfo.vertexAttributeDescriptionCount = attributeCount;
    inputStateCreateInfo.pVertexAttributeDescriptions = &attributeDescriptions[ 0 ];
}

void ae3d::VertexBuffer::GenerateDynamic( int faceCount, int vertexCount )
{
    vertexFormat = VertexFormat::PTNTC;
    elementCount = faceCount * 3;

    CreateBuffer( stagingBuffers.vertices.buffer, vertexCount * sizeof( VertexPTNTC ), stagingBuffers.vertices.memory, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "dynamic vertex buffer" );
    stagingBuffers.vertices.size = vertexCount * sizeof( VertexPTNTC );

    VkResult err = vkMapMemory( GfxDeviceGlobal::device, stagingBuffers.vertices.memory, 0, stagingBuffers.vertices.size, 0, &stagingBuffers.vertices.mappedData );
    AE3D_CHECK_VULKAN( err, "vkMapMemory GenerateDynamic" );

    CreateBuffer( stagingBuffers.indices.buffer, elementCount * 2, stagingBuffers.indices.memory, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "dynamic index buffer" );
    stagingBuffers.indices.size = elementCount * 2;

    err = vkMapMemory( GfxDeviceGlobal::device, stagingBuffers.indices.memory, 0, stagingBuffers.indices.size, 0, &stagingBuffers.indices.mappedData );
    AE3D_CHECK_VULKAN( err, "vkMapMemory GenerateDynamic" );

    vertexBuffer = stagingBuffers.vertices.buffer;
    indexBuffer = stagingBuffers.indices.buffer;

    verticesPTNTC.Allocate( vertexCount );

    CreateInputState( sizeof( VertexPTNTC ) );
}

void ae3d::VertexBuffer::UpdateDynamic( const Face* faces, int faceCount, const VertexPTC* vertices, int vertexCount )
{
    System::Assert( stagingBuffers.indices.mappedData != nullptr, "Index buffer not initialized!" );
    System::Assert( stagingBuffers.indices.buffer != VK_NULL_HANDLE, "Index buffer not initialized!" );
    System::Assert( stagingBuffers.indices.size >= faceCount * 3 * 2, "Index buffer too small!" );
    System::Assert( (std::size_t)stagingBuffers.vertices.size >= vertexCount * sizeof( VertexPTNTC ), "Vertex buffer too small!" );

    std::memcpy( stagingBuffers.indices.mappedData, faces, faceCount * 3 * 2 );

    for (unsigned vertexInd = 0; vertexInd < verticesPTNTC.count; ++vertexInd)
    {
        verticesPTNTC[ vertexInd ].position = vertices[ vertexInd ].position;
        verticesPTNTC[ vertexInd ].u = vertices[ vertexInd ].u;
        verticesPTNTC[ vertexInd ].v = vertices[ vertexInd ].v;
        verticesPTNTC[ vertexInd ].normal = Vec3( 0, 0, 1 );
        verticesPTNTC[ vertexInd ].tangent = Vec4( 1, 0, 0, 0 );
        verticesPTNTC[ vertexInd ].color = vertices[ vertexInd ].color;
    }

    std::memcpy( stagingBuffers.vertices.mappedData, verticesPTNTC.elements, vertexCount * sizeof( VertexPTNTC ) );
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTC* vertices, int vertexCount, Storage /*storage*/ )
{
    vertexFormat = VertexFormat::PTNTC;
    elementCount = faceCount * 3;

    Array< VertexPTNTC > verticesPTNTC2;
    verticesPTNTC2.Allocate( vertexCount );
    
    for (unsigned vertexInd = 0; vertexInd < verticesPTNTC2.count; ++vertexInd)
    {
        verticesPTNTC2[ vertexInd ].position = vertices[ vertexInd ].position;
        verticesPTNTC2[ vertexInd ].u = vertices[ vertexInd ].u;
        verticesPTNTC2[ vertexInd ].v = vertices[ vertexInd ].v;
        verticesPTNTC2[ vertexInd ].normal = Vec3( 0, 0, 1 );
        verticesPTNTC2[ vertexInd ].tangent = Vec4( 1, 0, 0, 0 );
        verticesPTNTC2[ vertexInd ].color = vertices[ vertexInd ].color;
    }

    GenerateVertexBuffer( static_cast< const void*>( verticesPTNTC2.elements ), vertexCount * sizeof( VertexPTNTC ), sizeof( VertexPTNTC ), static_cast< const void* >(faces), elementCount * 2 );
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTN* vertices, int vertexCount )
{
    vertexFormat = VertexFormat::PTNTC;
    elementCount = faceCount * 3;

    Array< VertexPTNTC > verticesPTNTC2;
    verticesPTNTC2.Allocate( vertexCount );

    for (unsigned vertexInd = 0; vertexInd < verticesPTNTC2.count; ++vertexInd)
    {
        verticesPTNTC2[ vertexInd ].position = vertices[ vertexInd ].position;
        verticesPTNTC2[ vertexInd ].u = vertices[ vertexInd ].u;
        verticesPTNTC2[ vertexInd ].v = vertices[ vertexInd ].v;
        verticesPTNTC2[ vertexInd ].normal = vertices[ vertexInd ].normal;
        verticesPTNTC2[ vertexInd ].tangent = Vec4( 1, 0, 0, 0 );
        verticesPTNTC2[ vertexInd ].color = Vec4( 1, 1, 1, 1 );
    }

    GenerateVertexBuffer( static_cast< const void*>( verticesPTNTC2.elements ), vertexCount * sizeof( VertexPTNTC ), sizeof( VertexPTNTC ), static_cast< const void* >( faces ), elementCount * 2 );
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTNTC* vertices, int vertexCount )
{
    vertexFormat = VertexFormat::PTNTC;
    elementCount = faceCount * 3;
    GenerateVertexBuffer( static_cast< const void*>( vertices ), vertexCount * sizeof( VertexPTNTC ), sizeof( VertexPTNTC ), static_cast< const void*>( faces ), elementCount * 2 );
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTNTC_Skinned* vertices, int vertexCount )
{
    vertexFormat = VertexFormat::PTNTC_Skinned;
    elementCount = faceCount * 3;
    GenerateVertexBuffer( static_cast< const void*>( vertices ), vertexCount * sizeof( VertexPTNTC_Skinned ), sizeof( VertexPTNTC_Skinned ), static_cast< const void*>( faces ), elementCount * 2 );
}
