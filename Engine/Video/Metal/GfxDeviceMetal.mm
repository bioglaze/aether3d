#import <Foundation/Foundation.h>
#if AETHER3D_IOS
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>
#endif
#include "GfxDevice.hpp"
#include "Renderer.hpp"

extern ae3d::Renderer renderer;

id <MTLDevice> device;
id <MTLCommandQueue> commandQueue;
id <MTLLibrary> defaultLibrary;
id <MTLRenderPipelineState> pipelineState;
id <MTLDepthStencilState> depthState;
id <CAMetalDrawable> currentDrawable;
CAMetalLayer* metalLayer;
MTLRenderPassDescriptor *renderPassDescriptor = nullptr;
id <MTLTexture> depthTex;
id <MTLBuffer> uniformBuffer;
id<MTLRenderCommandEncoder> renderEncoder;
id<MTLCommandBuffer> commandBuffer;
id<CAMetalDrawable> drawable;
dispatch_semaphore_t inflight_semaphore;
bool pipelineCreated = false;
id<MTLTexture> texture0;

void PlatformInitGamePad()
{

}

struct Uniforms
{
    float mvp[ 16 ];
};

namespace
{
    int textureHandle = 0;
    float clearColor[] = { 0, 0, 0, 1 };

id<CAMetalDrawable> GetCurrentDrawable()
{
    while (currentDrawable == nil)
    {
        currentDrawable = [metalLayer nextDrawable];
        if (!currentDrawable)
        {
            NSLog(@"CurrentDrawable is nil");
        }
    }
    
    return currentDrawable;
}

void setupRenderPassDescriptor( id <MTLTexture> texture )
{
    if (renderPassDescriptor == nil)
    {
        renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    }
    
    renderPassDescriptor.colorAttachments[0].texture = texture;
    renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
    renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    
    if (!depthTex || (depthTex && (depthTex.width != texture.width || depthTex.height != texture.height)))
    {
        //  If we need a depth texture and don't have one, or if the depth texture we have is the wrong size
        //  Then allocate one of the proper size
        
        MTLTextureDescriptor* desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat: MTLPixelFormatDepth32Float width: texture.width height: texture.height mipmapped: NO];
        depthTex = [device newTextureWithDescriptor: desc];
        depthTex.label = @"Depth";
        
        renderPassDescriptor.depthAttachment.texture = depthTex;
        renderPassDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
        renderPassDescriptor.depthAttachment.clearDepth = 1.0f;
        renderPassDescriptor.depthAttachment.storeAction = MTLStoreActionDontCare;
    }
}
}

void ae3d::GfxDevice::Init( CAMetalLayer* aMetalLayer )
{
    int max_inflight_buffers = 1;
    inflight_semaphore = dispatch_semaphore_create( max_inflight_buffers );
    
    // Find a usable device
    device = MTLCreateSystemDefaultDevice();

    metalLayer = aMetalLayer;
    metalLayer.device = device;
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    // Change this to NO if the compute encoder is used as the last pass on the drawable texture
    metalLayer.framebufferOnly = YES;

    commandQueue = [device newCommandQueue];
    
    // Load all the shader files with a metal file extension in the project
    defaultLibrary = [device newDefaultLibrary];
    
    MTLDepthStencilDescriptor *depthStateDesc = [[MTLDepthStencilDescriptor alloc] init];
    depthStateDesc.depthCompareFunction = MTLCompareFunctionLess;
    depthStateDesc.depthWriteEnabled = YES;
    depthState = [device newDepthStencilStateWithDescriptor:depthStateDesc];
    
    uniformBuffer = [device newBufferWithLength:sizeof(Uniforms) options:MTLResourceOptionCPUCacheModeDefault];
}

id <MTLDevice> ae3d::GfxDevice::GetMetalDevice()
{
    return device;
}

id <MTLLibrary> ae3d::GfxDevice::GetDefaultMetalShaderLibrary()
{
    return defaultLibrary;
}

void ae3d::GfxDevice::ClearScreen( unsigned clearFlags )
{
    // Handled by setupRenderPassDescriptor().
}

void ae3d::GfxDevice::DrawVertexBuffer( id<MTLBuffer> vertexBuffer, id<MTLBuffer> indexBuffer, int elementCount, int indexOffset )
{
    // Configure and issue our draw call
    [renderEncoder setRenderPipelineState:pipelineState];
    [renderEncoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
    [renderEncoder setVertexBuffer:uniformBuffer offset:0 atIndex:1];
    [renderEncoder setFragmentTexture:texture0 atIndex:0];
    [renderEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                    indexCount:elementCount
                                     indexType:MTLIndexTypeUInt16
                                   indexBuffer:indexBuffer
                             indexBufferOffset:indexOffset];
}

void ae3d::GfxDevice::BeginFrame()
{
    // Create a reusable pipeline state
    if (!pipelineCreated)
    {
        MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        pipelineStateDescriptor.label = @"MyPipeline";
        [pipelineStateDescriptor setSampleCount: 1];
        [pipelineStateDescriptor setVertexFunction:renderer.builtinShaders.spriteRendererShader.vertexProgram];
        [pipelineStateDescriptor setFragmentFunction:renderer.builtinShaders.spriteRendererShader.fragmentProgram];
        pipelineStateDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        pipelineStateDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
        NSError* error = NULL;
        pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
        if (!pipelineState)
        {
            NSLog(@"Failed to created pipeline state, error %@", error);
        }
        pipelineCreated = true;
    }

    drawable = GetCurrentDrawable();
    setupRenderPassDescriptor( drawable.texture );

    commandBuffer = [commandQueue commandBuffer];
    commandBuffer.label = @"MyCommand";

    renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    renderEncoder.label = @"MyRenderEncoder";

    dispatch_semaphore_wait(inflight_semaphore, DISPATCH_TIME_FOREVER);
}

void ae3d::GfxDevice::PresentDrawable()
{
    [renderEncoder endEncoding];

    // Call the view's completion handler which is required by the view since it will signal its semaphore and set up the next buffer
    __block dispatch_semaphore_t block_sema = inflight_semaphore;
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
        dispatch_semaphore_signal(block_sema);
    }];
    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];
    currentDrawable = nullptr;
}

void ae3d::GfxDevice::SetClearColor( float red, float green, float blue )
{
    clearColor[ 0 ] = red;
    clearColor[ 1 ] = green;
    clearColor[ 2 ] = blue;
}

unsigned ae3d::GfxDevice::CreateBufferId()
{
    return 0;
}

unsigned ae3d::GfxDevice::CreateTextureId()
{
    return textureHandle++;
}

unsigned ae3d::GfxDevice::CreateVaoId()
{
    return 0;
}

unsigned ae3d::GfxDevice::CreateShaderId( unsigned shaderType )
{
    return 0;
}

unsigned ae3d::GfxDevice::CreateProgramId()
{
    return 0;
}

void ae3d::GfxDevice::IncDrawCalls()
{
}

int ae3d::GfxDevice::GetDrawCalls()
{
    return 0;
}

void ae3d::GfxDevice::ResetFrameStatistics()
{
}

void ae3d::GfxDevice::ReleaseGPUObjects()
{
}

void ae3d::GfxDevice::SetBlendMode( BlendMode blendMode )
{
}

void ae3d::GfxDevice::ErrorCheck(char const*)
{
    
}
