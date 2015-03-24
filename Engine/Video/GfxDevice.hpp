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
        unsigned CreateIboId();
        unsigned CreateTextureId();
        unsigned CreateVaoId();
        unsigned CreateVboId();
        void ReleaseGPUObjects();
    }
}
#endif
