#include "ComputeShader.hpp"
#include "FileSystem.hpp"
#include "GfxDevice.hpp"
#include "RenderTexture.hpp"
#include "System.hpp"
#include "Texture2D.hpp"

void UploadPerObjectUbo();

extern id <MTLCommandQueue> commandQueue;

namespace GfxDeviceGlobal
{
    extern PerObjectUboStruct perObjectUboStruct;
}

void ae3d::ComputeShader::Load( const char* source )
{
    NSLog( @"ComputeShader: not implemented method\n" );
}

void ae3d::ComputeShader::Load( const char* metalShaderName, const FileSystem::FileContentsData& dataHLSL, const FileSystem::FileContentsData& dataSPIRV )
{
    NSString* shaderName = [NSString stringWithUTF8String:metalShaderName ];
    function = [GfxDevice::GetDefaultMetalShaderLibrary() newFunctionWithName:shaderName];
    
    if (function == nullptr)
    {
        NSLog( @"ComputeShader: Could not load %s!\n", metalShaderName );
        return;
    }

    NSError *error = nil;
    pipeline = [GfxDevice::GetMetalDevice() newComputePipelineStateWithFunction:function error:&error];

    if (!pipeline)
    {
        NSLog( @"Error occurred when building compute pipeline for function %s: %@", metalShaderName, [error localizedDescription] );
    }
}

void ae3d::ComputeShader::SetRenderTexture( unsigned slotIndex, RenderTexture* renderTexture )
{
    if (slotIndex < SLOT_COUNT)
    {
        renderTextures[ slotIndex ] = renderTexture;
    }
    else
    {
        System::Print( "ComputeShader: Too high slot count %u in SetRenderTexture!\n", slotIndex );
    }
}

void ae3d::ComputeShader::SetTexture2D( unsigned slotIndex, Texture2D* texture )
{
    if (slotIndex < SLOT_COUNT)
    {
        textures[ slotIndex ] = texture;
    }
    else
    {
        System::Print( "ComputeShader: Too high slot count %u in SetTexture2D!\n", slotIndex );
    }
}

void ae3d::ComputeShader::SetUniform( UniformName uniform, float x, float y )
{
    if( uniform == UniformName::TilesZW )
    {
        GfxDeviceGlobal::perObjectUboStruct.tilesXY.z = x;
        GfxDeviceGlobal::perObjectUboStruct.tilesXY.w = y;
    }
}

void ae3d::ComputeShader::SetUniformBuffer( unsigned slotIndex, id< MTLBuffer > buffer )
{
    if (slotIndex < SLOT_COUNT)
    {
        uniforms[ slotIndex ] = buffer;
    }
    else
    {
        System::Print( "ComputeShader: Too high slot count %u in SetRenderUniformBuffer!\n", slotIndex );
    }
}

void ae3d::ComputeShader::Dispatch( unsigned groupCountX, unsigned groupCountY, unsigned groupCountZ )
{
    MTLSize threadgroupCounts = MTLSizeMake( 16, 16, 1 );
    MTLSize threadgroups = MTLSizeMake( groupCountX, groupCountY, groupCountZ );

    SetUniformBuffer( 0, GfxDevice::GetCurrentUniformBuffer() );
    UploadPerObjectUbo();
    GfxDevice::GetNewUniformBuffer();
    id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
    commandBuffer.label = @"ComputeCommand";
    
    id<MTLComputeCommandEncoder> commandEncoder = [commandBuffer computeCommandEncoder];

    [commandEncoder setComputePipelineState:pipeline];

    for (std::size_t i = 0; i < SLOT_COUNT; ++i)
    {
        [commandEncoder setBuffer:uniforms[ i ] offset:0 atIndex:i];
        [commandEncoder setTexture:(renderTextures[ i ] ? renderTextures[ i ]->GetMetalTexture() : nullptr) atIndex:i];
        
        if (!renderTextures[ i ])
        {
            [commandEncoder setTexture:(textures[ i ] ? textures[ i ]->GetMetalTexture() : nullptr) atIndex:i];
        }
    }

    [commandEncoder dispatchThreadgroups:threadgroups threadsPerThreadgroup:threadgroupCounts];
    [commandEncoder endEncoding];
    
    [commandBuffer commit];
}
