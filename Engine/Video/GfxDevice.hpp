#ifndef GFX_DEVICE_H
#define GFX_DEVICE_H

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
        
        void Init( int width, int height );
#if RENDERER_METAL
        void InitMetal( id <MTLDevice> metalDevice, MTKView* view );
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
        void BeginRenderPassAndCommandBuffer();
        void EndRenderPassAndCommandBuffer();
#endif
        void ClearScreen( unsigned clearFlags );
        void Draw( VertexBuffer& vertexBuffer, int startIndex, int endIndex, Shader& shader, BlendMode blendMode, DepthFunc depthFunc );
        void ErrorCheck( const char* info );

        void SetClearColor( float red, float green, float blue );
        void SetRenderTarget( RenderTexture* target, unsigned cubeMapFace );
        void SetMultiSampling( bool enable );
        void Set_sRGB_Writes( bool enable );
        
        // TODO: Remove this to prepare for modern API style draw submission (these are provided in Draw())
        void SetBackFaceCulling( bool enable );

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
