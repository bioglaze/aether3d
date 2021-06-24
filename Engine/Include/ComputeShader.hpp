#pragma once

#if RENDERER_METAL
#import <Metal/Metal.h>
#endif
#if RENDERER_VULKAN
#include <vulkan/vulkan.h>
#endif
#if RENDERER_D3D12
#include <d3d12.h>
#endif

namespace ae3d
{
    class Texture2D;
    
    namespace FileSystem
    {
        struct FileContentsData;
    }
    
    /// Shader program containing a compute shader.
    class ComputeShader
    {
    public:
        /// Uniform. These correspond to variables in shader include file ubo.h and MetalCommon.h.
		enum class UniformName { TilesZW, BloomThreshold, BloomIntensity };

        /// Call before Dispatch.
        void Begin();

        /// Call after Dispatch.
        void End();
        
        /// Dispatches the shader.
        /// \param groupCountX X count
        /// \param groupCountY Y count
        /// \param groupCountZ Z count
        /// \param debugName Debug group name that is shown in some profilers.
        void Dispatch( unsigned groupCountX, unsigned groupCountY, unsigned groupCountZ, const char* debugName );

        /// Sets projection matrix and its inverse into the uniform structure.
        /// \param projection Projection matrix.
        void SetProjectionMatrix( const struct Matrix44& projection );
        
        /// Sets a uniform.
        /// \param uniform Uniform.
        /// \param x X
        /// \param y Y
        void SetUniform( UniformName uniform, float x, float y );
        
        /// Internal loading method. End-users should use the method that takes all languages as parameters.
        /// \param source Source string.
        void Load( const char* source );

#if RENDERER_VULKAN
        /// Destroys shaders.
        static void DestroyShaders();

        /// Loads SPIR-V shader.
        /// \param contents SPIR-V file contents.
        void LoadSPIRV( const FileSystem::FileContentsData& contents );

        /// \return Shader info.
        VkPipelineShaderStageCreateInfo& GetInfo() { return info; }

        /// \return PSO
        VkPipeline GetPSO() const { return pso; }

#endif
        /// \param metalShaderName Vertex shader name for Metal renderer. Must be referenced by the application's Xcode project.
        /// \param dataHLSL HLSL shader file contents.
        /// \param dataSPIRV SPIR-V vertex file contents.
        void Load( const char* metalShaderName,
                   const FileSystem::FileContentsData& dataHLSL,
                   const FileSystem::FileContentsData& dataSPIRV );

        /// Sets a texture.
        /// \param texture texture.
        /// \param slot slot index. Range is 0-14.
        void SetTexture2D( Texture2D* texture, unsigned slot );
        
#if RENDERER_METAL
        void LoadFromLibrary( const char* vertexShaderName, const char* fragmentShaderName );

        /// Sets a uniform buffer.
        /// \param slot slot index. Range is 0-14.
        /// \param buffer Uniform buffer
        void SetUniformBuffer( unsigned slot, id< MTLBuffer > buffer );
#endif
        /// Sets a render texture into a slot.
        /// \param renderTexture render texture.
        /// \param slot slot index. Range is 0-14.
        void SetRenderTexture( class RenderTexture* renderTexture, unsigned slot );
        
#if RENDERER_D3D12
        void SetCBV( unsigned slot, ID3D12Resource* buffer );
        void SetSRV( unsigned slot, ID3D12Resource* buffer, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc );
        void SetUAV( unsigned slot, ID3D12Resource* buffer, const D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc );
        ID3DBlob* blobShader = nullptr;
        ID3D12PipelineState* pso = nullptr;
#endif

    private:
        static const int SLOT_COUNT = 15;
        
#if RENDERER_VULKAN
        VkPipelineShaderStageCreateInfo info;
        VkPipeline pso = VK_NULL_HANDLE;
        Texture2D* textures[ SLOT_COUNT ];
#endif
#if RENDERER_METAL
        id <MTLFunction> function;
        id<MTLComputePipelineState> pipeline;
        id<MTLBuffer> uniforms[ SLOT_COUNT ];
        Texture2D* textures[ SLOT_COUNT ];
#endif
        RenderTexture* renderTextures[ SLOT_COUNT ];
#if RENDERER_D3D12
        ID3D12Resource* uniformBuffers[ SLOT_COUNT ];
        ID3D12Resource* textureBuffers[ SLOT_COUNT ];
        ID3D12Resource* uavBuffers[ SLOT_COUNT ];
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDescs[ SLOT_COUNT ];
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDescs[ SLOT_COUNT ];
#endif
    };
}
