#if !VULKAN
#define layout(a,b)  
#else
#define register(a) blank
#endif

#include "ubo.h"

#define USE_MINMAX_Z 0
#define TILE_RES 16
#define NUM_THREADS_X TILE_RES
#define NUM_THREADS_Y TILE_RES
#define NUM_THREADS_PER_TILE (NUM_THREADS_X * NUM_THREADS_Y)
#define MAX_NUM_LIGHTS_PER_TILE 544
#define LIGHT_INDEX_BUFFER_SENTINEL 0x7fffffff
#define FLT_MAX 3.402823466e+38F

groupshared uint ldsLightIdxCounter;
groupshared uint ldsLightIdx[ MAX_NUM_LIGHTS_PER_TILE ];
groupshared uint ldsZMax;
groupshared uint ldsZMin;

uint GetNumTilesX()
{
    return (uint)((windowWidth + TILE_RES - 1) / (float)TILE_RES);
}

uint GetNumTilesY()
{
    return (uint)((windowHeight + TILE_RES - 1) / (float)TILE_RES);
}

// convert a point from post-projection space into view space
float4 ConvertProjToView( float4 p )
{
    p = mul( clipToView, p );
    p /= p.w;
#if VULKAN
    p.y = -p.y;
#endif
    return p;
}

// this creates the standard Hessian-normal-form plane equation from three points, 
// except it is simplified for the case where the first point is the origin
float4 CreatePlaneEquation( float3 b, float3 c )
{
    float3 n;

    // normalize(cross( b.xyz-a.xyz, c.xyz-a.xyz )), except we know "a" is the origin
    n.xyz = normalize( cross( b.xyz, c.xyz ) );

    // -(n dot a), except we know "a" is the origin
    //n.w = 0;

    return float4( n, 0 );
}

// point-plane distance, simplified for the case where 
// the plane passes through the origin
float GetSignedDistanceFromPlane( float3 p, float3 eqn )
{
    // dot( eqn.xyz, p.xyz ) + eqn.w, , except we know eqn.w is zero 
    // (see CreatePlaneEquation above)
    return dot( eqn, p );
}

void CalculateMinMaxDepthInLds( uint3 globalThreadIdx, uint depthBufferSampleIdx )
{
    float viewPosZ = tex.Load( uint3( globalThreadIdx.x, globalThreadIdx.y, 0 ) ).x;
    
    uint z = asuint( viewPosZ );

    if (viewPosZ != 0.f)
    {
        uint r;
        InterlockedMax( ldsZMax, z, r );
        InterlockedMin( ldsZMin, z, r );
    }
}

