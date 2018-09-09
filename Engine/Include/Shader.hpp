#pragma once

#include <string>
#if RENDERER_METAL
#import <Metal/Metal.h>
#endif
#if RENDERER_D3D12
#include <d3d12.h>
#endif
#if RENDERER_VULKAN
#include <vulkan/vulkan.h>
#include <cstdint>
#endif
#include "Array.hpp"

namespace ae3d
{
    namespace FileSystem
    {
        struct FileContentsData;
    }
    
    /// Shader program containing a vertex and pixel shader.
    class Shader
    {
    public:
        /// Loads a HLSL shader from source code. For portability it's better to call the other
        /// load method that can take all shaders as input.
        /// \param vertexSource Vertex shader source.
        /// \param fragmentSource Fragment shader source.
        void Load( const char* vertexSource, const char* fragmentSource );
#if RENDERER_VULKAN
        bool IsValid() const { return true; }

        /// Loads SPIR-V shader.
        /// \param spirvData SPIR-V file contents.
        void LoadSPIRV( const FileSystem::FileContentsData& vertexData, const FileSystem::FileContentsData& fragmentData );
        VkPipelineShaderStageCreateInfo& GetVertexInfo() { return vertexInfo; }
        VkPipelineShaderStageCreateInfo& GetFragmentInfo() { return fragmentInfo; }
#endif
        /// \param metalVertexShaderName Vertex shader name for Metal renderer. Must be referenced by the application's Xcode project.
        /// \param metalFragmentShaderName Fragment shader name for Metal renderer. Must be referenced by the application's Xcode project.
        /// \param vertexDataHLSL HLSL Vertex shader file contents.
        /// \param fragmentDataHLSL HLSL Fragment shader file contents.
        /// \param vertexDataSPIRV SPIR-V vertex shader file contents.
        /// \param fragmentDataSPIRV SPIR-V fragment shader file contents.
        void Load( const char* metalVertexShaderName, const char* metalFragmentShaderName,
                   const FileSystem::FileContentsData& vertexDataHLSL, const FileSystem::FileContentsData& fragmentDataHLSL,
                   const FileSystem::FileContentsData& vertexDataSPIRV, const FileSystem::FileContentsData& fragmentDataSPIRV );
        
        /// Activates the shader to be used in a draw call.
        void Use();

        /// \param name Matrix uniform name.
        /// \param matrix4x4 Contents of Matrix44.
        void SetMatrix( const char* name, const float* matrix4x4 );

        /// \param name Matrix array uniform name.
        /// \param matrix4x4s Contents of Matrix44 array.
        /// \param count Matrix count in the array.
        void SetMatrixArray( const char* name, const float* matrix4x4s, int count );

        /// \param name Texture uniform name.
        /// \param texture Texture.
        /// \param textureUnit Texture unit.
        void SetTexture( const char* name, class Texture2D* texture, int textureUnit );

        /// \param name Texture uniform name.
        /// \param texture Texture.
        /// \param textureUnit Texture unit.
        void SetTexture( const char* name, class TextureCube* texture, int textureUnit );

        /// \param name Texture uniform name.
        /// \param renderTexture RenderTexture.
        /// \param textureUnit Texture unit.
        void SetRenderTexture( const char* name, class RenderTexture* renderTexture, int textureUnit );

        /// \param name Integer uniform name.
        /// \param value Value.
        void SetInt( const char* name, int value );

        /// \param name Float uniform name.
        /// \param value Value.
        void SetFloat( const char* name, float value );

        /// \param name Vector uniform name.
        /// \param vec3 Vec3 contents.
        void SetVector3( const char* name, const float* vec3 );

        /// \param name Vector uniform name.
        /// \param vec4 Vec4 contents.
        void SetVector4( const char* name, const float* vec4 );

        /// \param offset Offset into the currently bound uniform buffer.
        /// \param data Data to be copied into the uniform buffer.
        /// \param dataBytes Copied data's size in bytes.
        void SetUniform( int offset, void* data, int dataBytes );

        /// \return Vertex shader path.
        const std::string& GetVertexShaderPath() const { return vertexPath; }

#if RENDERER_D3D12
        bool IsValid() const { return blobShaderVertex != nullptr; }
        ID3DBlob* blobShaderVertex = nullptr;
        ID3DBlob* blobShaderPixel = nullptr;
#endif

#if RENDERER_METAL
        void LoadFromLibrary( const char* vertexShaderName, const char* fragmentShaderName );
        bool IsValid() const { return vertexProgram != nullptr; }
        const std::string& GetMetalVertexShaderName() const { return metalVertexShaderName; }
        void LoadUniforms( MTLRenderPipelineReflection* reflection );

        id <MTLFunction> vertexProgram;
        id <MTLFunction> fragmentProgram;
#endif
        /// Destroys all shaders. Called internally at exit.
        static void DestroyShaders();

    private:
        struct UniformLocation
        {
            char uniformName[ 128 ];
            /// -1 means unused/missing uniform.
            int offset = -1;
        };

        int GetUniformLocation( const char* name );
        
        Array< UniformLocation > uniformLocations;
        std::string vertexPath;
        std::string fragmentPath;

#if RENDERER_D3D12
        void ReflectVariables();
#endif
#if RENDERER_VULKAN
        VkPipelineShaderStageCreateInfo vertexInfo;
        VkPipelineShaderStageCreateInfo fragmentInfo;        
#endif
#if RENDERER_METAL
        std::string metalVertexShaderName;
#endif
    };
}
