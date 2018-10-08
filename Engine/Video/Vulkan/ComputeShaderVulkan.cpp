#include "ComputeShader.hpp"
#include "Array.hpp"
#include "FileSystem.hpp"
#include "System.hpp"
#include "Macros.hpp"
#include "VulkanUtils.hpp"

void BindComputeDescriptorSet();

namespace GfxDeviceGlobal
{
    extern VkDevice device;
    extern VkCommandBuffer computeCmdBuffer;
    extern VkPipelineLayout pipelineLayout;
    extern VkPipelineCache pipelineCache;
}

namespace ComputeShaderGlobal
{
    Array< VkShaderModule > modulesToReleaseAtExit;
    Array< VkPipeline > psosToReleaseAtExit;
}

void ae3d::ComputeShader::DestroyShaders()
{
    for (int moduleIndex = 0; moduleIndex < ComputeShaderGlobal::modulesToReleaseAtExit.count; ++moduleIndex)
    {
        vkDestroyShaderModule( GfxDeviceGlobal::device, ComputeShaderGlobal::modulesToReleaseAtExit[ moduleIndex ], nullptr );
    }

    for (int psoIndex = 0; psoIndex < ComputeShaderGlobal::psosToReleaseAtExit.count; ++psoIndex)
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
    VkShaderModuleCreateInfo moduleCreateInfo;
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = nullptr;
    moduleCreateInfo.codeSize = contents.data.size();
    moduleCreateInfo.pCode = (const std::uint32_t*)contents.data.data();
    moduleCreateInfo.flags = 0;

    VkShaderModule shaderModule;
    VkResult err = vkCreateShaderModule( GfxDeviceGlobal::device, &moduleCreateInfo, nullptr, &shaderModule );
    AE3D_CHECK_VULKAN( err, "vkCreateShaderModule compute" );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)shaderModule, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, contents.path.c_str() );
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
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)pso, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, "light tiler PSO" );
    ComputeShaderGlobal::psosToReleaseAtExit.Add( pso );
}

void ae3d::ComputeShader::SetRenderTexture( class RenderTexture* renderTexture, unsigned slot )
{
    if (slot < SLOT_COUNT)
    {
        renderTextures[ slot ] = renderTexture;
    }
    else
    {
        System::Print( "ComputeShader:SetRenderTexture: Too high slot!\n" );
    }
}

void ae3d::ComputeShader::Dispatch( unsigned groupCountX, unsigned groupCountY, unsigned groupCountZ )
{
    System::Assert( GfxDeviceGlobal::computeCmdBuffer != VK_NULL_HANDLE, "Uninitialized compute command buffer" );

    BindComputeDescriptorSet();

    vkCmdBindPipeline( GfxDeviceGlobal::computeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pso );
    vkCmdDispatch( GfxDeviceGlobal::computeCmdBuffer, groupCountX, groupCountY, groupCountZ );
}
