#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include <chrono>
#include <unordered_map>
#include <sstream>
#include <list>
#include "GfxDevice.hpp"
#include "LightTiler.hpp"
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
id <MTLDepthStencilState> depthStateLessEqualWriteOn;
id <MTLDepthStencilState> depthStateLessEqualWriteOff;
id <MTLDepthStencilState> depthStateNoneWriteOff;
id <CAMetalDrawable> currentDrawable; // This frame's framebuffer drawable
id <CAMetalDrawable> drawable; // Render texture or currentDrawable
int currentCubeMapFace = 0;

MTLRenderPassDescriptor* renderPassDescriptorApp = nullptr;
MTLRenderPassDescriptor* renderPassDescriptorFBO = nullptr;
id<MTLRenderCommandEncoder> renderEncoder;
id<MTLCommandBuffer> commandBuffer;
id<MTLTexture> texture0;
id<MTLTexture> texture1;
id<MTLTexture> msaaColorTarget;
id<MTLTexture> msaaDepthTarget;

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
    int createUniformBufferCalls = 0;
    float frameTimeMS = 0;
}

namespace GfxDeviceGlobal
{
    int backBufferWidth = 0;
    int backBufferHeight = 0;
    int sampleCount = 1;
    bool isRenderingToTexture = false;
    ae3d::GfxDevice::ClearFlags clearFlags = ae3d::GfxDevice::ClearFlags::Depth;
    std::unordered_map< std::string, id <MTLRenderPipelineState> > psoCache;
    id<MTLSamplerState> samplerStates[ 2 ];
    std::list< id<MTLBuffer> > uniformBuffers;
    ae3d::RenderTexture::DataType currentRenderTargetDataType = ae3d::RenderTexture::DataType::UByte;
    ae3d::LightTiler lightTiler;
    std::chrono::time_point< std::chrono::steady_clock > startFrameTimePoint;

    struct Samplers
    {
        id<MTLSamplerState> linearRepeat;
        id<MTLSamplerState> linearClamp;
        id<MTLSamplerState> pointRepeat;
        id<MTLSamplerState> pointClamp;
    } samplers;
    
    void CreateSamplers()
    {
        MTLSamplerDescriptor *samplerDescriptor = [MTLSamplerDescriptor new];

        samplerDescriptor.minFilter = MTLSamplerMinMagFilterLinear;
        samplerDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
        samplerDescriptor.sAddressMode = MTLSamplerAddressModeRepeat;
        samplerDescriptor.tAddressMode = MTLSamplerAddressModeRepeat;
        samplerDescriptor.rAddressMode = MTLSamplerAddressModeRepeat;
        samplerDescriptor.label = @"linear repeat";
        samplers.linearRepeat = [device newSamplerStateWithDescriptor:samplerDescriptor];

        samplerDescriptor.minFilter = MTLSamplerMinMagFilterLinear;
        samplerDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
        samplerDescriptor.sAddressMode = MTLSamplerAddressModeClampToEdge;
        samplerDescriptor.tAddressMode = MTLSamplerAddressModeClampToEdge;
        samplerDescriptor.rAddressMode = MTLSamplerAddressModeClampToEdge;
        samplerDescriptor.label = @"linear clamp";
        samplers.linearClamp = [device newSamplerStateWithDescriptor:samplerDescriptor];

        samplerDescriptor.minFilter = MTLSamplerMinMagFilterNearest;
        samplerDescriptor.magFilter = MTLSamplerMinMagFilterNearest;
        samplerDescriptor.sAddressMode = MTLSamplerAddressModeRepeat;
        samplerDescriptor.tAddressMode = MTLSamplerAddressModeRepeat;
        samplerDescriptor.rAddressMode = MTLSamplerAddressModeRepeat;
        samplerDescriptor.label = @"point repeat";
        samplers.pointRepeat = [device newSamplerStateWithDescriptor:samplerDescriptor];

        samplerDescriptor.minFilter = MTLSamplerMinMagFilterNearest;
        samplerDescriptor.magFilter = MTLSamplerMinMagFilterNearest;
        samplerDescriptor.sAddressMode = MTLSamplerAddressModeClampToEdge;
        samplerDescriptor.tAddressMode = MTLSamplerAddressModeClampToEdge;
        samplerDescriptor.rAddressMode = MTLSamplerAddressModeClampToEdge;
        samplerDescriptor.label = @"point clamp";
        samplers.pointClamp = [device newSamplerStateWithDescriptor:samplerDescriptor];
        
        samplerStates[ 0 ] = samplers.pointClamp;
        samplerStates[ 1 ] = samplers.pointClamp;
    }
    
