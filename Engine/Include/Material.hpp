#pragma once

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
        /// \param renderTexture Render texture.
        static void SetGlobalRenderTexture( class RenderTexture* renderTexture );

        /// \return shader.
        class Shader* GetShader() { return shader; }

        float GetRoughness() const { return roughness; }
        
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

        /// \param rough Roughness. Defaults to 0.5f. Range is usually 0-1.
        void SetRoughness( float rough ) { roughness = rough; }
        
        /// \param texture Texture.
        void SetTexture( class TextureCube* texture );

        /// \param renderTexture Render texture.
        /// \param slot Slot in the shader.
        void SetRenderTexture( RenderTexture* renderTexture, int slot );
        
        /// \return Alpha threshold. Values below it are discarded.
        float GetAlphaThreshold() const { return alphaThreshold; }

        /// \return True, if the mesh is alpha tested.
        bool IsAlphaTested() const { return alphaTest != nullptr; }

        /// \param texture If not null, enable alpha testing. Currently only used for shadow rendering!
        /// \param threshold Alpha threshold.
        void SetAlphaTest( Texture2D* texture, float threshold )
        {
            alphaTest = texture;
            alphaThreshold = threshold;
        }

  private:
        static RenderTexture* sTexRT;

        static constexpr int TEXTURE_SLOT_COUNT = 13;
        Texture2D* tex2dSlots[ TEXTURE_SLOT_COUNT ] = {};
        TextureCube* texCubeSlots[ TEXTURE_SLOT_COUNT ] = {};
        RenderTexture* rtSlots[ TEXTURE_SLOT_COUNT ] = {};
        Shader* shader = nullptr;
        DepthFunction depthFunction = DepthFunction::LessOrEqualWriteOn;
        BlendingMode blendingMode = BlendingMode::Off;
        float depthFactor = 0;
        float depthUnits = 0;
        float f0 = 0.5f;
        float roughness = 0.5f;
        float alphaThreshold = 0.5f;
        Texture2D* alphaTest = nullptr;
        bool cullBackFaces = true;
    };
}
