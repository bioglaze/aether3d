#ifndef GFX_DEVICE_H
#define GFX_DEVICE_H

#include <cstdint>
#if RENDERER_METAL
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/CAMetalLayer.h>
#endif
#include "Matrix.hpp"
#include "Vec3.hpp"

struct PerObjectUboStruct
{
    ae3d::Matrix44 localToClip;
    ae3d::Matrix44 localToView;
    ae3d::Matrix44 localToWorld;
    ae3d::Matrix44 localToShadowClip;
    ae3d::Matrix44 clipToView;
    ae3d::Vec4 lightPosition;
    ae3d::Vec4 lightDirection;
    ae3d::Vec4 lightColor = ae3d::Vec4( 1, 1, 1, 1 );
    float lightConeAngleCos = 0;
    int lightType = 0;
    float minAmbient = 0.2f;
    unsigned maxNumLightsPerTile = 0;
    unsigned windowWidth = 1;
    unsigned windowHeight = 1;
    unsigned numLights = 0; // 16 bits for point light count, 16 for spot light count
    unsigned padding = 0;
    ae3d::Matrix44 boneMatrices[ 80 ];
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

        void DrawUI( int vpX, int vpY, int vpWidth, int vpHeight, int elemCount, void* offset );
        void MapUIVertexBuffer( int vertexSize, int indexSize, void** outMappedVertices, void** outMappedIndices );
        void UnmapUIVertexBuffer();
        void Init( int width, int height );
        int CreateLineBuffer( const Vec3* lines, int lineCount, const Vec3& color );
#if RENDERER_D3D12
        void ResetCommandList();
        void  CreateNewUniformBuffer();
        void* GetCurrentMappedConstantBuffer();
        void* GetCurrentConstantBuffer();
#endif
#if RENDERER_METAL
        static const int UNIFORM_BUFFER_SIZE = sizeof( PerObjectUboStruct ) + 64 * 81;
        void InitMetal( id <MTLDevice> metalDevice, MTKView* view, int sampleCount, int uiVBSize, int uiIBSize );
        void SetCurrentDrawableMetal( id <CAMetalDrawable> drawable, MTLRenderPassDescriptor* renderPass );
        void DrawVertexBuffer( id<MTLBuffer> vertexBuffer, id<MTLBuffer> indexBuffer, int elementCount, int indexOffset );
        id <MTLDevice> GetMetalDevice();
        id <MTLLibrary> GetDefaultMetalShaderLibrary();
        id <MTLBuffer> GetNewUniformBuffer();
        id <MTLBuffer> GetCurrentUniformBuffer();
        void PresentDrawable();
        void BeginFrame();
        void BeginBackBufferEncoding();
        void EndBackBufferEncoding();
#endif
#if RENDERER_VULKAN
        void GetNewUniformBuffer();
        void CreateUniformBuffers();
        std::uint8_t* GetCurrentUbo();
        void BeginRenderPassAndCommandBuffer();
        void EndRenderPassAndCommandBuffer();
        void BeginFrame();
#endif
        void ClearScreen( unsigned clearFlags );
        void Draw( VertexBuffer& vertexBuffer, int startIndex, int endIndex, Shader& shader, BlendMode blendMode, DepthFunc depthFunc, CullMode cullMode, FillMode fillMode, PrimitiveTopology topology );
        void DrawLines( int handle, Shader& shader );
        void ErrorCheck( const char* info );

        void BeginDepthNormalsGpuQuery();
        void EndDepthNormalsGpuQuery();
        void BeginShadowMapGpuQuery();
        void EndShadowMapGpuQuery();

        void SetClearColor( float red, float green, float blue );
        void SetRenderTarget( RenderTexture* target, unsigned cubeMapFace );
        void SetMultiSampling( bool enable );
        void Set_sRGB_Writes( bool enable );
        void SetViewport( int viewport[ 4 ] );
        void SetScissor( int scissor[ 4 ] );

        void PushGroupMarker( const char* name );
        void PopGroupMarker();

        void SetPolygonOffset( bool enable, float factor, float units );

        void GetGpuMemoryUsage( unsigned& outUsedMBytes, unsigned& outBudgetMBytes );

        void Present();
        void ReleaseGPUObjects();

#if RENDERER_OPENGL
        unsigned CreateBufferId();
        unsigned CreateTextureId();
        unsigned CreateVaoId();
        unsigned CreateShaderId( unsigned shaderType );
        unsigned CreateProgramId();
        unsigned CreateRboId();
        unsigned CreateFboId();

        void SetBackBufferDimensionAndFBO( int width, int height );
        void ErrorCheckFBO();
        bool HasExtension( const char* glExtension );
        void DebugBlitFBO( unsigned handle, int width, int height );
#endif
    }
}
#endif
