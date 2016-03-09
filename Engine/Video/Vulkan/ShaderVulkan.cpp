#include "Shader.hpp"
#include "FileSystem.hpp"
#include "Matrix.hpp"
#include "System.hpp"
#include "Vec3.hpp"

namespace GfxDeviceGlobal
{
    extern VkDevice device;
    extern ae3d::Texture2D* texture0;
}

namespace ae3d
{
    void CheckVulkanResult( VkResult result, const char* message ); // Defined in GfxDeviceVulkan.cpp 
    VkBool32 GetMemoryType( std::uint32_t typeBits, VkFlags properties, std::uint32_t* typeIndex ); // Defined in GfxDeviceVulkan.cpp 
}

void ae3d::Shader::Load( const char* vertexSource, const char* fragmentSource )
{
    CreateConstantBuffer();
}

void ae3d::Shader::LoadSPIRV( const FileSystem::FileContentsData& vertexData, const FileSystem::FileContentsData& fragmentData )
{
    // Vertex shader
    {
        VkShaderModuleCreateInfo moduleCreateInfo;
        moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.pNext = nullptr;
        moduleCreateInfo.codeSize = vertexData.data.size();
        moduleCreateInfo.pCode = (std::uint32_t*)vertexData.data.data();
        moduleCreateInfo.flags = 0;

        VkShaderModule shaderModule;
        VkResult err = vkCreateShaderModule( GfxDeviceGlobal::device, &moduleCreateInfo, nullptr, &shaderModule );
        CheckVulkanResult( err, "vkCreateShaderModule vertex" );

        vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexInfo.module = shaderModule;
        vertexInfo.pName = "main";
        System::Assert( vertexInfo.module != VK_NULL_HANDLE, "vertex shader module not created" );
    }

    // Fragment shader
    {
        VkShaderModuleCreateInfo moduleCreateInfo;
        moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.pNext = nullptr;
        moduleCreateInfo.codeSize = fragmentData.data.size();
        moduleCreateInfo.pCode = (std::uint32_t*)fragmentData.data.data();
        moduleCreateInfo.flags = 0;

        VkShaderModule shaderModule;
        VkResult err = vkCreateShaderModule( GfxDeviceGlobal::device, &moduleCreateInfo, nullptr, &shaderModule );
        CheckVulkanResult( err, "vkCreateShaderModule vertex" );

        fragmentInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragmentInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentInfo.module = shaderModule;
        fragmentInfo.pName = "main";
        System::Assert( fragmentInfo.module != VK_NULL_HANDLE, "fragment shader module not created" );
    }

    CreateConstantBuffer();
}

void ae3d::Shader::Load( const FileSystem::FileContentsData& /*vertexGLSL*/, const FileSystem::FileContentsData& /*fragmentGLSL*/,
    const char* /*metalVertexShaderName*/, const char* /*metalFragmentShaderName*/,
    const FileSystem::FileContentsData& /*vertexHLSL*/, const FileSystem::FileContentsData& /*fragmentHLSL*/,
    const FileSystem::FileContentsData& vertexDataSPIRV, const FileSystem::FileContentsData& fragmentDataSPIRV )
{
    LoadSPIRV( vertexDataSPIRV, fragmentDataSPIRV );
}

void ae3d::Shader::CreateConstantBuffer()
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof( Matrix44 );
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    VkResult err = vkCreateBuffer( GfxDeviceGlobal::device, &bufferInfo, nullptr, &ubo );
    CheckVulkanResult( err, "vkCreateBuffer UBO" );

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements( GfxDeviceGlobal::device, ubo, &memReqs );

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.allocationSize = memReqs.size;
    VkBool32 res = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &allocInfo.memoryTypeIndex );
    System::Assert( res == VK_TRUE, "UBO: could not get memory type" );
    err = vkAllocateMemory( GfxDeviceGlobal::device, &allocInfo, nullptr, &uboMemory );
    CheckVulkanResult( err, "vkAllocateMemory UBO" );

    err = vkBindBufferMemory( GfxDeviceGlobal::device, ubo, uboMemory, 0 );
    CheckVulkanResult( err, "vkBindBufferMemory UBO" );

    uboDesc.buffer = ubo;
    uboDesc.offset = 0;
    uboDesc.range = sizeof( Matrix44 );
}

void ae3d::Shader::UpdateUniformBuffers()
{
    ae3d::System::Assert( uboMemory != VK_NULL_HANDLE, "ubo memory not allocated" );

    uint8_t *pData;
    VkResult err = vkMapMemory( GfxDeviceGlobal::device, uboMemory, 0, sizeof( Matrix44 ), 0, (void **)&pData );
    CheckVulkanResult( err, "vkMapMemory UBO" );

    memcpy( pData, &tempMat4[ 0 ], sizeof( Matrix44 ) );
    vkUnmapMemory( GfxDeviceGlobal::device, uboMemory );
}

void ae3d::Shader::Use()
{
}

void ae3d::Shader::SetMatrix( const char* name, const float* matrix4x4 )
{
    memcpy( &tempMat4[ 0 ], &matrix4x4[0], sizeof( Matrix44 ) );
    UpdateUniformBuffers();
}

void ae3d::Shader::SetTexture( const char* name, const ae3d::Texture2D* texture, int textureUnit )
{
    GfxDeviceGlobal::texture0 = const_cast< Texture2D* >(texture);
}

void ae3d::Shader::SetTexture( const char* name, const ae3d::TextureCube* texture, int textureUnit )
{
}

void ae3d::Shader::SetRenderTexture( const char* name, const ae3d::RenderTexture* texture, int textureUnit )
{
}

void ae3d::Shader::SetInt( const char* name, int value )
{
}

void ae3d::Shader::SetFloat( const char* name, float value )
{
}

void ae3d::Shader::SetVector3( const char* name, const float* vec3 )
{
}

void ae3d::Shader::SetVector4( const char* name, const float* vec4 )
{
}