    void SetSampler( int textureUnit, ae3d::TextureFilter filter, ae3d::TextureWrap wrap )
    {
        if (textureUnit > 1)
        {
            ae3d::System::Print( "Trying to set a sampler with too high index\n" );
            return;
        }
        
        if (filter == ae3d::TextureFilter::Nearest && wrap == ae3d::TextureWrap::Clamp)
        {
            samplerStates[ textureUnit ] = samplers.pointClamp;
        }
        else if (filter == ae3d::TextureFilter::Nearest && wrap == ae3d::TextureWrap::Repeat)
        {
            samplerStates[ textureUnit ] = samplers.pointRepeat;
        }
        else if (filter == ae3d::TextureFilter::Linear && wrap == ae3d::TextureWrap::Clamp)
        {
            samplerStates[ textureUnit ] = samplers.linearClamp;
        }
        else if (filter == ae3d::TextureFilter::Linear && wrap == ae3d::TextureWrap::Repeat)
        {
            samplerStates[ textureUnit ] = samplers.linearRepeat;
        }
        else
        {
            ae3d::System::Assert( false, "unhandled texture state" );
        }
    }
}

namespace ae3d
{
    namespace System
    {
        namespace Statistics
        {
            std::string GetStatistics()
            {
                std::stringstream stm;
                stm << "frame time: " << ::Statistics::frameTimeMS << "\n";
                stm << "draw calls: " << ::Statistics::drawCalls << "\n";
                stm << "create uniform buffer calls: " << ::Statistics::createUniformBufferCalls << "\n";
                return stm.str();
            }
        }
    }
}

void UpdateFrameTiming()
{
    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff = std::chrono::duration<double, std::milli>( tEnd - GfxDeviceGlobal::startFrameTimePoint ).count();
    Statistics::frameTimeMS = static_cast< float >(tDiff);
}

namespace
{
    float clearColor[] = { 0, 0, 0, 1 };
    
    void setupRenderPassDescriptor( id <MTLTexture> texture, id <MTLTexture> depthTexture )
    {
        MTLLoadAction texLoadAction = MTLLoadActionLoad;
        MTLLoadAction depthLoadAction = MTLLoadActionLoad;

        auto renderPassDescriptor = GfxDeviceGlobal::isRenderingToTexture ? renderPassDescriptorFBO : renderPassDescriptorApp;

        if (GfxDeviceGlobal::clearFlags & ae3d::GfxDevice::ClearFlags::Color)
        {
            texLoadAction = MTLLoadActionClear;
        }
        
        if (GfxDeviceGlobal::clearFlags & ae3d::GfxDevice::ClearFlags::Depth)
        {
            depthLoadAction = MTLLoadActionClear;
        }
        
        if (GfxDeviceGlobal::sampleCount > 1 && !GfxDeviceGlobal::isRenderingToTexture)
        {
            renderPassDescriptor.colorAttachments[0].texture = msaaColorTarget;
            renderPassDescriptor.colorAttachments[0].resolveTexture = texture;
            renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionMultisampleResolve;
        }
        else
        {
            renderPassDescriptor.colorAttachments[0].slice = currentCubeMapFace;
            renderPassDescriptor.colorAttachments[0].texture = texture;
            renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
        }
        renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake( clearColor[0], clearColor[1], clearColor[2], clearColor[3] );
        renderPassDescriptor.colorAttachments[0].loadAction = texLoadAction;
        
        renderPassDescriptor.stencilAttachment = nil;

        if (GfxDeviceGlobal::sampleCount > 1 && !GfxDeviceGlobal::isRenderingToTexture)
        {
            renderPassDescriptor.depthAttachment.texture = msaaDepthTarget;
            renderPassDescriptor.depthAttachment.loadAction = depthLoadAction;
            renderPassDescriptor.depthAttachment.clearDepth = 1.0;
        }
        else if (GfxDeviceGlobal::sampleCount == 1 && GfxDeviceGlobal::isRenderingToTexture)
        {
            renderPassDescriptor.depthAttachment.slice = currentCubeMapFace;
            renderPassDescriptor.depthAttachment.texture = depthTexture;
            renderPassDescriptor.depthAttachment.loadAction = depthLoadAction;
            renderPassDescriptor.depthAttachment.clearDepth = 1.0;
        }
    }
}

