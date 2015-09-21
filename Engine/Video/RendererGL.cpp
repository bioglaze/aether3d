#include "Renderer.hpp"

ae3d::Renderer renderer;

void ae3d::BuiltinShaders::Load()
{
    const char* spriteVertexSource =
    "#version 410 core\n \
    \
    layout (location = 0) in vec3 aPosition;\
    layout (location = 1) in vec2 aTexCoord;\
    layout (location = 2) in vec4 aColor;\
    uniform mat4 _ProjectionModelMatrix;\
    \
    out vec2 vTexCoord;\
    out vec4 vColor;\
    \
    void main()\
    {\
    gl_Position = _ProjectionModelMatrix * vec4(aPosition.xyz, 1.0);\
    vTexCoord = aTexCoord;\
    vColor = aColor;\
    }";
    
    const char* spriteFragmentSource =
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
    
    spriteRendererShader.Load( spriteVertexSource, spriteFragmentSource );

    const char* sdfVertexSource =
    "#version 410 core\n \
    \
    layout (location = 0) in vec3 aPosition;\
    layout (location = 1) in vec2 aTexCoord;\
    layout (location = 2) in vec4 aColor;\
    uniform mat4 _ProjectionModelMatrix;\
    \
    out vec2 vTexCoord;\
    out vec4 vColor;\
    \
    void main()\
    {\
    gl_Position = _ProjectionModelMatrix * vec4(aPosition.xyz, 1.0);\
    vTexCoord = aTexCoord;\
    vColor = aColor;\
    }";
    
    const char* sdfFragmentSource =
        "#version 410 core\n\
            \
                in vec2 vTexCoord;\
                    in vec4 vColor;\
                        out vec4 fragColor;\
                            \
                                uniform sampler2D textureMap;\
                                    \
                                            const float edgeDistance = 0.5;\
    void main()\
    {\
        float distance = texture(textureMap, vTexCoord).r;\
        float edgeWidth = 0.7 * length( vec2( dFdx( distance ), dFdy( distance ) ) );\
    float opacity = smoothstep( edgeDistance - edgeWidth, edgeDistance + edgeWidth, distance );\
        fragColor = vec4( vColor.rgb, opacity );\
    }";
    
    sdfShader.Load( sdfVertexSource, sdfFragmentSource );

    const char* skyboxVertexSource =
"#version 410 core\n \
\
layout (location = 0) in vec3 aPosition;\
uniform mat4 _ModelViewProjectionMatrix;\
\
out vec3 vTexCoord;\
\
void main()\
{\
gl_Position = _ModelViewProjectionMatrix * vec4(aPosition.xyz, 1.0);\
vTexCoord = aPosition;\
}";

const char* skyboxFragmentSource =
"#version 410 core\n\
\
in vec3 vTexCoord;\
out vec4 fragColor;\
\
uniform samplerCube skyMap;\
\
void main()\
{\
fragColor = texture( skyMap, vTexCoord );\
}";

    skyboxShader.Load( skyboxVertexSource, skyboxFragmentSource );
    
    const char* momentsVertexSource =
    "#version 410 core\n \
    \
    layout (location = 0) in vec3 aPosition;\
    \
    uniform mat4 _ModelViewProjectionMatrix;\
    \
    void main()\
    {\
        gl_Position = _ModelViewProjectionMatrix * vec4( aPosition, 1.0 );\
    }";
    
    const char* momentsFragmentSource =
    "#version 410 core\n \
    \
    out vec4 fragColor;\
    \
    void main()\
    {\
        float linearDepth = gl_FragCoord.z;\
    \
        float dx = dFdx( linearDepth );\
        float dy = dFdy( linearDepth );\
    \
        float moment1 = linearDepth;\
        float moment2 = linearDepth * linearDepth + 0.25 * (dx * dx + dy * dy);\
    \
        fragColor = vec4( moment1, moment2, 0.0, 1.0 );\
    }\
    ";

    momentsShader.Load( momentsVertexSource, momentsFragmentSource );
}
