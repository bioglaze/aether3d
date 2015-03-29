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
        void ErrorCheck( const char* info );
        void SetClearColor(float red, float green, float blue);
        unsigned CreateBufferId();
        unsigned CreateTextureId();
        unsigned CreateVaoId();
        void ReleaseGPUObjects();
    }
}
#endif
