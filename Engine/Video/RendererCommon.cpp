#include "Renderer.hpp"
#include <vector>
#include <math.h>
#include "Array.hpp"
#include "CameraComponent.hpp"
#include "FileSystem.hpp"
#include "LightTiler.hpp"
#include "GfxDevice.hpp"
#include "Matrix.hpp"
#include "System.hpp"
#include "Vec3.hpp"
#include "VertexBuffer.hpp"

namespace GfxDeviceGlobal
{
    extern PerObjectUboStruct perObjectUboStruct;
    extern std::vector< ae3d::VertexBuffer > lineBuffers;
    extern unsigned backBufferHeight;
}

namespace MathUtil
{
    int Max( int x, int y );
}

void ae3d::Renderer::GenerateTextures()
{
    whiteTexture.Load( FileSystem::FileContents( "default_white.png" ), TextureWrap::Repeat, TextureFilter::Nearest, Mipmaps::None, ColorSpace::SRGB, Anisotropy::k1 );
}

void ae3d::Renderer::GenerateSkybox()
{
    const float s = 50;

    const VertexBuffer::VertexPTC vertices[ 8 ] =
    {
        { Vec3( -s, -s, s ), 0, 0 },
        { Vec3( s, -s, s ), 0, 0 },
        { Vec3( s, -s, -s ), 0, 0 },
        { Vec3( -s, -s, -s ), 0, 0 },
        { Vec3( -s, s, s ), 0, 0 },
        { Vec3( s, s, s ), 0, 0 },
        { Vec3( s, s, -s ), 0, 0 },
        { Vec3( -s, s, -s ), 0, 0 }
    };

    const VertexBuffer::Face indices[ 12 ] =
    {
        { 0, 4, 1 },
        { 4, 5, 1 },
        { 1, 5, 2 },
        { 2, 5, 6 },
        { 2, 6, 3 },
        { 3, 6, 7 },
        { 3, 7, 0 },
        { 0, 7, 4 },
        { 4, 7, 5 },
        { 5, 7, 6 },
        { 3, 0, 2 },
        { 2, 0, 1 }
    };

    skyboxBuffer.Generate( indices, 12, vertices, 8, VertexBuffer::Storage::GPU );
}

void ae3d::Renderer::GenerateQuadBuffer()
{
    const VertexBuffer::VertexPTC vertices[ 4 ] =
    {
        { Vec3( 0, 0, 0 ), 0, 0 },
        { Vec3( 1, 0, 0 ), 1, 0 },
        { Vec3( 1, 1, 0 ), 1, 1 },
        { Vec3( 0, 1, 0 ), 0, 1 }
    };
    
    VertexBuffer::Face indices[ 2 ] =
    {
        { 0, 1, 2 },
        { 2, 3, 0 }
    };
    
    quadBuffer.Generate( indices, 2, vertices, 4, VertexBuffer::Storage::GPU );
}

void ae3d::Renderer::RenderSkybox( TextureCube* skyTexture, const CameraComponent& camera )
{
    Matrix44 localToClip;
    Matrix44::Multiply( camera.GetView(), camera.GetProjection(), localToClip );
    
    builtinShaders.skyboxShader.Use();
    builtinShaders.skyboxShader.SetTexture( "skyMap", skyTexture, 0 );
    GfxDeviceGlobal::perObjectUboStruct.localToClip = localToClip;
#if AE3D_OPENVR
    GfxDeviceGlobal::perObjectUboStruct.isVR = 1;
#endif

    GfxDevice::PushGroupMarker( "Skybox" );
    GfxDevice::Draw( skyboxBuffer, 0, skyboxBuffer.GetFaceCount() / 3, builtinShaders.skyboxShader, GfxDevice::BlendMode::Off,
                     GfxDevice::DepthFunc::LessOrEqualWriteOff, GfxDevice::CullMode::Off, GfxDevice::FillMode::Solid, GfxDevice::PrimitiveTopology::Triangles );
    GfxDevice::PopGroupMarker();
}

int ae3d::GfxDevice::CreateLineBuffer( const Vec3* lines, int lineCount, const Vec3& color )
{
    if (lineCount == 0)
    {
        return -1;
    }

    Array< VertexBuffer::Face > faces;
    faces.Allocate( lineCount * 2 );
    Array< VertexBuffer::VertexPTC > vertices;
    vertices.Allocate( lineCount );
    
    for (int lineIndex = 0; lineIndex < lineCount; ++lineIndex)
    {
        vertices[ lineIndex ].position = lines[ lineIndex ];
        vertices[ lineIndex ].color = Vec4( color, 1 );
    }

    // Not used, but needs to be set to something.
    for (unsigned short faceIndex = 0; faceIndex < (unsigned short)(faces.count / 2); ++faceIndex)
    {
        faces[ faceIndex * 2 + 0 ].a = faceIndex;
        faces[ faceIndex * 2 + 1 ].b = faceIndex + 1;
    }

    GfxDeviceGlobal::lineBuffers.push_back( VertexBuffer() );
    GfxDeviceGlobal::lineBuffers.back().Generate( faces.elements, faces.count, vertices.elements, vertices.count, VertexBuffer::Storage::GPU );
    GfxDeviceGlobal::lineBuffers.back().SetDebugName( "line buffer" );

    return int( GfxDeviceGlobal::lineBuffers.size() ) - 1;
}

void ae3d::LightTiler::SetPointLightParameters( int bufferIndex, const Vec3& position, float radius, const Vec4& color )
{
    System::Assert( bufferIndex < MaxLights, "tried to set a too high light index" );

    if (bufferIndex < MaxLights)
    {
        activePointLights = MathUtil::Max( bufferIndex + 1, activePointLights );
        pointLightCenterAndRadius[ bufferIndex ] = Vec4( position.x, position.y, position.z, radius );
        pointLightColors[ bufferIndex ] = color;
    }
}

void ae3d::LightTiler::SetSpotLightParameters( int bufferIndex, Vec3& position, float radius, const Vec4& color, const Vec3& direction, float coneAngle, float falloffRadius )
{
    System::Assert( bufferIndex < MaxLights, "tried to set a too high light index" );

    if (bufferIndex < MaxLights)
    {
        activeSpotLights = MathUtil::Max( bufferIndex + 1, activeSpotLights );
        spotLightCenterAndRadius[ bufferIndex ] = Vec4( position.x, position.y, position.z, radius );
        spotLightParams[ bufferIndex ] = Vec4( direction.x, direction.y, direction.z, cos( coneAngle * 3.14159265f / 180.0f ) );
		spotLightColors[ bufferIndex ] = Vec4( color.x, color.y, color.z, falloffRadius );
    }
}

unsigned ae3d::LightTiler::GetMaxNumLightsPerTile() const
{
    const unsigned kAdjustmentMultipier = 32;

    // I haven't tested at greater than 1080p, so cap it
    const unsigned uHeight = (GfxDeviceGlobal::backBufferHeight > 1080) ? 1080 : GfxDeviceGlobal::backBufferHeight;

    // adjust max lights per tile down as height increases
    return (MaxLightsPerTile - (kAdjustmentMultipier * (uHeight / 120)));
}
