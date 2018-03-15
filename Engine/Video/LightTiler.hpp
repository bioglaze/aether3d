#ifndef LIGHT_TILER_HPP
#define LIGHT_TILER_HPP

#if RENDERER_METAL
#import <Metal/Metal.h>
#endif
#if RENDERER_VULKAN
#include <vulkan/vulkan.h>
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
        void SetPointLightParameters( int bufferIndex, const Vec3& position, float radius, const Vec4& color );
        void SetSpotLightParameters( int bufferIndex, Vec3& position, float radius, const Vec4& color, const Vec3& direction, float coneAngle, float falloffRadius );
        void UpdateLightBuffers();
        void CullLights( class ComputeShader& shader, const struct Matrix44& projection, const Matrix44& view,  class RenderTexture& depthNormalTarget );
        
#if RENDERER_METAL
        id< MTLBuffer > GetPerTileLightIndexBuffer() const { return perTileLightIndexBuffer; }
        id< MTLBuffer > GetPointLightCenterAndRadiusBuffer() const { return pointLightCenterAndRadiusBuffer; }
        id< MTLBuffer > GetPointLightColorBuffer() const { return pointLightColorBuffer; }
        id< MTLBuffer > GetSpotLightCenterAndRadiusBuffer() const { return spotLightCenterAndRadiusBuffer; }
        id< MTLBuffer > GetSpotLightParamsBuffer() const { return spotLightParamsBuffer; }
        id< MTLBuffer > GetSpotLightColorBuffer() const { return spotLightColorBuffer; }
#endif
        /// Destroys graphics API objects.
        void DestroyBuffers();
        
#if RENDERER_D3D12
        ID3D12Resource* GetSpotLightCenterAndRadiusBuffer() const { return spotLightCenterAndRadiusBuffer; }
        ID3D12Resource* GetSpotLightColorBuffer() const { return spotLightColorBuffer; }
        ID3D12Resource* GetSpotLightParamsBuffer() const { return spotLightParamsBuffer; }
        ID3D12Resource* GetPointLightCenterAndRadiusBuffer() const { return pointLightCenterAndRadiusBuffer; }
        ID3D12Resource* GetPointLightColorBuffer() const { return pointLightColorBuffer; }
#endif
        int GetPointLightCount() const { return activePointLights; }
        int GetSpotLightCount() const { return activeSpotLights; }
        unsigned GetMaxNumLightsPerTile() const;
        
#if RENDERER_VULKAN
        VkBuffer GetPointLightBuffer() const { return pointLightCenterAndRadiusBuffer; }
        VkBufferView* GetPointLightBufferView() { return &pointLightBufferView; }
        VkBuffer GetPointLightColorBuffer() const { return pointLightColorBuffer; }
        VkBufferView* GetPointLightColorBufferView() { return &pointLightColorView; }
        VkBuffer GetSpotLightBuffer() const { return spotLightCenterAndRadiusBuffer; }
        VkBuffer GetSpotLightColorBuffer() const { return spotLightColorBuffer; }
        VkBufferView* GetSpotLightColorBufferView() { return &spotLightColorView; }
        VkBufferView* GetSpotLightBufferView() { return &spotLightBufferView; }
        VkBufferView* GetSpotLightParamsView() { return &spotLightParamsView; }
        VkBufferView* GetLightIndexBufferView() { return &perTileLightIndexBufferView; }
#endif
    private:
        unsigned GetNumTilesX() const;
        unsigned GetNumTilesY() const;
        
#if RENDERER_METAL
        id< MTLBuffer > pointLightCenterAndRadiusBuffer;
        id< MTLBuffer > pointLightColorBuffer;
        id< MTLBuffer > spotLightCenterAndRadiusBuffer;
        id< MTLBuffer > spotLightParamsBuffer;
        id< MTLBuffer > spotLightColorBuffer;
        id< MTLBuffer > perTileLightIndexBuffer;
#endif
#if RENDERER_D3D12
        ID3D12Resource* perTileLightIndexBuffer = nullptr;
        ID3D12Resource* pointLightCenterAndRadiusBuffer = nullptr;
        ID3D12Resource* pointLightColorBuffer = nullptr;
        ID3D12Resource* spotLightCenterAndRadiusBuffer = nullptr;
        ID3D12Resource* spotLightColorBuffer = nullptr;
        ID3D12Resource* spotLightParamsBuffer = nullptr;
#endif
#if RENDERER_VULKAN
        VkBuffer pointLightCenterAndRadiusBuffer = VK_NULL_HANDLE;
        VkDeviceMemory pointLightCenterAndRadiusMemory = VK_NULL_HANDLE;
        void* mappedPointLightCenterAndRadiusMemory = nullptr;
        VkBufferView pointLightBufferView;
        
        VkBuffer pointLightColorBuffer = VK_NULL_HANDLE;
        VkDeviceMemory pointLightColorMemory = VK_NULL_HANDLE;
        void* mappedPointLightColorMemory = nullptr;
        VkBufferView pointLightColorView;

        VkBuffer spotLightColorBuffer = VK_NULL_HANDLE;
        VkDeviceMemory spotLightColorMemory = VK_NULL_HANDLE;
        void* mappedSpotLightColorMemory = nullptr;
        VkBufferView spotLightColorView;

        VkBuffer spotLightCenterAndRadiusBuffer = VK_NULL_HANDLE;
        VkDeviceMemory spotLightCenterAndRadiusMemory = VK_NULL_HANDLE;
        void* mappedSpotLightCenterAndRadiusMemory = nullptr;
        VkBufferView spotLightBufferView;

        VkBuffer spotLightParamsBuffer = VK_NULL_HANDLE;
        VkDeviceMemory spotLightParamsMemory = VK_NULL_HANDLE;
        void* mappedSpotLightParamsMemory = nullptr;
        VkBufferView spotLightParamsView;
        
        VkBuffer perTileLightIndexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory perTileLightIndexBufferMemory = VK_NULL_HANDLE;
        VkBufferView perTileLightIndexBufferView;
        VkPipeline pso;
        VkPipelineLayout psoLayout;
#endif
        static const int TileRes = 16;
        static const int MaxLights = 2048;
        static const unsigned MaxLightsPerTile = 544;
        Vec4 pointLightCenterAndRadius[ MaxLights ];
        Vec4 pointLightColors[ MaxLights ];
        Vec4 spotLightColors[ MaxLights ];
        Vec4 spotLightCenterAndRadius[ MaxLights ];
        Vec4 spotLightParams[ MaxLights ];
        int activePointLights = 0;
        int activeSpotLights = 0;
    };
}

#endif

