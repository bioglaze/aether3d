#include <metal_stdlib>
#include <metal_atomic>
#include <simd/simd.h>

using namespace metal;
#include "MetalCommon.h"

struct Particle
{
    float4 position;
    float4 color;
    float4 clipPosition;
};

float rand_1_05( float2 uv )
{
    float2 nois = (fract( sin( dot( uv, float2( 12.9898, 78.233) * 2.0 ) ) * 43758.5453 ) );
    return abs( nois.x + nois.y ) * 0.5;
}

kernel void particle_simulation(
                  constant Uniforms& uniforms [[ buffer(0) ]],
                  device Particle* particleBufferOut [[ buffer(1) ]],
                  ushort2 gid [[thread_position_in_grid]],
                  ushort2 tid [[thread_position_in_threadgroup]],
                  ushort2 dtid [[threadgroup_position_in_grid]])
{
    float2 uv = (float2)gid.xy;
    float x = rand_1_05( uv );
    float4 position = float4( gid.x * x * 8, sin( uniforms.timeStamp ) * 8, 0, 1 );
    particleBufferOut[ gid.x ].position = position;
    particleBufferOut[ gid.x ].color = float4( 1, 0, 0, 1 );
    particleBufferOut[ gid.x ].clipPosition = position;//mul( viewToClip, position );

}

kernel void particle_draw(
                  constant Uniforms& uniforms [[ buffer(0) ]],
                  device Particle* particleBuffer [[ buffer(1) ]],
                  texture2d<float, access::read> inTexture [[texture(0)]],
                  texture2d<float, access::write> outTexture [[texture(1)]],
                  texture2d<float, access::read> inTextureDepth [[texture(2)]],
                  ushort2 gid [[thread_position_in_grid]],
                  ushort2 tid [[thread_position_in_threadgroup]],
                  ushort2 dtid [[threadgroup_position_in_grid]])
{
    float4 color = inTexture.read( gid );
    float depth = inTextureDepth.read( gid ).r;
    
    for (uint i = 0; i < 10; ++i)
    {
        float4 clipPos = uniforms.viewToClip * particleBuffer[ i ].position;
        clipPos.y = - clipPos.y;
        float3 ndc = clipPos.xyz / clipPos.w;
        float3 unscaledWindowCoords = 0.5f * ndc + float3( 0.5f, 0.5f, 0.5f );
        float3 windowCoords = float3( uniforms.windowWidth * unscaledWindowCoords.x, uniforms.windowHeight * unscaledWindowCoords.y, unscaledWindowCoords.z );

        float dist = distance( windowCoords.xy, (float2)gid.xy );
        const float radius = 5;
        if (dist < radius /*&& clipPos.z < depth*/)
        {
            color = particleBuffer[ i ].color;
        }
    }

    outTexture.write( color, gid );
}
