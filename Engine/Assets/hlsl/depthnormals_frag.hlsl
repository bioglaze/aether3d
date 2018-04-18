struct VSOutput
{
    float4 pos : SV_Position;
    float3 mvPosition : POSITION;
    float3 normal : NORMAL;
};

float4 main( VSOutput vsOut ) : SV_Target
{
    float linearDepth = vsOut.mvPosition.z;
    return float4( linearDepth, vsOut.normal );
}
