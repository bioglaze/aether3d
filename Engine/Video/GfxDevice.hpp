#ifndef GFX_DEVICE_H
#define GFX_DEVICE_H

#if AETHER3D_METAL
#import <Metal/Metal.h>
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
            Depth = 1 << 1
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
#if AETHER3D_METAL
        void Init( CAMetalLayer* metalLayer );
        void DrawVertexBuffer( id<MTLBuffer> vertexBuffer, id<MTLBuffer> indexBuffer, int elementCount, int indexOffset );
        id <MTLDevice> GetMetalDevice();
        id <MTLLibrary> GetDefaultMetalShaderLibrary();
        id <MTLBuffer> GetNewUniformBuffer();
        id <MTLBuffer> GetCurrentUniformBuffer();
        void ResetUniformBuffers();
        void PresentDrawable();
        void BeginFrame();
#endif
        void ClearScreen( unsigned clearFlags );
        void Draw( VertexBuffer& vertexBuffer, int startIndex, int endIndex, Shader& shader, BlendMode blendMode, DepthFunc depthFunc );
        void ErrorCheck( const char* info );

        void SetClearColor( float red, float green, float blue );
        void SetRenderTarget( RenderTexture* target, unsigned cubeMapFace );
        void SetMultiSampling( bool enable );
        
        // TODO: Remove this to prepare for modern API style draw submission (these are provided in Draw())
        void SetBackFaceCulling( bool enable );

        void IncDrawCalls();
        int GetDrawCalls();
        void IncVertexBufferBinds();
        int GetVertexBufferBinds();
        void IncTextureBinds();
        int GetTextureBinds();
        void ResetFrameStatistics();

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
