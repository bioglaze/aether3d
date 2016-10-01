#include <metal_stdlib>
#include <metal_atomic>
#include <simd/simd.h>

using namespace metal;

#define TILE_RES 16
#define NUM_THREADS_PER_TILE (TILE_RES * TILE_RES)
#define MAX_NUM_LIGHTS_PER_TILE 544
#define LIGHT_INDEX_BUFFER_SENTINEL 0x7fffffff

struct Uniforms
{
    uint windowWidth;
    uint windowHeight;
    uint numLights;
    int maxNumLightsPerTile;
    matrix_float4x4 invProjection;
    matrix_float4x4 viewMatrix;
};

//float4 PointLightCenterAndRadius;

static float4 ConvertProjToView( float4 p, matrix_float4x4 invProjection )
{
    p = invProjection * p;
    p /= p.w;
    // FIXME: Added the following line, not needed in D3D11 or ForwardPlus11 example. [2014-07-07]
    p.y = -p.y;
    return p;
}

// this creates the standard Hessian-normal-form plane equation from three points,
// except it is simplified for the case where the first point is the origin
static float4 CreatePlaneEquation( float4 b, float4 c )
{
    float4 n;

    // normalize(cross( b.xyz-a.xyz, c.xyz-a.xyz )), except we know "a" is the origin
    n.xyz = normalize( cross( b.xyz, c.xyz ) );

    // -(n dot a), except we know "a" is the origin
    n.w = 0;

    return n;
}

static uint GetNumTilesX( uint windowWidth )
{
    return uint(( ( windowWidth + TILE_RES - 1 ) / float(TILE_RES) ));
}

static uint GetNumTilesY( uint windowHeight )
{
    return uint(( ( windowHeight + TILE_RES - 1 ) / float(TILE_RES) ));
}

// point-plane distance, simplified for the case where
// the plane passes through the origin
static float GetSignedDistanceFromPlane( float4 p, float4 eqn )
{
    // dot( eqn.xyz, p.xyz ) + eqn.w, , except we know eqn.w is zero
    // (see CreatePlaneEquation above)
    return dot( eqn.xyz, p.xyz );
}

// https://developer.apple.com/library/content/documentation/Metal/Reference/MetalShadingLanguageGuide/func-var-qual/func-var-qual.html

