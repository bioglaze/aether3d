#include "VertexBuffer.hpp"
#include "System.hpp"

namespace GfxDeviceGlobal
{
    extern VkDevice device;
    extern VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
}

namespace ae3d
{
    void CheckVulkanResult( VkResult result, const char* message ); // Defined in GfxDeviceVulkan.cpp 
    
    VkBool32 GetMemoryType( const VkPhysicalDeviceMemoryProperties& deviceMemoryProperties, uint32_t typeBits, VkFlags properties, uint32_t * typeIndex )
    {
        for (int i = 0; i < 32; i++)
        {
            if ((typeBits & 1) == 1)
            {
                if ((deviceMemoryProperties.memoryTypes[ i ].propertyFlags & properties) == properties)
                {
                    *typeIndex = i;
                    return true;
                }
            }
            typeBits >>= 1;
        }
        return false;
    }
}

void ae3d::VertexBuffer::GenerateVertexBuffer( void* vertexData, int vertexBufferSize, int vertexStride, void* indexData, int indexBufferSize )
{
    System::Assert( GfxDeviceGlobal::device != VK_NULL_HANDLE, "device not initialized" );
    System::Assert( vertexData != nullptr, "vertexData not initialized" );
    System::Assert( indexData != nullptr, "indexData not initialized" );

    VkBufferCreateInfo bufCreateInfo = {};
    bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufCreateInfo.pNext = nullptr;
    bufCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufCreateInfo.size = vertexBufferSize;
    bufCreateInfo.flags = 0;
    VkResult err = vkCreateBuffer( GfxDeviceGlobal::device, &bufCreateInfo, nullptr, &vertexBuffer );
    CheckVulkanResult( err, "vkCreateBuffer" );

    VkMemoryAllocateInfo memAllocInfo = {};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.pNext = nullptr;
    memAllocInfo.allocationSize = 0;
    memAllocInfo.memoryTypeIndex = 0;

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements( GfxDeviceGlobal::device, vertexBuffer, &memReqs );
    memAllocInfo.allocationSize = memReqs.size;
    GetMemoryType( GfxDeviceGlobal::deviceMemoryProperties, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAllocInfo.memoryTypeIndex );
    err = vkAllocateMemory( GfxDeviceGlobal::device, &memAllocInfo, nullptr, &vertexMem );
    CheckVulkanResult( err, "vkAllocateMemory" );

    void* data = nullptr;
    err = vkMapMemory( GfxDeviceGlobal::device, vertexMem, 0, vertexBufferSize, 0, &data );
    CheckVulkanResult( err, "vkAllocateMemory" );

    memcpy( data, vertexData, vertexBufferSize );
    
    vkUnmapMemory( GfxDeviceGlobal::device, vertexMem );
    err = vkBindBufferMemory( GfxDeviceGlobal::device, vertexBuffer, vertexMem, 0 );
    CheckVulkanResult( err, "vkBindBufferMemory" );

    VkBufferCreateInfo ibCreateInfo = {};
    ibCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    ibCreateInfo.pNext = nullptr;
    ibCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    ibCreateInfo.size = indexBufferSize;
    ibCreateInfo.flags = 0;
    err = vkCreateBuffer( GfxDeviceGlobal::device, &ibCreateInfo, nullptr, &indexBuffer );
    CheckVulkanResult( err, "vkCreateBuffer" );

    vkGetBufferMemoryRequirements( GfxDeviceGlobal::device, indexBuffer, &memReqs );
    memAllocInfo.allocationSize = memReqs.size;
    GetMemoryType( GfxDeviceGlobal::deviceMemoryProperties, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAllocInfo.memoryTypeIndex );
    err = vkAllocateMemory( GfxDeviceGlobal::device, &memAllocInfo, nullptr, &indexMem );
    CheckVulkanResult( err, "vkAllocateMemory" );

    err = vkMapMemory( GfxDeviceGlobal::device, indexMem, 0, indexBufferSize, 0, &data );
    CheckVulkanResult( err, "vkMapMemory" );

    memcpy( data, indexData, indexBufferSize );
    vkUnmapMemory( GfxDeviceGlobal::device, indexMem );
    err = vkBindBufferMemory( GfxDeviceGlobal::device, indexBuffer, indexMem, 0 );
    CheckVulkanResult( err, "vkBindBufferMemory index buffer" );

    // Binding description
    bindingDescriptions.resize( 1 );
    bindingDescriptions[ 0 ].binding = VERTEX_BUFFER_BIND_ID;
    bindingDescriptions[ 0 ].stride = vertexStride;
    bindingDescriptions[ 0 ].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // Attribute descriptions
    // Describes memory layout and shader attribute locations
    attributeDescriptions.resize( 2 );
    // Location 0 : Position
    attributeDescriptions[ 0 ].binding = VERTEX_BUFFER_BIND_ID;
    attributeDescriptions[ 0 ].location = 0;
    attributeDescriptions[ 0 ].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[ 0 ].offset = 0;
    attributeDescriptions[ 0 ].binding = 0;
    // Location 1 : Color
    attributeDescriptions[ 1 ].binding = VERTEX_BUFFER_BIND_ID;
    attributeDescriptions[ 1 ].location = 1;
    attributeDescriptions[ 1 ].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[ 1 ].offset = sizeof( float ) * 3;
    attributeDescriptions[ 1 ].binding = 0;

    inputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    inputStateCreateInfo.pNext = nullptr;
    inputStateCreateInfo.vertexBindingDescriptionCount = (uint32_t)bindingDescriptions.size();
    inputStateCreateInfo.pVertexBindingDescriptions = bindingDescriptions.data();
    inputStateCreateInfo.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptions.size();
    inputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTC* vertices, int vertexCount )
{
    vertexFormat = VertexFormat::PTC;
    elementCount = faceCount * 3;
    //GenerateVertexBuffer( (void*)vertices, vertexCount * sizeof( VertexPTC ), sizeof( VertexPTC ), (void*)faces, elementCount * sizeof( Face ) );
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTN* vertices, int vertexCount )
{
    vertexFormat = VertexFormat::PTN;
    elementCount = faceCount * 3;
    //GenerateVertexBuffer( (void*)vertices, vertexCount * sizeof( VertexPTN ), sizeof( VertexPTN ), (void*)faces, elementCount * sizeof( Face ) );
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTNTC* vertices, int vertexCount )
{
    vertexFormat = VertexFormat::PTNTC;
    elementCount = faceCount * 3;
    //GenerateVertexBuffer( (void*)vertices, vertexCount * sizeof( VertexPTNTC ), sizeof( VertexPTNTC ), (void*)faces, elementCount * sizeof( Face ) );
}
