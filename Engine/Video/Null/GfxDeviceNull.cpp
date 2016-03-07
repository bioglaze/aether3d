#include "GfxDevice.hpp"
#include "Renderer.hpp"
#include "RenderTexture.hpp"

ae3d::Renderer renderer;

void ae3d::BuiltinShaders::Load()
{
}

void ae3d::GfxDevice::Draw(ae3d::VertexBuffer&, int, int, ae3d::Shader&, ae3d::GfxDevice::BlendMode, ae3d::GfxDevice::DepthFunc)
{
}

void ae3d::GfxDevice::Init( int, int )
{
}

int ae3d::GfxDevice::GetDrawCalls()
{
    return 0;
}

int ae3d::GfxDevice::GetVertexBufferBinds()
{
    return 0;
}


int ae3d::GfxDevice::GetTextureBinds()
{
    return 0;
}

int ae3d::GfxDevice::GetRenderTargetBinds()
{
    return 0;
}

int ae3d::GfxDevice::GetShaderBinds()
{
    return 0;
}

void ae3d::GfxDevice::ResetFrameStatistics()
{
}

void ae3d::GfxDevice::SetClearColor( float, float, float )
{
}

void ae3d::GfxDevice::SetRenderTarget( RenderTexture*, unsigned )
{
}

void ae3d::GfxDevice::ClearScreen( unsigned )
{
}

void ae3d::GfxDevice::ErrorCheck( const char* )
{
}

void ae3d::GfxDevice::SetBackFaceCulling( bool )
{
}

void ae3d::GfxDevice::ReleaseGPUObjects()
{
}
