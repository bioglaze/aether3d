#ifndef GFX_DEVICE_H
#define GFX_DEVICE_H

#if defined(AETHER3D_IOS)
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#endif

namespace ae3d
{
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
        
#if AETHER3D_IOS
        void Init( CAMetalLayer* metalLayer );
        void DrawVertexBuffer( id<MTLBuffer> vertexBuffer, id<MTLBuffer> indexBuffer, int elementCount, int indexOffset );
        id <MTLDevice> GetMetalDevice();
        id <MTLLibrary> GetDefaultMetalShaderLibrary();
        void PresentDrawable();
        void BeginFrame();
#endif

        void ClearScreen( unsigned clearFlags );
        void SetClearColor(float red, float green, float blue);

        unsigned CreateBufferId();
        unsigned CreateTextureId();
        unsigned CreateVaoId();
        unsigned CreateShaderId( unsigned shaderType );
        unsigned CreateProgramId();
        
        void IncDrawCalls();
        int GetDrawCalls();
        void ResetFrameStatistics();

        void ReleaseGPUObjects();
        void SetBlendMode( BlendMode blendMode );
        void ErrorCheck( const char* info );
#if RENDERER_OPENGL
        bool HasExtension( const char* glExtension );
#endif
    }
}
#endif
