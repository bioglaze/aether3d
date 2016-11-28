#include "LightTiler.hpp"
#include "ComputeShader.hpp"
#include "GfxDevice.hpp"
#include "Matrix.hpp"
#include "RenderTexture.hpp"
#include "System.hpp"
#include "Vec3.hpp"

namespace GfxDeviceGlobal
{
    extern int backBufferWidth;
    extern int backBufferHeight;
}

using namespace ae3d;

struct CullerUniforms
{
    Matrix44 invProjection;
    Matrix44 viewMatrix;
    unsigned windowWidth;
    unsigned windowHeight;
    unsigned numLights;
    int maxNumLightsPerTile;
};

void ae3d::LightTiler::Init()
{
    pointLightCenterAndRadius.resize( MaxLights );
    pointLightCenterAndRadius[ 0 ] = Vec4( -2, 4, -8, 1.5f ); // top left
    pointLightCenterAndRadius[ 1 ] = Vec4(  0, 0, -7, 0.5f );

    activePointLights = 2;

    pointLightCenterAndRadiusBuffer = [GfxDevice::GetMetalDevice() newBufferWithLength:MaxLights * sizeof( Vec4 )
                                 options:MTLResourceCPUCacheModeDefaultCache];
    pointLightCenterAndRadiusBuffer.label = @"pointLightCenterAndRadiusBuffer";

    uint8_t* bufferPointer = (uint8_t *)[pointLightCenterAndRadiusBuffer contents];
    memcpy( bufferPointer, pointLightCenterAndRadius.data(), pointLightCenterAndRadius.size() * 4 * sizeof( float ) );

    const unsigned numTiles = GetNumTilesX() * GetNumTilesY();
    const unsigned maxNumLightsPerTile = GetMaxNumLightsPerTile();

    // TODO: make storage mode MTLResourceStorageModePrivate if it works and is faster.
    perTileLightIndexBuffer = [GfxDevice::GetMetalDevice() newBufferWithLength:maxNumLightsPerTile * numTiles * sizeof( unsigned )
                  options:MTLResourceCPUCacheModeDefaultCache];
    perTileLightIndexBuffer.label = @"perTileLightIndexBuffer";

    uniformBuffer = [GfxDevice::GetMetalDevice() newBufferWithLength:sizeof( CullerUniforms )
                                 options:MTLResourceCPUCacheModeDefaultCache];
    uniformBuffer.label = @"CullerUniforms";
}

int ae3d::LightTiler::GetNextPointLightBufferIndex()
{
    if (activePointLights < MaxLights)
    {
        ++activePointLights;
        return activePointLights - 1;
    }
    
    System::Assert( false, "tried to get a point light when buffer is full" );
    return -1;
}

unsigned ae3d::LightTiler::GetNumTilesX() const
{
    return (unsigned)((GfxDeviceGlobal::backBufferWidth * 2 + TileRes - 1) / (float)TileRes);
}

unsigned ae3d::LightTiler::GetNumTilesY() const
{
    return (unsigned)((GfxDeviceGlobal::backBufferHeight * 2 + TileRes - 1) / (float)TileRes);
}

void ae3d::LightTiler::SetPointLightPositionAndRadius( int handle, Vec3& position, float radius )
{
    System::Assert( handle < MaxLights, "tried to set a too high light index" );

    pointLightCenterAndRadius[ handle ] = Vec4( position.x, position.y, position.z, radius );
}

unsigned ae3d::LightTiler::GetMaxNumLightsPerTile() const
{
    // FIXME: Should this be the same as the tile size?
    const unsigned adjustmentMultipier = 32;
    
    // I haven't tested at greater than 1080p, so cap it
    const unsigned height = (GfxDeviceGlobal::backBufferHeight > 1080) ? 1080 : GfxDeviceGlobal::backBufferHeight;
    
    // adjust max lights per tile down as height increases
    return (MaxLightsPerTile - (adjustmentMultipier * (height / 120)));
}

void ae3d::LightTiler::CullLights( ComputeShader& shader, const Matrix44& projection, const Matrix44& view, RenderTexture& depthNormalTarget )
{
    shader.SetRenderTexture( &depthNormalTarget, 0 );
    CullerUniforms uniforms;

    Matrix44::Invert( projection, uniforms.invProjection );

    uniforms.viewMatrix = view;
    uniforms.windowWidth = depthNormalTarget.GetWidth() * 2; // * 2 because of retina resolution
    uniforms.windowHeight = depthNormalTarget.GetHeight() * 2;
    unsigned activeSpotLights = 0;
    uniforms.numLights = (((unsigned)activeSpotLights & 0xFFFFu) << 16) | ((unsigned)activePointLights & 0xFFFFu);
    uniforms.maxNumLightsPerTile = GetMaxNumLightsPerTile();

    cullerUniformsCreated = true;
    
    uint8_t* bufferPointer = (uint8_t *)[uniformBuffer contents];
    memcpy( bufferPointer, &uniforms, sizeof( CullerUniforms ) );

    shader.SetUniformBuffer( 0, uniformBuffer );
    shader.SetUniformBuffer( 1, pointLightCenterAndRadiusBuffer );
    shader.SetUniformBuffer( 2, perTileLightIndexBuffer);

    shader.Dispatch( GetNumTilesX(), GetNumTilesY(), 1 );
}
