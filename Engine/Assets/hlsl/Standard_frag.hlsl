#if !VULKAN
#define layout(a,b)  
#else
#define register(a) blank
#endif

//#define DEBUG_LIGHT_COUNT

#include "ubo.h"

struct PS_INPUT
{
    float4 pos : SV_Position;
    float4 positionVS_u : TEXCOORD0;
    float4 positionWS_v : TEXCOORD1;
    float3 tangentVS : TANGENT;
    float3 bitangentVS : BINORMAL;
    float3 normalVS : NORMAL;
};

layout(set=0, binding=1) Texture2D tex : register(t0);
layout(set=0, binding=2) SamplerState sLinear : register(s0);
layout(set=0, binding=3) Buffer<float4> pointLightBufferCenterAndRadius : register(t1);
layout(set=0, binding=4) RWBuffer<uint> perTileLightIndexBuffer : register(u1);
layout(set=0, binding=5) Texture2D normalTex : register(t1);
layout(set=0, binding=7) Buffer<float4> pointLightColors : register(t2);
layout(set=0, binding=8) Buffer<float4> spotLightBufferCenterAndRadius : register(t3);
layout(set=0, binding=9) Buffer<float4> spotLightParams : register(t4);

#define TILE_RES 16
#define LIGHT_INDEX_BUFFER_SENTINEL 0x7fffffff

uint GetNumLightsInThisTile( uint tileIndex )
{
    uint numLightsInThisTile = 0;
    uint index = maxNumLightsPerTile * tileIndex;
    uint nextLightIndex = perTileLightIndexBuffer[ index ];

    // count point lights
    while (nextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL)
    {
        ++numLightsInThisTile;
        ++index;
        nextLightIndex = perTileLightIndexBuffer[ index ];
    }

    ++index;
    nextLightIndex = perTileLightIndexBuffer[ index ];
    
    // count spot lights
    while (nextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL)
    {
        ++numLightsInThisTile;
        ++index;
        nextLightIndex = perTileLightIndexBuffer[ index ];
    }

    return numLightsInThisTile;
}

uint GetTileIndex( float2 screenPos )
{
    const float tileRes = (float)TILE_RES;
    uint numCellsX = (windowWidth + TILE_RES - 1) / TILE_RES;
    uint tileIdx = floor( screenPos.x / tileRes ) + floor( screenPos.y / tileRes ) * numCellsX;
    return tileIdx;
}

float3 tangentSpaceTransform( float3 tangent, float3 bitangent, float3 normal, float3 v )
{
    return normalize( v.x * normalize( tangent ) + v.y * normalize( bitangent ) + v.z * normalize( normal ) );
}

