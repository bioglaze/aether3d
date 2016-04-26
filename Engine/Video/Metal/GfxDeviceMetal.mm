#import <Foundation/Foundation.h>
#if AETHER3D_METAL
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#endif
#include <unordered_map>
#include <list>
#include "GfxDevice.hpp"
#include "VertexBuffer.hpp"
#include "Shader.hpp"
#include "System.hpp"
#include "Renderer.hpp"
#include "RenderTexture.hpp"
#include "Texture2D.hpp"

extern ae3d::Renderer renderer;

id <MTLDevice> device;
id <MTLCommandQueue> commandQueue;
id <MTLLibrary> defaultLibrary;
id <MTLDepthStencilState> depthStateLessWriteOn;
id <CAMetalDrawable> currentDrawable; // This frame's framebuffer drawable
id <CAMetalDrawable> drawable; // Render texture or currentDrawable

MTLRenderPassDescriptor* renderPassDescriptor = nullptr;
//id <MTLTexture> depthTex;
id<MTLRenderCommandEncoder> renderEncoder;
id<MTLCommandBuffer> commandBuffer;
id<MTLTexture> texture0;
id<MTLTexture> texture1;
id<MTLTexture> currentRenderTarget;
id<MTLTexture> msaaColorTarget;
id<MTLTexture> msaaDepthTarget;
MTKView* appView = nullptr;

// TODO: Move somewhere else.
void PlatformInitGamePad()
{
}

namespace Statistics
{
    int drawCalls = 0;
    int textureBinds = 0;
    int vertexBufferBinds = 0;
    int renderTargetBinds = 0;
    int shaderBinds = 0;
}

namespace GfxDeviceGlobal
{
    int backBufferWidth = 0;
    int backBufferHeight = 0;
    int sampleCount = 1;
    ae3d::GfxDevice::ClearFlags clearFlags = ae3d::GfxDevice::ClearFlags::Depth;
    std::unordered_map< std::string, id <MTLRenderPipelineState> > psoCache;
    
    std::list< id<MTLBuffer> > uniformBuffers;
}

namespace
{
    float clearColor[] = { 0, 0, 0, 1 };
    
    void setupRenderPassDescriptor( id <MTLTexture> texture )
    {
        MTLLoadAction texLoadAction = MTLLoadActionLoad;
        MTLLoadAction depthLoadAction = MTLLoadActionLoad;
        
        if (GfxDeviceGlobal::clearFlags & ae3d::GfxDevice::ClearFlags::Color)
        {
            texLoadAction = MTLLoadActionClear;
        }
        
        if (GfxDeviceGlobal::clearFlags & ae3d::GfxDevice::ClearFlags::Depth)
        {
            depthLoadAction = MTLLoadActionClear;
        }
        
        if (GfxDeviceGlobal::sampleCount > 1)
        {
            renderPassDescriptor.colorAttachments[0].texture = msaaColorTarget;
            renderPassDescriptor.colorAttachments[0].resolveTexture = texture;
            renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionMultisampleResolve;
        }
        else
        {
            renderPassDescriptor.colorAttachments[0].texture = texture;
            renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
        }
        renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake( clearColor[0], clearColor[1], clearColor[2], clearColor[3] );
        renderPassDescriptor.colorAttachments[0].loadAction = texLoadAction;
        
        renderPassDescriptor.stencilAttachment = nil;

        if (GfxDeviceGlobal::sampleCount > 1)
        {
            renderPassDescriptor.depthAttachment.texture = msaaDepthTarget;
            renderPassDescriptor.depthAttachment.loadAction = depthLoadAction;
            renderPassDescriptor.depthAttachment.clearDepth = 1.0;
        }
    }
}

id <MTLBuffer> ae3d::GfxDevice::GetNewUniformBuffer()
{
    id<MTLBuffer> uniformBuffer = [GfxDevice::GetMetalDevice() newBufferWithLength:256 options:MTLResourceCPUCacheModeDefaultCache];
    uniformBuffer.label = @"uniform buffer";
    
    GfxDeviceGlobal::uniformBuffers.push_back( uniformBuffer );
    return uniformBuffer;
}

id <MTLBuffer> ae3d::GfxDevice::GetCurrentUniformBuffer()
{
    if (GfxDeviceGlobal::uniformBuffers.empty())
    {
        return GetNewUniformBuffer();
    }
    
    return GfxDeviceGlobal::uniformBuffers.back();
}

