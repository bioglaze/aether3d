#ifndef GFX_DEVICE_H
#define GFX_DEVICE_H

#if AETHER3D_IOS
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#endif

namespace ae3d
{
    class RenderTexture2D;
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
            LessOrEqualWriteOn
        };
        
        void Init( int width, int height );
#if AETHER3D_IOS
        void Init( CAMetalLayer* metalLayer );
        void DrawVertexBuffer( id<MTLBuffer> vertexBuffer, id<MTLBuffer> indexBuffer, int elementCount, int indexOffset );
        id <MTLDevice> GetMetalDevice();
        id <MTLLibrary> GetDefaultMetalShaderLibrary();
        void PresentDrawable();
        void BeginFrame();
#endif
        void ClearScreen( unsigned clearFlags );
        void Draw( VertexBuffer& vertexBuffer, Shader& shader, BlendMode blendMode, DepthFunc depthFunc );
        void ErrorCheck( const char* info );

        void SetClearColor( float red, float green, float blue );
        void SetRenderTarget( RenderTexture2D* target );
        void SetBackFaceCulling( bool enable );
        void SetMultiSampling( bool enable );
        void SetBlendMode( BlendMode blendMode );
        void SetDepthFunc( DepthFunc depthFunc );

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

        void SetBackBufferDimensionAndFBO( int width, int height );
        void ErrorCheckFBO();
        bool HasExtension( const char* glExtension );
#endif
#if AETHER3D_D3D12
        void WaitForCommandQueueFence();
#endif
    }
}
#endif
