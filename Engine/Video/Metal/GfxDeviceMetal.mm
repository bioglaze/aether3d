#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include <cmath>
#include <unordered_map>
#include <sstream>
#include <list>
#include "GfxDevice.hpp"
#include "LightTiler.hpp"
#include "VertexBuffer.hpp"
#include "Shader.hpp"
#include "System.hpp"
#include "Statistics.hpp"
#include "Renderer.hpp"
#include "RenderTexture.hpp"
#include "Texture2D.hpp"

float GetFloatAnisotropy( ae3d::Anisotropy anisotropy );

extern ae3d::Renderer renderer;

id <MTLDevice> device;
id <MTLCommandQueue> commandQueue;
id <MTLLibrary> defaultLibrary;
id <MTLDepthStencilState> depthStateLessEqualWriteOn;
id <MTLDepthStencilState> depthStateLessEqualWriteOff;
id <MTLDepthStencilState> depthStateNoneWriteOff;
id <CAMetalDrawable> currentDrawable; // This frame's framebuffer drawable

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

enum SamplerIndexByAnisotropy : int
{
    One = 0,
    Two,
    Four,
    Eight
};

int GetSamplerIndexByAnisotropy( ae3d::Anisotropy anisotropy )
{
    const float floatAnisotropy = GetFloatAnisotropy( anisotropy );
    
    if (floatAnisotropy == 1)
    {
        return SamplerIndexByAnisotropy::One;
    }
    if (floatAnisotropy == 2)
    {
        return SamplerIndexByAnisotropy::Two;
    }
    if (floatAnisotropy == 4)
    {
        return SamplerIndexByAnisotropy::Four;
    }
    if (floatAnisotropy == 8)
    {
        return SamplerIndexByAnisotropy::Eight;
    }

    return SamplerIndexByAnisotropy::One;
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
    std::vector< id<MTLBuffer> > uniformBuffers;
    int currentUboIndex;
    ae3d::RenderTexture::DataType currentRenderTargetDataType = ae3d::RenderTexture::DataType::UByte;
    ae3d::LightTiler lightTiler;
    std::vector< ae3d::VertexBuffer > lineBuffers;
    int viewport[ 4 ];
    
    struct Samplers
    {
        id<MTLSamplerState> linearRepeat;
        id<MTLSamplerState> linearClamp;
        id<MTLSamplerState> pointRepeat;
        id<MTLSamplerState> pointClamp;
    } samplers[ 5 ];
    
    void CreateSamplers()
    {
        for (int samplerIndex = 0; samplerIndex < 4; ++samplerIndex)
        {
            MTLSamplerDescriptor *samplerDescriptor = [MTLSamplerDescriptor new];

            samplerDescriptor.minFilter = MTLSamplerMinMagFilterLinear;
            samplerDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
            samplerDescriptor.sAddressMode = MTLSamplerAddressModeRepeat;
            samplerDescriptor.tAddressMode = MTLSamplerAddressModeRepeat;
            samplerDescriptor.rAddressMode = MTLSamplerAddressModeRepeat;
            samplerDescriptor.maxAnisotropy = (NSUInteger)std::pow( 2, samplerIndex );
            samplerDescriptor.label = @"linear repeat";
            samplers[ samplerIndex ].linearRepeat = [device newSamplerStateWithDescriptor:samplerDescriptor];

            samplerDescriptor.minFilter = MTLSamplerMinMagFilterLinear;
            samplerDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
            samplerDescriptor.sAddressMode = MTLSamplerAddressModeClampToEdge;
            samplerDescriptor.tAddressMode = MTLSamplerAddressModeClampToEdge;
            samplerDescriptor.rAddressMode = MTLSamplerAddressModeClampToEdge;
            samplerDescriptor.maxAnisotropy = (NSUInteger)std::pow( 2, samplerIndex );
            samplerDescriptor.label = @"linear clamp";
            samplers[ samplerIndex ].linearClamp = [device newSamplerStateWithDescriptor:samplerDescriptor];

            samplerDescriptor.minFilter = MTLSamplerMinMagFilterNearest;
            samplerDescriptor.magFilter = MTLSamplerMinMagFilterNearest;
            samplerDescriptor.sAddressMode = MTLSamplerAddressModeRepeat;
            samplerDescriptor.tAddressMode = MTLSamplerAddressModeRepeat;
            samplerDescriptor.rAddressMode = MTLSamplerAddressModeRepeat;
            samplerDescriptor.maxAnisotropy = (NSUInteger)std::pow( 2, samplerIndex );
            samplerDescriptor.label = @"point repeat";
            samplers[ samplerIndex ].pointRepeat = [device newSamplerStateWithDescriptor:samplerDescriptor];

            samplerDescriptor.minFilter = MTLSamplerMinMagFilterNearest;
            samplerDescriptor.magFilter = MTLSamplerMinMagFilterNearest;
            samplerDescriptor.sAddressMode = MTLSamplerAddressModeClampToEdge;
            samplerDescriptor.tAddressMode = MTLSamplerAddressModeClampToEdge;
            samplerDescriptor.rAddressMode = MTLSamplerAddressModeClampToEdge;
            samplerDescriptor.maxAnisotropy = (NSUInteger)std::pow( 2, samplerIndex );
            samplerDescriptor.label = @"point clamp";
            samplers[ samplerIndex ].pointClamp = [device newSamplerStateWithDescriptor:samplerDescriptor];
        }

        samplerStates[ 0 ] = samplers[ SamplerIndexByAnisotropy::One ].pointClamp;
        samplerStates[ 1 ] = samplers[ SamplerIndexByAnisotropy::One ].pointClamp;
    }
    
    void SetSampler( int textureUnit, ae3d::TextureFilter filter, ae3d::TextureWrap wrap, ae3d::Anisotropy anisotropy )
    {
        if (textureUnit > 1)
        {
            ae3d::System::Print( "Trying to set a sampler with too high index\n" );
            return;
        }
        
        if (filter == ae3d::TextureFilter::Nearest && wrap == ae3d::TextureWrap::Clamp)
        {
            samplerStates[ textureUnit ] = samplers[ GetSamplerIndexByAnisotropy( anisotropy ) ].pointClamp;
        }
        else if (filter == ae3d::TextureFilter::Nearest && wrap == ae3d::TextureWrap::Repeat)
        {
            samplerStates[ textureUnit ] = samplers[ GetSamplerIndexByAnisotropy( anisotropy ) ].pointRepeat;
        }
        else if (filter == ae3d::TextureFilter::Linear && wrap == ae3d::TextureWrap::Clamp)
        {
            samplerStates[ textureUnit ] = samplers[ GetSamplerIndexByAnisotropy( anisotropy ) ].linearClamp;
        }
        else if (filter == ae3d::TextureFilter::Linear && wrap == ae3d::TextureWrap::Repeat)
        {
            samplerStates[ textureUnit ] = samplers[ GetSamplerIndexByAnisotropy( anisotropy ) ].linearRepeat;
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
                stm << "frame time: " << ::Statistics::GetFrameTimeMS() << "\n";
                stm << "shadow map time: " << ::Statistics::GetShadowMapTimeMS() << " ms\n";
                stm << "depth pass time: " << ::Statistics::GetDepthNormalsTimeMS() << " ms\n";
                stm << "draw calls: " << ::Statistics::GetDrawCalls() << "\n";
                stm << "create uniform buffer calls: " << ::Statistics::GetCreateConstantBufferCalls() << "\n";
                return stm.str();
            }
        }
    }
}

