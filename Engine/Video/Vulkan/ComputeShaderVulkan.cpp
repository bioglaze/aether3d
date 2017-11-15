#include "ComputeShader.hpp"
#include "FileSystem.hpp"
#include "System.hpp"
#include "Macros.hpp"
#include "VulkanUtils.hpp"

namespace GfxDeviceGlobal
{
    extern VkDevice device;
    extern VkCommandBuffer computeCmdBuffer;
}

namespace ComputeShaderGlobal
{
    std::vector< VkShaderModule > modulesToReleaseAtExit;
}

void ae3d::ComputeShader::DestroyShaders()
{
    for (std::size_t moduleIndex = 0; moduleIndex < ComputeShaderGlobal::modulesToReleaseAtExit.size(); ++moduleIndex)
    {
        vkDestroyShaderModule( GfxDeviceGlobal::device, ComputeShaderGlobal::modulesToReleaseAtExit[ moduleIndex ], nullptr );
    }
}

void ae3d::ComputeShader::Load( const char* /*source*/ )
{

}

void ae3d::ComputeShader::Load( const char* /*metalShaderName*/, const FileSystem::FileContentsData& /*dataHLSL*/, const FileSystem::FileContentsData& dataSPIRV )
{
    VkShaderModuleCreateInfo moduleCreateInfo;
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = nullptr;
    moduleCreateInfo.codeSize = dataSPIRV.data.size();
    moduleCreateInfo.pCode = (const std::uint32_t*)dataSPIRV.data.data();
    moduleCreateInfo.flags = 0;

    VkShaderModule shaderModule;
    VkResult err = vkCreateShaderModule( GfxDeviceGlobal::device, &moduleCreateInfo, nullptr, &shaderModule );
    AE3D_CHECK_VULKAN( err, "vkCreateShaderModule compute" );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)shaderModule, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, "compute shader" );
    ComputeShaderGlobal::modulesToReleaseAtExit.push_back( shaderModule );

    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    info.module = shaderModule;
    info.pName = "CSMain";
    info.pNext = nullptr;
    info.flags = 0;
    info.pSpecializationInfo = nullptr;

    System::Assert( info.module != VK_NULL_HANDLE, "compute shader module not created" );
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
    ComputeShaderGlobal::modulesToReleaseAtExit.push_back( shaderModule );

    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    info.module = shaderModule;
    info.pName = "CSMain";
    info.pNext = nullptr;
    info.flags = 0;
    info.pSpecializationInfo = nullptr;

    System::Assert( info.module != VK_NULL_HANDLE, "compute shader module not created" );
}

void ae3d::ComputeShader::SetRenderTexture( class RenderTexture* /*renderTexture*/, unsigned /*slot*/ )
{
	System::Print("SetRenderTexture unimplemented\n");
}

void ae3d::ComputeShader::Dispatch( unsigned groupCountX, unsigned groupCountY, unsigned groupCountZ )
{
    System::Assert( GfxDeviceGlobal::computeCmdBuffer != VK_NULL_HANDLE, "Uninitialized compute command buffer" );

    vkCmdDispatch( GfxDeviceGlobal::computeCmdBuffer, groupCountX, groupCountY, groupCountZ );
}
