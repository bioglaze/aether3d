#ifndef RENDERER_H
#define RENDERER_H

#include "Shader.hpp"
#include "VertexBuffer.hpp"

namespace ae3d
{
    class TextureCube;
    class Vec3;
    class CameraComponent;
    
    struct BuiltinShaders
    {
        void Load();
        
        Shader spriteRendererShader;
        Shader sdfShader;
        Shader skyboxShader;
    };

    /// High-level rendering stuff.
    class Renderer
    {
    public:
        void RenderSkybox( const TextureCube* skyTexture, const CameraComponent& camera );

        BuiltinShaders builtinShaders;
        
    private:
        VertexBuffer skyboxBuffer;
    };    
}

#endif
