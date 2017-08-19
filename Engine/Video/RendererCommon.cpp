#include "Renderer.hpp"
#include <vector>
#include "CameraComponent.hpp"
#include "FileSystem.hpp"
#include "GfxDevice.hpp"
#include "Matrix.hpp"
#include "Vec3.hpp"
#include "VertexBuffer.hpp"

namespace GfxDeviceGlobal
{
    extern PerObjectUboStruct perObjectUboStruct;
}

void ae3d::Renderer::GenerateTextures()
{
    whiteTexture.Load( FileSystem::FileContents( "default_white.png" ), TextureWrap::Repeat, TextureFilter::Nearest, Mipmaps::None, ColorSpace::SRGB, Anisotropy::k1 );
}

void ae3d::Renderer::GenerateSkybox()
{
    const float s = 50;

    const std::vector< VertexBuffer::VertexPTC > vertices =
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

    const std::vector< VertexBuffer::Face > indices =
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

    skyboxBuffer.Generate( indices.data(), static_cast< int >( indices.size() ), vertices.data(), static_cast< int >( vertices.size() ) );
}

void ae3d::Renderer::GenerateQuadBuffer()
{
    const std::vector< VertexBuffer::VertexPTC > vertices =
    {
        { Vec3( 0, 0, 0 ), 0, 0 },
        { Vec3( 1, 0, 0 ), 1, 0 },
        { Vec3( 1, 1, 0 ), 1, 1 },
        { Vec3( 0, 1, 0 ), 0, 1 }
    };
    
    const std::vector< VertexBuffer::Face > indices =
    {
        { 0, 1, 2 },
        { 2, 3, 0 }
    };
    
    quadBuffer.Generate( indices.data(), static_cast< int >( indices.size() ), vertices.data(), static_cast< int >( vertices.size() ) );
}

void ae3d::Renderer::RenderSkybox( TextureCube* skyTexture, const CameraComponent& camera )
{
    Matrix44 localToClip;
    Matrix44::Multiply( camera.GetView(), camera.GetProjection(), localToClip );
    
    builtinShaders.skyboxShader.Use();
#if RENDERER_OPENGL
    GfxDeviceGlobal::perObjectUboStruct.localToClip = localToClip;
#else
    builtinShaders.skyboxShader.SetMatrix( "_ModelViewProjectionMatrix", localToClip.m );
#endif
    builtinShaders.skyboxShader.SetTexture( "skyMap", skyTexture, 0 );

    GfxDevice::PushGroupMarker( "Skybox" );
    GfxDevice::Draw( skyboxBuffer, 0, skyboxBuffer.GetFaceCount() / 3, builtinShaders.skyboxShader, GfxDevice::BlendMode::Off,
                     GfxDevice::DepthFunc::LessOrEqualWriteOff, GfxDevice::CullMode::Off, GfxDevice::FillMode::Solid, GfxDevice::PrimitiveTopology::Triangles );
    GfxDevice::PopGroupMarker();
}
