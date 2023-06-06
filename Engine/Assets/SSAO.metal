#include <metal_stdlib>
#include <metal_atomic>
#include <simd/simd.h>

using namespace metal;
#include "MetalCommon.h"

float ssaoInternal( float3x3 kernelBasis, float3 originPos, float radius, constant Uniforms& uniforms,
                    texture2d<float, access::read> depthNormalsTexture )
{
    float occlusion = 0.0f;

    for (int i = 0; i < uniforms.kernelSize; ++i)
    {
        float3 samplePos = kernelBasis * uniforms.kernelOffsets[ i ].xyz;
        samplePos = samplePos * radius + originPos;

        // project sample position:
        float4 offset = uniforms.viewToClip * float4( samplePos, 1.0f );
        offset.xy /= offset.w; // only need xy
        offset.xy = offset.xy * 0.5f + 0.5f; // scale/bias to texcoords

        int depthWidth = uniforms.windowWidth, depthHeight = uniforms.windowHeight;
        float sampleDepth = -depthNormalsTexture.read( uint2( offset.xy * float2( depthWidth, depthHeight ) ), 0 ).r;

        const float diff = abs( originPos.z - sampleDepth );
        if (diff < 0.0001f)
        {
            occlusion += (sampleDepth < (samplePos.z - 0.025f) ? 1.0f : 0.0f);
            continue;
        }
        
        float rangeCheck = smoothstep( 0.0f, 1.0f, radius / diff );
        occlusion += (sampleDepth < (samplePos.z - 0.025f) ? 1.0f : 0.0f) * rangeCheck;
    }

    occlusion = 1.0f - (occlusion / float( uniforms.kernelSize ));
    float uPower = 1.8f;
    return pow( abs( occlusion ), uPower );
}

// Normally argument "dipatchGridDim" is parsed through a constant buffer. However, if for some reason it is a
// static value, some DXC compiler versions will be unable to compile the code.
// If that's the case for you, flip DXC_STATIC_DISPATCH_GRID_DIM definition from 0 to 1.
#define DXC_STATIC_DISPATCH_GRID_DIM 0

// On my MacBook Pro 2013 (GeForce GT 750M) this swizzling reduces the SSAO GPU time from 43.6 ms to 42.4 ms when the kernel sample count is 32.
// https://developer.nvidia.com/blog/optimizing-compute-shaders-for-l2-locality-using-thread-group-id-swizzling/
// Divide the 2D-Dispatch_Grid into tiles of dimension [N, DipatchGridDim.y]
// “CTA” (Cooperative Thread Array) == Thread Group in DirectX terminology
uint2 ThreadGroupTilingX(
    const uint2 dipatchGridDim,        // Arguments of the Dispatch call (typically from a ConstantBuffer)
    const uint2 ctaDim,            // Already known in HLSL, eg:[numthreads(8, 8, 1)] -> uint2(8, 8)
    const uint maxTileWidth,        // User parameter (N). Recommended values: 8, 16 or 32.
    const uint2 groupThreadID,        // SV_GroupThreadID
    const uint2 groupId            // SV_GroupID
)
{
    // A perfect tile is one with dimensions = [maxTileWidth, dipatchGridDim.y]
    const uint Number_of_CTAs_in_a_perfect_tile = maxTileWidth * dipatchGridDim.y;

    // Possible number of perfect tiles
    const uint Number_of_perfect_tiles = dipatchGridDim.x / maxTileWidth;

    // Total number of CTAs present in the perfect tiles
    const uint Total_CTAs_in_all_perfect_tiles = Number_of_perfect_tiles * maxTileWidth * dipatchGridDim.y;
    const uint vThreadGroupIDFlattened = dipatchGridDim.x * groupId.y + groupId.x;

    // Tile_ID_of_current_CTA : current CTA to TILE-ID mapping.
    const uint Tile_ID_of_current_CTA = vThreadGroupIDFlattened / Number_of_CTAs_in_a_perfect_tile;
    const uint Local_CTA_ID_within_current_tile = vThreadGroupIDFlattened % Number_of_CTAs_in_a_perfect_tile;
    uint Local_CTA_ID_y_within_current_tile;
    uint Local_CTA_ID_x_within_current_tile;

    if (Total_CTAs_in_all_perfect_tiles <= vThreadGroupIDFlattened)
    {
        // Path taken only if the last tile has imperfect dimensions and CTAs from the last tile are launched.
        uint X_dimension_of_last_tile = dipatchGridDim.x % maxTileWidth;
    #ifdef DXC_STATIC_DISPATCH_GRID_DIM
        X_dimension_of_last_tile = max(1U, X_dimension_of_last_tile);
    #endif
        Local_CTA_ID_y_within_current_tile = Local_CTA_ID_within_current_tile / X_dimension_of_last_tile;
        Local_CTA_ID_x_within_current_tile = Local_CTA_ID_within_current_tile % X_dimension_of_last_tile;
    }
    else
    {
        Local_CTA_ID_y_within_current_tile = Local_CTA_ID_within_current_tile / maxTileWidth;
        Local_CTA_ID_x_within_current_tile = Local_CTA_ID_within_current_tile % maxTileWidth;
    }

    const uint Swizzled_vThreadGroupIDFlattened =
        Tile_ID_of_current_CTA * maxTileWidth +
        Local_CTA_ID_y_within_current_tile * dipatchGridDim.x +
        Local_CTA_ID_x_within_current_tile;

    uint2 SwizzledvThreadGroupID;
    SwizzledvThreadGroupID.y = Swizzled_vThreadGroupIDFlattened / dipatchGridDim.x;
    SwizzledvThreadGroupID.x = Swizzled_vThreadGroupIDFlattened % dipatchGridDim.x;

    uint2 SwizzledvThreadID;
    SwizzledvThreadID.x = ctaDim.x * SwizzledvThreadGroupID.x + groupThreadID.x;
    SwizzledvThreadID.y = ctaDim.y * SwizzledvThreadGroupID.y + groupThreadID.y;

    return SwizzledvThreadID.xy;
}

