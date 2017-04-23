#ifndef GFX_DEVICE_H
#define GFX_DEVICE_H

#include <cstdint>
#include <vector>
#if RENDERER_METAL
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/CAMetalLayer.h>
#endif
#include "Matrix.hpp"
#include "Vec3.hpp"

struct PerObjectUboStruct
{
    ae3d::Matrix44 projectionModelMatrix;
};

namespace ae3d
{
    class RenderTexture;
    class VertexBuffer;
    class Shader;

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

        void Init( int width, int height );
        int CreateLineBuffer( const std::vector< Vec3 >& lines, const Vec3& color );
#if RENDERER_D3D12
        void ResetCommandList();
        void  CreateNewUniformBuffer();
        void* GetCurrentUniformBuffer();
#endif
#if RENDERER_METAL
        static const int UNIFORM_BUFFER_SIZE = 512;
        void InitMetal( id <MTLDevice> metalDevice, MTKView* view, int sampleCount );
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

        void SetClearColor( float red, float green, float blue );
        void SetRenderTarget( RenderTexture* target, unsigned cubeMapFace );
        void UnsetRenderTarget();
        void SetMultiSampling( bool enable );
        void Set_sRGB_Writes( bool enable );
        void SetViewport( int viewport[ 4 ] );
        
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

        unsigned GetSystemFBO();
        void SetSystemFBO( unsigned fbo );

        void UploadPerObjectUbo();
        void SetBackBufferDimensionAndFBO( int width, int height );
        void ErrorCheckFBO();
        bool HasExtension( const char* glExtension );
        void DebugBlitFBO( unsigned handle, int width, int height );
#endif
    }
}
#endif
