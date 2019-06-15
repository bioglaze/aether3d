layout(set=0, binding=0) cbuffer cbPerFrame : register(b0)
{
    matrix localToClip;
    matrix localToView;
    matrix localToWorld;
    matrix localToShadowClip;
    matrix clipToView;
    float4 lightPosition;
    float4 lightDirection;
    float4 lightColor;
    float lightConeAngle;
    int lightType;
    float minAmbient;
    uint maxNumLightsPerTile;
    uint windowWidth;
    uint windowHeight;
    uint numLights; // 16 bits for point light count, 16 for spot light count
    float f0;
    float4 tex0scaleOffset;
    float4 tilesXY;
    matrix boneMatrices[ 80 ];
    int isVR;
};
