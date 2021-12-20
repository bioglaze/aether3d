// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "ComputeShader.hpp"
#include "Array.hpp"
#include "FileSystem.hpp"
#include "FileWatcher.hpp"
#include "GfxDevice.hpp"
#include "Macros.hpp"
#include "RenderTexture.hpp"
#include "System.hpp"
#include "Statistics.hpp"
#include "Texture2D.hpp"
#include "VulkanUtils.hpp"

extern ae3d::FileWatcher fileWatcher;

void BindComputeDescriptorSet();
void UploadPerObjectUbo();

namespace GfxDeviceGlobal
{
    extern VkDevice device;
    extern VkQueue computeQueue;
    extern VkCommandBuffer computeCmdBuffer;
    extern VkPipelineLayout pipelineLayout;
    extern VkPipelineCache pipelineCache;
    extern PerObjectUboStruct perObjectUboStruct;
    extern VkImageView boundViews[ ae3d::ComputeShader::SLOT_COUNT ];
}

namespace ComputeShaderGlobal
{
    Array< VkShaderModule > modulesToReleaseAtExit;
    Array< VkPipeline > psosToReleaseAtExit;
}

struct ComputeShaderCacheEntry
{
    ComputeShaderCacheEntry() {}
    ComputeShaderCacheEntry( const std::string& aPath, ae3d::ComputeShader* aShader ) : path( aPath ), shader( aShader ) {}

    std::string path;
    ae3d::ComputeShader* shader = nullptr;
};

Array< ComputeShaderCacheEntry > computeCacheEntries;

void ComputeShaderReload( const std::string& path )
{
    ae3d::System::Print( "Reloading shader %s\n", path.c_str() );

    for (unsigned i = 0; i < computeCacheEntries.count; ++i)
    {
        if (computeCacheEntries[ i ].path == path)
        {
            computeCacheEntries[ i ].shader->LoadSPIRV( ae3d::FileSystem::FileContents( computeCacheEntries[ i ].path.c_str() ) );
        }
    }

    ae3d::GfxDevice::ResetPSOCache();
}

void ae3d::ComputeShader::DestroyShaders()
{
    for (unsigned moduleIndex = 0; moduleIndex < ComputeShaderGlobal::modulesToReleaseAtExit.count; ++moduleIndex)
    {
        vkDestroyShaderModule( GfxDeviceGlobal::device, ComputeShaderGlobal::modulesToReleaseAtExit[ moduleIndex ], nullptr );
    }

    for (unsigned psoIndex = 0; psoIndex < ComputeShaderGlobal::psosToReleaseAtExit.count; ++psoIndex)
    {
        vkDestroyPipeline( GfxDeviceGlobal::device, ComputeShaderGlobal::psosToReleaseAtExit[ psoIndex ], nullptr );
    }
}

void ae3d::ComputeShader::Load( const char* /*source*/ )
{

}

void ae3d::ComputeShader::Load( const char* /*metalShaderName*/, const FileSystem::FileContentsData& /*dataHLSL*/, const FileSystem::FileContentsData& dataSPIRV )
{
    LoadSPIRV( dataSPIRV );
}

void ae3d::ComputeShader::LoadSPIRV( const ae3d::FileSystem::FileContentsData& contents )
{
    const bool addToCache = info.module == VK_NULL_HANDLE;

    VkShaderModuleCreateInfo moduleCreateInfo;
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = nullptr;
    moduleCreateInfo.codeSize = contents.data.size();
    moduleCreateInfo.pCode = (const std::uint32_t*)contents.data.data();
    moduleCreateInfo.flags = 0;

    VkShaderModule shaderModule;
    VkResult err = vkCreateShaderModule( GfxDeviceGlobal::device, &moduleCreateInfo, nullptr, &shaderModule );
    AE3D_CHECK_VULKAN( err, "vkCreateShaderModule compute" );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)shaderModule, VK_OBJECT_TYPE_SHADER_MODULE, contents.path.c_str() );
    ComputeShaderGlobal::modulesToReleaseAtExit.Add( shaderModule );

    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    info.module = shaderModule;
    info.pName = "CSMain";
    info.pNext = nullptr;
    info.flags = 0;
    info.pSpecializationInfo = nullptr;

    System::Assert( info.module != VK_NULL_HANDLE, "compute shader module not created" );

    VkComputePipelineCreateInfo psoInfo = {};
    psoInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    psoInfo.layout = GfxDeviceGlobal::pipelineLayout;
    psoInfo.stage = GetInfo();

    err = vkCreateComputePipelines( GfxDeviceGlobal::device, GfxDeviceGlobal::pipelineCache, 1, &psoInfo, nullptr, &pso );
    AE3D_CHECK_VULKAN( err, "Compute PSO" );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)pso, VK_OBJECT_TYPE_PIPELINE, "light tiler PSO" );
    ComputeShaderGlobal::psosToReleaseAtExit.Add( pso );

    if( addToCache )
    {
        fileWatcher.AddFile( contents.path, ComputeShaderReload );
        ComputeShaderCacheEntry entry{ contents.path, this };
        computeCacheEntries.Add( entry );
    }
}

