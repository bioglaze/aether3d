#if !VULKAN
#define layout(a,b)  
#else
#define register(a) blank
#endif

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

layout(set=0, binding=1) Texture2DMS<float4> tex : register(t0);

float4 main( VSOutput vsOut ) : SV_Target
{
    float textureWidth;
    float textureHeight;
    float numSamples;
    tex.GetDimensions( textureWidth, textureHeight, numSamples );

    return tex.Load( vsOut.uv * float2( textureWidth, textureHeight), 0 );
}
