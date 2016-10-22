#include <metal_stdlib>
#include <simd/simd.h>

#define TILE_RES 16
#define LIGHT_INDEX_BUFFER_SENTINEL 0x7fffffff

using namespace metal;

struct StandardUniforms
{
    matrix_float4x4 _ModelViewProjectionMatrix;
    matrix_float4x4 _ShadowProjectionMatrix;
    float4 tintColor;
};

struct StandardColorInOut
{
    float4 position [[position]];
    float4 projCoord;
    float2 texCoords;
    half4  color;
};

struct StandardVertex
{
    float3 position [[attribute(0)]];
    float2 texcoord [[attribute(1)]];
    float3 normal [[attribute(2)]];
    float4 tangent [[attribute(3)]];
    float4 color [[attribute(4)]];
};

struct CullerUniforms
{
    matrix_float4x4 invProjection;
    matrix_float4x4 viewMatrix;
    uint windowWidth;
    uint windowHeight;
    uint numLights;
    int maxNumLightsPerTile;
};

static uint GetNumLightsInThisTile( uint tileIndex, uint maxNumLightsPerTile, const device uint* perTileLightIndexBuffer )
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

vertex StandardColorInOut standard_vertex(StandardVertex vert [[stage_in]],
                               constant StandardUniforms& uniforms [[ buffer(5) ]],
                               unsigned int vid [[ vertex_id ]])
{
    StandardColorInOut out;
    
    float4 in_position = float4( float3( vert.position ), 1.0 );
    out.position = uniforms._ModelViewProjectionMatrix * in_position;
    
    out.color = half4( vert.color );
    out.texCoords = float2( vert.texcoord );
    out.projCoord = uniforms._ShadowProjectionMatrix * in_position;
    return out;
}

fragment half4 standard_fragment( StandardColorInOut in [[stage_in]],
                               texture2d<float, access::sample> textureMap [[texture(0)]],
                               texture2d<float, access::sample> _ShadowMap [[texture(1)]],
                               const device uint* perTileLightIndexBuffer [[ buffer(6) ]],
                               //constant float* pointLightBufferCenterAndRadius [[ buffer(7) ]],
                               constant CullerUniforms& cullerUniforms  [[ buffer(8) ]],
                               sampler sampler0 [[sampler(0)]] )
{
    half4 sampledColor = half4( textureMap.sample( sampler0, in.texCoords ) );

    const uint tileIndex = GetTileIndex( in.position.xy, cullerUniforms.windowWidth );
    const uint numLights = GetNumLightsInThisTile( tileIndex, cullerUniforms.maxNumLightsPerTile, perTileLightIndexBuffer );

    if (numLights == 0)
    {
        sampledColor = half4( 0, 0, 0, 1 );
    }
    else if (numLights == 1)
    {
        sampledColor = half4( 0, 1, 0, 1 );
    }
    else if (numLights == 2)
    {
        sampledColor = half4( 1, 1, 0, 1 );
    }
    else
    {
        sampledColor = half4( 1, 0, 0, 1 );
    }

    return sampledColor;
}