namespace
{
    float clearColor[] = { 0, 0, 0, 1 };
}

id <MTLBuffer> ae3d::GfxDevice::GetNewUniformBuffer()
{
    GfxDeviceGlobal::currentUboIndex = (GfxDeviceGlobal::currentUboIndex + 1) % GfxDeviceGlobal::uniformBuffers.size();
    return GfxDeviceGlobal::uniformBuffers[ GfxDeviceGlobal::currentUboIndex ];
}

id <MTLBuffer> ae3d::GfxDevice::GetCurrentUniformBuffer()
{
    return GfxDeviceGlobal::uniformBuffers[ GfxDeviceGlobal::currentUboIndex ];
}

void ae3d::GfxDevice::SetPolygonOffset( bool enable, float factor, float units )
{
}

void ae3d::GfxDevice::SetViewport( int viewport[ 4 ] )
{
    GfxDeviceGlobal::viewport[ 0 ] = viewport[ 0 ];
    GfxDeviceGlobal::viewport[ 1 ] = viewport[ 1 ];
    GfxDeviceGlobal::viewport[ 2 ] = viewport[ 2 ];
    GfxDeviceGlobal::viewport[ 3 ] = viewport[ 3 ];
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
    GfxDeviceGlobal::uniformBuffers.resize( 1400 );
    
    for (std::size_t uboIndex = 0; uboIndex < GfxDeviceGlobal::uniformBuffers.size(); ++uboIndex)
    {
        GfxDeviceGlobal::uniformBuffers[ uboIndex ] = [GfxDevice::GetMetalDevice() newBufferWithLength:UNIFORM_BUFFER_SIZE options:MTLResourceCPUCacheModeDefaultCache];
        GfxDeviceGlobal::uniformBuffers[ uboIndex ].label = @"uniform buffer";
    }
    
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
                       ae3d::VertexBuffer::VertexFormat vertexFormat, ae3d::RenderTexture::DataType pixelFormat,
                       MTLPixelFormat depthFormat, int sampleCount, ae3d::GfxDevice::PrimitiveTopology topology )
{
    std::string hashString;
    hashString += std::to_string( (ptrdiff_t)&shader.vertexProgram );
    hashString += std::to_string( (ptrdiff_t)&shader.fragmentProgram );
    hashString += std::to_string( (unsigned)blendMode );
    hashString += std::to_string( ((unsigned)depthFunc) );
    hashString += std::to_string( ((unsigned)vertexFormat) );
    hashString += std::to_string( ((unsigned)pixelFormat) );
    hashString += std::to_string( ((unsigned)depthFormat) );
    hashString += std::to_string( sampleCount );
    hashString += std::to_string( ((unsigned)topology) );
    return hashString;
}

