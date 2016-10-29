#ifndef RENDERER_H
#define RENDERER_H

#include "ComputeShader.hpp"
#include "Shader.hpp"
#include "VertexBuffer.hpp"
#include "Texture2D.hpp"

namespace ae3d
{
    class TextureCube;
    class CameraComponent;
    
    struct BuiltinShaders
    {
        void Load();
        
        Shader spriteRendererShader;
        Shader sdfShader;
        Shader skyboxShader;
        Shader momentsShader;
        Shader depthNormalsShader;
        ComputeShader lightCullShader;
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

        void RenderSkybox( TextureCube* skyTexture, const CameraComponent& camera );

        Texture2D& GetWhiteTexture() { return whiteTexture; }

        BuiltinShaders builtinShaders;
        
    private:
        VertexBuffer skyboxBuffer;
        VertexBuffer quadBuffer;
        Texture2D whiteTexture;
    };    
}

#endif
