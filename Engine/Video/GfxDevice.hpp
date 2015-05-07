#ifndef GFX_DEVICE_H
#define GFX_DEVICE_H

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
#if RENDERER_OPENGL
        void ErrorCheck( const char* info );
        bool HasExtension( const char* glExtension );
#endif
    }
}
#endif
