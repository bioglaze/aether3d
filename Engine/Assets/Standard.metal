#include <metal_stdlib>
#include <simd/simd.h>

#define TILE_RES 16
#define NUM_THREADS_PER_TILE (TILE_RES * TILE_RES)
#define LIGHT_INDEX_BUFFER_SENTINEL 0x7fffffff

//#define DEBUG_LIGHT_COUNT

using namespace metal;

#include "MetalCommon.h"

struct StandardColorInOut
{
    float4 position [[position]];
    float4 projCoord;
    float3 positionVS;
    float3 positionWS;
    float4 tangentVS_u;
    float4 bitangentVS_v;
    float3 normalVS;
    half4  color;
};

struct StandardVertex
{
    float3 position [[attribute(0)]];
    float2 texcoord [[attribute(1)]];
    float3 normal [[attribute(3)]];
    float4 tangent [[attribute(4)]];
    float4 color [[attribute(2)]];
};

static uint GetNumLightsInThisTile( uint tileIndex, uint maxNumLightsPerTile, const device uint* perTileLightIndexBuffer )
{
    uint numLightsInThisTile = 0;
    uint index = maxNumLightsPerTile * tileIndex;
    uint nextLightIndex = perTileLightIndexBuffer[ index ];

    // count point lights
    while (nextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL)
    {
        //++numLightsInThisTile;
        ++index;
        nextLightIndex = perTileLightIndexBuffer[ index ];
    }

    // count spot lights
    
    // Moves past sentinel
    ++index;

    while (nextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL)
    {
        ++numLightsInThisTile;
        ++index;
        nextLightIndex = perTileLightIndexBuffer[ index ];
    }

    return numLightsInThisTile;
}

static uint GetTileIndex( float2 ScreenPos, int windowWidth )
{
    float tileRes = float( TILE_RES );
    uint numCellsX = (windowWidth + TILE_RES - 1) / TILE_RES;
    uint tileIdx = uint( floor( ScreenPos.x / tileRes ) + floor( ScreenPos.y / tileRes ) * numCellsX );
    return tileIdx;
}

// calculate the number of tiles in the horizontal direction
static uint GetNumTilesX( int windowWidth )
{
    return uint(((windowWidth + TILE_RES - 1) / float(TILE_RES)));
}

// calculate the number of tiles in the vertical direction
static uint GetNumTilesY( int windowHeight )
{
    return uint(((windowHeight + TILE_RES - 1) / float(TILE_RES)));
}

float3 tangentSpaceTransform( float3 tangent, float3 bitangent, float3 normal, float3 v )
{
    return normalize( v.x * normalize( tangent ) + v.y * normalize( bitangent ) + v.z * normalize( normal ) );
}

vertex StandardColorInOut standard_vertex( StandardVertex vert [[stage_in]],
                               constant Uniforms& uniforms [[ buffer(5) ]],
                               unsigned int vid [[ vertex_id ]] )
{
    StandardColorInOut out;
    
    float4 in_position = float4( vert.position, 1.0 );
    out.position = uniforms.localToClip * in_position;
    out.positionVS = (uniforms.localToView * in_position).xyz;
    out.positionWS = (uniforms.localToWorld * in_position).xyz;
    
    out.color = half4( vert.color );
    out.projCoord = uniforms.localToShadowClip * in_position;
    
    out.tangentVS_u.xyz = (uniforms.localToView * float4( vert.tangent.xyz, 0 )).xyz;
    out.tangentVS_u.w = vert.texcoord.x;
    float3 ct = cross( vert.normal, vert.tangent.xyz ) * vert.tangent.w;
    out.bitangentVS_v.xyz = normalize( uniforms.localToView * float4( ct, 0 ) ).xyz;
    out.bitangentVS_v.w = vert.texcoord.y;
    out.normalVS = (uniforms.localToView * float4( vert.normal.xyz, 0 )).xyz;
    
    return out;
}

