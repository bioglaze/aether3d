#include "Shader.hpp"
#include "GfxDevice.hpp"
#include "FileSystem.hpp"
#include "Macros.hpp"
#include "Matrix.hpp"
#include "System.hpp"
#include "Texture2D.hpp"
#include "TextureCube.hpp"
#include "RenderTexture.hpp"
#include "VulkanUtils.hpp"
#include "Vec3.hpp"
#include <cstring>

constexpr int MaxModuleCount = 200;

namespace GfxDeviceGlobal
{
    extern VkDevice device;
    extern VkImageView boundViews[ 2 ];
    extern VkSampler boundSamplers[ 2 ];
	extern PerObjectUboStruct perObjectUboStruct;
    extern VkCommandBuffer texCmdBuffer;
}

namespace ShaderGlobal
{
    int moduleIndex = 0;
    VkShaderModule modulesToReleaseAtExit[ MaxModuleCount ];
}

void ae3d::Shader::DestroyShaders()
{
    for (int moduleIndex = 0; moduleIndex < ShaderGlobal::moduleIndex; ++moduleIndex)
    {
        vkDestroyShaderModule( GfxDeviceGlobal::device, ShaderGlobal::modulesToReleaseAtExit[ moduleIndex ], nullptr );
    }
}

void ae3d::Shader::Load( const char* /*vertexSource*/, const char* /*fragmentSource*/ )
{
}

void ae3d::Shader::LoadSPIRV( const FileSystem::FileContentsData& vertexData, const FileSystem::FileContentsData& fragmentData )
{
    System::Assert( GfxDeviceGlobal::device != VK_NULL_HANDLE, "device not initialized" );
    System::Assert( ShaderGlobal::moduleIndex + 2 < MaxModuleCount, "too many shader modules" );
    
    if (!vertexData.isLoaded || !fragmentData.isLoaded)
    {
        return;
    }

    // Vertex shader
    {
        std::uint32_t magic;
        std::memcpy( &magic, vertexData.data.data(), 4 );
        
        if (magic != 0x07230203)
        {
            System::Print( "Invalid magic in %s\n", vertexData.path.c_str() );
        }
        
        vertexPath = vertexData.path;

        VkShaderModuleCreateInfo moduleCreateInfo = {};
        moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.codeSize = vertexData.data.size();
        moduleCreateInfo.pCode = (const std::uint32_t*)vertexData.data.data();

        VkShaderModule shaderModule;
        VkResult err = vkCreateShaderModule( GfxDeviceGlobal::device, &moduleCreateInfo, nullptr, &shaderModule );
        AE3D_CHECK_VULKAN( err, "vkCreateShaderModule vertex" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)shaderModule, VK_OBJECT_TYPE_SHADER_MODULE, vertexData.path.c_str() );
        ShaderGlobal::modulesToReleaseAtExit[ ShaderGlobal::moduleIndex++ ] = shaderModule;

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
        std::uint32_t magic;
        std::memcpy( &magic, fragmentData.data.data(), 4 );
        
        if (magic != 0x07230203)
        {
            System::Print( "Invalid magic in %s\n", fragmentData.path.c_str() );
        }

        fragmentPath = fragmentData.path;

        VkShaderModuleCreateInfo moduleCreateInfo = {};
        moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.codeSize = fragmentData.data.size();
        moduleCreateInfo.pCode = (const std::uint32_t*)fragmentData.data.data();

        VkShaderModule shaderModule;
        VkResult err = vkCreateShaderModule( GfxDeviceGlobal::device, &moduleCreateInfo, nullptr, &shaderModule );
        AE3D_CHECK_VULKAN( err, "vkCreateShaderModule vertex" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)shaderModule, VK_OBJECT_TYPE_SHADER_MODULE, fragmentData.path.c_str() );
        ShaderGlobal::modulesToReleaseAtExit[ ShaderGlobal::moduleIndex++ ] = shaderModule;

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

void ae3d::Shader::Load( const char* /*metalVertexShaderName*/, const char* /*metalFragmentShaderName*/,
    const FileSystem::FileContentsData& /*vertexHLSL*/, const FileSystem::FileContentsData& /*fragmentHLSL*/,
    const FileSystem::FileContentsData& vertexDataSPIRV, const FileSystem::FileContentsData& fragmentDataSPIRV )
{
    LoadSPIRV( vertexDataSPIRV, fragmentDataSPIRV );
}

