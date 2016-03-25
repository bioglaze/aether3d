#ifndef SHADER_H
#define SHADER_H

#include <map>
#include <string>
#if RENDERER_METAL
#import <Metal/Metal.h>
#endif
#if RENDERER_D3D12
#include <d3d12.h>
#include <d3dcompiler.h>
#endif
#if RENDERER_VULKAN
#include <vulkan/vulkan.h>
#include <cstdint>
#endif

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
        /// Loads a GLSL or HLSL shader from source code. For portability it's better to call the other
        /// load method that can take all shaders as input.
        /// \param vertexSource Vertex shader source. Language depends on the renderer.
        /// \param fragmentSource Fragment shader source. Language depends on the renderer.
        void Load( const char* vertexSource, const char* fragmentSource );
#if RENDERER_VULKAN
        bool IsValid() const { return true; }

        /// Loads SPIR-V shader.
        /// \param spirvData SPIR-V file contents.
        void LoadSPIRV( const FileSystem::FileContentsData& vertexData, const FileSystem::FileContentsData& fragmentData );
#endif
        /// \param vertexDataGLSL GLSL Vertex shader file contents.
        /// \param fragmentDataGLSL GLSL Fragment shader file contents.
        /// \param metalVertexShaderName Vertex shader name for Metal renderer. Must be referenced by the application's Xcode project.
        /// \param metalFragmentShaderName Fragment shader name for Metal renderer. Must be referenced by the application's Xcode project.
        /// \param vertexDataHLSL HLSL Vertex shader file contents.
        /// \param fragmentDataHLSL HLSL Fragment shader file contents.
        /// \param vertexDataSPIRV SPIR-V vertex shader file contents.
        /// \param fragmentDataSPIRV SPIR-V fragment shader file contents.
        void Load( const FileSystem::FileContentsData& vertexDataGLSL, const FileSystem::FileContentsData& fragmentDataGLSL,
                   const char* metalVertexShaderName, const char* metalFragmentShaderName,
                   const FileSystem::FileContentsData& vertexDataHLSL, const FileSystem::FileContentsData& fragmentDataHLSL,
                   const FileSystem::FileContentsData& vertexDataSPIRV, const FileSystem::FileContentsData& fragmentDataSPIRV );
        
#if RENDERER_METAL
        void LoadFromLibrary( const char* vertexShaderName, const char* fragmentShaderName );
#endif
#if RENDERER_OPENGL
        /// \return True if the shader has been succesfully compiled and linked.
        bool IsValid() const { return handle != 0; }
        unsigned GetHandle() const { return handle; }
#endif
        
        /// Activates the shader to be used in a draw call.
        void Use();

        /// \param name Matrix uniform name.
        /// \param matrix4x4 Contents of Matrix44.
        void SetMatrix( const char* name, const float* matrix4x4 );

        /// \param name Texture uniform name.
        /// \param texture Texture.
        /// \param textureUnit Texture unit.
        void SetTexture( const char* name, const class Texture2D* texture, int textureUnit );

        /// \param name Texture uniform name.
        /// \param texture Texture.
        /// \param textureUnit Texture unit.
        void SetTexture( const char* name, const class TextureCube* texture, int textureUnit );

        /// \param name Texture uniform name.
        /// \param renderTexture RenderTexture.
        /// \param textureUnit Texture unit.
        void SetRenderTexture( const char* name, const class RenderTexture* renderTexture, int textureUnit );

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

#if RENDERER_D3D12
        bool IsValid() const { return blobShaderVertex != nullptr; }
        ID3DBlob* blobShaderVertex = nullptr;
        ID3DBlob* blobShaderPixel = nullptr;
#endif

#if RENDERER_METAL
        bool IsValid() const { return vertexProgram != nullptr; }

        enum class UniformType { Float, Float2, Float3, Float4, Matrix4x4 };
        
        struct Uniform
        {
            UniformType type = UniformType::Float;
            unsigned long offsetFromBufferStart = 0;
            float floatValue[ 4 ];
            float matrix4x4[ 16 ];
        };
        
        std::map< std::string, Uniform > uniforms;
        
        void LoadUniforms( MTLRenderPipelineReflection* reflection );

        id <MTLFunction> vertexProgram;
        id <MTLFunction> fragmentProgram;
#endif
#if RENDERER_VULKAN
        VkPipelineShaderStageCreateInfo& GetVertexInfo() { return vertexInfo; }
        VkPipelineShaderStageCreateInfo& GetFragmentInfo() { return fragmentInfo; }
#endif
        /// Wraps an int that is defaulted to -1. Needed for uniform handling.
        struct IntDefaultedToMinusOne
        {
            /// -1 means unused/missing uniform.
            int i = -1;
        };

    private:
        std::string fragmentPath;

#if RENDERER_D3D12
        void CreateConstantBuffer();
        void ReflectVariables();

        ID3D12ShaderReflection* reflector = nullptr;
        std::map<std::string, IntDefaultedToMinusOne > uniformLocations;
#endif
#if RENDERER_VULKAN
        VkPipelineShaderStageCreateInfo vertexInfo;
        VkPipelineShaderStageCreateInfo fragmentInfo;        
#endif
#if RENDERER_OPENGL
        unsigned handle = 0;
        std::map<std::string, IntDefaultedToMinusOne > uniformLocations;
#endif
    };
}
#endif
