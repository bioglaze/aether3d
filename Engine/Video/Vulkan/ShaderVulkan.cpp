// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "Shader.hpp"
#include "Array.hpp"
#include "GfxDevice.hpp"
#include "FileSystem.hpp"
#include "FileWatcher.hpp"
#include "Macros.hpp"
#include "Matrix.hpp"
#include "System.hpp"
#include "Texture2D.hpp"
#include "TextureCube.hpp"
#include "RenderTexture.hpp"
#include "VulkanUtils.hpp"
#include "Vec3.hpp"
#include <cstring>

extern ae3d::FileWatcher fileWatcher;

constexpr int MaxModuleCount = 600;

namespace GfxDeviceGlobal
{
    extern VkDevice device;
    extern VkImageView boundViews[ 13 ];
    extern VkSampler boundSamplers[ 2 ];
	extern PerObjectUboStruct perObjectUboStruct;
    extern VkCommandBuffer texCmdBuffer;
    extern ae3d::RenderTexture* renderTexture0;
}

namespace ShaderGlobal
{
    int moduleIndex = 0;
    VkShaderModule modulesToReleaseAtExit[ MaxModuleCount ];
}

struct ShaderCacheEntry
{
    ShaderCacheEntry() {}
    ShaderCacheEntry( const std::string& vp, const std::string& fp, ae3d::Shader* aShader )
        : vertexPath( vp )
        , fragmentPath( fp )
        , shader( aShader ) {}

    std::string vertexPath;
    std::string fragmentPath;
    ae3d::Shader* shader = nullptr;
};

Array< ShaderCacheEntry > cacheEntries;

void ShaderReload( const std::string& path )
{
    ae3d::System::Print("Reloading shader %s\n", path.c_str());

    for (unsigned i = 0; i < cacheEntries.count; ++i)
    {
        if (cacheEntries[ i ].vertexPath == path || cacheEntries[ i ].fragmentPath == path)
        {
            cacheEntries[ i ].shader->LoadSPIRV( ae3d::FileSystem::FileContents( cacheEntries[ i ].vertexPath.c_str() ), ae3d::FileSystem::FileContents( cacheEntries[ i ].fragmentPath.c_str() ) );
        }
    }

    ae3d::GfxDevice::ResetPSOCache();
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

    const bool addToCache = ( fragmentInfo.module == VK_NULL_HANDLE || vertexInfo.module == VK_NULL_HANDLE );

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

    if( addToCache )
    {
        fileWatcher.AddFile( vertexData.path, ShaderReload );
        fileWatcher.AddFile( fragmentData.path, ShaderReload );
        ShaderCacheEntry entry{ vertexData.path, fragmentData.path, this };
        cacheEntries.Add( entry );
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

void ae3d::Shader::SetUniform( int offset, void* data, int dataBytes )
{
    System::Assert( GfxDevice::GetCurrentUbo() != nullptr, "null ubo" );
    std::memcpy( &GfxDevice::GetCurrentUbo()[ offset ], data, dataBytes );
}

void ae3d::Shader::SetTexture( Texture2D* texture, int textureUnit )
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
    }
    else if (textureUnit == 1)
    {
        GfxDeviceGlobal::boundViews[ 1 ] = texture->GetView();
        GfxDeviceGlobal::boundSamplers[ 1 ] = texture->GetSampler();
    }
    else if (textureUnit == 2)
    {
        GfxDeviceGlobal::boundViews[ 2 ] = texture->GetView();
        GfxDeviceGlobal::boundSamplers[ 1 ] = texture->GetSampler(); // TODO: Add sampler2 to shader.
    }
    else if (textureUnit == 3)
    {
        GfxDeviceGlobal::boundViews[ 3 ] = texture->GetView();
        GfxDeviceGlobal::boundSamplers[ 1 ] = texture->GetSampler(); // TODO: Add sampler3 to shader.
    }
    else
    {
        System::Print( "Shader tries to set a texture2d to too high unit %d\n", textureUnit );
    }
}

void ae3d::Shader::SetTexture( TextureCube* texture, int textureUnit )
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
    else if (textureUnit == 4)
    {
        GfxDeviceGlobal::boundViews[ 4 ] = texture->GetView();
        //GfxDeviceGlobal::boundSamplers[ 1 ] = texture->GetSampler();
    }
    else
    {
        System::Print( "Shader tries to set a textureCube to too high unit %d\n", textureUnit );
    }
}

void ae3d::Shader::SetRenderTexture( RenderTexture* texture, int textureUnit )
{
    if (texture == nullptr)
    {
        return;
    }

    // Prevents feedback.
    if (texture == GfxDeviceGlobal::renderTexture0)
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
    else if (textureUnit == 2)
    {
        GfxDeviceGlobal::boundViews[ 2 ] = texture->GetColorView();
        GfxDeviceGlobal::boundSamplers[ 1 ] = texture->GetSampler(); // TODO: Add sampler2 to shader.
    }
    else if (textureUnit == 3)
    {
        GfxDeviceGlobal::boundViews[ 3 ] = texture->GetColorView();
        GfxDeviceGlobal::boundSamplers[ 1 ] = texture->GetSampler(); // TODO: Add sampler3 to shader.
    }
    else if (textureUnit == 4)
    {
        if (!texture->IsCube())
        {
            System::Print( "Shader::SetRenderTexture: Texture set to slot 4 must be a cube map!\n" );
        }
        else
        {
            GfxDeviceGlobal::boundViews[ 4 ] = texture->GetColorView();
            GfxDeviceGlobal::boundSamplers[ 1 ] = texture->GetSampler(); // TODO: Add sampler4 to shader.
        }
    }
    else
    {
        System::Print( "Shader tries to set a render texture to too high unit %d\n", textureUnit );
    }
}
