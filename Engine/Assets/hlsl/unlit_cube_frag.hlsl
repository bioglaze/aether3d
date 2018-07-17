struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 uv : TEXCOORD;
    float4 color : COLOR;
};

TextureCube<float4> tex : register(t0);
SamplerState sLinear : register(s0);

float4 main( VSOutput vsOut ) : SV_Target
{
    return tex.Sample( sLinear, vsOut.uv );
}
