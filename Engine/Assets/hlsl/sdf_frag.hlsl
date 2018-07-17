struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

Texture2D< float4 > tex : register(t0);
SamplerState sLinear : register(s0);

float4 main( VSOutput vsOut ) : SV_Target
{
    const float edgeDistance = 0.5f;
    float distance = tex.Sample( sLinear, vsOut.uv ).r;
    float edgeWidth = 0.7f * length( float2( ddx( distance ), ddy( distance ) ) );
    float opacity = smoothstep( edgeDistance - edgeWidth, edgeDistance + edgeWidth, distance );
    return float4( vsOut.color.rgb, opacity );
}
