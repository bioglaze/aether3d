struct VSOutput
{
    float4 pos : SV_Position;
};

float4 main( VSOutput vsOut ) : SV_Target
{
    float linearDepth = vsOut.pos.z;
    
    float dx = ddx( linearDepth );
    float dy = ddy( linearDepth );
    
    float moment1 = linearDepth;
    float moment2 = linearDepth * linearDepth + 0.25f * (dx * dx + dy * dy);
    
    return float4( moment1, moment2, 0.0f, 1.0f );
}
