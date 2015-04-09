#include "Renderer.hpp"

ae3d::Renderer renderer;

void ae3d::BuiltinShaders::Load()
{
    const char* vertexSource =
    "#version 410 core\n \
    \
    layout (location = 0) in vec3 aPosition;\
    layout (location = 1) in vec2 aTexCoord;\
    layout (location = 2) in vec4 aColor;\
    uniform mat4 _ProjectionModelMatrix;\
    uniform vec4 textureMap_ST;\
    \
    out vec2 vTexCoord;\
    out vec4 vColor;\
    \
    void main()\
    {\
    gl_Position = _ProjectionModelMatrix * vec4(aPosition.xyz, 1.0);\
    vTexCoord = aTexCoord * textureMap_ST.xy + textureMap_ST.zw;\
    vColor = aColor;\
    }";
    
    const char* fragmentSource =
    "#version 410 core\n\
    \
    in vec2 vTexCoord;\
    in vec4 vColor;\
    out vec4 fragColor;\
    \
    uniform sampler2D textureMap;\
    \
    void main()\
    {\
    fragColor = texture( textureMap, vTexCoord ) * vColor;\
    }";
    
    spriteRendererShader.Load( vertexSource, fragmentSource );
}