[numthreads( TILE_RES, TILE_RES, 1 )]
void CSMain( uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID )
{
    uint localIdxFlattened = localIdx.x + localIdx.y * TILE_RES;
    uint tileIdxFlattened = groupIdx.x + groupIdx.y * GetNumTilesX();

    if (localIdxFlattened == 0)
    {
        ldsZMin = 0x7f7fffff; // FLT_MAX as uint
        ldsZMax = 0;
        ldsLightIdxCounter = 0;
    }

    float3 frustumEqn[ 4 ];
    {  
        // construct frustum for this tile
        uint pxm = TILE_RES * groupIdx.x;
        uint pym = TILE_RES * groupIdx.y;
        uint pxp = TILE_RES * (groupIdx.x + 1);
        uint pyp = TILE_RES * (groupIdx.y + 1);

        uint uWindowWidthEvenlyDivisibleByTileRes = TILE_RES * GetNumTilesX();
        uint uWindowHeightEvenlyDivisibleByTileRes = TILE_RES * GetNumTilesY();

        // four corners of the tile, clockwise from top-left
        float4 frustum[ 4 ];
        frustum[ 0 ] = ConvertProjToView( float4(pxm / (float)uWindowWidthEvenlyDivisibleByTileRes * 2.0f - 1.0f, (uWindowHeightEvenlyDivisibleByTileRes - pym) / (float)uWindowHeightEvenlyDivisibleByTileRes * 2.0f - 1.0f, 1.0f, 1.0f) );
        frustum[ 1 ] = ConvertProjToView( float4(pxp / (float)uWindowWidthEvenlyDivisibleByTileRes * 2.0f - 1.0f, (uWindowHeightEvenlyDivisibleByTileRes - pym) / (float)uWindowHeightEvenlyDivisibleByTileRes * 2.0f - 1.0f, 1.0f, 1.0f) );
        frustum[ 2 ] = ConvertProjToView( float4(pxp / (float)uWindowWidthEvenlyDivisibleByTileRes * 2.0f - 1.0f, (uWindowHeightEvenlyDivisibleByTileRes - pyp) / (float)uWindowHeightEvenlyDivisibleByTileRes * 2.0f - 1.0f, 1.0f, 1.0f) );
        frustum[ 3 ] = ConvertProjToView( float4(pxm / (float)uWindowWidthEvenlyDivisibleByTileRes * 2.0f - 1.0f, (uWindowHeightEvenlyDivisibleByTileRes - pyp) / (float)uWindowHeightEvenlyDivisibleByTileRes * 2.0f - 1.0f, 1.0f, 1.0f) );

        // create plane equations for the four sides of the frustum, 
        // with the positive half-space outside the frustum (and remember, 
        // view space is left handed, so use the left-hand rule to determine 
        // cross product direction)
        for (uint i = 0; i < 4; ++i)
        {
            frustumEqn[ i ] = CreatePlaneEquation( frustum[ i ].xyz, frustum[ (i + 1) & 3 ].xyz ).xyz;
        }
    }

    // Blocks execution of all threads in a group until all group shared accesses 
    // have been completed and all threads in the group have reached this call.
    GroupMemoryBarrierWithGroupSync();

#if USE_MINMAX_Z
    // calculate the min and max depth for this tile, 
    // to form the front and back of the frustum

    float minZ = FLT_MAX;
    float maxZ = 0.0f;

    CalculateMinMaxDepthInLds( globalIdx, 0 );

    GroupMemoryBarrierWithGroupSync();

    // FIXME: swapped min and max to prevent false culling near depth discontinuities.
    //        AMD's ForwarPlus11 sample uses reverse depth test so maybe that's why this swap is needed. [2014-07-07]
    minZ = asfloat( ldsZMax );
    maxZ = asfloat( ldsZMin );
#endif
    
    // loop over the lights and do a sphere vs. frustum intersection test
    uint uNumPointLights = numLights & 0xFFFFu;

    for (uint i = 0; i < uNumPointLights; i += NUM_THREADS_PER_TILE)
    {
        uint il = localIdxFlattened + i;
        if (il < uNumPointLights)
        {
            float4 center = pointLightBufferCenterAndRadius[ il ];
            float radius = center.w;
            center.xyz = mul( localToView, float4( center.xyz, 1 ) ).xyz;

            // test if sphere is intersecting or inside frustum
#if USE_MINMAX_Z
            if (-center.z + minZ < radius && center.z - maxZ < radius)
#else
            if (center.z < radius)
#endif
            {
                if ((GetSignedDistanceFromPlane( center.xyz, frustumEqn[ 0 ] ) < radius) &&
                    (GetSignedDistanceFromPlane( center.xyz, frustumEqn[ 1 ] ) < radius) &&
                    (GetSignedDistanceFromPlane( center.xyz, frustumEqn[ 2 ] ) < radius) &&
                    (GetSignedDistanceFromPlane( center.xyz, frustumEqn[ 3 ] ) < radius))
                {
                    // do a thread-safe increment of the list counter 
                    // and put the index of this light into the list
                    uint dstIdx = 0;
                    InterlockedAdd( ldsLightIdxCounter, 1, dstIdx );
                    ldsLightIdx[ dstIdx ] = il;
                }
            }
        }
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    // Spot lights.
    uint uNumPointLightsInThisTile = ldsLightIdxCounter;
    uint numSpotLights = (numLights & 0xFFFF0000u) >> 16;

    for (uint j = 0; j < numSpotLights; j += NUM_THREADS_PER_TILE)
    {
        uint jl = localIdxFlattened + j;

        if (jl < numSpotLights)
        {
            float4 center = spotLightBufferCenterAndRadius[ jl ];
            float radius = 20.0;
            //float radius = center.w;
            center.xyz = mul( localToView, float4( center.xyz, 1 ) ).xyz;

            // test if sphere is intersecting or inside frustum
#if USE_MINMAX_Z
            if (-center.z - minZ < radius && center.z - maxZ < radius)
#else
            // Nothing to do here
#endif
            {
                if ((GetSignedDistanceFromPlane( center.xyz, frustumEqn[ 0 ] ) < radius) &&
                    (GetSignedDistanceFromPlane( center.xyz, frustumEqn[ 1 ] ) < radius) &&
                    (GetSignedDistanceFromPlane( center.xyz, frustumEqn[ 2 ] ) < radius) &&
                    (GetSignedDistanceFromPlane( center.xyz, frustumEqn[ 3 ] ) < radius))
                {
                    // do a thread-safe increment of the list counter 
                    // and put the index of this light into the list
                    uint dstIdx = 0;
                    InterlockedAdd( ldsLightIdxCounter, 1, dstIdx );
                    ldsLightIdx[ dstIdx ] = jl;
                }
            }
        }
    }

    GroupMemoryBarrierWithGroupSync();

    {   // write back
        uint startOffset = maxNumLightsPerTile * tileIdxFlattened;

        for (uint i = localIdxFlattened; i < uNumPointLightsInThisTile; i += NUM_THREADS_PER_TILE)
        {
            perTileLightIndexBuffer[ startOffset + i ] = ldsLightIdx[ i ];
        }

        for (uint j = (localIdxFlattened + uNumPointLightsInThisTile); j < ldsLightIdxCounter; j += NUM_THREADS_PER_TILE)
        {
            perTileLightIndexBuffer[ startOffset + j + 1 ] = ldsLightIdx[ j ];
        }

        if (localIdxFlattened == 0)
        {
            // mark the end of each per-tile list with a sentinel (point lights)
            perTileLightIndexBuffer[ startOffset + uNumPointLightsInThisTile ] = LIGHT_INDEX_BUFFER_SENTINEL;

            // mark the end of each per-tile list with a sentinel (spot lights)
            perTileLightIndexBuffer[ startOffset + ldsLightIdxCounter + 1 ] = LIGHT_INDEX_BUFFER_SENTINEL;
        }
    }
}
