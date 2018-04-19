struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};
    
VSOutput main( float2 pos : POSITION, float2 uv : TEXCOORD )
{
    VSOutput vsOut;
    vsOut.pos = float4( pos, 0, 1 );
#ifdef VULKAN
    pos.y = -pos.y;
#endif
    vsOut.uv = uv;
    return vsOut;
}
