#include "Renderer.hpp"
#include "CameraComponent.hpp"
#include "GfxDevice.hpp"
#include "Matrix.hpp"
#include <vector>
#include "Vec3.hpp"
#include "VertexBuffer.hpp"

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

void ae3d::Renderer::RenderSkybox( const TextureCube* skyTexture, const CameraComponent& camera )
{
    Matrix44 modelViewProjection;
    Matrix44::Multiply( camera.GetView(), camera.GetProjection(), modelViewProjection );
    
    builtinShaders.skyboxShader.Use();
    builtinShaders.skyboxShader.SetMatrix( "_ModelViewProjectionMatrix", modelViewProjection.m );
    builtinShaders.skyboxShader.SetTexture( "skyMap", skyTexture, 0 );

    GfxDevice::Draw( skyboxBuffer, 0, skyboxBuffer.GetFaceCount() / 3, builtinShaders.skyboxShader, GfxDevice::BlendMode::Off, GfxDevice::DepthFunc::LessOrEqualWriteOff );
}
