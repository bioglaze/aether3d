#if !VULKAN
#define layout(a,b)  
#else
#define register(a) blank
#endif

struct VSOutput
{
    float4 pos : SV_Position;
    float4 color : COLOR;
    float3 uv : TEXCOORD;
};

layout (set=0, binding=1) TextureCube< float4 > skyMap : register(t0);
layout (set=0, binding=2) SamplerState sLinear : register(s0);

float4 main( VSOutput vsOut ) : SV_Target
{
    return skyMap.Sample( sLinear, vsOut.uv );
}
