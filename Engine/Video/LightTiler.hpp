#ifndef LIGHT_TILER_HPP
#define LIGHT_TILER_HPP

#include <vector>
#if RENDERER_METAL
#import <Metal/Metal.h>
#endif
#include "Vec3.hpp"

struct ID3D12Resource;

namespace ae3d
{
    /// Implements Forward+ light culler
    class LightTiler
    {
    public:
        void Init();
        void SetPointLightPositionAndRadius( int bufferIndex, Vec3& position, float radius );
        void UpdateLightBuffers();
        void CullLights( class ComputeShader& shader, const struct Matrix44& projection, const Matrix44& view,  class RenderTexture& depthNormalTarget );
        bool CullerUniformsCreated() const { return cullerUniformsCreated; }
        
#if RENDERER_METAL
        id< MTLBuffer > GetPerTileLightIndexBuffer() { return perTileLightIndexBuffer; }
        id< MTLBuffer > GetPointLightCenterAndRadiusBuffer() { return pointLightCenterAndRadiusBuffer; }
        id< MTLBuffer > GetCullerUniforms() { return uniformBuffer; }
#endif
        /// Destroys graphics API objects.
        void DestroyBuffers();

#if RENDERER_D3D12
        ID3D12Resource* GetPointLightCenterAndRadiusBuffer() const { return pointLightCenterAndRadiusBuffer; }
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
#if RENDERER_D3D12
        ID3D12Resource* uniformBuffer = nullptr;
        ID3D12Resource* mappedUniformBuffer = nullptr;
        ID3D12Resource* perTileLightIndexBuffer = nullptr;
        ID3D12Resource* pointLightCenterAndRadiusBuffer = nullptr;
#endif
        std::vector< Vec4 > pointLightCenterAndRadius;
        int activePointLights = 0;
        bool cullerUniformsCreated = false;
        static const int TileRes = 16;
        static const int MaxLights = 2048;
        static const unsigned MaxLightsPerTile = 544;
    };
}

#endif
