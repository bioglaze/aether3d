#ifndef SHADER_H
#define SHADER_H

#include <map>
#include <string>
#if AETHER3D_IOS
#import <Metal/Metal.h>
#endif

namespace ae3d
{
    namespace FileSystem
    {
        struct FileContentsData;
    }
    
    class Texture2D;
    class TextureCube;

    /// Shader program containing a vertex and pixel shader.
    class Shader
    {
    public:
        /// \return True if the shader has been succesfully compiled and linked.
        bool IsValid() const { return id != 0; }
        
        /// \param vertexSource Vertex shader source.
        /// \param fragmentSource Fragment shader source.
        void Load( const char* vertexSource, const char* fragmentSource );

        /// \param vertexData GLSL Vertex shader file contents.
        /// \param fragmentData GLSL Fragment shader file contents.
        /// \param metalVertexShaderName Vertex shader name for Metal renderer. Must be referenced by the application's Xcode project.
        /// \param metalFragmentShaderName Fragment shader name for Metal renderer. Must be referenced by the application's Xcode project.
        void Load( const FileSystem::FileContentsData& vertexData, const FileSystem::FileContentsData& fragmentData, const char* metalVertexShaderName, const char* metalFragmentShaderName );
        
#if AETHER3D_IOS
        void LoadFromLibrary( const char* vertexShaderName, const char* fragmentShaderName );
#endif

        /// Activates the shader to be used in a draw call.
        void Use();

        /// \param name Matrix uniform name.
        /// \param matrix4x4 Contents of Matrix44.
        void SetMatrix( const char* name, const float* matrix4x4 );

        /// \param name Texture uniform name.
        /// \param texture Texture.
        /// \param textureUnit Texture unit.
        void SetTexture( const char* name, const Texture2D* texture, int textureUnit );

        /// \param name Texture uniform name.
        /// \param texture Texture.
        /// \param textureUnit Texture unit.
        void SetTexture( const char* name, const TextureCube* texture, int textureUnit );

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

#if AETHER3D_IOS
        id <MTLFunction> vertexProgram;
        id <MTLFunction> fragmentProgram;
#endif
        struct IntDefaultedToMinusOne
        {
            int i = -1;
        };

    private:
        unsigned id = 0;
        std::map<std::string, IntDefaultedToMinusOne > uniformLocations;
    };    
}
#endif
