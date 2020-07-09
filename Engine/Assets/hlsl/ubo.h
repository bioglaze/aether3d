layout( set = 0, binding = 0 ) Texture2D tex : register(t0);
layout( set = 0, binding = 1 ) Texture2D normalTex : register(t1);
layout( set = 0, binding = 2 ) Texture2D specularTex : register(t2);
layout( set = 0, binding = 3 ) Texture2D shadowTex : register(t3);
layout( set = 0, binding = 4 ) TextureCube<float4> texCube : register(t4);
layout( set = 0, binding = 5 ) SamplerState sLinear : register(s0);
layout( set = 0, binding = 6 ) SamplerState sampler1 : register(s1);
layout( set = 0, binding = 7 ) cbuffer cbPerFrame : register(b0)
{
    matrix localToClip;
    matrix localToView;
    matrix localToWorld;
    matrix localToShadowClip;
    matrix clipToView;
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
};
layout( set = 0, binding = 8 ) Buffer<float4> pointLightBufferCenterAndRadius : register(t5);
layout( set = 0, binding = 9 ) RWBuffer<uint> perTileLightIndexBuffer : register(u0);
layout( set = 0, binding = 10 ) Buffer<float4> pointLightColors : register(t6);
layout( set = 0, binding = 11 ) Buffer<float4> spotLightBufferCenterAndRadius : register(t7);
layout( set = 0, binding = 12 ) Buffer<float4> spotLightParams : register(t8);
layout( set = 0, binding = 13 ) Buffer<float4> spotLightColors : register(t9);
layout( set = 0, binding = 14 ) RWTexture2D<float4> rwTexture : register(u1);