void ae3d::GfxDevice::ResetUniformBuffers()
{
    GfxDeviceGlobal::uniformBuffers.clear();
}

void ae3d::GfxDevice::Init( int width, int height )
{
    GfxDeviceGlobal::backBufferWidth = width;
    GfxDeviceGlobal::backBufferHeight = height;
}

void ae3d::GfxDevice::SetCurrentDrawableMetal( id <CAMetalDrawable> aDrawable, MTLRenderPassDescriptor* renderPass )
{
    currentDrawable = aDrawable;
    renderPassDescriptor = renderPass;
}

void ae3d::GfxDevice::PushGroupMarker( const char* name )
{
    NSString* fName = [NSString stringWithUTF8String: name];
    [renderEncoder pushDebugGroup:fName];
}

void ae3d::GfxDevice::PopGroupMarker()
{
    [renderEncoder popDebugGroup];
}

void ae3d::GfxDevice::InitMetal( id <MTLDevice> metalDevice, MTKView* view, int sampleCount )
{
    if (sampleCount < 1 || sampleCount > 8)
    {
        sampleCount = 1;
    }
    
    device = metalDevice;
    appView = view;

    commandQueue = [device newCommandQueue];
    defaultLibrary = [device newDefaultLibrary];

    GfxDeviceGlobal::backBufferWidth = view.bounds.size.width;
    GfxDeviceGlobal::backBufferHeight = view.bounds.size.height;
    GfxDeviceGlobal::sampleCount = sampleCount;
    
    MTLDepthStencilDescriptor *depthStateDesc = [[MTLDepthStencilDescriptor alloc] init];
    depthStateDesc.depthCompareFunction = MTLCompareFunctionLess;
    depthStateDesc.depthWriteEnabled = YES;
    depthStateDesc.label = @"less write on";
    depthStateLessWriteOn = [device newDepthStencilStateWithDescriptor:depthStateDesc];
    
    if (sampleCount == 1)
    {
        return;
    }
    
    MTLTextureDescriptor* desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                        width:GfxDeviceGlobal::backBufferWidth * 2
                                                                                       height:GfxDeviceGlobal::backBufferHeight * 2
                                                                                    mipmapped:NO];
        
    desc.textureType = MTLTextureType2DMultisample;
    desc.sampleCount = sampleCount;
    desc.resourceOptions = MTLResourceStorageModePrivate;
    desc.usage = MTLTextureUsageRenderTarget;
    
    msaaColorTarget = [device newTextureWithDescriptor:desc];

    MTLTextureDescriptor* depthDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                                                                    width:GfxDeviceGlobal::backBufferWidth * 2
                                                                                   height:GfxDeviceGlobal::backBufferHeight * 2
                                                                                mipmapped:NO];

    depthDesc.textureType = MTLTextureType2DMultisample;
    depthDesc.sampleCount = sampleCount;
    depthDesc.resourceOptions = MTLResourceStorageModePrivate;
    depthDesc.usage = MTLTextureUsageRenderTarget;
    
    msaaDepthTarget = [device newTextureWithDescriptor:depthDesc];
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
    return Statistics::textureBinds;
}

int ae3d::GfxDevice::GetVertexBufferBinds()
{
    return Statistics::vertexBufferBinds;
}

int ae3d::GfxDevice::GetRenderTargetBinds()
{
    return Statistics::renderTargetBinds;
}

int ae3d::GfxDevice::GetShaderBinds()
{
    return Statistics::shaderBinds;
}

int ae3d::GfxDevice::GetBarrierCalls()
{
    return 0;
}

int ae3d::GfxDevice::GetFenceCalls()
{
    return 0;
}

void ae3d::GfxDevice::GetGpuMemoryUsage( unsigned& outGpuUsageMBytes, unsigned& outGpuBudgetMBytes )
{
    outGpuUsageMBytes = 0;
    outGpuBudgetMBytes = 0;
}