kernel void light_culler(texture2d<float, access::read> depthNormalsTexture [[texture(0)]],
                         constant Uniforms& uniforms [[ buffer(0) ]],
                         constant float4* pointLightBufferCenterAndRadius [[ buffer(1) ]],
                         device int* perTileLightIndexBufferOut [[ buffer(2) ]],
                          uint2 gid [[thread_position_in_grid]],
                          uint2 tid [[thread_position_in_threadgroup]],
                          uint2 dtid [[threadgroup_position_in_grid]])
{
    threadgroup uint ldsLightIdx[ MAX_NUM_LIGHTS_PER_TILE ];
    threadgroup atomic_uint ldsZMax;
    threadgroup atomic_uint ldsZMin;
    threadgroup atomic_uint ldsLightIdxCounter;

    uint2 globalIdx = dtid;
    uint2 localIdx = tid;
    uint2 groupIdx = dtid;

    uint localIdxFlattened = localIdx.x + localIdx.y * TILE_RES;
    uint tileIdxFlattened = groupIdx.x + groupIdx.y * GetNumTilesX( uniforms.windowWidth );

    if (localIdxFlattened == 0)
    {
        atomic_store_explicit( &ldsZMin, 0xffffffff, memory_order_relaxed );
        atomic_store_explicit( &ldsZMax, 0, memory_order_relaxed );
        atomic_store_explicit( &ldsLightIdxCounter, 0, memory_order_relaxed );
    }

    float4 frustumEqn[ 4 ];
    {
        // construct frustum for this tile
        uint pxm = TILE_RES * groupIdx.x;
        uint pym = TILE_RES * groupIdx.y;
        uint pxp = TILE_RES * (groupIdx.x + 1);
        uint pyp = TILE_RES * (groupIdx.y + 1);

        uint uWindowWidthEvenlyDivisibleByTileRes  = TILE_RES * GetNumTilesX( uniforms.windowWidth );
        uint uWindowHeightEvenlyDivisibleByTileRes = TILE_RES * GetNumTilesY( uniforms.windowHeight );

        // four corners of the tile, clockwise from top-left
        float4 frustum[ 4 ];
        frustum[ 0 ] = ConvertProjToView( float4(pxm / float( uWindowWidthEvenlyDivisibleByTileRes ) * 2.0f - 1.0f, (uWindowHeightEvenlyDivisibleByTileRes - pym) / float( uWindowHeightEvenlyDivisibleByTileRes ) * 2.0f - 1.0f, 1.0f, 1.0f), uniforms.invProjection );
        frustum[ 1 ] = ConvertProjToView( float4(pxp / float( uWindowWidthEvenlyDivisibleByTileRes ) * 2.0f - 1.0f, (uWindowHeightEvenlyDivisibleByTileRes - pym) / float( uWindowHeightEvenlyDivisibleByTileRes ) * 2.0f - 1.0f, 1.0f, 1.0f), uniforms.invProjection );
        frustum[ 2 ] = ConvertProjToView( float4(pxp / float( uWindowWidthEvenlyDivisibleByTileRes ) * 2.0f - 1.0f, (uWindowHeightEvenlyDivisibleByTileRes - pyp) / float( uWindowHeightEvenlyDivisibleByTileRes ) * 2.0f - 1.0f, 1.0f, 1.0f), uniforms.invProjection );
        frustum[ 3 ] = ConvertProjToView( float4(pxm / float( uWindowWidthEvenlyDivisibleByTileRes ) * 2.0f - 1.0f, (uWindowHeightEvenlyDivisibleByTileRes - pyp) / float( uWindowHeightEvenlyDivisibleByTileRes ) * 2.0f - 1.0f, 1.0f, 1.0f), uniforms.invProjection );

        // create plane equations for the four sides of the frustum,
        // with the positive half-space outside the frustum (and remember,
        // view space is left handed, so use the left-hand rule to determine
        // cross product direction)
        for (uint i = 0; i < 4; i++)
        {
            frustumEqn[ i ] = CreatePlaneEquation( frustum[ i ], frustum[ (i + 1) & 3 ] );
        }
    }

    threadgroup_barrier( mem_flags::mem_threadgroup );

    // calculate the min and max depth for this tile,
    // to form the front and back of the frustum

    float minZ = FLT_MAX;
    float maxZ = 0.0f;

    float depth = -depthNormalsTexture.read( uint2( globalIdx.x, globalIdx.y ) ).x;
    uint z = as_type< uint >( depth );

    if (depth != 0.f)
    {
        /*uint i =*/ atomic_fetch_min_explicit( &ldsZMin, z, memory_order::memory_order_relaxed );
        /*uint j =*/ atomic_fetch_max_explicit( &ldsZMax, z, memory_order::memory_order_relaxed );
    }

    threadgroup_barrier( mem_flags::mem_threadgroup );

    // FIXME: swapped min and max to prevent false culling near depth discontinuities.
    //        AMD's ForwarPlus11 sample uses reverse depth test so maybe that's why this swap is needed. [2014-07-07]
    uint zMin = atomic_load_explicit( &ldsZMin, memory_order::memory_order_relaxed );
    uint zMax = atomic_load_explicit( &ldsZMax, memory_order::memory_order_relaxed );
    minZ = as_type< float >( zMax );
    maxZ = as_type< float >( zMin );

    // loop over the point lights and do a sphere vs. frustum intersection test
    uint numPointLights = uniforms.numLights & 0xFFFFu;

    for (uint i = 0; i < numPointLights; i += NUM_THREADS_PER_TILE)
    {
        uint il = localIdxFlattened + i;

        if (il < numPointLights)
        {
            float4 center = pointLightBufferCenterAndRadius[ int( il ) ];
            float r = center.w;
            center.xyz = (uniforms.viewMatrix * float4( center.xyz, 1.0f ) ).xyz;

            // test if sphere is intersecting or inside frustum
            if (-center.z + minZ < r && center.z - maxZ < r)
            {
                if ((GetSignedDistanceFromPlane( center, frustumEqn[ 0 ] ) < r) &&
                    (GetSignedDistanceFromPlane( center, frustumEqn[ 1 ] ) < r) &&
                    (GetSignedDistanceFromPlane( center, frustumEqn[ 2 ] ) < r) &&
                    (GetSignedDistanceFromPlane( center, frustumEqn[ 3 ] ) < r))
                {
                    // do a thread-safe increment of the list counter
                    // and put the index of this light into the list
                    uint dstIdx = atomic_fetch_add_explicit( &ldsLightIdxCounter, 1, memory_order::memory_order_relaxed );
                    ldsLightIdx[ dstIdx ] = il;
                }
            }
        }
    }

    threadgroup_barrier( mem_flags::mem_threadgroup );

    uint uNumPointLightsInThisTile = atomic_load_explicit( &ldsLightIdxCounter, memory_order::memory_order_relaxed );

    // Spot lights.
    uint numSpotLights = (uniforms.numLights & 0xFFFF0000u) >> 16;

    for (uint i = 0; i < numSpotLights; i += NUM_THREADS_PER_TILE)
    {
        uint il = localIdxFlattened + i;

        if (il < numSpotLights)
        {
            float4 center = float4( 0, 0, 0, 1 );//spotLightBufferCenterAndRadius[ il ];
            float r = center.w * 5.0f; // FIXME: Multiply was added, but more clever culling should be done instead.
            center.xyz = (uniforms.viewMatrix * float4( center.xyz, 1.0f )).xyz;

            // test if sphere is intersecting or inside frustum
            if (-center.z + minZ < r && center.z - maxZ < r)
            {
                if ((GetSignedDistanceFromPlane( center, frustumEqn[ 0 ] ) < r) &&
                    (GetSignedDistanceFromPlane( center, frustumEqn[ 1 ] ) < r) &&
                    (GetSignedDistanceFromPlane( center, frustumEqn[ 2 ] ) < r) &&
                    (GetSignedDistanceFromPlane( center, frustumEqn[ 3 ] ) < r))
                {
                    // do a thread-safe increment of the list counter
                    // and put the index of this light into the list
                    uint dstIdx = atomic_fetch_add_explicit( &ldsLightIdxCounter, 1, memory_order::memory_order_relaxed );
                    ldsLightIdx[ dstIdx ] = il;
                }
            }
        }
    }
    threadgroup_barrier( mem_flags::mem_threadgroup );

    {   // write back
        uint startOffset = uniforms.maxNumLightsPerTile * tileIdxFlattened;

        for (uint i = localIdxFlattened; i < uNumPointLightsInThisTile; i += NUM_THREADS_PER_TILE)
        {
            // per-tile list of light indices
            perTileLightIndexBufferOut[ startOffset + i ] = int( ldsLightIdx[ i ] );
        }

        uint jMax = atomic_load_explicit( &ldsLightIdxCounter, memory_order::memory_order_relaxed );
        for (uint j = (localIdxFlattened + uNumPointLightsInThisTile); j < jMax; j += NUM_THREADS_PER_TILE)
        {
            // per-tile list of light indices
            perTileLightIndexBufferOut[ startOffset + j + 1 ] = int( ldsLightIdx[ j ] );
        }

        if (localIdxFlattened == 0)
        {
            // mark the end of each per-tile list with a sentinel (point lights)
            perTileLightIndexBufferOut[ startOffset + uNumPointLightsInThisTile ] = int( LIGHT_INDEX_BUFFER_SENTINEL );

            // mark the end of each per-tile list with a sentinel (spot lights)
            uint offs = atomic_load_explicit( &ldsLightIdxCounter, memory_order::memory_order_relaxed );
            perTileLightIndexBufferOut[ startOffset + offs + 1 ] = int( LIGHT_INDEX_BUFFER_SENTINEL );
        }
    }
}
