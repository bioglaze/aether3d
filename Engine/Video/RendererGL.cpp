#include "Renderer.hpp"

ae3d::Renderer renderer;

void ae3d::BuiltinShaders::Load()
{
    const char* vertexSource =
    "#version 410 core\n \
    \
    layout (location = 0) in vec3 aPosition;\
    layout (location = 1) in vec2 aTexCoord;\
    uniform mat4 _ProjectionMatrix;\
    uniform vec4 textureMap_ST;\
    uniform vec3 _Position;\
    \
    out vec2 vTexCoord;\
    \
    void main()\
    {\
    gl_Position = _ProjectionMatrix * vec4(aPosition.xyz /*+ _Position*/, 1.0);\
    vTexCoord = aTexCoord;\
    }";
    
    const char* fragmentSource =
    "#version 410 core\n\
    \
    in vec2 vTexCoord;\
    out vec4 fragColor;\
    \
    uniform sampler2D textureMap;\
    \
    void main()\
    {\
    fragColor = texture( textureMap, vTexCoord );\
    }";
    
    spriteRendererShader.Load( vertexSource, fragmentSource );
}
