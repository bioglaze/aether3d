#include <simd/simd.h>

struct Uniforms
{
    matrix_float4x4 localToClip;
    matrix_float4x4 localToView;
    matrix_float4x4 localToWorld;
    matrix_float4x4 localToShadowClip;
    matrix_float4x4 clipToView;
    matrix_float4x4 viewToClip;
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
    int kernelSize;
    float3 kernelOffsets[ 128 ];
};

static float linstep( float low, float high, float v )
{
    return clamp( (v - low) / (high - low), 0.0f, 1.0f );
}

static constexpr sampler shadowSampler( coord::normalized, address::clamp_to_zero, filter::linear, mip_filter::linear );

static float VSM( texture2d<float, access::sample> shadowMap, float4 projCoord, float depth, int lightType )
{
    float2 uv = (projCoord.xy / projCoord.w) * 0.5f + 0.5f;
    uv.y = 1.0f - uv.y;
    
    // Spot light
    if (lightType == 1 && (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1 || depth < 0 || depth > 1))
    {
        return 0;
    }

    // Directional light
    if (lightType == 2 && (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1 || depth < 0 || depth > 1))
    {
        return 1;
    }

    float2 moments = shadowMap.sample( shadowSampler, uv ).rg;
    
    float variance = max( moments.y - moments.x * moments.x, -0.001f );

    float delta = depth - moments.x;
    float p = smoothstep( depth - 0.02f, depth, moments.x );
    float pMax = linstep( 0.2f, 1.0f, variance / (variance + delta * delta) );
    
    return clamp( max( p, pMax ), 0.0f, 1.0f );
}
