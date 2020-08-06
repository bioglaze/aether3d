#if !VULKAN
#define layout(a,b)  
#else
#define register(a) blank
#endif

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
    float4 projCoord : TANGENT;
};
    
#include "ubo.h"

float linstep( float low, float high, float v )
{
    return saturate( (v - low) / (high - low) );
}

float VSM( float depth, float4 projCoord )
{
    float2 uv = projCoord.xy / projCoord.w;
    
    // Spot light
    if (lightType == 1 && (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1))
    {
        return 0;
    }

    if (lightType == 1 && (depth < 0 || depth > 1))
    {
        return 0;
    }

    float2 moments = shadowTex.SampleLevel( sLinear, uv, 0 ).rg;

    float variance = max( moments.y - moments.x * moments.x, -0.001f );

    float delta = depth - moments.x;
    float p = smoothstep( depth - 0.02f, depth, moments.x );
    float pMax = linstep( minAmbient, 1.0f, variance / (variance + delta * delta) );

    return saturate( max( p, pMax ) );
}

float4 main( VSOutput vsOut ) : SV_Target
{
    float depth = vsOut.projCoord.z / vsOut.projCoord.w;

    float shadow = lightType == 0 ? 1.0f : max( minAmbient, VSM( depth, vsOut.projCoord ) );
    float4 texColor = tex.Sample( sLinear, vsOut.uv );
    float4 outColor = texColor * shadow;
    outColor.a = texColor.a;
    return outColor;
}
