#ifndef RENDERER_H
#define RENDERER_H

#include "Shader.hpp"

namespace ae3d
{
    struct BuiltinShaders
    {
        void Load();
        
        Shader spriteRendererShader;
    };

    // High-level rendering stuff.
    class Renderer
    {
    public:
        //void AddSprite( const Matrix44* projectionMatrix, const Texture2D* texture );
        
#pragma message("TODO: make private.")
        BuiltinShaders builtinShaders;
        
    private:
    };
}

#endif