kernel void compose(texture2d<float, access::read> inputTexture [[texture(0)]],
                    texture2d<float, access::write> resultTexture [[texture(1)]],
                    texture2d<float, access::read> ssaoTexture [[texture(2)]],
                  constant Uniforms& uniforms [[ buffer(0) ]],
                  ushort2 gid [[thread_position_in_grid]],
                  ushort2 tid [[thread_position_in_threadgroup]],
                  ushort2 dtid [[threadgroup_position_in_grid]])
{
    float4 color = inputTexture.read( uint2( gid.x, gid.y ) ) * ssaoTexture.read( uint2( gid.x, gid.y ) );

    resultTexture.write( color, gid.xy );
}

kernel void ssao(
                  texture2d<float, access::read> depthNormalsTexture [[texture(1)]],
                  texture2d<float, access::write> outTexture [[texture(2)]],
                  texture2d<float, access::read> noiseTexture [[texture(3)]],
                  constant Uniforms& uniforms [[ buffer(0) ]],
                  ushort2 globalIdx [[thread_position_in_grid]], // SV_DispatchThreadID
                  ushort2 tid [[thread_position_in_threadgroup]], // SV_GroupThreadID
                  ushort2 dtid [[threadgroup_position_in_grid]]) // SV_GroupID
{
    ushort2 gid = (ushort2)ThreadGroupTilingX( uint2( uniforms.windowWidth / 16, uniforms.windowHeight / 16 ), uint2( 16, 16 ), 8, (uint2)tid, (uint2)dtid );
    //ushort2 gid = globalIdx.xy;
    
    float depthWidth = uniforms.windowWidth;
    float depthHeight = uniforms.windowHeight;

    float2 uv = float2( gid ) / float2( depthWidth, depthHeight );
    // get view space origin:
    float originDepth = -depthNormalsTexture.read( uint2( gid ), 0 ).r;
    float uTanHalfFov = tan( uniforms.cameraParams.x * 0.5f );
    float uAspectRatio = depthWidth / (float)depthHeight;
    float2 xy = uv * 2 - 1;
    
    float3 viewDirection = float3( -xy.x * uTanHalfFov * uAspectRatio, -xy.y * uTanHalfFov, 1.0f );
    float3 originPos = viewDirection * originDepth;
    
    // get view space normal:
    float3 normal = -normalize( depthNormalsTexture.read( uint2( gid ), 0 ).gba );
    
    // construct kernel basis matrix:
    float3 rvec = normalize( float3( noiseTexture.read( uint2( globalIdx.x % 64, globalIdx.y % 64 ), 0 ).rg, 0 ) );
    float3 tangent = normalize( rvec - normal * dot( rvec, normal ) );
    float3 bitangent = cross( tangent, normal );
    float3x3 kernelBasis = float3x3( tangent, bitangent, normal );
    
    float s = ssaoInternal( kernelBasis, originPos, 0.5f, uniforms, depthNormalsTexture );
    
    const float4 color = float4( 1, 1, 1, 1 );
    outTexture.write( color * s, gid );
}
