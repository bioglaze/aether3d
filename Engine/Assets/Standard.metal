#include <metal_stdlib>
#include <simd/simd.h>

#define TILE_RES 16
#define NUM_THREADS_PER_TILE (TILE_RES * TILE_RES)
#define LIGHT_INDEX_BUFFER_SENTINEL 0x7fffffff
//#define DEBUG_LIGHT_COUNT
//#define DEBUG_ALBEDO
//#define DEBUG_FAST

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

struct StandardVertexSkin
{
    float3 position [[attribute(0)]];
    float2 texcoord [[attribute(1)]];
    float3 normal [[attribute(3)]];
    float4 tangent [[attribute(4)]];
    float4 color [[attribute(2)]];
    int4 boneIndex [[attribute(5)]];
    float4 boneWeights [[attribute(6)]];
};

#ifdef DEBUG_LIGHT_COUNT
static int GetNumLightsInThisTile( uint tileIndex, uint maxNumLightsPerTile, const device uint* perTileLightIndexBuffer )
{
    int numLightsInThisTile = 0;
    int index = maxNumLightsPerTile * tileIndex;
    int nextLightIndex = perTileLightIndexBuffer[ index ];

    // count point lights
    while (nextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL)
    {
        ++numLightsInThisTile;
        ++index;
        nextLightIndex = perTileLightIndexBuffer[ index ];
    }

    // count spot lights
    
    // Moves past sentinel
    ++index;
    nextLightIndex = perTileLightIndexBuffer[ index ];
    
    while (nextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL)
    {
        ++numLightsInThisTile;
        ++index;
        nextLightIndex = perTileLightIndexBuffer[ index ];
    }

    return numLightsInThisTile;
}
#endif

static int GetTileIndex( float2 ScreenPos, int windowWidth )
{
    float tileRes = 1.0f / float( TILE_RES );
    int numCellsX = (windowWidth + TILE_RES - 1) / TILE_RES;
    int tileIdx = int( floor( ScreenPos.x * tileRes ) + floor( ScreenPos.y * tileRes ) * numCellsX );
    return tileIdx;
}

float3 tangentSpaceTransform( float3 tangent, float3 bitangent, float3 normal, float3 v )
{
    return normalize( v.x * tangent + v.y * bitangent + v.z * normal );
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
    // FIXME: This is not what MikkTSpace does, it should be cross( normal, tangent )! But the renderer is not yet
    //        fully MikkTSpace compatible, and this is needed to get light direction working correctly.
    float3 ct = cross( vert.tangent.xyz, vert.normal ) * vert.tangent.w;
    out.bitangentVS_v.xyz = ( uniforms.localToView * float4( ct, 0 ) ).xyz;
    out.bitangentVS_v.w = vert.texcoord.y;
    out.normalVS = (uniforms.localToView * float4( vert.normal, 0 )).xyz;
    
    return out;
}

vertex StandardColorInOut standard_skin_vertex( StandardVertexSkin vert [[stage_in]],
                               constant Uniforms& uniforms [[ buffer(5) ]],
                               unsigned int vid [[ vertex_id ]] )
{
    StandardColorInOut out;

    matrix_float4x4 boneTransform = uniforms.boneMatrices[ vert.boneIndex.x ] * vert.boneWeights.x +
                                    uniforms.boneMatrices[ vert.boneIndex.y ] * vert.boneWeights.y +
                                    uniforms.boneMatrices[ vert.boneIndex.z ] * vert.boneWeights.z +
                                    uniforms.boneMatrices[ vert.boneIndex.w ] * vert.boneWeights.w;
    
    float4 skinnedPposition = boneTransform * float4( vert.position, 1.0f );
    float4 skinnedNormal = boneTransform * float4( vert.normal, 0.0f );
    float4 skinnedTangent = boneTransform * float4( vert.tangent.xyz, 0.0f );
    
    out.position = uniforms.localToClip * skinnedPposition;
    out.positionVS = (uniforms.localToView * skinnedPposition).xyz;
    out.positionWS = (uniforms.localToWorld * skinnedPposition).xyz;
    
    out.color = half4( vert.color );
    out.projCoord = uniforms.localToShadowClip * skinnedPposition;
    
    out.tangentVS_u.xyz = (uniforms.localToView * float4( skinnedTangent.xyz, 0 )).xyz;
    out.tangentVS_u.w = vert.texcoord.x;
    float3 ct = cross( skinnedNormal.xyz, skinnedTangent.xyz ) * vert.tangent.w;
    out.bitangentVS_v.xyz = normalize( uniforms.localToView * float4( ct, 0 ) ).xyz;
    out.bitangentVS_v.w = vert.texcoord.y;
    out.normalVS = (uniforms.localToView * skinnedNormal).xyz;
    
    return out;
}

