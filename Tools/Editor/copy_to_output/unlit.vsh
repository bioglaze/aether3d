#version 330 core

uniform mat4 _ModelViewProjectionMatrix;
uniform mat4 _ModelViewMatrix;
uniform mat4 _ShadowProjectionMatrix;
uniform vec4 textureMap_ST;

layout (location = 0) in vec4 aPosition;
layout (location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

#define SHADOW_MAP
#ifdef SHADOW_MAP
out vec4 vProjCoord;
#endif

void main()
{
    gl_Position = _ModelViewProjectionMatrix * aPosition;
    vTexCoord = aTexCoord * textureMap_ST.xy + textureMap_ST.zw;
#ifdef SHADOW_MAP
    vProjCoord = _ShadowProjectionMatrix * aPosition;
#endif
}
