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

// was shared
//shared uint ldsLightIdx[ MAX_NUM_LIGHTS_PER_TILE ];
//shared float ldsZMax;
//shared float ldsZMin;

float4 ConvertProjToView( float4 p, matrix_float4x4 invProjection )
{
    p = invProjection * p;
    p /= p.w;
    // FIXME: Added the following line, not needed in D3D11 or ForwardPlus11 example. [2014-07-07]
    p.y = -p.y;
    return p;
}

// this creates the standard Hessian-normal-form plane equation from three points,
// except it is simplified for the case where the first point is the origin
float4 CreatePlaneEquation( float4 b, float4 c )
{
    float4 n;

    // normalize(cross( b.xyz-a.xyz, c.xyz-a.xyz )), except we know "a" is the origin
    n.xyz = normalize( cross( b.xyz, c.xyz ) );

    // -(n dot a), except we know "a" is the origin
    n.w = 0;

    return n;
}

uint GetNumTilesX( uint windowWidth )
{
    return uint(( ( windowWidth + TILE_RES - 1 ) / float(TILE_RES) ));
}

uint GetNumTilesY( uint windowHeight )
{
    return uint(( ( windowHeight + TILE_RES - 1 ) / float(TILE_RES) ));
}

// point-plane distance, simplified for the case where
// the plane passes through the origin
float GetSignedDistanceFromPlane( float4 p, float4 eqn )
{
    // dot( eqn.xyz, p.xyz ) + eqn.w, , except we know eqn.w is zero
    // (see CreatePlaneEquation above)
    return dot( eqn.xyz, p.xyz );
}

void CalculateMinMaxDepthInLds( uint2 globalThreadIdx, texture2d<float, access::read> depthNormalsTexture )
{
    float depth = -depthNormalsTexture.read( uint2( globalThreadIdx.x, globalThreadIdx.y ) ).x;

    if (depth != 0.f)
    {
        atomicMax( ldsZMax, depth );
        atomicMin( ldsZMin, depth );
    }
}

// https://developer.apple.com/library/content/documentation/Metal/Reference/MetalShadingLanguageGuide/func-var-qual/func-var-qual.html

kernel void light_culler(texture2d<float, access::read> depthNormalsTexture [[texture(0)]],
                         texture2d<float, access::read> pointLightBufferCenterAndRadius [[texture(1)]],
                         texture2d<uint, access::write> perTileLightIndexBufferOut [[texture(2)]],
                         constant Uniforms& uniforms [[ buffer(0) ]],
                          uint2 gid [[thread_position_in_grid]],
                          uint2 tid [[thread_position_in_threadgroup]],
                          uint2 dtid [[threadgroup_position_in_grid]],
                          uint ldsLightIdxCounter [[ threadgroup(0) ]])
{
    uint2 globalIdx = tid;
    uint2 localIdx = dtid;
    uint2 groupIdx = gid;

    uint localIdxFlattened = localIdx.x + localIdx.y * TILE_RES;
    uint tileIdxFlattened = groupIdx.x + groupIdx.y * GetNumTilesX( uniforms.windowWidth );

    if (localIdxFlattened == 0)
    {
        ldsZMin = 0xffffffff;
        ldsZMax = 0;
        ldsLightIdxCounter = 0;
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

    CalculateMinMaxDepthInLds( globalIdx, depthNormalsTexture );

    threadgroup_barrier( mem_flags::mem_threadgroup );

    // FIXME: swapped min and max to prevent false culling near depth discontinuities.
    //        AMD's ForwarPlus11 sample uses reverse depth test so maybe that's why this swap is needed. [2014-07-07]
    minZ = ldsZMax;
    maxZ = ldsZMin;

    // loop over the point lights and do a sphere vs. frustum intersection test
    uint numPointLights = uniforms.numLights & 0xFFFFu;

    for (uint i = 0; i < numPointLights; i += NUM_THREADS_PER_TILE)
    {
        uint il = localIdxFlattened + i;

        if (il < numPointLights)
        {
            //float4 center = imageLoad( pointLightBufferCenterAndRadius, int( il ) );
            float4 center = pointLightBufferCenterAndRadius.read( int( il ) );
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
                    //uint dstIdx = atomicAdd( ldsLightIdxCounter, 1 );
                    uint dstIdx = atomic_fetch_add_explicit( ldsLightIdxCounter, 1, memory_order::memory_order_relaxed );
                    ldsLightIdx[ dstIdx ] = il;
                }
            }
        }
    }

    threadgroup_barrier( mem_flags::mem_threadgroup );

    uint uNumPointLightsInThisTile = ldsLightIdxCounter;
#if 0
    // Spot lights.
    uint numSpotLights = (numLights & 0xFFFF0000u) >> 16;

    for (uint i = 0; i < numSpotLights; i += NUM_THREADS_PER_TILE)
    {
        uint il = localIdxFlattened + i;

        if (il < numSpotLights)
        {
            vec4 center = imageLoad( spotLightBufferCenterAndRadius, int( il ) );
            float r = center.w * 5.0f; // FIXME: Multiply was added, but more clever culling should be done instead.
            center.xyz = (viewMatrix * vec4( center.xyz, 1.0f )).xyz;

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
                    uint dstIdx = atomicAdd( ldsLightIdxCounter, 1 );
                    ldsLightIdx[ dstIdx ] = il;
                }
            }
        }
    }
#endif
    threadgroup_barrier( mem_flags::mem_threadgroup );

    {   // write back
        uint startOffset = uniforms.maxNumLightsPerTile * tileIdxFlattened;

        for (uint i = localIdxFlattened; i < uNumPointLightsInThisTile; i += NUM_THREADS_PER_TILE)
        {
            // per-tile list of light indices
            //imageStore( perTileLightIndexBufferOut, int( startOffset + i ), uint4( ldsLightIdx[ i ] ) );
            perTileLightIndexBufferOut.write( uint4( ldsLightIdx[ i ] ), int( startOffset + i ) );
        }

        for (uint j = (localIdxFlattened + uNumPointLightsInThisTile); j < ldsLightIdxCounter; j += NUM_THREADS_PER_TILE)
        {
            // per-tile list of light indices
            //imageStore( perTileLightIndexBufferOut, int( startOffset + j + 1), uvec4( ldsLightIdx[ j ] ) );
            perTileLightIndexBufferOut.write( uint4( ldsLightIdx[ j ] ), int( startOffset + j + 1 ) );
        }

        if (localIdxFlattened == 0)
        {
            // mark the end of each per-tile list with a sentinel (point lights)
            //imageStore( perTileLightIndexBufferOut, int( startOffset + uNumPointLightsInThisTile ), uvec4( LIGHT_INDEX_BUFFER_SENTINEL ) );
            perTileLightIndexBufferOut.write( uint4( LIGHT_INDEX_BUFFER_SENTINEL ), int( startOffset + uNumPointLightsInThisTile ) );

            // mark the end of each per-tile list with a sentinel (spot lights)
            //g_PerTileLightIndexBufferOut[ startOffset + ldsLightIdxCounter + 1 ] = LIGHT_INDEX_BUFFER_SENTINEL;
            //imageStore( perTileLightIndexBufferOut, int( startOffset + ldsLightIdxCounter + 1 ), uvec4( LIGHT_INDEX_BUFFER_SENTINEL ) );
            perTileLightIndexBufferOut.write( uint4( LIGHT_INDEX_BUFFER_SENTINEL ), int( startOffset + ldsLightIdxCounter + 1 ) );
        }
    }
}