void ae3d::ComputeShader::SetProjectionMatrix( const struct Matrix44& projection )
{
    GfxDeviceGlobal::perObjectUboStruct.viewToClip = projection;
    Matrix44::Invert( GfxDeviceGlobal::perObjectUboStruct.viewToClip, GfxDeviceGlobal::perObjectUboStruct.clipToView );
}

void ae3d::ComputeShader::SetUniform( UniformName uniform, float x, float y )
{
    if( uniform == UniformName::TilesZW )
    {
        GfxDeviceGlobal::perObjectUboStruct.tilesXY.z = x;
        GfxDeviceGlobal::perObjectUboStruct.tilesXY.w = y;
    }
    else if (uniform == UniformName::BloomThreshold)
    {
        GfxDeviceGlobal::perObjectUboStruct.bloomThreshold = x;
    }
    else if (uniform == UniformName::BloomIntensity)
    {
        GfxDeviceGlobal::perObjectUboStruct.bloomIntensity = x;
    }
}

void ae3d::ComputeShader::SetRenderTexture( class RenderTexture* renderTexture, unsigned slot )
{
    if (slot < SLOT_COUNT)
    {
        GfxDeviceGlobal::boundViews[ slot ] = renderTexture->GetColorView();
    }
    else
    {
        System::Print( "ComputeShader:SetRenderTexture: Too high slot!\n" );
    }
}

void ae3d::ComputeShader::SetRenderTextureDepth( class RenderTexture* renderTexture, unsigned slot )
{
    if (slot < SLOT_COUNT)
    {
        GfxDeviceGlobal::boundViews[ slot ] = renderTexture->GetDepthView();
    }
    else
    {
        System::Print( "ComputeShader:SetRenderTextureDepth: Too high slot!\n" );
    }
}

void ae3d::ComputeShader::SetTexture2D( Texture2D* texture, unsigned slot )
{
    if (slot < SLOT_COUNT)
    {
        GfxDeviceGlobal::boundViews[ slot ] = texture->GetView();
    }
    else
    {
        System::Print( "ComputeShader:SetRenderTexture: Too high slot!\n" );
    }
}

void ae3d::ComputeShader::Begin()
{
    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkResult err = vkBeginCommandBuffer( GfxDeviceGlobal::computeCmdBuffer, &cmdBufInfo );
    AE3D_CHECK_VULKAN( err, "vkBeginCommandBuffer" );
}

void ae3d::ComputeShader::End()
{
    vkEndCommandBuffer( GfxDeviceGlobal::computeCmdBuffer );

    VkPipelineStageFlags pipelineStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = &pipelineStages;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &GfxDeviceGlobal::computeCmdBuffer;

    VkResult err = vkQueueSubmit( GfxDeviceGlobal::computeQueue, 1, &submitInfo, VK_NULL_HANDLE );
    AE3D_CHECK_VULKAN( err, "vkQueueSubmit compute" );
    Statistics::IncQueueSubmitCalls();

    System::BeginTimer();
    err = vkQueueWaitIdle( GfxDeviceGlobal::computeQueue );
    Statistics::IncQueueWaitTime( System::EndTimer() );
    AE3D_CHECK_VULKAN( err, "vkQueueWaitIdle" );
}

void ae3d::ComputeShader::Dispatch( unsigned groupCountX, unsigned groupCountY, unsigned groupCountZ, const char* debugName )
{
    System::Assert( GfxDeviceGlobal::computeCmdBuffer != VK_NULL_HANDLE, "Uninitialized compute command buffer" );

    debug::BeginRegion( GfxDeviceGlobal::computeCmdBuffer, debugName, 0, 1, 0 );
    
    BindComputeDescriptorSet();
    UploadPerObjectUbo();

    vkCmdBindPipeline( GfxDeviceGlobal::computeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pso );
    vkCmdDispatch( GfxDeviceGlobal::computeCmdBuffer, groupCountX, groupCountY, groupCountZ );
    Statistics::IncPSOBindCalls();

    debug::EndRegion( GfxDeviceGlobal::computeCmdBuffer );
}
