#include "Shader.hpp"
#include "GfxDevice.hpp"
#include "FileSystem.hpp"
#include "Macros.hpp"
#include "Matrix.hpp"
#include "System.hpp"
#include "Vec3.hpp"
#include <cstring>

namespace GfxDeviceGlobal
{
    extern VkDevice device;
    extern ae3d::Texture2D* texture2d0;
    extern ae3d::TextureCube* textureCube0;
}

namespace ae3d
{
    void GetMemoryType( std::uint32_t typeBits, VkFlags properties, std::uint32_t* typeIndex ); // Defined in GfxDeviceVulkan.cpp 
}

void ae3d::Shader::Load( const char* /*vertexSourceGLSL*/, const char* /*fragmentSourceGLSL*/ )
{
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
        AE3D_CHECK_VULKAN( err, "vkCreateShaderModule vertex" );

        vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexInfo.module = shaderModule;
        vertexInfo.pName = "main";
        vertexInfo.pNext = nullptr;
        vertexInfo.flags = 0;
        vertexInfo.pSpecializationInfo = nullptr;

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
        AE3D_CHECK_VULKAN( err, "vkCreateShaderModule vertex" );

        fragmentInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragmentInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentInfo.module = shaderModule;
        fragmentInfo.pName = "main";
        fragmentInfo.pNext = nullptr;
        fragmentInfo.flags = 0;
        fragmentInfo.pSpecializationInfo = nullptr;

        System::Assert( fragmentInfo.module != VK_NULL_HANDLE, "fragment shader module not created" );
    }
}

void ae3d::Shader::Load( const FileSystem::FileContentsData& /*vertexGLSL*/, const FileSystem::FileContentsData& /*fragmentGLSL*/,
    const char* /*metalVertexShaderName*/, const char* /*metalFragmentShaderName*/,
    const FileSystem::FileContentsData& /*vertexHLSL*/, const FileSystem::FileContentsData& /*fragmentHLSL*/,
    const FileSystem::FileContentsData& vertexDataSPIRV, const FileSystem::FileContentsData& fragmentDataSPIRV )
{
    LoadSPIRV( vertexDataSPIRV, fragmentDataSPIRV );
}

void ae3d::Shader::Use()
{
    System::Assert( IsValid(), "no valid shader" );
    GfxDevice::CreateNewUniformBuffer();
}

void ae3d::Shader::SetMatrix( const char* name, const float* matrix4x4 )
{
    System::Assert( GfxDevice::GetCurrentUbo() != nullptr, "null ubo" );
    std::memcpy( &GfxDevice::GetCurrentUbo()[ 0 ], &matrix4x4[0], sizeof( Matrix44 ) );
}

void ae3d::Shader::SetTexture( const char* name, const ae3d::Texture2D* texture, int textureUnit )
{
    GfxDeviceGlobal::textureCube0 = nullptr;
    GfxDeviceGlobal::texture2d0 = const_cast< Texture2D* >(texture);
}

void ae3d::Shader::SetTexture( const char* name, const ae3d::TextureCube* texture, int textureUnit )
{
    GfxDeviceGlobal::texture2d0 = nullptr;
    GfxDeviceGlobal::textureCube0 = const_cast< TextureCube* >(texture);
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
