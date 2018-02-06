#include "LightTiler.hpp"
#include <cmath>
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
    Matrix44 worldToView;
    unsigned windowWidth;
    unsigned windowHeight;
    unsigned numLights;
    int maxNumLightsPerTile;
};

void ae3d::LightTiler::Init()
{
    pointLightCenterAndRadiusBuffer = [GfxDevice::GetMetalDevice() newBufferWithLength:MaxLights * sizeof( Vec4 )
                                 options:MTLResourceCPUCacheModeDefaultCache];
    pointLightCenterAndRadiusBuffer.label = @"pointLightCenterAndRadiusBuffer";

    pointLightColorBuffer = [GfxDevice::GetMetalDevice() newBufferWithLength:MaxLights * sizeof( Vec4 )
                                         options:MTLResourceCPUCacheModeDefaultCache];
    pointLightColorBuffer.label = @"pointLightColorBuffer";

    spotLightCenterAndRadiusBuffer = [GfxDevice::GetMetalDevice() newBufferWithLength:MaxLights * sizeof( Vec4 )
                                         options:MTLResourceCPUCacheModeDefaultCache];
    spotLightCenterAndRadiusBuffer.label = @"spotLightCenterAndRadiusBuffer";

    spotLightParamsBuffer = [GfxDevice::GetMetalDevice() newBufferWithLength:MaxLights * sizeof( Vec4 )
                                        options:MTLResourceCPUCacheModeDefaultCache];
    spotLightParamsBuffer.label = @"spotLightParamsBuffer";

    const unsigned numTiles = GetNumTilesX() * GetNumTilesY();
    const unsigned maxNumLightsPerTile = GetMaxNumLightsPerTile();

    perTileLightIndexBuffer = [GfxDevice::GetMetalDevice() newBufferWithLength:maxNumLightsPerTile * numTiles * sizeof( unsigned )
                  options:MTLResourceStorageModePrivate];
    perTileLightIndexBuffer.label = @"perTileLightIndexBuffer";

#if !TARGET_OS_IPHONE
    uniformBuffer = [GfxDevice::GetMetalDevice() newBufferWithLength:sizeof( CullerUniforms )
                                 options:MTLStorageModeManaged];
#else
    uniformBuffer = [GfxDevice::GetMetalDevice() newBufferWithLength:sizeof( CullerUniforms )
                       options:MTLStorageModeShared];    
#endif
    uniformBuffer.label = @"CullerUniforms";
}

unsigned ae3d::LightTiler::GetNumTilesX() const
{
    return (unsigned)((GfxDeviceGlobal::backBufferWidth * 2 + TileRes - 1) / (float)TileRes);
}

unsigned ae3d::LightTiler::GetNumTilesY() const
{
    return (unsigned)((GfxDeviceGlobal::backBufferHeight * 2 + TileRes - 1) / (float)TileRes);
}

void ae3d::LightTiler::CullLights( ComputeShader& shader, const Matrix44& viewToClip, const Matrix44& worldToView, RenderTexture& depthNormalTarget )
{
    shader.SetRenderTexture( &depthNormalTarget, 0 );
    CullerUniforms uniforms;

    Matrix44::Invert( viewToClip, uniforms.invProjection );

    uniforms.worldToView = worldToView;
    uniforms.windowWidth = depthNormalTarget.GetWidth();
    uniforms.windowHeight = depthNormalTarget.GetHeight();
    uniforms.numLights = (((unsigned)activeSpotLights & 0xFFFFu) << 16) | ((unsigned)activePointLights & 0xFFFFu);
    uniforms.maxNumLightsPerTile = GetMaxNumLightsPerTile();

    cullerUniformsCreated = true;
    
    uint8_t* bufferPointer = (uint8_t *)[uniformBuffer contents];
    memcpy( bufferPointer, &uniforms, sizeof( CullerUniforms ) );

    shader.SetUniformBuffer( 0, uniformBuffer );
    shader.SetUniformBuffer( 1, pointLightCenterAndRadiusBuffer );
    shader.SetUniformBuffer( 2, perTileLightIndexBuffer);
    shader.SetUniformBuffer( 3, spotLightCenterAndRadiusBuffer );
    
    shader.Dispatch( GetNumTilesX(), GetNumTilesY(), 1 );
}

void ae3d::LightTiler::UpdateLightBuffers()
{
    uint8_t* bufferPointer = (uint8_t *)[pointLightCenterAndRadiusBuffer contents];
    memcpy( bufferPointer, &pointLightCenterAndRadius[ 0 ], MaxLights * 4 * sizeof( float ) );

    bufferPointer = (uint8_t *)[pointLightColorBuffer contents];
    memcpy( bufferPointer, &pointLightColors[ 0 ], MaxLights * 4 * sizeof( float ) );

    bufferPointer = (uint8_t *)[spotLightCenterAndRadiusBuffer contents];
    memcpy( bufferPointer, &spotLightCenterAndRadius[ 0 ], MaxLights * 4 * sizeof( float ) );

    bufferPointer = (uint8_t *)[spotLightParamsBuffer contents];
    memcpy( bufferPointer, &spotLightParams[ 0 ], MaxLights * 4 * sizeof( float ) );
}

