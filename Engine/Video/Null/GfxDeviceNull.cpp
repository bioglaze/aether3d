#include "GfxDevice.hpp"
#include "Renderer.hpp"
#include "RenderTexture.hpp"

ae3d::Renderer renderer;

void ae3d::BuiltinShaders::Load()
{
}

void ae3d::GfxDevice::Draw( VertexBuffer& vertexBuffer, int startIndex, int endIndex, Shader& shader, BlendMode blendMode, DepthFunc depthFunc, CullMode cullMode )
{
}

void ae3d::GfxDevice::Init( int, int )
{
}

void ae3d::GfxDevice::SetPolygonOffset( bool enable, float factor, float units )
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

int ae3d::GfxDevice::GetBarrierCalls()
{
    return 0;
}

int ae3d::GfxDevice::GetFenceCalls()
{
    return 0;
}

void ae3d::GfxDevice::GetGpuMemoryUsage( unsigned& outUsedMBytes, unsigned& outBudgetMBytes )
{
    outUsedMBytes = 0;
    outBudgetMBytes = 0;
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

void ae3d::GfxDevice::ReleaseGPUObjects()
{
}