void ae3d::Shader::Use()
{
    System::Assert( IsValid(), "no valid shader" );
    GfxDevice::GetNewUniformBuffer();
}

void ae3d::Shader::SetMatrix( const char* /*name*/, const float* matrix4x4 )
{
    System::Assert( GfxDevice::GetCurrentUbo() != nullptr, "null ubo" );
    std::memcpy( &GfxDevice::GetCurrentUbo()[ 0 ], &matrix4x4[0], sizeof( Matrix44 ) );
}

void ae3d::Shader::SetMatrixArray( const char* /*name*/, const float* matrix4x4s, int count )
{
    System::Assert( GfxDevice::GetCurrentUbo() != nullptr, "null ubo" );
    std::memcpy( &GfxDevice::GetCurrentUbo()[ 0 ], &matrix4x4s[ 0 ], sizeof( Matrix44 ) * count );
}

void ae3d::Shader::SetUniform( int offset, void* data, int dataBytes )
{
    System::Assert( GfxDevice::GetCurrentUbo() != nullptr, "null ubo" );
    std::memcpy( &GfxDevice::GetCurrentUbo()[ offset ], data, dataBytes );
}

void ae3d::Shader::SetTexture( const char* /*name*/, Texture2D* texture, int textureUnit )
{
    if (texture == nullptr)
    {
        return;
    }
    
    if (textureUnit == 0)
    {
		GfxDeviceGlobal::perObjectUboStruct.tex0scaleOffset = texture->GetScaleOffset();
        GfxDeviceGlobal::boundViews[ 0 ] = texture->GetView();
        GfxDeviceGlobal::boundSamplers[ 0 ] = texture->GetSampler();

        /*VkCommandBufferBeginInfo cmdBufInfo = {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkResult err = vkBeginCommandBuffer( GfxDeviceGlobal::texCmdBuffer, &cmdBufInfo );
        AE3D_CHECK_VULKAN( err, "vkBeginCommandBuffer in Texture2D" );

        SetImageLayout(
            GfxDeviceGlobal::texCmdBuffer,
            texture->GetImage(),
            VK_IMAGE_ASPECT_COLOR_BIT, texture->layout,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 0, 1 );
            vkEndCommandBuffer( GfxDeviceGlobal::texCmdBuffer );*/
    }
    else if (textureUnit == 1)
    {
        GfxDeviceGlobal::boundViews[ 1 ] = texture->GetView();
        GfxDeviceGlobal::boundSamplers[ 1 ] = texture->GetSampler();
    }
    else
    {
        System::Print( "Shader tries to set a texture to too high unit %d\n", textureUnit );
    }
}

void ae3d::Shader::SetTexture( const char* /*name*/, TextureCube* texture, int textureUnit )
{
    if (texture == nullptr)
    {
        return;
    }
    
    if (textureUnit == 0)
    {
        GfxDeviceGlobal::boundViews[ 0 ] = texture->GetView();
        GfxDeviceGlobal::boundSamplers[ 0 ] = texture->GetSampler();
    }
    else if (textureUnit == 1)
    {
        GfxDeviceGlobal::boundViews[ 1 ] = texture->GetView();
        GfxDeviceGlobal::boundSamplers[ 1 ] = texture->GetSampler();
    }
    else
    {
        System::Print( "Shader tries to set a texture to too high unit %d\n", textureUnit );
    }
}

void ae3d::Shader::SetRenderTexture( const char* /*name*/, RenderTexture* texture, int textureUnit )
{
    if (texture == nullptr)
    {
        return;
    }
    
    if (textureUnit == 0)
    {
        GfxDeviceGlobal::boundViews[ 0 ] = texture->GetColorView();
        GfxDeviceGlobal::boundSamplers[ 0 ] = texture->GetSampler();
    }
    else if (textureUnit == 1)
    {
        GfxDeviceGlobal::boundViews[ 1 ] = texture->GetColorView();
        GfxDeviceGlobal::boundSamplers[ 1 ] = texture->GetSampler();
    }
    else
    {
        System::Print( "Shader tries to set a texture to too high unit %d\n", textureUnit );
    }

}

void ae3d::Shader::SetInt( const char* /*name*/, int /*value*/ )
{
}

void ae3d::Shader::SetFloat( const char* /*name*/, float /*value*/ )
{
}

void ae3d::Shader::SetVector3( const char* /*name*/, const float* /*vec3*/ )
{
}

void ae3d::Shader::SetVector4( const char* /*name*/, const float* /*vec4*/ )
{
}
