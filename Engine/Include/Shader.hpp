#ifndef SHADER_H
#define SHADER_H

#include <map>
#include <string>
#if AETHER3D_IOS
#import <Metal/Metal.h>
#endif

namespace ae3d
{
    class Texture2D;
    class TextureCube;

    /// Shader program containing a vertex and pixel shader.
    class Shader
    {
    public:
        /// \param vertexSource Vertex shader source.
        /// \param fragmentSource Fragment shader source.
        void Load( const char* vertexSource, const char* fragmentSource );
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

    private:
        struct IntDefaultedToMinusOne
        {
            int i = -1;
        };

        std::map<std::string, IntDefaultedToMinusOne > GetUniformLocations();
        
        unsigned id = 0;
        std::map<std::string, IntDefaultedToMinusOne > uniformLocations;
    };    
}
#endif