id <MTLBuffer> ae3d::GfxDevice::GetNewUniformBuffer()
{
    id<MTLBuffer> uniformBuffer = [GfxDevice::GetMetalDevice() newBufferWithLength:256 options:MTLResourceCPUCacheModeDefaultCache];
    uniformBuffer.label = @"uniform buffer";

    ++Statistics::createUniformBufferCalls;
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

void ae3d::GfxDevice::SetPolygonOffset( bool enable, float factor, float units )
{
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
    renderPassDescriptorApp = renderPass;
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

    commandQueue = [device newCommandQueue];
    defaultLibrary = [device newDefaultLibrary];

    renderPassDescriptorFBO = [MTLRenderPassDescriptor renderPassDescriptor];

    GfxDeviceGlobal::backBufferWidth = view.bounds.size.width;
    GfxDeviceGlobal::backBufferHeight = view.bounds.size.height;
    GfxDeviceGlobal::sampleCount = sampleCount;
    
    MTLDepthStencilDescriptor *depthStateDesc = [[MTLDepthStencilDescriptor alloc] init];
    depthStateDesc.depthCompareFunction = MTLCompareFunctionLessEqual;
    depthStateDesc.depthWriteEnabled = YES;
    depthStateDesc.label = @"lessEqual write on";
    depthStateLessEqualWriteOn = [device newDepthStencilStateWithDescriptor:depthStateDesc];

    depthStateDesc.depthCompareFunction = MTLCompareFunctionLessEqual;
    depthStateDesc.depthWriteEnabled = NO;
    depthStateDesc.label = @"lessEqual write off";
    depthStateLessEqualWriteOff = [device newDepthStencilStateWithDescriptor:depthStateDesc];

    depthStateDesc.depthCompareFunction = MTLCompareFunctionAlways;
    depthStateDesc.depthWriteEnabled = NO;
    depthStateDesc.label = @"none write off";
    depthStateNoneWriteOff = [device newDepthStencilStateWithDescriptor:depthStateDesc];

    GfxDeviceGlobal::CreateSamplers();
    GfxDeviceGlobal::lightTiler.Init();

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
    msaaColorTarget.label = @"MSAA Color Target";
    
    MTLTextureDescriptor* depthDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                                                                    width:GfxDeviceGlobal::backBufferWidth * 2
                                                                                   height:GfxDeviceGlobal::backBufferHeight * 2
                                                                                mipmapped:NO];

    depthDesc.textureType = MTLTextureType2DMultisample;
    depthDesc.sampleCount = sampleCount;
    depthDesc.resourceOptions = MTLResourceStorageModePrivate;
    depthDesc.usage = MTLTextureUsageRenderTarget;
    
    msaaDepthTarget = [device newTextureWithDescriptor:depthDesc];
    msaaDepthTarget.label = @"MSAA Depth Target";
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

std::string GetPSOHash( ae3d::Shader& shader, ae3d::GfxDevice::BlendMode blendMode, ae3d::GfxDevice::DepthFunc depthFunc,
                       ae3d::VertexBuffer::VertexFormat vertexFormat, ae3d::RenderTexture::DataType pixelFormat, int sampleCount )
{
    std::string hashString;
    hashString += std::to_string( (ptrdiff_t)&shader.vertexProgram );
    hashString += std::to_string( (ptrdiff_t)&shader.fragmentProgram );
    hashString += std::to_string( (unsigned)blendMode );
    hashString += std::to_string( ((unsigned)depthFunc) );
    hashString += std::to_string( ((unsigned)vertexFormat) );
    hashString += std::to_string( ((unsigned)pixelFormat) );
    hashString += std::to_string( sampleCount );
    return hashString;
}

id <MTLRenderPipelineState> GetPSO( ae3d::Shader& shader, ae3d::GfxDevice::BlendMode blendMode, ae3d::GfxDevice::DepthFunc depthFunc,
                                    ae3d::VertexBuffer::VertexFormat vertexFormat, ae3d::RenderTexture::DataType pixelFormat,
                                    int sampleCount )
{
    const std::string psoHash = GetPSOHash( shader, blendMode, depthFunc, vertexFormat, pixelFormat, sampleCount );

    if (GfxDeviceGlobal::psoCache.find( psoHash ) == std::end( GfxDeviceGlobal::psoCache ))
    {
        MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        pipelineStateDescriptor.sampleCount = sampleCount;
        pipelineStateDescriptor.vertexFunction = shader.vertexProgram;
        pipelineStateDescriptor.fragmentFunction = shader.fragmentProgram;
#if !TARGET_OS_IPHONE
        pipelineStateDescriptor.inputPrimitiveTopology = MTLPrimitiveTopologyClassTriangle;
#endif
        pipelineStateDescriptor.colorAttachments[0].pixelFormat = (pixelFormat == ae3d::RenderTexture::DataType::UByte ? MTLPixelFormatBGRA8Unorm : MTLPixelFormatRGBA32Float);
        pipelineStateDescriptor.colorAttachments[0].blendingEnabled = blendMode != ae3d::GfxDevice::BlendMode::Off;
        pipelineStateDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        pipelineStateDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pipelineStateDescriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        pipelineStateDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
        pipelineStateDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pipelineStateDescriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        // This must match app's view's format.
        pipelineStateDescriptor.depthAttachmentPixelFormat = (GfxDeviceGlobal::sampleCount > 1 && GfxDeviceGlobal::isRenderingToTexture) ?  MTLPixelFormatInvalid : MTLPixelFormatDepth32Float;

        MTLVertexDescriptor* vertexDesc = [MTLVertexDescriptor vertexDescriptor];
        
        if (vertexFormat == ae3d::VertexBuffer::VertexFormat::PTNTC)
        {
            pipelineStateDescriptor.label = @"pipeline PTNTC";

            // Position
            vertexDesc.attributes[0].format = MTLVertexFormatFloat3;
            vertexDesc.attributes[0].bufferIndex = 0;
            vertexDesc.attributes[0].offset = 0;
            
            // Texcoord
            vertexDesc.attributes[1].format = MTLVertexFormatFloat2;
            vertexDesc.attributes[1].bufferIndex = 0;
            vertexDesc.attributes[1].offset = 4 * 3;

            // Normal
            vertexDesc.attributes[3].format = MTLVertexFormatFloat3;
            vertexDesc.attributes[3].bufferIndex = 0;
            vertexDesc.attributes[3].offset = 4 * 5;

            // Tangent
            vertexDesc.attributes[4].format = MTLVertexFormatFloat4;
            vertexDesc.attributes[4].bufferIndex = 0;
            vertexDesc.attributes[4].offset = 4 * 8;

            // Color
            vertexDesc.attributes[2].format = MTLVertexFormatFloat4;
            vertexDesc.attributes[2].bufferIndex = 0;
            vertexDesc.attributes[2].offset = 4 * 12;

            vertexDesc.layouts[0].stride = sizeof( ae3d::VertexBuffer::VertexPTNTC );
            vertexDesc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
        }
        else if (vertexFormat == ae3d::VertexBuffer::VertexFormat::PTN)
        {
            pipelineStateDescriptor.label = @"pipeline PTN";
            
            // Position
            vertexDesc.attributes[0].format = MTLVertexFormatFloat3;
            vertexDesc.attributes[0].bufferIndex = 0;
            vertexDesc.attributes[0].offset = 0;
            
            // Texcoord
            vertexDesc.attributes[1].format = MTLVertexFormatFloat2;
            vertexDesc.attributes[1].bufferIndex = 0;
            vertexDesc.attributes[1].offset = 4 * 3;

            // Color
            vertexDesc.attributes[2].format = MTLVertexFormatFloat4;
            vertexDesc.attributes[2].bufferIndex = 1;
            vertexDesc.attributes[2].offset = 0;

            // Normal
            vertexDesc.attributes[3].format = MTLVertexFormatFloat3;
            vertexDesc.attributes[3].bufferIndex = 0;
            vertexDesc.attributes[3].offset = 4 * 5;
            
            vertexDesc.layouts[0].stride = sizeof( ae3d::VertexBuffer::VertexPTN );
            vertexDesc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;

            vertexDesc.layouts[1].stride = 4 * 4;
            vertexDesc.layouts[1].stepFunction = MTLVertexStepFunctionPerVertex;
        }
        else if (vertexFormat == ae3d::VertexBuffer::VertexFormat::PTC)
        {
            pipelineStateDescriptor.label = @"pipeline PTC";
            
            // Position
            vertexDesc.attributes[0].format = MTLVertexFormatFloat3;
            vertexDesc.attributes[0].bufferIndex = 0;
            vertexDesc.attributes[0].offset = 0;
            
            // Texcoord
            vertexDesc.attributes[1].format = MTLVertexFormatFloat2;
            vertexDesc.attributes[1].bufferIndex = 0;
            vertexDesc.attributes[1].offset = 4 * 3;
            
            // Color
            vertexDesc.attributes[2].format = MTLVertexFormatFloat4;
            vertexDesc.attributes[2].bufferIndex = 0;
            vertexDesc.attributes[2].offset = 4 * 5;
            
            // Normal
            vertexDesc.attributes[3].format = MTLVertexFormatFloat3;
            vertexDesc.attributes[3].bufferIndex = 1;
            vertexDesc.attributes[3].offset = 0;
            
            vertexDesc.layouts[0].stride = sizeof( ae3d::VertexBuffer::VertexPTC );
            vertexDesc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
            
            vertexDesc.layouts[1].stride = 3 * 4;
            vertexDesc.layouts[1].stepFunction = MTLVertexStepFunctionPerVertex;
        }
        else
        {
            ae3d::System::Assert( false, "unimplemented vertex format" );
        }
        
        pipelineStateDescriptor.vertexDescriptor = vertexDesc;
        
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
    if (!GfxDeviceGlobal::lightTiler.CullerUniformsCreated())
    {
        return;
    }

    ++Statistics::drawCalls;

    if (!texture0)
    {
        texture0 = Texture2D::GetDefaultTexture()->GetMetalTexture();
    }
    if (!texture1)
    {
        texture1 = Texture2D::GetDefaultTexture()->GetMetalTexture();
    }
    
    RenderTexture::DataType pixelFormat = GfxDeviceGlobal::currentRenderTargetDataType;

    const int sampleCount = GfxDeviceGlobal::isRenderingToTexture ? 1 : GfxDeviceGlobal::sampleCount;
    [renderEncoder setRenderPipelineState:GetPSO( shader, blendMode, depthFunc, vertexBuffer.GetVertexFormat(), pixelFormat, sampleCount )];
    [renderEncoder setVertexBuffer:vertexBuffer.GetVertexBuffer() offset:0 atIndex:0];
    [renderEncoder setVertexBuffer:GetCurrentUniformBuffer() offset:0 atIndex:5];

    if (shader.GetMetalVertexShaderName() == "standard_vertex")
    {
        [renderEncoder setFragmentBuffer:GfxDeviceGlobal::lightTiler.GetPerTileLightIndexBuffer() offset:0 atIndex:6];
        [renderEncoder setFragmentBuffer:GfxDeviceGlobal::lightTiler.GetCullerUniforms() offset:0 atIndex:8];
    }

    [renderEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
    [renderEncoder setCullMode:(cullMode == CullMode::Back) ? MTLCullModeBack : MTLCullModeNone];
    [renderEncoder setFragmentTexture:texture0 atIndex:0];
    [renderEncoder setFragmentTexture:texture1 atIndex:1];
    
    if (depthFunc == DepthFunc::LessOrEqualWriteOff)
    {
        [renderEncoder setDepthStencilState:depthStateLessEqualWriteOff];
    }
    else if (depthFunc == DepthFunc::LessOrEqualWriteOn)
    {
        [renderEncoder setDepthStencilState:depthStateLessEqualWriteOn];
    }
    else if (depthFunc == DepthFunc::NoneWriteOff)
    {
        [renderEncoder setDepthStencilState:depthStateNoneWriteOff];
    }
    else
    {
        System::Assert( false, "unhandled depth function" );
        [renderEncoder setDepthStencilState:depthStateLessEqualWriteOn];
    }
    
    [renderEncoder setFragmentSamplerState:GfxDeviceGlobal::samplerStates[ 0 ] atIndex:0];
    [renderEncoder setFragmentSamplerState:GfxDeviceGlobal::samplerStates[ 1 ] atIndex:1];
    
    if (vertexBuffer.GetVertexFormat() == VertexBuffer::VertexFormat::PTNTC)
    {
        // No need to set extra buffers as vertexBuffer contains all attributes.
    }
    else if (vertexBuffer.GetVertexFormat() == VertexBuffer::VertexFormat::PTN)
    {
        [renderEncoder setVertexBuffer:vertexBuffer.colorBuffer offset:0 atIndex:1];
    }
    else if (vertexBuffer.GetVertexFormat() == VertexBuffer::VertexFormat::PTC)
    {
        [renderEncoder setVertexBuffer:vertexBuffer.normalBuffer offset:0 atIndex:1];
    }
    else
    {
        System::Assert( false, "Unhandled vertex format" );
    }
    
    [renderEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                              indexCount:(endIndex - startIndex) * 3
                               indexType:MTLIndexTypeUInt16
                             indexBuffer:vertexBuffer.GetIndexBuffer()
                       indexBufferOffset:startIndex * 2 * 3];    
}

void ae3d::GfxDevice::InsertDebugBoundary()
{
    [commandQueue insertDebugCaptureBoundary];
}

void ae3d::GfxDevice::BeginFrame()
{
    ResetUniformBuffers();
    
    commandBuffer = [commandQueue commandBuffer];
    commandBuffer.label = @"MyCommand";

    renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:GfxDeviceGlobal::isRenderingToTexture ? renderPassDescriptorFBO : renderPassDescriptorApp];
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
    Statistics::createUniformBufferCalls = 0;
    GfxDeviceGlobal::startFrameTimePoint = std::chrono::high_resolution_clock::now();
}

void ae3d::GfxDevice::ReleaseGPUObjects()
{
}

void ae3d::GfxDevice::SetMultiSampling( bool enable )
{
}

void ae3d::GfxDevice::SetRenderTarget( ae3d::RenderTexture* renderTexture2d, unsigned cubeMapFace )
{
    GfxDeviceGlobal::isRenderingToTexture = renderTexture2d != nullptr;

    currentCubeMapFace = cubeMapFace;
    drawable = currentDrawable;
    setupRenderPassDescriptor( renderTexture2d != nullptr ? renderTexture2d->GetMetalTexture() : drawable.texture,
                               renderTexture2d != nullptr ? renderTexture2d->GetMetalDepthTexture() : nil );
    
    if (!renderTexture2d)
    {
        GfxDeviceGlobal::currentRenderTargetDataType = ae3d::RenderTexture::DataType::UByte;
    }
    else
    {
        GfxDeviceGlobal::currentRenderTargetDataType = renderTexture2d->GetDataType();
    }
}

void ae3d::GfxDevice::ErrorCheck( const char* )
{
}