id <MTLRenderPipelineState> GetPSO( ae3d::Shader& shader, ae3d::GfxDevice::BlendMode blendMode, ae3d::GfxDevice::DepthFunc depthFunc,
                                    ae3d::VertexBuffer::VertexFormat vertexFormat, ae3d::RenderTexture::DataType pixelFormat,
                                   MTLPixelFormat depthFormat, int sampleCount, ae3d::GfxDevice::PrimitiveTopology topology )
{
    const std::string psoHash = GetPSOHash( shader, blendMode, depthFunc, vertexFormat, pixelFormat, depthFormat, sampleCount, topology );

    if (GfxDeviceGlobal::psoCache.find( psoHash ) == std::end( GfxDeviceGlobal::psoCache ))
    {
        MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        pipelineStateDescriptor.sampleCount = sampleCount;
        pipelineStateDescriptor.vertexFunction = shader.vertexProgram;
        pipelineStateDescriptor.fragmentFunction = shader.fragmentProgram;
#if !TARGET_OS_IPHONE
        pipelineStateDescriptor.inputPrimitiveTopology = (topology == ae3d::GfxDevice::PrimitiveTopology::Triangles) ?
        MTLPrimitiveTopologyClassTriangle : MTLPrimitiveTopologyClassLine;
#endif
        MTLPixelFormat format = MTLPixelFormatBGRA8Unorm;

        if (pixelFormat == ae3d::RenderTexture::DataType::R32G32)
        {
            format = MTLPixelFormatRG32Float;
        }
        else if (pixelFormat == ae3d::RenderTexture::DataType::Float)
        {
            format = MTLPixelFormatRGBA32Float;
        }

        pipelineStateDescriptor.colorAttachments[0].pixelFormat = format;
        pipelineStateDescriptor.colorAttachments[0].blendingEnabled = blendMode != ae3d::GfxDevice::BlendMode::Off;
        pipelineStateDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        pipelineStateDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pipelineStateDescriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        pipelineStateDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
        pipelineStateDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pipelineStateDescriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        pipelineStateDescriptor.depthAttachmentPixelFormat = depthFormat;

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

int ae3d::GfxDevice::CreateLineBuffer( const std::vector< Vec3 >& lines, const Vec3& color )
{
    std::vector< VertexBuffer::Face > faces( lines.size() * 2 );
    
    std::vector< VertexBuffer::VertexPTC > vertices( lines.size() );
    
    for (std::size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex)
    {
        vertices[ lineIndex ].position = lines[ lineIndex ];
        vertices[ lineIndex ].color = Vec4( color, 1 );
    }
    
    for (unsigned short faceIndex = 0; faceIndex < (unsigned short)(faces.size() / 2); ++faceIndex)
    {
        faces[ faceIndex * 2 + 0 ].a = faceIndex;
        faces[ faceIndex * 2 + 1 ].b = faceIndex + 1;
    }
    
    GfxDeviceGlobal::lineBuffers.push_back( VertexBuffer() );
    GfxDeviceGlobal::lineBuffers.back().Generate( faces.data(), int( faces.size() ), vertices.data(), int( vertices.size() ) );
    GfxDeviceGlobal::lineBuffers.back().SetDebugName( "line buffer" );
    
    return int( GfxDeviceGlobal::lineBuffers.size() ) - 1;
}

void ae3d::GfxDevice::DrawLines( int handle, Shader& shader )
{
    if (handle < 0 || handle >= int( GfxDeviceGlobal::lineBuffers.size() ))
    {
        return;
    }
    
    Draw( GfxDeviceGlobal::lineBuffers[ handle ], 0, GfxDeviceGlobal::lineBuffers[ handle ].GetFaceCount(), shader, BlendMode::Off, DepthFunc::NoneWriteOff, CullMode::Off, FillMode::Solid, GfxDevice::PrimitiveTopology::Lines );
}

void ae3d::GfxDevice::Draw( VertexBuffer& vertexBuffer, int startIndex, int endIndex, Shader& shader, BlendMode blendMode, DepthFunc depthFunc, CullMode cullMode, FillMode fillMode, PrimitiveTopology topology )
{
    if (!GfxDeviceGlobal::lightTiler.CullerUniformsCreated() && shader.GetMetalVertexShaderName() == "standard_vertex")
    {
        return;
    }
    
    Statistics::IncDrawCalls();

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
    MTLPixelFormat depthFormat = MTLPixelFormatDepth32Float;

#if TARGET_OS_IPHONE
    if (GfxDeviceGlobal::sampleCount == 1)
    {
        depthFormat = MTLPixelFormatDepth32Float;
    }
    if (GfxDeviceGlobal::sampleCount > 1)
    {
        depthFormat = MTLPixelFormatDepth32Float;
    }
    if (GfxDeviceGlobal::isRenderingToTexture)
    {
        depthFormat = MTLPixelFormatInvalid;
    }
    if (GfxDeviceGlobal::sampleCount == 1 && GfxDeviceGlobal::isRenderingToTexture)
    {
        depthFormat = MTLPixelFormatDepth32Float;
    }

#else
    if (GfxDeviceGlobal::sampleCount == 1 && GfxDeviceGlobal::isRenderingToTexture)
    {
        depthFormat = MTLPixelFormatDepth32Float;
    }
    if (GfxDeviceGlobal::sampleCount > 1 && GfxDeviceGlobal::isRenderingToTexture)
    {
        depthFormat = MTLPixelFormatInvalid;
    }
#endif

    [renderEncoder setRenderPipelineState:GetPSO( shader, blendMode, depthFunc, vertexBuffer.GetVertexFormat(), pixelFormat,
                                                  depthFormat, sampleCount, topology )];
    [renderEncoder setVertexBuffer:vertexBuffer.GetVertexBuffer() offset:0 atIndex:0];
    [renderEncoder setVertexBuffer:GetCurrentUniformBuffer() offset:0 atIndex:5];
    
    MTLViewport viewport;
    viewport.originX = GfxDeviceGlobal::viewport[ 0 ];
    viewport.originY = GfxDeviceGlobal::viewport[ 1 ];
    viewport.width = GfxDeviceGlobal::viewport[ 2 ];
    viewport.height = GfxDeviceGlobal::viewport[ 3 ];
    viewport.znear = 0;
    viewport.zfar = 1;
    [renderEncoder setViewport:viewport];
    
    if (shader.GetMetalVertexShaderName() == "standard_vertex")
    {
        [renderEncoder setFragmentBuffer:GfxDeviceGlobal::lightTiler.GetPerTileLightIndexBuffer() offset:0 atIndex:6];
        [renderEncoder setFragmentBuffer:GfxDeviceGlobal::lightTiler.GetPointLightCenterAndRadiusBuffer() offset:0 atIndex:7];
        [renderEncoder setFragmentBuffer:GfxDeviceGlobal::lightTiler.GetCullerUniforms() offset:0 atIndex:8];
        [renderEncoder setFragmentBuffer:GetCurrentUniformBuffer() offset:0 atIndex:5];
    }

    [renderEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
    [renderEncoder setCullMode:(cullMode == CullMode::Back) ? MTLCullModeBack : MTLCullModeNone];
    [renderEncoder setTriangleFillMode:(fillMode == FillMode::Solid ? MTLTriangleFillMode::MTLTriangleFillModeFill : MTLTriangleFillMode::MTLTriangleFillModeLines)];
    [renderEncoder setFragmentTexture:texture0 atIndex:0];
    [renderEncoder setFragmentTexture:texture1 atIndex:1];
    [renderEncoder setFragmentTexture:texture0 atIndex:2];
    [renderEncoder setFragmentTexture:texture0 atIndex:3];
    
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
        [renderEncoder setVertexBuffer:nil offset:0 atIndex:1];
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
    
    if (topology == PrimitiveTopology::Triangles)
    {
        [renderEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                  indexCount:(endIndex - startIndex) * 3
                               indexType:MTLIndexTypeUInt16
                             indexBuffer:vertexBuffer.GetIndexBuffer()
                       indexBufferOffset:startIndex * 2 * 3];
    }
    else // MTLPrimitiveTypeLine
    {
        [renderEncoder drawPrimitives:MTLPrimitiveTypeLine vertexStart:0 vertexCount:vertexBuffer.GetFaceCount()];
    }
}

void ae3d::GfxDevice::BeginFrame()
{
    commandBuffer = [commandQueue commandBuffer];
    commandBuffer.label = @"MyCommand";
    
    texture0 = nullptr;
    texture1 = nullptr;
}

void ae3d::GfxDevice::PresentDrawable()
{
    if (currentDrawable != nil)
    {
        [commandBuffer presentDrawable:currentDrawable];
        [commandBuffer commit];
    }
}

void ae3d::GfxDevice::SetClearColor( float red, float green, float blue )
{
    clearColor[ 0 ] = red;
    clearColor[ 1 ] = green;
    clearColor[ 2 ] = blue;
}

void ae3d::GfxDevice::ReleaseGPUObjects()
{
}

void ae3d::GfxDevice::SetMultiSampling( bool enable )
{
}

void ae3d::GfxDevice::BeginBackBufferEncoding()
{
    renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptorApp];
    renderEncoder.label = @"BackBufferRenderEncoder";

    GfxDeviceGlobal::currentRenderTargetDataType = ae3d::RenderTexture::DataType::UByte;
}

void ae3d::GfxDevice::EndBackBufferEncoding()
{
    [renderEncoder endEncoding];
}

void ae3d::GfxDevice::SetRenderTarget( ae3d::RenderTexture* renderTexture, unsigned cubeMapFace )
{
    GfxDeviceGlobal::isRenderingToTexture = renderTexture != nullptr;

    if (!renderTexture || renderTexture->GetMetalTexture() == nil)
    {
        return;
    }

    auto texture = renderTexture->GetMetalTexture();
    auto depthTexture = renderTexture->GetMetalDepthTexture();

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

    renderPassDescriptorFBO.colorAttachments[0].slice = cubeMapFace;
    renderPassDescriptorFBO.colorAttachments[0].texture = texture;
    renderPassDescriptorFBO.colorAttachments[0].storeAction = MTLStoreActionStore;
    renderPassDescriptorFBO.colorAttachments[0].clearColor = MTLClearColorMake( clearColor[0], clearColor[1], clearColor[2], clearColor[3] );
    renderPassDescriptorFBO.colorAttachments[0].loadAction = texLoadAction;
    renderPassDescriptorFBO.stencilAttachment = nil;

    if (GfxDeviceGlobal::sampleCount == 1)
    {
        renderPassDescriptorFBO.depthAttachment.slice = cubeMapFace;
        renderPassDescriptorFBO.depthAttachment.texture = depthTexture;
        renderPassDescriptorFBO.depthAttachment.loadAction = depthLoadAction;
        //renderPassDescriptorFBO.depthAttachment.storeAction = MTLStoreActionStore;
        renderPassDescriptorFBO.depthAttachment.clearDepth = 1.0;
    }

    renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptorFBO];
    renderEncoder.label = @"FboRenderEncoder";

    GfxDeviceGlobal::currentRenderTargetDataType = renderTexture->GetDataType();
}

void ae3d::GfxDevice::UnsetRenderTarget()
{
    if (GfxDeviceGlobal::isRenderingToTexture)
    {
        [renderEncoder endEncoding];
    }
    GfxDeviceGlobal::isRenderingToTexture = false;
}

void ae3d::GfxDevice::ErrorCheck( const char* )
{
}