float D_GGX( float dotNH, float a )
{
    float a2 = a * a;
    float f = (dotNH * a2 - dotNH) * dotNH + 1.0f;
    return a2 / (3.14159265f * f * f);
}

float3 F_Schlick( float dotVH, float3 f0 )
{
    return f0 + (float3( 1.0f ) - f0) * pow( 1.0f - dotVH, 5.0f );
}

float V_SmithGGXCorrelated( float dotNV, float dotNL, float a )
{
    float a2 = a * a;
    float GGXL = dotNV * sqrt( (-dotNL * a2 + dotNL) * dotNL + a2 );
    float GGXV = dotNL * sqrt( (-dotNV * a2 + dotNV) * dotNV + a2 );
    return 0.5f / (GGXV + GGXL);
}

float Fd_Lambert()
{
    return 1.0f / 3.14159265f;
}

float getSquareFalloffAttenuation( float3 posToLight, float lightInvRadius )
{
    float distanceSquare = dot( posToLight, posToLight );
    float factor = distanceSquare * lightInvRadius * lightInvRadius;
    float smoothFactor = max( 1.0f - factor * factor, 0.0f );
    return (smoothFactor * smoothFactor) / max( distanceSquare, 1e-4 );
}

float pointLightAttenuation( float d, float r )
{
    return 2.0f / ( d * d + r * r + d * sqrt( d * d + r * r ) );
}

fragment half4 standard_shadow_point_fragment( StandardColorInOut in [[stage_in]],
                               texture2d<float, access::sample> albedoSmoothnessMap [[texture(0)]],
                               texture2d<float, access::sample> normalMap [[texture(1)]],
                               texturecube<float, access::sample> specularMap [[texture(2)]],
                               texture2d<float, access::sample> _ShadowMap [[texture(3)]],
                               texturecube<float, access::sample> cubeMap [[texture(4)]],
                               constant Uniforms& uniforms [[ buffer(5) ]],
                               const device uint* perTileLightIndexBuffer [[ buffer(6) ]],
                               const device float4* pointLightBufferCenterAndRadius [[ buffer(7) ]],
                               const device float4* spotLightBufferCenterAndRadius [[ buffer(8) ]],
                               const device float4* pointLightBufferColors [[ buffer(9) ]],
                               const device float4* spotLightParams [[ buffer(10) ]],
                               const device float4* spotLightBufferColors [[ buffer(11) ]],
                               sampler sampler0 [[sampler(0)]] )
{
    const float2 uv = float2( in.tangentVS_u.w, in.bitangentVS_v.w );
    const half4 albedoColor = half4( albedoSmoothnessMap.sample( sampler0, uv ) );
    
    float depth = in.projCoord.z / in.projCoord.w;

    float shadow = uniforms.lightType == 0 ? 1.0f : max( uniforms.minAmbient, VSMPoint( specularMap, in.projCoord, depth, uniforms.lightType ) );

    return albedoColor * half4( shadow, shadow, shadow, 1 );// * cubeReflection;
}

