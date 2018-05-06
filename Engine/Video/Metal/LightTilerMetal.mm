#include "LightTiler.hpp"
#include <cmath>
#include "ComputeShader.hpp"
#include "GfxDevice.hpp"
#include "Matrix.hpp"
#include "RenderTexture.hpp"
#include "System.hpp"
#include "Vec3.hpp"

void UploadPerObjectUbo();

namespace GfxDeviceGlobal
{
    extern int backBufferWidth;
    extern int backBufferHeight;
    extern PerObjectUboStruct perObjectUboStruct;
}

using namespace ae3d;

void ae3d::LightTiler::Init()
{
#if !TARGET_OS_IPHONE
    auto options = MTLResourceStorageModeManaged;
#else
    auto options = MTLResourceCPUCacheModeDefaultCache;
#endif
    pointLightCenterAndRadiusBuffer = [GfxDevice::GetMetalDevice() newBufferWithLength:MaxLights * sizeof( Vec4 )
                                 options:options];
    pointLightCenterAndRadiusBuffer.label = @"pointLightCenterAndRadiusBuffer";

    pointLightColorBuffer = [GfxDevice::GetMetalDevice() newBufferWithLength:MaxLights * sizeof( Vec4 )
                                         options:options];
    pointLightColorBuffer.label = @"pointLightColorBuffer";

    spotLightCenterAndRadiusBuffer = [GfxDevice::GetMetalDevice() newBufferWithLength:MaxLights * sizeof( Vec4 )
                                         options:options];
    spotLightCenterAndRadiusBuffer.label = @"spotLightCenterAndRadiusBuffer";

    spotLightParamsBuffer = [GfxDevice::GetMetalDevice() newBufferWithLength:MaxLights * sizeof( Vec4 )
                                        options:options];
    spotLightParamsBuffer.label = @"spotLightParamsBuffer";

    spotLightColorBuffer = [GfxDevice::GetMetalDevice() newBufferWithLength:MaxLights * sizeof( Vec4 )
                               options:options];
    spotLightColorBuffer.label = @"spotLightColorBuffer";

    const unsigned numTiles = GetNumTilesX() * GetNumTilesY();
    const unsigned maxNumLightsPerTile = GetMaxNumLightsPerTile();

    perTileLightIndexBuffer = [GfxDevice::GetMetalDevice() newBufferWithLength:maxNumLightsPerTile * numTiles * sizeof( unsigned )
                  options:MTLResourceStorageModePrivate];
    perTileLightIndexBuffer.label = @"perTileLightIndexBuffer";
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

    Matrix44::Invert( viewToClip, GfxDeviceGlobal::perObjectUboStruct.clipToView );

    GfxDeviceGlobal::perObjectUboStruct.localToView = worldToView;
    GfxDeviceGlobal::perObjectUboStruct.windowWidth = depthNormalTarget.GetWidth();
    GfxDeviceGlobal::perObjectUboStruct.windowHeight = depthNormalTarget.GetHeight();
    GfxDeviceGlobal::perObjectUboStruct.numLights = (((unsigned)activeSpotLights & 0xFFFFu) << 16) | ((unsigned)activePointLights & 0xFFFFu);
    GfxDeviceGlobal::perObjectUboStruct.maxNumLightsPerTile = GetMaxNumLightsPerTile();
    
    UploadPerObjectUbo();

    shader.SetUniformBuffer( 0, GfxDevice::GetCurrentUniformBuffer() );
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

    bufferPointer = (uint8_t *)[spotLightColorBuffer contents];
    memcpy( bufferPointer, &spotLightColors[ 0 ], MaxLights * 4 * sizeof( float ) );
    
#if !TARGET_OS_IPHONE
    [pointLightCenterAndRadiusBuffer didModifyRange:NSMakeRange( 0, MaxLights * 4 * sizeof( float ) )];
    [pointLightColorBuffer didModifyRange:NSMakeRange( 0, MaxLights * 4 * sizeof( float ) )];
    [spotLightCenterAndRadiusBuffer didModifyRange:NSMakeRange( 0, MaxLights * 4 * sizeof( float ) )];
    [spotLightParamsBuffer didModifyRange:NSMakeRange( 0, MaxLights * 4 * sizeof( float ) )];
    [spotLightColorBuffer didModifyRange:NSMakeRange( 0, MaxLights * 4 * sizeof( float ) )];
#endif
}

