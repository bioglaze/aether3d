#ifndef MATERIAL_H
#define MATERIAL_H

#include <string>
#include <unordered_map>
#include "Vec3.hpp"
#include "Matrix.hpp"

namespace ae3d
{
    class Shader;
    class Texture2D;
    class TextureCube;
    class RenderTexture;
    struct Matrix44;
    
    /// Material is used to render a mesh.
    class Material
    {
  public:
        /// Sets a texture into every material, overriding textures set by SetTexture.
        /// \param name Texture uniform name.
        /// \param renderTexture Render texture.
        static void SetGlobalRenderTexture( const char* name, RenderTexture* renderTexture );

        /// \return shader.
        Shader* GetShader() { return shader; }

        /// \return True if the shader exists and has been compiled and linked successfully.
        bool IsValidShader() const;
        
        /// Applies the uniforms into the shader. Called internally.
        void Apply();

        /// \param enable Enable backface culling. Defaults to true.
        void SetBackFaceCulling( bool enable ) { cullBackFaces = enable; }
        
        /// \param name Name. This is a uniform in the shader.
        /// \param matrix 4x4 matrix.
        void SetMatrix( const char* name, const Matrix44& matrix );
        
        /// \param shader Shader.
        void SetShader( Shader* shader );

        /// \param name Name. This is a uniform in the shader.
        /// \param texture Texture.
        void SetTexture( const char* name, Texture2D* texture );

        /// \param name Texture uniform name.
        /// \param texture Texture.
        void SetTexture( const char* name, TextureCube* texture );

        /// \param name Texture uniform name.
        /// \param renderTexture Render texture.
        void SetRenderTexture( const char* name, RenderTexture* renderTexture );
        
        /// \param name Integer uniform name.
        /// \param value Value.
        void SetInt( const char* name, int value );
        
        /// \param name Float uniform name.
        /// \param value Value.
        void SetFloat( const char* name, float value );
        
        /// \param name Vector uniform name.
        /// \param vec3 Vector.
        void SetVector( const char* name, const Vec3& vec3 );

        /// \param name Name. This is a uniform in the shader.
        /// \param vec Vector.
        void SetVector( const char* name, const Vec4& vec );

  private:
        // TODO: String hash instead of string and get rid of STL *map.
        static std::unordered_map< std::string, RenderTexture* > sTexRTs;

        std::unordered_map< std::string, float > floats;
        std::unordered_map< std::string, int > ints;
        std::unordered_map< std::string, Vec3 > vec3s;
        std::unordered_map< std::string, Vec4 > vec4s;
        std::unordered_map< std::string, Texture2D* > tex2ds;
        std::unordered_map< std::string, TextureCube* > texCubes;
        std::unordered_map< std::string, RenderTexture* > texRTs;
        std::unordered_map< std::string, Matrix44 > mat4s;
        Shader* shader = nullptr;
        bool cullBackFaces = true;
    };
}

#endif
