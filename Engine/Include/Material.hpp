#pragma once

#include <string>
#include <unordered_map>
#include "Vec3.hpp"

namespace ae3d
{
    /// Material is used to render a mesh.
    class Material
    {
  public:
        /// Blending mode
        enum class BlendingMode { Alpha, Off };
        
        /// Depth function used when rendering a mesh using material.
        enum class DepthFunction { NoneWriteOff, LessOrEqualWriteOn };

        /// Sets a texture into every material, overriding textures set by SetTexture.
        /// \param name Texture uniform name.
        /// \param renderTexture Render texture.
        static void SetGlobalRenderTexture( const char* name, class RenderTexture* renderTexture );
        
        /// Sets a texture into every material, overriding textures set by SetTexture.
        /// \param name Texture uniform name.
        /// \param texture2d Texture.
        static void SetGlobalTexture2D( const char* name, class Texture2D* texture2d );

        /// Sets a float into every material, overriding floats set by SetFloat.
        /// \param name Float uniform name.
        /// \param value Float value.
        static void SetGlobalFloat( const char* name, float value );

        /// Sets an int into every material, overriding ints set by SetInt.
        /// \param name Int uniform name.
        /// \param value Int value.
        static void SetGlobalInt( const char* name, int value );

        /// Sets a Vec3 into every material, overriding Vec3s set by SetVector.
        /// \param name Vec3 uniform name.
        /// \param value Vec3 value.
        static void SetGlobalVector( const char* name, const Vec3& value );

        /// \return shader.
        class Shader* GetShader() { return shader; }

        /// \return True if the shader exists and has been compiled and linked successfully.
        bool IsValidShader() const;
        
        /// Applies the uniforms into the shader. Called internally.
        void Apply();

        /// \return True, if backfaces are culled.
        bool IsBackFaceCulled() const { return cullBackFaces; }

        /// \param enable Enable backface culling. Defaults to true.
        void SetBackFaceCulling( bool enable ) { cullBackFaces = enable; }
        
        /// \return Depth function.
        DepthFunction GetDepthFunction() const { return depthFunction; }
        
        /// \param aDepthFunction Depth function.
        void SetDepthFunction( DepthFunction aDepthFunction ) { depthFunction = aDepthFunction; }
        
        /// \param aOffset Depth offset
        /// \param aSlope Depth slope
        void SetDepthOffset( float aOffset, float aSlope ) { depthFactor = aOffset; depthUnits = aSlope; }
        
        /// \return blending mode.
        BlendingMode GetBlendingMode() const { return blendingMode; }
        
        /// \param blending Blending mode. Defaults to off.
        void SetBlendingMode( BlendingMode blending ) { blendingMode = blending; }
        
        /// \param shader Shader.
        void SetShader( Shader* shader );

        /// \param texture Texture.
        /// \param slot Slot index.
        void SetTexture( class Texture2D* texture, int slot );

        /// \param f0 Reflectance at grazing angle.
        void SetF0( float f0 );

        /// \param name Texture uniform name.
        /// \param texture Texture.
        void SetTexture( const char* name, class TextureCube* texture );

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
        static std::unordered_map< std::string, RenderTexture* > sTexRTs;
        static std::unordered_map< std::string, Texture2D* > sTex2ds;
        static std::unordered_map< std::string, float > sFloats;
        static std::unordered_map< std::string, int > sInts;
        static std::unordered_map< std::string, Vec3 > sVec3s;

        std::unordered_map< std::string, float > floats;
        std::unordered_map< std::string, int > ints;
        std::unordered_map< std::string, Vec3 > vec3s;
        std::unordered_map< std::string, Vec4 > vec4s;
        std::unordered_map< std::string, class TextureCube* > texCubes;
        std::unordered_map< std::string, RenderTexture* > texRTs;
        static constexpr int TEXTURE_SLOT_COUNT = 5;
        Texture2D* tex2dSlots[ TEXTURE_SLOT_COUNT ] = {};
        Shader* shader = nullptr;
        DepthFunction depthFunction = DepthFunction::LessOrEqualWriteOn;
        BlendingMode blendingMode = BlendingMode::Off;
        float depthFactor = 0;
        float depthUnits = 0;
        float f0 = 0.5f;
        bool cullBackFaces = true;
    };
}
