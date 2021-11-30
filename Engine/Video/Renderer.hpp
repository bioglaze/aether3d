#pragma once

#include "ComputeShader.hpp"
#include "Shader.hpp"
#include "VertexBuffer.hpp"
#include "Texture2D.hpp"

namespace ae3d
{
    class TextureCube;
    class CameraComponent;
    struct Vec4;
    
    struct Particle
    {
        ae3d::Vec4 position;
        ae3d::Vec4 color;
        ae3d::Vec4 clipPosition;
    };

    struct BuiltinShaders
    {
        // Loads shader binaries from running directory's 'shaders' subfolder.
        void Load();
        
        Shader spriteRendererShader;
        Shader sdfShader;
        Shader skyboxShader;
        Shader momentsShader;
        Shader momentsSkinShader;
        Shader depthNormalsShader;
        Shader depthNormalsSkinShader;
        Shader uiShader;
        ComputeShader lightCullShader;
        ComputeShader particleSimulationShader;
        ComputeShader particleCullShader;
        ComputeShader particleDrawShader;
    };

    /// High-level rendering stuff.
    class Renderer
    {
    public:
        VertexBuffer& GetQuadBuffer() { return quadBuffer; }
        
        /// \return True if skybox has been generated.
        bool IsSkyboxGenerated() const { return skyboxBuffer.IsGenerated(); }
        
        void GenerateQuadBuffer();
        void GenerateSkybox();
        void GenerateTextures();
        void GenerateSSAOKernel( int count, Vec4* outKernel );
        void RenderSkybox( TextureCube* skyTexture, const CameraComponent& camera );

        unsigned GetNumParticleTilesX() const;
        unsigned GetNumParticleTilesY() const;

        Texture2D& GetWhiteTexture() { return whiteTexture; }

        BuiltinShaders builtinShaders;
        
    private:
        VertexBuffer skyboxBuffer;
        VertexBuffer quadBuffer;
        Texture2D whiteTexture;
    };    
}
