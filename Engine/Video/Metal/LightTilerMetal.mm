#include "LightTiler.hpp"
#include "ComputeShader.hpp"
#include "Matrix.hpp"
#include "RenderTexture.hpp"
#include "System.hpp"
#include "Vec3.hpp"

namespace GfxDeviceGlobal
{
    extern int backBufferWidth;
    extern int backBufferHeight;
}

void ae3d::LightTiler::Init()
{
    pointLightCenterAndRadius.resize( MaxLights );
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
    return (unsigned)((GfxDeviceGlobal::backBufferWidth + TileRes - 1) / (float)TileRes);
}

unsigned ae3d::LightTiler::GetNumTilesY() const
{
    return (unsigned)((GfxDeviceGlobal::backBufferWidth + TileRes - 1) / (float)TileRes);
}

void ae3d::LightTiler::SetPointLightPositionAndRadius( int handle, Vec3& position, float radius )
{
    System::Assert( handle < MaxLights, "tried to set a too high light index" );
    
    pointLightCenterAndRadius[ handle ] = Vec4( position.x, position.y, position.z, radius );
}

unsigned ae3d::LightTiler::GetMaxNumLightsPerTile() const
{
    const unsigned adjustmentMultipier = 32;
    
    // I haven't tested at greater than 1080p, so cap it
    const unsigned height = (GfxDeviceGlobal::backBufferHeight > 1080) ? 1080 : GfxDeviceGlobal::backBufferHeight;
    
    // adjust max lights per tile down as height increases
    return (MaxLightsPerTile - (adjustmentMultipier * (height / 120)));
}

using namespace ae3d;

struct CullerUniforms
{
    unsigned windowWidth;
    unsigned windowHeight;
    unsigned numLights;
    int maxNumLightsPerTile;
    Matrix44 invProjection;
    Matrix44 viewMatrix;
};

void ae3d::LightTiler::CullLights( ComputeShader& shader, const Matrix44& projection, const Matrix44& view, RenderTexture& depthNormalTarget )
{
    if (pointLightCenterAndRadiusRT.GetID() == 0)
    {
        pointLightCenterAndRadiusRT.Create2D( GfxDeviceGlobal::backBufferWidth, GfxDeviceGlobal::backBufferHeight,
                                              ae3d::RenderTexture::DataType::Float, ae3d::TextureWrap::Clamp, ae3d::TextureFilter::Nearest );
    }

    if (perTileLightIndexBufferRT.GetID() == 0)
    {
        perTileLightIndexBufferRT.Create2D( GfxDeviceGlobal::backBufferWidth, GfxDeviceGlobal::backBufferHeight,
                                            ae3d::RenderTexture::DataType::Float, ae3d::TextureWrap::Clamp, ae3d::TextureFilter::Nearest );
    }


    shader.SetRenderTexture( &depthNormalTarget, 0 );
    shader.SetRenderTexture( &pointLightCenterAndRadiusRT, 1 );
    shader.SetRenderTexture( &perTileLightIndexBufferRT, 2 );
    CullerUniforms uniforms;

    Matrix44::Invert( projection, uniforms.invProjection );
    uniforms.viewMatrix = view;
    uniforms.windowWidth = depthNormalTarget.GetWidth();
    uniforms.windowHeight = depthNormalTarget.GetHeight();
    unsigned activeSpotLights = 0;
    uniforms.numLights = (((unsigned)activeSpotLights & 0xFFFFu) << 16) | ((unsigned)activePointLights & 0xFFFFu);
    uniforms.maxNumLightsPerTile = GetMaxNumLightsPerTile();

    shader.SetUniformBuffer( &uniforms, sizeof( CullerUniforms ) );
    
    shader.Dispatch( GetNumTilesX(), GetNumTilesY(), 1 );
}
