#include "ubo.h"

struct PS_INPUT
{
    float4 pos : SV_Position;
    float4 positionVS_u : TEXCOORD0;
    float4 positionWS_v : TEXCOORD1;
    float3 tangentVS : TANGENT;
    float3 bitangentVS : BINORMAL;
    float3 normalVS : NORMAL;
    float4 projCoord : TEXCOORD2;
};

#define TILE_RES 16
#define LIGHT_INDEX_BUFFER_SENTINEL 0x7fffffff
//#define DEBUG_LIGHT_COUNT

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
    return normalize( v.x * tangent + v.y * bitangent + v.z * normal );
}

float D_GGX( float dotNH, float a )
{
    float a2 = a * a;
    float f = (dotNH * a2 - dotNH) * dotNH + 1.0f;
    return a2 / (3.14159265f * f * f);
}

float3 F_Schlick( float dotVH, float3 f0 )
{
    return f0 + (float3( 1, 1, 1 ) - f0) * pow( 1.0f - dotVH, 5.0f );
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

float quadraticAttenuation( float d, float kc, float kl, float kq )
{
    return 1.0f / (kc + kl * d + kq * d * d );
}

float pointLightAttenuation( float d, float r )
{
    return 2.0f / ( d * d + r * r + d * sqrt( d * d + r * r ) );
}

float linstep( float low, float high, float v )
{
    return saturate( (v - low) / (high - low) );
}

float VSM( float depth, float4 projCoord )
{
    float2 uv = projCoord.xy / projCoord.w;

    // Spot light
    if (lightType == 1 && (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1))
    {
        return 0;
    }

    if (lightType == 1 && (depth < 0 || depth > 1))
    {
        return 0;
    }

    float2 moments = shadowTex.SampleLevel( sLinear, uv, 0 ).rg;

    float variance = max( moments.y - moments.x * moments.x, -0.001f );

    float delta = depth - moments.x;
    float p = smoothstep( depth - 0.02f, depth, moments.x );
    float pMax = linstep( minAmbient, 1.0f, variance / (variance + delta * delta) );

    return saturate( max( p, pMax ) );
}

float4 main( PS_INPUT input ) : SV_Target
{
    const float4 albedo = tex.Sample( sLinear, float2( input.positionVS_u.w, input.positionWS_v.w ) );
    const float4 normalTS = float4( normalTex.Sample( sLinear, float2(input.positionVS_u.w, input.positionWS_v.w) ).xyz * 2 - 1, 0 );

    const uint tileIndex = GetTileIndex( input.pos.xy );
    uint index = maxNumLightsPerTile * tileIndex;
    uint nextLightIndex = perTileLightIndexBuffer[ index ];

    const float3 normalVS = tangentSpaceTransform( input.tangentVS, input.bitangentVS, input.normalVS, normalTS.xyz );

    float3 accumDiffuseAndSpecular = lightColor.rgb;
    const float3 surfaceToLightVS = lightDirection.xyz;

    const float3 N = normalize( normalVS );
    const float3 V = normalize( input.positionVS_u.xyz );
    const float3 L = -lightDirection.xyz;
    const float3 H = normalize( L + V );

    const float dotNV = abs( dot( N, V ) ) + 1e-5f;
    const float dotNL = saturate( dot( N, -surfaceToLightVS ) );
    const float dotLH = saturate( dot( L, H ) );
    const float dotNH = saturate( dot( N, H ) );

    float3 f0v = float3(f0, f0, f0);
    float roughness = 0.5f;
    float a = roughness * roughness;
    float D = D_GGX( dotNH, a );
    float3 F = F_Schlick( dotLH, f0v );
    float v = V_SmithGGXCorrelated( dotNV, dotNL, a );
    float3 Fr = (D * v) * F;
    float3 Fd = Fd_Lambert();

    //accumDiffuseAndSpecular *= dotNL;

    const float3 dirColor = Fd + Fr;
    accumDiffuseAndSpecular.rgb *= dirColor * dotNL;

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

        const float3 L = normalize( vecToLightVS );
        const float3 H = normalize( L + V );

        const float dotNV = abs( dot( N, V ) ) + 1e-5f; 
        const float dotNL = saturate( dot( N, lightDirVS ) );
        const float dotVH = saturate( dot( V, H ) );
        const float dotLH = saturate( dot( L, H ) );
        const float dotNH = saturate( dot( N, H ) );
        
        const float lightDistance = length( vecToLightWS );

        float3 f0v = float3( f0, f0, f0 );
        float roughness = 0.5f;
        float a = roughness * roughness;
        float D = D_GGX( dotNH, a );
        float3 F = F_Schlick( dotLH, f0v );
        float v = V_SmithGGXCorrelated( dotNV, dotNL, a );
        float3 Fr = (D * v) * F;
        float3 Fd = Fd_Lambert();
        
        if (lightDistance < radius)
        {
            const float attenuation = pointLightAttenuation( length( vecToLightWS ), 1.0f / radius );
            const float3 color = Fd + Fr;
            accumDiffuseAndSpecular.rgb += (color * pointLightColors[ lightIndex ].rgb) * attenuation * dotNL;
        }
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
        float3 spotLightDir = spotParams.xyz;

        const float3 vecToLight = normalize( centerAndRadius.xyz - input.positionWS_v.xyz );
        const float spotAngle = dot( -spotLightDir.xyz, vecToLight );
        const float cosineOfConeAngle = abs( spotParams.w );

        const float3 vecToLightVS = (mul( localToView, float4( vecToLight, 1.0f ) ) ).xyz;
        const float3 lightDirVS = normalize( vecToLightVS );

        const float3 L = normalize( vecToLightVS );
        const float3 H = normalize( L + V );

        const float dotNV = abs( dot( N, V ) ) + 1e-5f; 
        const float dotNL = saturate( dot( N, lightDirVS ) );
        const float dotVH = saturate( dot( V, H ) );
        const float dotLH = saturate( dot( L, H ) );
        const float dotNH = saturate( dot( N, H ) );

        float3 f0v = float3( f0, f0, f0 );
        float roughness = 0.5f;
        float a = roughness * roughness;
        float D = D_GGX( dotNH, a );
        float3 F = F_Schlick( dotLH, f0v );
        float v = V_SmithGGXCorrelated( dotNV, dotNL, a );
        float3 Fr = (D * v) * F;
        float3 Fd = Fd_Lambert();

        // Falloff
        const float dist = distance( input.positionWS_v.xyz, centerAndRadius.xyz );
        const float fa = dist / centerAndRadius.w + 1.0f;
        const float attenuation = 1;//1.0f / (fa * fa);
        const float3 color = spotLightColors[ lightIndex ].rgb;
        const float3 shadedColor = Fd + Fr;
        accumDiffuseAndSpecular.rgb += spotAngle > cosineOfConeAngle ? ((shadedColor * color) * attenuation * dotNL) : float3( 0, 0, 0 );
    }
    
    accumDiffuseAndSpecular.rgb = max( float3( minAmbient, minAmbient, minAmbient ), accumDiffuseAndSpecular.rgb );
    //accumDiffuseAndSpecular.rgb += texCube.Sample( sLinear, N ).rgb;

#ifdef ENABLE_SHADOWS
    float depth = input.projCoord.z / input.projCoord.w;
    float shadow = max( minAmbient, VSM( depth, input.projCoord ) );
    accumDiffuseAndSpecular.rgb *= shadow;
#endif

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

    return float4(albedo.rgb * accumDiffuseAndSpecular, 1.0f);
}
