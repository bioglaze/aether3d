#ifndef LIGHT_TILER_HPP
#define LIGHT_TILER_HPP

#include <vector>
#if RENDERER_METAL
#import <Metal/Metal.h>
#endif
#include "Vec3.hpp"

namespace ae3d
{
    /// Implements Forward+ light culler
    class LightTiler
    {
    public:
        void Init();
        int GetNextPointLightBufferIndex();
        void SetPointLightPositionAndRadius( int bufferIndex, Vec3& position, float radius );
        void CullLights( class ComputeShader& shader, const struct Matrix44& projection, const Matrix44& view,  class RenderTexture& depthNormalTarget );

#if RENDERER_METAL
        id< MTLBuffer > GetPerTileLightIndexBuffer() { return perTileLightIndexBuffer; }
        id< MTLBuffer > GetPointLightCenterAndRadiusBuffer() { return pointLightCenterAndRadiusBuffer; }
        id< MTLBuffer > GetCullerUniforms() { return uniformBuffer; }
#endif

    private:
        unsigned GetNumTilesX() const;
        unsigned GetNumTilesY() const;
        unsigned GetMaxNumLightsPerTile() const;

#if RENDERER_METAL
        id< MTLBuffer > uniformBuffer;
        id< MTLBuffer > pointLightCenterAndRadiusBuffer;
        id< MTLBuffer > perTileLightIndexBuffer;
#endif
        std::vector< Vec4 > pointLightCenterAndRadius;
        int activePointLights = 0;
        static const int TileRes = 16;
        static const int MaxLights = 2048;
        static const unsigned MaxLightsPerTile = 544;
    };
}

#endif