float4 main( PS_INPUT input ) : SV_Target
{
    const float4 albedo = tex.Sample( sLinear, float2( input.positionVS_u.w, input.positionWS_v.w ) );
    const float4 normalTS = normalTex.Sample( sLinear, float2( input.positionVS_u.w, input.positionWS_v.w ) );

    const uint tileIndex = GetTileIndex( input.pos.xy );
    uint index = maxNumLightsPerTile * tileIndex;
    uint nextLightIndex = perTileLightIndexBuffer[ index ];

    const float3 normalVS = tangentSpaceTransform( input.tangentVS, input.bitangentVS, input.normalVS, normalTS.xyz );

    float3 accumDiffuseAndSpecular = lightColor.rgb;
    const float3 surfaceToLightVS = lightDirection.xyz;
    const float3 diffuseDirectional = max( 0.0f, dot( input.normalVS, surfaceToLightVS ) );
    accumDiffuseAndSpecular *= diffuseDirectional;
    accumDiffuseAndSpecular = max( float3( 0.25f, 0.25f, 0.25f ), accumDiffuseAndSpecular );

    const float3 surfaceToCameraVS = -input.positionVS_u.xyz;
    const float specularDirectional = pow( max( 0.0f, dot( surfaceToCameraVS, reflect( -surfaceToLightVS, input.normalVS ) ) ), 0.2f );
    //accumDiffuseAndSpecular.rgb += specularDirectional;

    //[loop] // Point lights
    while (nextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL)
    {
        uint lightIndex = nextLightIndex;
        ++index;
        nextLightIndex = perTileLightIndexBuffer[ index ];

        const float4 centerAndRadius = pointLightBufferCenterAndRadius[ lightIndex ];
        const float radius = centerAndRadius.w;

        const float3 vecToLightVS = (mul( localToView, float4( centerAndRadius.xyz, 1.0f ) ) ).xyz - input.positionVS_u.xyz;
        const float3 vecToLightWS = centerAndRadius.xyz - input.positionWS_v.xyz;
        const float3 lightDirVS = normalize( vecToLightVS );

        const float dotNL = saturate( dot( normalize( normalVS ), lightDirVS ) );
        const float lightDistance = length( vecToLightWS );
        float falloff = 1.0f;

        if (lightDistance < radius)
        {
            const float x = lightDistance / radius;
            falloff = -0.05f + 1.05f / (1.0f + 20.0f * x * x);
            //accumDiffuseAndSpecular.rgb += max( 0.0f, dotNL );// * falloff;
            //accumDiffuseAndSpecular.rgb = float3( 1.0f, 0.0f, 0.0f );
            accumDiffuseAndSpecular.rgb += pointLightColors[ lightIndex ].rgb * falloff * 2;
            //accumDiffuseAndSpecular.rgb += pointLightColors[ lightIndex ].rgb * abs( dot( lightDirVS, normalize( input.normalVS ) ) );
        }

        //outColor.rgb += lightDistance < radius ? abs(dot( lightDirVS, normalize( in.normalVS ) )) : 0;
        //outColor.rgb += -dot( lightDirVS, normalize( in.normalVS ) );
        //accumDiffuseAndSpecular += lightDistance < radius ? 1.0f : 0.25f;
    }

    // move past the first sentinel to get to the spot lights
    ++index;
    nextLightIndex = perTileLightIndexBuffer[ index ];

    //[loop] // Spot lights
    while (nextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL)
    {
        uint lightIndex = nextLightIndex;
        ++index;
        nextLightIndex = perTileLightIndexBuffer[ index ];

        const float4 centerAndRadius = spotLightBufferCenterAndRadius[ lightIndex ];
        const float4 spotParams = spotLightParams[ lightIndex ];
        
        // reconstruct z component of the light dir from x and y
        float3 spotLightDir;
        spotLightDir.xy = spotParams.xy;
        spotLightDir.z = sqrt( 1 - spotLightDir.x * spotLightDir.x - spotLightDir.y * spotLightDir.y );

        // the sign bit for cone angle is used to store the sign for the z component of the light dir
        spotLightDir.z = (spotParams.z > 0) ? spotLightDir.z : -spotLightDir.z;

        const float3 vecToLight = normalize( centerAndRadius.xyz - input.positionWS_v.xyz );
        const float spotAngle = dot( -spotLightDir.xyz, vecToLight );
        const float cosineOfConeAngle = (spotParams.z > 0) ? spotParams.z : -spotParams.z;

        // Falloff
        const float dist = distance( input.positionWS_v.xyz, centerAndRadius.xyz );
        const float a = dist / centerAndRadius.w + 1.0f;
        const float att = 1.0f / (a * a);
        const float3 color = /*spotLightColors[ nLightIndex ].rgb*/ float3( 1, 1, 1 ) * att;// * specularTex;

        accumDiffuseAndSpecular += spotAngle > cosineOfConeAngle ? color : float3( 0, 0, 0 );

        //accumDiffuseAndSpecular += LightColorDiffuse + LightColorSpecular;
        //accumDiffuseAndSpecular = float3( 0, 1, 0 );
    }
        
    //return float4(accumDiffuseAndSpecular, 1 );
#ifdef DEBUG_LIGHT_COUNT
    const uint numLights = GetNumLightsInThisTile( tileIndex );

    if (numLights == 0)
    {
        return float4( 1, 1, 1, 1 ) * albedo;
    }
    else if (numLights < 10)
    {
        return float4( 0, 1, 0, 1 ) * albedo;
    }
    else if (numLights < 20)
    {
        return float4( 1, 1, 0, 1 ) * albedo;
    }
    else
    {
        return float4( 1, 0, 0, 1 ) * albedo;
    }
#endif

    return float4( albedo.rgb * accumDiffuseAndSpecular, 1.0f );
}
