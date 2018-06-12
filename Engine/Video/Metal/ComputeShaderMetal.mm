#include "ComputeShader.hpp"
#include "FileSystem.hpp"
#include "GfxDevice.hpp"
#include "RenderTexture.hpp"
#include "System.hpp"
#include "Texture2D.hpp"

extern id <MTLCommandQueue> commandQueue;

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

void ae3d::ComputeShader::SetRenderTexture( RenderTexture* renderTexture, unsigned slot )
{
    System::Assert( slot < SLOT_COUNT, "Too high slot" );
    renderTextures[ slot ] = renderTexture;
}

void ae3d::ComputeShader::SetUniformBuffer( int slotIndex, id< MTLBuffer > buffer )
{
    System::Assert( slotIndex < SLOT_COUNT, "Too high slot" );
    uniforms[ slotIndex ] = buffer;
}

void ae3d::ComputeShader::Dispatch( unsigned groupCountX, unsigned groupCountY, unsigned groupCountZ )
{
    MTLSize threadgroupCounts = MTLSizeMake( 16, 16, 1 );
    MTLSize threadgroups = MTLSizeMake( groupCountX, groupCountY, groupCountZ );

    id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
    commandBuffer.label = @"ComputeCommand";
    
    id<MTLComputeCommandEncoder> commandEncoder = [commandBuffer computeCommandEncoder];
    for (std::size_t i = 0; i < SLOT_COUNT; ++i)
    {
        [commandEncoder setBuffer:uniforms[ i ] offset:0 atIndex:i];
        [commandEncoder setTexture:(renderTextures[ i ] ? renderTextures[ i ]->GetMetalTexture() : nullptr) atIndex:i];
    }

    [commandEncoder setComputePipelineState:pipeline];
    
    [commandEncoder dispatchThreadgroups:threadgroups threadsPerThreadgroup:threadgroupCounts];
    [commandEncoder endEncoding];
    
    [commandBuffer commit];
}
