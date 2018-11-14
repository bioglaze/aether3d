#pragma once

#if RENDERER_METAL
#import <Metal/Metal.h>
#endif
#if RENDERER_VULKAN
#include <vulkan/vulkan.h>
#endif

#if RENDERER_D3D12
struct ID3D12Resource;
typedef struct ID3D10Blob* LPD3D10BLOB;
typedef ID3D10Blob ID3DBlob;
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
        /// Call before Dispatch.
        void Begin();

        /// Call after Dispatch.
        void End();
        
        /// Dispatches the shader.
        /// \param groupCountX X count
        /// \param groupCountY Y count
        /// \param groupCountZ Z count
        void Dispatch( unsigned groupCountX, unsigned groupCountY, unsigned groupCountZ );

        /// \param x 1 if should blur in x direction.
        /// \param y 1 if should blur in x direction.
        void SetBlurDirection( float x, float y );
        
        /// Internal loading method. End-users should use the method that takes all languages as parameters.
        /// \param source Source string.
        void Load( const char* source );

#if RENDERER_VULKAN
        /// Destroys shaders.
        static void DestroyShaders();

        /// Loads SPIR-V shader.
        /// \param contents SPIR-V file contents.
        void LoadSPIRV( const FileSystem::FileContentsData& contents );
        VkPipelineShaderStageCreateInfo& GetInfo() { return info; }
        /// \return PSO
        VkPipeline GetPSO() const { return pso; }
        /// Sets a texture.
        /// \param slot slot index
        /// \param texture texture.
        void SetTexture2D( unsigned slot, Texture2D* texture );
#endif
        /// \param metalShaderName Vertex shader name for Metal renderer. Must be referenced by the application's Xcode project.
        /// \param dataHLSL HLSL shader file contents.
        /// \param dataSPIRV SPIR-V vertex file contents.
        void Load( const char* metalShaderName,
                   const FileSystem::FileContentsData& dataHLSL,
                   const FileSystem::FileContentsData& dataSPIRV );
        
#if RENDERER_METAL
        void LoadFromLibrary( const char* vertexShaderName, const char* fragmentShaderName );

        /// Sets a uniform buffer.
        /// \param slotIndex slot index
        /// \param buffer Uniform buffer
        void SetUniformBuffer( unsigned slot, id< MTLBuffer > buffer );

        /// Sets a texture.
        /// \param slot slot index
        /// \param texture texture.
        void SetTexture2D( unsigned slot, Texture2D* texture );
#endif
        /// Sets a render texture into a slot.
        /// \param slot slot index.
        /// \param renderTexture render texture.
        void SetRenderTexture( unsigned slot, class RenderTexture* renderTexture );
        
#if RENDERER_D3D12
        void SetUniformBuffer( unsigned slot, ID3D12Resource* buffer );
        void SetTextureBuffer( unsigned slot, ID3D12Resource* buffer );
        void SetUAVBuffer( unsigned slot, ID3D12Resource* buffer );
        void SetTexture2D( unsigned slot, Texture2D* texture );
        ID3DBlob* blobShader = nullptr;
#endif

    private:
        static const int SLOT_COUNT = 12;
        
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
#endif
    };
}
