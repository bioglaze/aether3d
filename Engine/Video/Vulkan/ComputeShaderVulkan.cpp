#include "ComputeShader.hpp"
#include "FileSystem.hpp"
#include "System.hpp"
#include "Macros.hpp"

namespace GfxDeviceGlobal
{
    extern VkDevice device;
    extern VkCommandBuffer computeCmdBuffer;
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
    moduleCreateInfo.pCode = (std::uint32_t*)dataSPIRV.data.data();
    moduleCreateInfo.flags = 0;

    VkShaderModule shaderModule;
    VkResult err = vkCreateShaderModule( GfxDeviceGlobal::device, &moduleCreateInfo, nullptr, &shaderModule );
    AE3D_CHECK_VULKAN( err, "vkCreateShaderModule vertex" );

    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    info.module = shaderModule;
    info.pName = "main";
    info.pNext = nullptr;
    info.flags = 0;
    info.pSpecializationInfo = nullptr;

    System::Assert( info.module != VK_NULL_HANDLE, "compute shader module not created" );

}

void ae3d::ComputeShader::LoadSPIRV( const ae3d::FileSystem::FileContentsData& contents )
{
    
}

void ae3d::ComputeShader::Dispatch( unsigned groupCountX, unsigned groupCountY, unsigned groupCountZ )
{
    System::Assert( GfxDeviceGlobal::computeCmdBuffer != VK_NULL_HANDLE, "Uninitialized compute command buffer" );

    vkCmdDispatch( GfxDeviceGlobal::computeCmdBuffer, groupCountX, groupCountY, groupCountZ );
}