fragment float4 standard_fragment( StandardColorInOut in [[stage_in]],
                               texture2d<float, access::sample> albedoSmoothnessMap [[texture(0)]],
                               texture2d<float, access::sample> _ShadowMap [[texture(1)]],
                               texture2d<float, access::sample> normalMap [[texture(2)]],
                               texture2d<float, access::sample> specularMap [[texture(3)]],
                               constant Uniforms& uniforms [[ buffer(5) ]],
                               const device uint* perTileLightIndexBuffer [[ buffer(6) ]],
                               constant float4* pointLightBufferCenterAndRadius [[ buffer(7) ]],
                               constant float4* spotLightBufferCenterAndRadius [[ buffer(8) ]],
                               constant float4* pointLightBufferColors [[ buffer(9) ]],
                               constant float4* spotLightParams [[ buffer(10) ]],
                               constant float4* spotLightBufferColors [[ buffer(11) ]],
                               sampler sampler0 [[sampler(0)]] )
{
    const float2 uv = float2( in.tangentVS_u.w, in.bitangentVS_v.w );
    const float4 albedoColor = float4( albedoSmoothnessMap.sample( sampler0, uv ) );
    const float smoothness = albedoColor.a;
    const float4 normalTS = float4( normalMap.sample( sampler0, uv ) );
    const float4 specular = float4( specularMap.sample( sampler0, uv ) );
    
    const float3 normalVS = tangentSpaceTransform( in.tangentVS_u.xyz, in.bitangentVS_v.xyz, in.normalVS, normalTS.xyz );
    
    const uint tileIndex = GetTileIndex( in.position.xy, uniforms.windowWidth );
    uint index = uniforms.maxNumLightsPerTile * tileIndex;
    uint nextLightIndex = perTileLightIndexBuffer[ index ];

    float4 outColor = float4( 0.25, 0.25, 0.25, 1 );

    while (nextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL)
    {
        uint lightIndex = nextLightIndex;
        index++;
        nextLightIndex = perTileLightIndexBuffer[ index ];

        float4 center = pointLightBufferCenterAndRadius[ lightIndex ];
        float radius = center.w;

        float3 vecToLightVS = (uniforms.localToView * float4( center.xyz, 1 )).xyz - in.positionVS.xyz;
        float3 vecToLightWS = center.xyz - in.positionWS.xyz;
        float3 lightDirVS = normalize( vecToLightVS );
        
        float dotNL = clamp( dot( normalize( in.normalVS ), lightDirVS ), 0.0, 1.0 );
        float lightDistance = length( vecToLightWS );
        float falloff = 1;
        
        if (lightDistance < radius)
        {
            float x = lightDistance / radius;
            falloff = -0.05 + 1.05 / (1.0 + 5.0 * x * x);
            //outColor.rgb += max( 0.0, dotNL );// * falloff;
            outColor.rgb += pointLightBufferColors[ lightIndex ].rgb * falloff;
        }
        
        //outColor.rgb += lightDistance < radius ? abs(dot( lightDirVS, normalize( in.normalVS ) )) : 0;
        
        //outColor.rgb += -dot( lightDirVS, normalize( in.normalVS ) );
        //outColor.rgb += lightDistance < radius ? 1 : 0.25;
    }
    
    // Moves past the first sentinel to get to the spot lights.
    ++index;
    nextLightIndex = perTileLightIndexBuffer[ index ];
    
    while (nextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL)
    {
        uint lightIndex = nextLightIndex;
        ++index;
        nextLightIndex = perTileLightIndexBuffer[ index ];
        
        const float4 params = spotLightParams[ lightIndex ];
        const float4 center = spotLightBufferCenterAndRadius[ lightIndex ];
        const float radius = center.w;
        
        float3 vecToLight = normalize( center.xyz - in.positionWS.xyz );
        float spotAngle = dot( -params.xyz, vecToLight );

        float cosineOfConeAngle = 0.5f;//(spotParams.z > 0.0f) ? spotParams.z : -spotParams.z;
        
        // Falloff
        float dist = distance( in.positionWS.xyz, center.xyz );
        float a = dist / radius + 1.0f;
        float att = 1.0f / (a * a);
        float3 color = spotLightBufferColors[ lightIndex ].rgb * att;// * specularTex;
        
        float3 accumDiffuseAndSpecular = spotAngle > cosineOfConeAngle ? color : float3( 0.0, 0.0, 0.0 );
        outColor.rgb += accumDiffuseAndSpecular;
    }
    
#ifdef DEBUG_LIGHT_COUNT
    const uint numLights = GetNumLightsInThisTile( tileIndex, uniforms.maxNumLightsPerTile, perTileLightIndexBuffer );

    if (numLights == 0)
    {
        outColor = float4( 1, 1, 1, 1 );
    }
    else if (numLights < 10)
    {
        outColor = float4( 0, 1, 0, 1 );
    }
    else if (numLights < 20)
    {
        outColor = float4( 1, 1, 0, 1 );
    }
    else
    {
        outColor = float4( 1, 0, 0, 1 );
    }
#endif
    
    return albedoColor * outColor;
}
