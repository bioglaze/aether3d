#include "LightTiler.hpp"
#include "ComputeShader.hpp"
#include "Matrix.hpp"
#include "RenderTexture.hpp"
#include "System.hpp"
#include "Vec3.hpp"

unsigned screenWidth = 480;
unsigned screenHeight = 480;

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
    return (unsigned)((screenWidth + TileRes - 1) / (float)TileRes);
}

unsigned ae3d::LightTiler::GetNumTilesY() const
{
    return (unsigned)((screenHeight + TileRes - 1) / (float)TileRes);
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
    const unsigned height = (screenHeight > 1080) ? 1080 : screenHeight;
    
    // adjust max lights per tile down as height increases
    return (MaxLightsPerTile - (adjustmentMultipier * (height / 120)));
}

using namespace ae3d;

struct CullerUniforms
{
    Matrix44 invProjection;
    Matrix44 view;
    Vec4 windowSize;
};

void ae3d::LightTiler::CullLights( ComputeShader& shader, const Matrix44& projection, const Matrix44& view, RenderTexture& depthNormalTarget )
{
    if (pointLightCenterAndRadiusRT.GetID() == 0)
    {
        pointLightCenterAndRadiusRT.Create2D( depthNormalTarget.GetWidth(), depthNormalTarget.GetHeight(),ae3d::RenderTexture::DataType::Float, ae3d::TextureWrap::Clamp, ae3d::TextureFilter::Nearest );
    }
    
    shader.SetRenderTexture( &depthNormalTarget, 0 );
    shader.SetRenderTexture( &pointLightCenterAndRadiusRT, 1 );
    shader.SetRenderTexture( &depthNormalTarget, 2 );
    CullerUniforms uniforms;

    Matrix44::Invert( projection, uniforms.invProjection );
    uniforms.view = view;
    uniforms.windowSize = Vec4( depthNormalTarget.GetWidth(), depthNormalTarget.GetHeight(), 0, 0 );
    shader.SetUniformBuffer( &uniforms, sizeof( CullerUniforms ) );
    
    shader.Dispatch( GetNumTilesX(), GetNumTilesY(), 1 );
}