fragment half4 standard_shadow_fragment( StandardColorInOut in [[stage_in]],
                               texture2d<float, access::sample> albedoSmoothnessMap [[texture(0)]],
                               texture2d<float, access::sample> normalMap [[texture(1)]],
                               texture2d<float, access::sample> specularMap [[texture(2)]],
                               texture2d<float, access::sample> _ShadowMap [[texture(3)]],
                               texturecube<float, access::sample> cubeMap [[texture(4)]],
                               constant Uniforms& uniforms [[ buffer(5) ]],
                               const device uint* perTileLightIndexBuffer [[ buffer(6) ]],
                               const device float4* pointLightBufferCenterAndRadius [[ buffer(7) ]],
                               const device float4* spotLightBufferCenterAndRadius [[ buffer(8) ]],
                               const device float4* pointLightBufferColors [[ buffer(9) ]],
                               const device float4* spotLightParams [[ buffer(10) ]],
                               const device float4* spotLightBufferColors [[ buffer(11) ]],
                               sampler sampler0 [[sampler(0)]] )
{
    const float2 uv = float2( in.tangentVS_u.w, in.bitangentVS_v.w );
    const half4 albedoColor = half4( albedoSmoothnessMap.sample( sampler0, uv ) );
    
    float depth = in.projCoord.z / in.projCoord.w;

    if (uniforms.lightType == 2)
    {
        depth = depth * 0.5f + 0.5f;
    }

    float shadow = uniforms.lightType == 0 ? 1.0f : max( uniforms.minAmbient, VSM( _ShadowMap, in.projCoord, depth, uniforms.lightType ) );

    return albedoColor * half4( shadow, shadow, shadow, 1 );// * cubeReflection;
}

