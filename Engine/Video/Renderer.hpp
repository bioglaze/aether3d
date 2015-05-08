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

    /** High-level rendering stuff. */
    class Renderer
    {
    public:
        BuiltinShaders builtinShaders;
        
    private:
    };    
}

#endif
