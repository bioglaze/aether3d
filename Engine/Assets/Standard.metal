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
    uint windowWidth;
    uint windowHeight;
    uint numLights;
    int maxNumLightsPerTile;
    matrix_float4x4 invProjection;
    matrix_float4x4 viewMatrix;
};

static uint GetNumLightsInThisTile( uint nTileIndex, uint maxNumLightsPerTile, constant int* perTileLightIndexBuffer )
{
    uint nNumLightsInThisTile = 0;
    uint nIndex = maxNumLightsPerTile * nTileIndex;
    uint nNextLightIndex = perTileLightIndexBuffer[ nIndex ];

    int iterations = 0;

    // count point lights
    while (nNextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL && iterations < 100)
    {
        nNumLightsInThisTile++;
        nIndex++;
        nNextLightIndex = perTileLightIndexBuffer[ nIndex ];

        ++iterations;
    }

    return nNumLightsInThisTile;
}

static uint GetTileIndex( float2 ScreenPos, int windowWidth )
{
    float fTileRes = float( TILE_RES );
    uint nNumCellsX = (windowWidth + TILE_RES - 1) / TILE_RES;
    uint nTileIdx = uint( floor( ScreenPos.x / fTileRes ) + floor( ScreenPos.y / fTileRes ) * nNumCellsX );
    return nTileIdx;
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
                               constant int* perTileLightIndexBuffer [[ buffer(6) ]],
                               //constant float* pointLightBufferCenterAndRadius [[ buffer(7) ]],
                               constant CullerUniforms* cullerUniforms  [[ buffer(8) ]],
                               sampler sampler0 [[sampler(0)]] )
{
    half4 sampledColor = half4( textureMap.sample( sampler0, in.texCoords ) );

    uint nTileIndex = GetTileIndex( in.position.xy, cullerUniforms->windowWidth );
    //uint nIndex = cullerUniforms->maxNumLightsPerTile * nTileIndex;
    //uint nNextLightIndex = perTileLightIndexBuffer[ nIndex ];

    // Loops over point lights.
    /*while (nNextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL)
    {
        uint nLightIndex = nNextLightIndex;
        nIndex++;
        nNextLightIndex = perTileLightIndexBuffer[ nIndex ];
    }*/

    uint numLights = GetNumLightsInThisTile( nTileIndex, cullerUniforms->maxNumLightsPerTile, perTileLightIndexBuffer );
    if (numLights == 0)
    {
        sampledColor = half4( 0, 0, 0, 1 );
    }
    else if (numLights == 0x7fffffff)
    {
        sampledColor = half4( 0, 0, 1, 1 );
    }
    else if (numLights == 1)
    {
        sampledColor = half4( 0, 1, 0, 1 );
    }
    else
    {
        sampledColor = half4( 1, 0, 0, 1 );
    }

    nTileIndex = numLights;
    sampledColor = half4( 1, 0, 0, 1 );
    if (nTileIndex % 1)
    {
        sampledColor = half4( 0, 1, 0, 1 );
    }
    if (nTileIndex % 2)
    {
        sampledColor = half4( 0, 0, 1, 1 );
    }
    if (nTileIndex % 3)
    {
        sampledColor = half4( 1, 1, 1, 1 );
    }
    return sampledColor;
}
