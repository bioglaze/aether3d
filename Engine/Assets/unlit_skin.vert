#version 450 core
    
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aColor;
layout (location = 5) in ivec4 boneIndex;
layout (location = 6) in vec4 boneWeights;

// Prevents generating code that needs ClipDistance which is not available.
out gl_PerVertex { vec4 gl_Position; };

layout (set = 0, binding = 0) uniform UBO 
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
    int lightType;
    float minAmbient;
    uint maxNumLightsPerTile;
    uint windowWidth;
    uint windowHeight;
    uint numLights; // 16 bits for point light count, 16 for spot light count
    uint padding;
    mat4 boneMatrices[ 80 ];
} ubo;

layout (location = 0) out vec2 vTexCoord;
layout (location = 1) out vec4 vColor;
    
void main()
{
    vec4 position4 = vec4( aPosition, 1.0 );
    vec4 position2 = ubo.boneMatrices[ boneIndex.x ] * position4 * boneWeights.x;
    position2 += ubo.boneMatrices[ boneIndex.y ] * position4 * boneWeights.y;
    position2 += ubo.boneMatrices[ boneIndex.z ] * position4 * boneWeights.z;
    position2 += ubo.boneMatrices[ boneIndex.w ] * position4 * boneWeights.w;
    gl_Position = ubo.localToClip * position2;

    vTexCoord = aTexCoord;
    vColor = aColor;
}
    