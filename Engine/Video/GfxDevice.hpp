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

        void ClearScreen( unsigned clearFlags );
        void SetClearColor(float red, float green, float blue);

        unsigned CreateBufferId();
        unsigned CreateTextureId();
        unsigned CreateVaoId();
        unsigned CreateShaderId( unsigned shaderType );
        unsigned CreateProgramId();
        
        // TODO: Disable statistic calls in Release builds.
        void IncDrawCalls();
        int GetDrawCalls();
        void ResetFrameStatistics();

        void ReleaseGPUObjects();
#if RENDERER_OPENGL
        void ErrorCheck( const char* info );
        bool HasExtension( const char* glExtension );
#endif
    }
}
#endif
