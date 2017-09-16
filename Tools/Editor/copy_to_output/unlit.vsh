#version 330 core

uniform vec4 textureMap_ST;

layout(std140) uniform PerObject
{
    mat4 localToClip;
    mat4 localToView;
    mat4 localToWorld;
    mat4 localToShadowClip;
    vec4 lightPosition;
    vec4 lightDirection;
    vec4 lightColor;
    float lightConeAngleCos;
    int lightType; // 0: None, 1: Spot, 2: Dir, 3: Point
    float minAmbient;
};

layout (location = 0) in vec4 aPosition;
layout (location = 1) in vec2 aTexCoord;
layout (location = 5) in ivec4 boneIndex;
layout (location = 6) in vec4 boneWeights;

out vec2 vTexCoord;

#define SHADOW_MAP
#ifdef SHADOW_MAP
out vec4 vProjCoord;
#endif

void main()
{
    gl_Position = localToClip * aPosition;
    vTexCoord = aTexCoord * textureMap_ST.xy + textureMap_ST.zw;
#ifdef SHADOW_MAP
    vProjCoord = localToShadowClip * aPosition;
#endif
}
