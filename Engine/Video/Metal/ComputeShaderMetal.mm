#include "ComputeShader.hpp"
#include "FileSystem.hpp"
#include "GfxDevice.hpp"
//#include "Macros.hpp"
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
    
    renderTextures.resize( 5 );
}

void ae3d::ComputeShader::SetRenderTexture( RenderTexture* renderTexture, unsigned slot )
{
    if (slot < renderTextures.size())
    {
        renderTextures[ slot ] = renderTexture;
    }
}

void ae3d::ComputeShader::SetUniformBuffer( int slotIndex, id< MTLBuffer > buffer )
{
    uniforms[ slotIndex ] = buffer;
}

void ae3d::ComputeShader::Dispatch( unsigned groupCountX, unsigned groupCountY, unsigned groupCountZ )
{
    MTLSize threadgroupCounts = MTLSizeMake( 16, 16, 1 );
    MTLSize threadgroups = MTLSizeMake( groupCountX, groupCountY, groupCountZ );

    //MTLSize threadgroupCounts = MTLSizeMake( groupCountX, groupCountY, groupCountZ );
    //MTLSize threadgroups = MTLSizeMake( 1, 1, 1 );

    id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
    commandBuffer.label = @"ComputeCommand";
    
    id<MTLComputeCommandEncoder> commandEncoder = [commandBuffer computeCommandEncoder];
    [commandEncoder setComputePipelineState:pipeline];
    [commandEncoder setBuffer:uniforms[ 0 ] offset:0 atIndex:0];
    [commandEncoder setBuffer:uniforms[ 1 ] offset:0 atIndex:1];
    [commandEncoder setBuffer:uniforms[ 2 ] offset:0 atIndex:2];

    for (std::size_t i = 0; i < renderTextures.size(); ++i)
    {
        [commandEncoder setTexture:(renderTextures[ i ] ? renderTextures[ i ]->GetMetalTexture() : nullptr) atIndex:i];
    }
    
    [commandEncoder dispatchThreadgroups:threadgroups threadsPerThreadgroup:threadgroupCounts];
    [commandEncoder endEncoding];
    
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
}

void ae3d::ComputeShader::Test( ae3d::Texture2D* texture, ae3d::RenderTexture* outTexture )
{
    MTLSize threadgroupCounts = MTLSizeMake( 8, 8, 1 );
    MTLSize threadgroups = MTLSizeMake( texture->GetWidth() / threadgroupCounts.width,
                                        texture->GetHeight() / threadgroupCounts.height,
                                       1);
    id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
    commandBuffer.label = @"ComputeCommand";
    
    id<MTLComputeCommandEncoder> commandEncoder = [commandBuffer computeCommandEncoder];
    [commandEncoder setComputePipelineState:pipeline];
    [commandEncoder setTexture:texture->GetMetalTexture() atIndex:0];
    [commandEncoder setTexture:outTexture->GetMetalTexture() atIndex:1];
    [commandEncoder setBuffer:uniforms[0] offset:0 atIndex:0];
    [commandEncoder dispatchThreadgroups:threadgroups threadsPerThreadgroup:threadgroupCounts];
    [commandEncoder endEncoding];
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
}
