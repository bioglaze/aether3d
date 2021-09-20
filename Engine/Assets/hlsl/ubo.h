// Must be kept in sync with ParticleSystemComponent.cpp
struct Particle
{
    float4 position;
    float4 color;
    float4 clipPosition; // Screen-space position.
};

#if !VULKAN
Texture2D tex : register(t0);
Texture2D normalTex : register(t1);
Texture2D specularTex : register(t2);
Texture2D shadowTex : register(t3);
TextureCube<float4> texCube : register(t4);
SamplerState sLinear : register(s0);
SamplerState sampler1 : register(s1);
cbuffer cbPerFrame : register(b0)
{
    matrix localToClip;
    matrix localToView;
    matrix localToWorld;
    matrix localToShadowClip;
    matrix clipToView;
    matrix viewToClip;
    float4 lightPosition;
    float4 lightDirection;
    float4 lightColor;
    float lightConeAngle;
    int lightType;
    float minAmbient;
    uint maxNumLightsPerTile;
    uint windowWidth;
    uint windowHeight;
    uint numLights; // 16 bits for point light count, 16 for spot light count
    float f0;
    float4 tex0scaleOffset;
    float4 tilesXY;
    matrix boneMatrices[ 80 ];
    int isVR;
    int kernelSize;
    float2 bloomParams;
    float4 kernelOffsets[ 16 ];
    float4 cameraParams; // .x: fov (radians), .y: aspect, .z: near, .w: far
    int particleCount;
};
Buffer<float4> pointLightBufferCenterAndRadius : register(t5);
RWBuffer<uint> perTileLightIndexBuffer : register(u0);
Buffer<float4> pointLightColors : register(t6);
Buffer<float4> spotLightBufferCenterAndRadius : register(t7);
Buffer<float4> spotLightParams : register(t8);
Buffer<float4> spotLightColors : register(t9);
RWTexture2D<float4> rwTexture : register(u1);
RWStructuredBuffer< Particle > particles : register(u2);

#else

[[vk::binding( 0 )]] Texture2D tex;
[[vk::binding( 1 )]] Texture2D normalTex;
[[vk::binding( 2 )]] Texture2D specularTex;
[[vk::binding( 3 )]] Texture2D shadowTex;
[[vk::binding( 4 )]] TextureCube<float4> texCube;
[[vk::binding( 5 )]] SamplerState sLinear;
[[vk::binding( 6 )]] SamplerState sampler1;
[[vk::binding( 7 )]] cbuffer cbPerFrame
{
    matrix localToClip;
    matrix localToView;
    matrix localToWorld;
    matrix localToShadowClip;
    matrix clipToView;
    matrix viewToClip;
    float4 lightPosition;
    float4 lightDirection;
    float4 lightColor;
    float lightConeAngle;
    int lightType;
    float minAmbient;
    uint maxNumLightsPerTile;
    uint windowWidth;
    uint windowHeight;
    uint numLights; // 16 bits for point light count, 16 for spot light count
    float f0;
    float4 tex0scaleOffset;
    float4 tilesXY;
    matrix boneMatrices[ 80 ];
    int isVR;
    int kernelSize;
    float2 bloomParams;
    float4 kernelOffsets[ 16 ];
    float4 cameraParams; // .x: fov (radians), .y: aspect, .z: near, .w: far
    int particleCount;
};
[[vk::binding( 8 )]] Buffer<float4> pointLightBufferCenterAndRadius;
[[vk::binding( 9 )]] RWBuffer<uint> perTileLightIndexBuffer;
[[vk::binding( 10 )]] Buffer<float4> pointLightColors;
[[vk::binding( 11 )]] Buffer<float4> spotLightBufferCenterAndRadius;
[[vk::binding( 12 )]] Buffer<float4> spotLightParams;
[[vk::binding( 13 )]] Buffer<float4> spotLightColors;
[[vk::binding( 14 )]] RWTexture2D<float4> rwTexture;
[[vk::binding( 15 )]] RWStructuredBuffer< Particle > particles;
#endif
