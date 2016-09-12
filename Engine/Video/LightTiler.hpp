#ifndef LIGHT_TILER_HPP
#define LIGHT_TILER_HPP

#include <vector>
#include "Vec3.hpp"
#include "RenderTexture.hpp"

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
        
    private:
        unsigned GetNumTilesX() const;
        unsigned GetNumTilesY() const;
        unsigned GetMaxNumLightsPerTile() const;

        RenderTexture pointLightCenterAndRadiusRT;
        std::vector< Vec4 > pointLightCenterAndRadius;
        int activePointLights = 0;
        static const int TileRes = 16;
        static const int MaxLights = 2048;
        static const unsigned MaxLightsPerTile = 544;
    };
}

#endif