//[[early_fragment_tests]]
fragment half4 standard_fragment( StandardColorInOut in [[stage_in]],
                               texture2d<float, access::sample> albedoMap [[texture(0)]],
                               texture2d<float, access::sample> normalMap [[texture(1)]],
                               texture2d<float, access::sample> specularMap [[texture(2)]],
                               texture2d<float, access::sample> _ShadowMap [[texture(3)]],
                               texturecube<float, access::sample> cubeMap [[texture(4)]],
                               constant Uniforms& uniforms [[ buffer(5) ]],
                               const device uint* perTileLightIndexBuffer [[ buffer(6) ]],
                               const device float4* pointLightBufferCenterAndRadius [[ buffer(7) ]],
                               const device float4* spotLightBufferCenterAndRadius [[ buffer(8) ]],
                               const device float4* pointLightBufferColors [[ buffer(9) ]],
                               const device float4* spotLightParams [[ buffer(10) ]],
                               const device float4* spotLightBufferColors [[ buffer(11) ]],
                               sampler sampler0 [[sampler(0)]] )
{
#ifdef DEBUG_FAST
    const float2 uv = float2( in.tangentVS_u.w, in.bitangentVS_v.w );
    const half4 albedoColor = half4( albedoMap.sample( sampler0, uv ) );
    return albedoColor;
#else
    const float2 uv = float2( in.tangentVS_u.w, in.bitangentVS_v.w );
    const half4 albedoColor = half4( albedoMap.sample( sampler0, uv ) );
    const float4 normalTS = float4( normalMap.sample( sampler0, uv ).rgb * 2.0f - 1.0f, 0.0f );
    //const float4 specular = float4( specularMap.sample( sampler0, uv ) );
    
    const float3 normalVS = tangentSpaceTransform( in.tangentVS_u.xyz, in.bitangentVS_v.xyz, in.normalVS, normalTS.xyz );
    const float3 surfaceToDirectionalLightVS = uniforms.lightDirection.xyz;

    const float3 N = normalize( normalVS );
    const float3 V = normalize( in.positionVS.xyz );
    const float3 L = normalize( -surfaceToDirectionalLightVS );
    const float3 H = normalize( L + V );

    const half4 cubeReflection = half4( cubeMap.sample( sampler0, N.xyz ) );
    
    const float dotNV = abs( dot( N, V ) ) + 1e-5f;
    const float dotNL = saturate( dot( N, -surfaceToDirectionalLightVS ) );
    const float dotLH = saturate( dot( L, H ) );
    const float dotNH = saturate( dot( N, H ) );

    const float3 f0 = float3( uniforms.f0, uniforms.f0, uniforms.f0 );
    
    const float roughness = 0.5f;
    const float a = roughness * roughness;
    const float D = D_GGX( dotNH, a );
    const float3 F = F_Schlick( dotLH, f0 );
    const float v = V_SmithGGXCorrelated( dotNV, dotNL, a );
    const float3 Fr = (D * v) * F;
    const float3 Fd = Fd_Lambert();

    const int tileIndex = GetTileIndex( in.position.xy, uniforms.windowWidth );
    int index = uniforms.maxNumLightsPerTile * tileIndex;
    int nextLightIndex = perTileLightIndexBuffer[ index ];

    float3 color = Fd + Fr;
    //float4 outColor = float4( color + uniforms.lightColor.rgb * dotNL, 1 );

    float4 outColor = float4( (uniforms.lightColor.rgb * color) * dotNL, 1 );
    
    
    while (nextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL)
    {
        const int lightIndex = nextLightIndex;
        index++;
        nextLightIndex = perTileLightIndexBuffer[ index ];

        const float4 center = pointLightBufferCenterAndRadius[ lightIndex ];
        const float radius = center.w;
        const float3 vecToLightWS = center.xyz - in.positionWS.xyz;
        const float lightDistance = length( vecToLightWS );
        
        if (lightDistance < radius)
        {
            const float3 vecToLightVS = (uniforms.localToView * float4( vecToLightWS, 0 )).xyz;
            const float3 L = normalize( -vecToLightVS );
            const float3 H = normalize( L + V );
            
            const float dotNL = saturate( dot( N, -L ) );
            const float dotLH = saturate( dot( L, H ) );
            const float dotNH = saturate( dot( N, H ) );
            
            const float3 f0 = float3( uniforms.f0 );
            
            const float roughness = 0.5f;
            const float a = roughness * roughness;
            const float D = D_GGX( dotNH, a );
            const float3 F = F_Schlick( dotLH, f0 );
            const float v = V_SmithGGXCorrelated( dotNV, dotNL, a );
            const float3 Fr = (D * v) * F;
            const float3 Fd = Fd_Lambert();

            const float attenuation = pointLightAttenuation( length( vecToLightWS ), 1.0f / radius );
            const float3 color = Fd + Fr;
            outColor.rgb += (color * pointLightBufferColors[ lightIndex ].rgb) * attenuation * dotNL;
        }
    }
    
    // Moves past the first sentinel to get to the spot lights.
    ++index;
    nextLightIndex = perTileLightIndexBuffer[ index ];
    
    while (nextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL)
    {
        const int lightIndex = nextLightIndex;
        ++index;
        nextLightIndex = perTileLightIndexBuffer[ index ];
        
        const float4 params = spotLightParams[ lightIndex ];
        const float4 center = spotLightBufferCenterAndRadius[ lightIndex ];
        
        const float3 vecToLight = normalize( center.xyz - in.positionWS.xyz );
        const float spotAngle = dot( -params.xyz, vecToLight );
        const float cosineOfConeAngle = abs( params.w );
        
        const float3 vecToLightVS = (uniforms.localToView * float4( vecToLight, 0 )).xyz;
        const float3 L = normalize( -vecToLightVS );
        const float3 H = normalize( L + V );
        
        const float dotNL = saturate( dot( N, -L ) );
        const float dotLH = saturate( dot( L, H ) );
        const float dotNH = saturate( dot( N, H ) );

        const float roughness = 0.5f;
        const float a = roughness * roughness;
        const float D = D_GGX( dotNH, a );
        const float3 F = F_Schlick( dotLH, f0 );
        const float v = V_SmithGGXCorrelated( dotNV, dotNL, a );
        const float3 Fr = (D * v) * F;
        const float3 Fd = Fd_Lambert();

        float cutOff = cosineOfConeAngle;
        float outerCutOff = cutOff - 0.1f;

        float theta     = spotAngle;
        float epsilon   = cutOff - outerCutOff;
        float attenuation = saturate( (theta - outerCutOff) / epsilon);

        const float3 shadedColor = Fd + Fr;
        outColor.rgb += (shadedColor * spotLightBufferColors[ lightIndex ].rgb) * (dotNL * attenuation);
    }
    
	outColor.rgb = max( outColor.rgb, float3( uniforms.minAmbient, uniforms.minAmbient, uniforms.minAmbient ) );
    //outColor.rgb += (float3)cubeReflection.rgb;
    
#ifdef DEBUG_LIGHT_COUNT
    const int numLights = GetNumLightsInThisTile( tileIndex, uniforms.maxNumLightsPerTile, perTileLightIndexBuffer );

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

#ifdef DEBUG_ALBEDO
    return albedoColor;
#else
    return albedoColor * half4( outColor );// * cubeReflection;
#endif
    //return cubeReflection;
#endif // DEBUG_FAST
}
