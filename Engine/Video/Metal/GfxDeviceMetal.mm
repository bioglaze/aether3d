#import <Foundation/Foundation.h>
#if AETHER3D_IOS
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>
#endif
#include <unordered_map>
#include "GfxDevice.hpp"
#include "VertexBuffer.hpp"
#include "Shader.hpp"
#include "Renderer.hpp"
#include "RenderTexture.hpp"

extern ae3d::Renderer renderer;

id <MTLDevice> device;
id <MTLCommandQueue> commandQueue;
id <MTLLibrary> defaultLibrary;
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
id<MTLTexture> texture0;
id<MTLTexture> currentRenderTarget;

// TODO: Move somewhere else.
void PlatformInitGamePad()
{
}

struct Uniforms
{
    float modelViewProjection[ 16 ];
};

namespace GfxDeviceGlobal
{
    int drawCalls = 0;
    int textureBinds = 0;
    int vertexBufferBinds = 0;
    int backBufferWidth = 0;
    int backBufferHeight = 0;
    std::unordered_map< std::string, id <MTLRenderPipelineState> > psoCache;
}

void ae3d::GfxDevice::Init( int width, int height )
{
    GfxDeviceGlobal::backBufferWidth = width;
    GfxDeviceGlobal::backBufferHeight = height;
}

namespace
{
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
    
    device = MTLCreateSystemDefaultDevice();

    metalLayer = aMetalLayer;
    metalLayer.device = device;
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    // Change this to NO if the compute encoder is used as the last pass on the drawable texture
    metalLayer.framebufferOnly = YES;

    commandQueue = [device newCommandQueue];
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

int ae3d::GfxDevice::GetTextureBinds()
{
    return GfxDeviceGlobal::textureBinds;
}

int ae3d::GfxDevice::GetVertexBufferBinds()
{
    return GfxDeviceGlobal::vertexBufferBinds;
}

void ae3d::GfxDevice::ClearScreen( unsigned clearFlags )
{
    // Handled by setupRenderPassDescriptor().
}

std::string GetPSOHash( ae3d::Shader& shader, ae3d::GfxDevice::BlendMode blendMode, ae3d::GfxDevice::DepthFunc depthFunc )
{
    std::string hashString;
    hashString += std::to_string( (ptrdiff_t)&shader.vertexProgram );
    hashString += std::to_string( (ptrdiff_t)&shader.fragmentProgram );
    hashString += std::to_string( (unsigned)blendMode );
    hashString += std::to_string( ((unsigned)depthFunc) + 4 );
    return hashString;
}

id <MTLRenderPipelineState> GetPSO( ae3d::Shader& shader, ae3d::GfxDevice::BlendMode blendMode, ae3d::GfxDevice::DepthFunc depthFunc )
{
    const std::string psoHash = GetPSOHash( shader, blendMode, depthFunc );

    if (GfxDeviceGlobal::psoCache.find( psoHash ) == std::end( GfxDeviceGlobal::psoCache ))
    {
        MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        pipelineStateDescriptor.label = @"Opaque sprite pipeline";
        [pipelineStateDescriptor setSampleCount: 1];
        [pipelineStateDescriptor setVertexFunction:shader.vertexProgram];
        [pipelineStateDescriptor setFragmentFunction:shader.fragmentProgram];
        pipelineStateDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        pipelineStateDescriptor.colorAttachments[0].blendingEnabled = blendMode != ae3d::GfxDevice::BlendMode::Off;
        pipelineStateDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        pipelineStateDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pipelineStateDescriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        pipelineStateDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
        pipelineStateDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pipelineStateDescriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        pipelineStateDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

        NSError* error = NULL;
        id <MTLRenderPipelineState> pso = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
        
        if (!pso)
        {
            NSLog(@"Failed to created opaque sprite pipeline state, error %@", error);
        }

        GfxDeviceGlobal::psoCache[ psoHash ] = pso;
    }
    

    return GfxDeviceGlobal::psoCache[ psoHash ];
}

void ae3d::GfxDevice::Draw( VertexBuffer& vertexBuffer, int startIndex, int endIndex, Shader& shader, BlendMode blendMode, DepthFunc depthFunc )
{
    ++GfxDeviceGlobal::drawCalls;

    [renderEncoder setRenderPipelineState:GetPSO( shader, blendMode, depthFunc )];
    [renderEncoder setVertexBuffer:vertexBuffer.GetVertexBuffer() offset:0 atIndex:0];
    [renderEncoder setVertexBuffer:uniformBuffer offset:0 atIndex:1];
    [renderEncoder setFragmentTexture:texture0 atIndex:0];
    [renderEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                              indexCount:(endIndex - startIndex) * 3
                               indexType:MTLIndexTypeUInt16
                             indexBuffer:vertexBuffer.GetIndexBuffer()
                       indexBufferOffset:startIndex * 2 * 3];
}

void ae3d::GfxDevice::BeginFrame()
{
    if (!currentRenderTarget)
    {
        drawable = GetCurrentDrawable();
        setupRenderPassDescriptor( drawable.texture );
    }
    else
    {
        setupRenderPassDescriptor( currentRenderTarget );
    }
    
    commandBuffer = [commandQueue commandBuffer];
    commandBuffer.label = @"MyCommand";

    renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    renderEncoder.label = @"MyRenderEncoder";

    dispatch_semaphore_wait(inflight_semaphore, DISPATCH_TIME_FOREVER);
    
    //[commandQueue insertDebugCaptureBoundary];
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

void ae3d::GfxDevice::IncDrawCalls()
{
    ++GfxDeviceGlobal::drawCalls;
}

int ae3d::GfxDevice::GetDrawCalls()
{
    return GfxDeviceGlobal::drawCalls;
}

void ae3d::GfxDevice::ResetFrameStatistics()
{
    GfxDeviceGlobal::drawCalls = 0;
}

void ae3d::GfxDevice::ReleaseGPUObjects()
{
}

void ae3d::GfxDevice::SetBlendMode( BlendMode blendMode )
{
}

void ae3d::GfxDevice::SetBackFaceCulling( bool enable )
{
}

void ae3d::GfxDevice::SetMultiSampling( bool enable )
{
}

void ae3d::GfxDevice::SetRenderTarget( ae3d::RenderTexture* renderTexture2d, unsigned cubeMapFace )
{
    if ((!currentRenderTarget && !renderTexture2d) ||
        (renderTexture2d != nullptr && renderTexture2d->GetMetalTexture() == currentRenderTarget))
    {
        return;
    }
    
    PresentDrawable();
    
    //setupRenderPassDescriptor( renderTexture2d != nullptr ? renderTexture2d->GetMetalTexture() : drawable.texture );
    currentRenderTarget = renderTexture2d != nullptr ? renderTexture2d->GetMetalTexture() : nullptr;//GetCurrentDrawable().texture;
    BeginFrame();
}

void ae3d::GfxDevice::ErrorCheck(char const*)
{
    
}
