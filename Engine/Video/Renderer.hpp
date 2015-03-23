#ifndef RENDERER_H
#define RENDERER_H

#include "Shader.hpp"
#include "VertexBuffer.hpp"

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
        // TODO: make private.
        BuiltinShaders builtinShaders;
        VertexBuffer quadBuffer;
        
    private:
    };
}

#endif
