#include <simd/simd.h>

struct Uniforms
{
    matrix_float4x4 localToClip;
    matrix_float4x4 localToView;
    matrix_float4x4 localToWorld;
    matrix_float4x4 localToShadowClip;
    matrix_float4x4 clipToView;
    float4 lightPosition;
    float4 lightDirection;
    float4 lightColor;
    float lightConeAngle;
    int lightType; // 0: None, 1: Spot, 2: Dir, 3: Point
    float minAmbient;
    uint maxNumLightsPerTile;
    uint windowWidth;
    uint windowHeight;
    uint numLights; // 16 bits for point light count, 16 for spot light count
    float f0;
    float4 tex0scaleOffset;
    float4 tilesXY;
    matrix_float4x4 boneMatrices[ 80 ];
    int isVR;
};

