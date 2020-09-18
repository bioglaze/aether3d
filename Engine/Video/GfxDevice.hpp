#pragma once

#include <cstdint>
#if RENDERER_METAL
#import <MetalKit/MetalKit.h>
#endif
#include "Matrix.hpp"
#include "Vec3.hpp"

struct PerObjectUboStruct
{
    enum LightType : int { Empty, Spot, Dir, Point };
    
    ae3d::Matrix44 localToClip;
    ae3d::Matrix44 localToView;
    ae3d::Matrix44 localToWorld;
    ae3d::Matrix44 localToShadowClip;
    ae3d::Matrix44 clipToView;
    ae3d::Matrix44 viewToClip;
    ae3d::Vec4 lightPosition;
    ae3d::Vec4 lightDirection;
    ae3d::Vec4 lightColor = ae3d::Vec4( 1, 1, 1, 1 );
    float lightConeAngleCos = 0;
    LightType lightType = LightType::Empty;
    float minAmbient = 0.2f;
    unsigned maxNumLightsPerTile = 0;
    unsigned windowWidth = 1;
    unsigned windowHeight = 1;
    unsigned numLights = 0; // 16 bits for point light count, 16 for spot light count
    float f0 = 0.8f;
    ae3d::Vec4 tex0scaleOffset = ae3d::Vec4( 1, 1, 0, 0 );
    ae3d::Vec4 tilesXY = ae3d::Vec4( 0, 0, 0, 0 );
    ae3d::Matrix44 boneMatrices[ 80 ];
    int isVR = 0;
    int kernelSize;
    ae3d::Vec3 kernelOffsets[ 128 ];
};

namespace ae3d
{
    class RenderTexture;
    class VertexBuffer;
    class Shader;
    class Texture2D;
    
    /// Low-level graphics state abstraction.
    namespace GfxDevice
    {
        enum ClearFlags : unsigned
        {
            Color = 1 << 0,
            Depth = 1 << 1,
            DontClear = 1 << 2
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
            LessOrEqualWriteOn,
            NoneWriteOff
        };
        
        enum class CullMode
        {
            Back,
            Front,
            Off
        };

        enum class FillMode
        {
            Solid,
            Wireframe
        };

        enum class PrimitiveTopology
        {
            Triangles,
            Lines
        };

        void DrawUI( int scX, int scY, int scWidth, int scHeight, int elemCount, int offset );
        void MapUIVertexBuffer( int vertexSize, int indexSize, void** outMappedVertices, void** outMappedIndices );
        void UnmapUIVertexBuffer();
        void Init( int width, int height );
        int CreateLineBuffer( const Vec3* lines, int lineCount, const Vec3& color );
        void UpdateLineBuffer( int lineHandle, const Vec3* lines, int lineCount, const Vec3& color );
        void GetNewUniformBuffer();
#if RENDERER_D3D12
        void ResetCommandList();
        void* GetCurrentMappedConstantBuffer();
        void* GetCurrentConstantBuffer();
#endif
#if RENDERER_METAL
        static const int UNIFORM_BUFFER_SIZE = sizeof( PerObjectUboStruct ) + 16 * 4;
        void InitMetal( id <MTLDevice> metalDevice, MTKView* view, int sampleCount, int uiVBSize, int uiIBSize );
        void SetCurrentDrawableMetal( MTKView* view );
        void DrawVertexBuffer( id<MTLBuffer> vertexBuffer, id<MTLBuffer> indexBuffer, int elementCount, int indexOffset );
        id <MTLDevice> GetMetalDevice();
        id <MTLLibrary> GetDefaultMetalShaderLibrary();
        id <MTLBuffer> GetCurrentUniformBuffer();
        void PresentDrawable();
        void BeginFrame();
        void BeginBackBufferEncoding();
        void EndBackBufferEncoding();
#endif
#if RENDERER_VULKAN
        void ResetPSOCache();
        void CreateUniformBuffers();
        std::uint8_t* GetCurrentUbo();
        void BeginRenderPassAndCommandBuffer();
        void BeginRenderPass();
        void EndRenderPassAndCommandBuffer();
        void EndRenderPass();
        void EndCommandBuffer();
        void BeginFrame();
#endif
        void ClearScreen( unsigned clearFlags );
        void Draw( VertexBuffer& vertexBuffer, int startIndex, int endIndex, Shader& shader, BlendMode blendMode, DepthFunc depthFunc, CullMode cullMode, FillMode fillMode, PrimitiveTopology topology );
        void DrawLines( int handle, Shader& shader );

        void BeginDepthNormalsGpuQuery();
        void EndDepthNormalsGpuQuery();
        void BeginShadowMapGpuQuery();
        void EndShadowMapGpuQuery();
        void BeginLightCullerGpuQuery();
        void EndLightCullerGpuQuery();

        void SetClearColor( float red, float green, float blue );
        void SetRenderTarget( RenderTexture* target, unsigned cubeMapFace );
        void SetViewport( int viewport[ 4 ] );
        void SetScissor( int scissor[ 4 ] );

        void PushGroupMarker( const char* name );
        void PopGroupMarker();

        void SetPolygonOffset( bool enable, float factor, float units );

        void GetGpuMemoryUsage( unsigned& outUsedMBytes, unsigned& outBudgetMBytes );

        void Present();
        void ReleaseGPUObjects();
    }
}