void ae3d::GfxDevice::ClearScreen( unsigned clearFlags )
{
    GfxDeviceGlobal::clearFlags = (ae3d::GfxDevice::ClearFlags)clearFlags;
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
        pipelineStateDescriptor.label = @"pipeline";
        pipelineStateDescriptor.sampleCount = GfxDeviceGlobal::sampleCount;
        pipelineStateDescriptor.vertexFunction = shader.vertexProgram;
        pipelineStateDescriptor.fragmentFunction = shader.fragmentProgram;
        pipelineStateDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        pipelineStateDescriptor.colorAttachments[0].blendingEnabled = blendMode != ae3d::GfxDevice::BlendMode::Off;
        pipelineStateDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        pipelineStateDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pipelineStateDescriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        pipelineStateDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
        pipelineStateDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pipelineStateDescriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        // This must match app's view's format.
        pipelineStateDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
        
        NSError* error = NULL;
        MTLRenderPipelineReflection* reflectionObj;
        MTLPipelineOption option = MTLPipelineOptionBufferTypeInfo | MTLPipelineOptionArgumentInfo;
        id <MTLRenderPipelineState> pso = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor options:option reflection:&reflectionObj error:&error];
        
        if (!pso)
        {
            NSLog(@"Failed to create pipeline state, error %@", error);
        }

        GfxDeviceGlobal::psoCache[ psoHash ] = pso;
        
        // FIXME: If two PSOs use the same set of shaders, are their uniform locations equal? This code assumes so. [TimoW, 2015-10-05]
        shader.LoadUniforms( reflectionObj );        
    }

    return GfxDeviceGlobal::psoCache[ psoHash ];
}

void ae3d::GfxDevice::Draw( VertexBuffer& vertexBuffer, int startIndex, int endIndex, Shader& shader, BlendMode blendMode, DepthFunc depthFunc, CullMode cullMode )
{
    ++Statistics::drawCalls;

    if (!texture0)
    {
        texture0 = Texture2D::GetDefaultTexture()->GetMetalTexture();
    }
    if (!texture1)
    {
        texture1 = Texture2D::GetDefaultTexture()->GetMetalTexture();
    }
    
    [renderEncoder setRenderPipelineState:GetPSO( shader, blendMode, depthFunc )];
    [renderEncoder setCullMode:(cullMode == CullMode::Back) ? MTLCullModeFront : MTLCullModeNone];
    [renderEncoder setVertexBuffer:vertexBuffer.GetVertexBuffer() offset:0 atIndex:0];
    [renderEncoder setVertexBuffer:GetCurrentUniformBuffer() offset:0 atIndex:1];
    [renderEncoder setFragmentTexture:texture0 atIndex:0];
    [renderEncoder setFragmentTexture:texture1 atIndex:1];
    [renderEncoder setDepthStencilState:depthStateLessWriteOn];
    
    [renderEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                              indexCount:(endIndex - startIndex) * 3
                               indexType:MTLIndexTypeUInt16
                             indexBuffer:vertexBuffer.GetIndexBuffer()
                       indexBufferOffset:startIndex * 2 * 3];
}

void ae3d::GfxDevice::BeginFrame()
{
    ResetUniformBuffers();
    
    commandBuffer = [commandQueue commandBuffer];
    commandBuffer.label = @"MyCommand";

    renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    renderEncoder.label = @"MyRenderEncoder";
}

void ae3d::GfxDevice::PresentDrawable()
{
    [renderEncoder endEncoding];
    
    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];
}

void ae3d::GfxDevice::SetClearColor( float red, float green, float blue )
{
    clearColor[ 0 ] = red;
    clearColor[ 1 ] = green;
    clearColor[ 2 ] = blue;
}

void ae3d::GfxDevice::IncDrawCalls()
{
    ++Statistics::drawCalls;
}

int ae3d::GfxDevice::GetDrawCalls()
{
    return Statistics::drawCalls;
}

void ae3d::GfxDevice::IncTextureBinds()
{
    ++Statistics::textureBinds;
}

void ae3d::GfxDevice::IncShaderBinds()
{
    ++Statistics::shaderBinds;
}

void ae3d::GfxDevice::ResetFrameStatistics()
{
    Statistics::drawCalls = 0;
    Statistics::renderTargetBinds = 0;
    Statistics::textureBinds = 0;
    Statistics::vertexBufferBinds = 0;
    Statistics::shaderBinds = 0;
}

void ae3d::GfxDevice::ReleaseGPUObjects()
{
}

void ae3d::GfxDevice::SetMultiSampling( bool enable )
{
}

void ae3d::GfxDevice::SetRenderTarget( ae3d::RenderTexture* renderTexture2d, unsigned cubeMapFace )
{
    drawable = currentDrawable;
    setupRenderPassDescriptor( renderTexture2d != nullptr ? renderTexture2d->GetMetalTexture() : drawable.texture );
}

void ae3d::GfxDevice::ErrorCheck( const char* )
{
}
