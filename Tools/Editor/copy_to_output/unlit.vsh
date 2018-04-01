#version 330 core

layout(std140) uniform PerObject
{
    mat4 localToClip;
    mat4 localToView;
    mat4 localToWorld;
    mat4 localToShadowClip;
    mat4 clipToView;
    vec4 lightPosition;
    vec4 lightDirection;
    vec4 lightColor;
    float lightConeAngle;
    int lightType; // 0: None, 1: Spot, 2: Dir, 3: Point
    float minAmbient;
    uint maxNumLightsPerTile;
    uint windowWidth;
    uint windowHeight;
    uint numLights; // 16 bits for point light count, 16 for spot light count
    uint padding;
    vec4 tex0scaleOffset;
    mat4 boneMatrices[ 80 ];
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
    vTexCoord = aTexCoord * tex0scaleOffset.xy + tex0scaleOffset.zw;
#ifdef SHADOW_MAP
    vProjCoord = localToShadowClip * aPosition;
#endif
}
