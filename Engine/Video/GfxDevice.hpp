#ifndef GFX_DEVICE_H
#define GFX_DEVICE_H

#include <cstdint>
#if RENDERER_METAL
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/CAMetalLayer.h>
#endif

namespace ae3d
{
    class RenderTexture;
    class VertexBuffer;
    class Shader;

    /// Low-level graphics state abstraction.
    namespace GfxDevice
    {
        enum ClearFlags : unsigned
        {
            Color = 1 << 0,
            Depth = 1 << 1,
            DontClear = 1 << 2
        };

        enum class BlendMode
        {
            AlphaBlend,
            Additive,
            Off
        };
        
        enum class DepthFunc
        {
            LessOrEqualWriteOff,
            LessOrEqualWriteOn,
            NoneWriteOff
        };
        
        enum class CullMode
        {
            Back,
            Front,
            Off
        };

        void Init( int width, int height );
#if RENDERER_D3D12
        void  CreateNewUniformBuffer();
        void* GetCurrentUniformBuffer();
#endif
#if RENDERER_METAL
        void InitMetal( id <MTLDevice> metalDevice, MTKView* view, int sampleCount );
        void SetCurrentDrawableMetal( id <CAMetalDrawable> drawable, MTLRenderPassDescriptor* renderPass );
        void DrawVertexBuffer( id<MTLBuffer> vertexBuffer, id<MTLBuffer> indexBuffer, int elementCount, int indexOffset );
        id <MTLDevice> GetMetalDevice();
        id <MTLLibrary> GetDefaultMetalShaderLibrary();
        id <MTLBuffer> GetNewUniformBuffer();
        id <MTLBuffer> GetCurrentUniformBuffer();
        void ResetUniformBuffers();
        void PresentDrawable();
        void BeginFrame();
#endif
#if RENDERER_VULKAN
        void CreateNewUniformBuffer();
        std::uint8_t* GetCurrentUbo();
        void BeginRenderPassAndCommandBuffer();
        void EndRenderPassAndCommandBuffer();
        void BeginFrame();
#endif
        void ClearScreen( unsigned clearFlags );
        void Draw( VertexBuffer& vertexBuffer, int startIndex, int endIndex, Shader& shader, BlendMode blendMode, DepthFunc depthFunc, CullMode cullMode );
        void ErrorCheck( const char* info );

        void SetClearColor( float red, float green, float blue );
        void SetRenderTarget( RenderTexture* target, unsigned cubeMapFace );
        void SetMultiSampling( bool enable );
        void Set_sRGB_Writes( bool enable );
        
        void PushGroupMarker( const char* name );
        void PopGroupMarker();

        void IncDrawCalls();
        int GetDrawCalls();
        void IncVertexBufferBinds();
        int GetVertexBufferBinds();
        void IncTextureBinds();
        int GetTextureBinds();
        void IncRenderTargetBinds();
        int GetRenderTargetBinds();
        void ResetFrameStatistics();
        void IncShaderBinds();
        int GetShaderBinds();
        int GetBarrierCalls();
        int GetFenceCalls();
        void GetGpuMemoryUsage( unsigned& outUsedMBytes, unsigned& outBudgetMBytes );

        void Present();
        void ReleaseGPUObjects();

#if RENDERER_OPENGL
        unsigned CreateBufferId();
        unsigned CreateTextureId();
        unsigned CreateVaoId();
        unsigned CreateShaderId( unsigned shaderType );
        unsigned CreateProgramId();
        unsigned CreateRboId();
        unsigned CreateFboId();

        unsigned GetSystemFBO();
        void SetSystemFBO( unsigned fbo );

        void SetBackBufferDimensionAndFBO( int width, int height );
        void ErrorCheckFBO();
        bool HasExtension( const char* glExtension );
        void DebugBlitFBO( unsigned handle, int width, int height );
#endif
    }
}
#endif
