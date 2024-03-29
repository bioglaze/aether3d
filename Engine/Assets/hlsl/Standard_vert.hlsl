#include "ubo.h"

struct VS_INPUT
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
};

struct PS_INPUT
{
    float4 pos : SV_Position;
    float4 positionVS_u : TEXCOORD0;
    float4 positionWS_v : TEXCOORD1;
    float3 tangentVS : TANGENT;
    float3 bitangentVS : BINORMAL;
    float3 normalVS : NORMAL;
    float4 projCoord : TEXCOORD2;
};

PS_INPUT main( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
    float4 position = float4( input.pos, 1.0f );

    output.pos = mul( localToClip, position );
    output.positionVS_u = float4( mul( localToView, position ).xyz, input.uv.x );
    output.positionWS_v = float4( mul( localToWorld, position ).xyz, input.uv.y );
    output.normalVS = mul( localToView, float4(input.normal, 0) ).xyz;
    output.tangentVS = mul( localToView, float4(input.tangent.xyz, 0) ).xyz;
    output.projCoord = mul( localToShadowClip, float4( input.pos, 1.0 ) );
    //float3 ct = cross( input.normal, input.tangent.xyz ) * input.tangent.w;
    // FIXME: This is not what MikkTSpace does, it does the above version! But the renderer is not yet
    //        fully MikkTSpace compatible, and this is needed to get light direction working correctly.
    float3 ct = cross( input.tangent.xyz, input.normal ) * input.tangent.w;
    output.bitangentVS.xyz = mul( localToView, float4( ct, 0 ) ).xyz;

    return output;
}
