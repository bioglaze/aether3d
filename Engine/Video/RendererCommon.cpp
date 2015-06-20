#include "Renderer.hpp"
#include "CameraComponent.hpp"
#include "GfxDevice.hpp"
#include "Matrix.hpp"
#include "Vec3.hpp"

void ae3d::Renderer::RenderSkybox( const TextureCube* skyTexture, const CameraComponent& camera )
{
    Matrix44 modelViewProjection;
    Matrix44::Multiply( camera.GetView(), camera.GetProjection(), modelViewProjection );
    
    builtinShaders.skyboxShader.Use();
    builtinShaders.skyboxShader.SetMatrix( "modelViewProjection", modelViewProjection.m );
    builtinShaders.skyboxShader.SetTexture( "textureMap", skyTexture, 0 );
    
    GfxDevice::SetDepthFunc( GfxDevice::DepthFunc::LessOrEqualWriteOff );
    skyboxBuffer.Bind();
    skyboxBuffer.Draw();
}
