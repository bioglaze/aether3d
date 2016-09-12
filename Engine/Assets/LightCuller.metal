#include <metal_stdlib>

using namespace metal;

#define TILE_RES 16
#define NUM_THREADS_PER_TILE (TILE_RES * TILE_RES)
#define MAX_NUM_LIGHTS_PER_TILE 544
#define LIGHT_INDEX_BUFFER_SENTINEL 0x7fffffff

kernel void light_culler(texture2d<float, access::read> depthNormalsTexture [[texture(0)]],
                         texture2d<float, access::read> pointLightBufferCenterAndRadius [[texture(1)]],
                         texture2d<float, access::write> perTileLightIndexBufferOut [[texture(2)]],
                          uint2 gid [[thread_position_in_grid]])
{
    //perTileLightIndexBufferOut.write( float4( 1, 0, 0, 1 ), gid );
    //perTileLightIndexBufferOut.write( depthNormalsTexture.read( gid ), gid );
}