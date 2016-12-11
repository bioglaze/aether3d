#include "LightTiler.hpp"
#include <d3d12.h>
#include "ComputeShader.hpp"
#include "Macros.hpp"
#include "Matrix.hpp"
#include "RenderTexture.hpp"
#include "System.hpp"
#include "Vec3.hpp"

using namespace ae3d;

namespace GfxDeviceGlobal
{
    extern int backBufferWidth;
    extern int backBufferHeight;
    extern ID3D12Device* device;
}

namespace MathUtil
{
    int Max( int x, int y );
}

void ae3d::LightTiler::DestroyBuffers()
{
    AE3D_SAFE_RELEASE( pointLightCenterAndRadiusBuffer );
    AE3D_SAFE_RELEASE( perTileLightIndexBuffer );
}

struct CullerUniforms
{
    Matrix44 invProjection;
    Matrix44 viewMatrix;
    unsigned windowWidth;
    unsigned windowHeight;
    unsigned numLights;
    int maxNumLightsPerTile;
};

void ae3d::LightTiler::Init()
{
    pointLightCenterAndRadius.resize( MaxLights );

    // Light index buffer
    {
        const unsigned numTiles = GetNumTilesX() * GetNumTilesY();
        const unsigned maxNumLightsPerTile = GetMaxNumLightsPerTile();

        D3D12_HEAP_PROPERTIES uploadProp = {};
        uploadProp.Type = D3D12_HEAP_TYPE_UPLOAD;
        uploadProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        uploadProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        uploadProp.CreationNodeMask = 1;
        uploadProp.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC bufferProp = {};
        bufferProp.Alignment = 0;
        bufferProp.DepthOrArraySize = 1;
        bufferProp.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferProp.Flags = D3D12_RESOURCE_FLAG_NONE;
        bufferProp.Format = DXGI_FORMAT_UNKNOWN;
        bufferProp.Height = 1;
        bufferProp.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufferProp.MipLevels = 1;
        bufferProp.SampleDesc.Count = 1;
        bufferProp.SampleDesc.Quality = 0;
        bufferProp.Width = maxNumLightsPerTile * numTiles * sizeof( unsigned );

        HRESULT hr = GfxDeviceGlobal::device->CreateCommittedResource(
            &uploadProp,
            D3D12_HEAP_FLAG_NONE,
            &bufferProp,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS( &perTileLightIndexBuffer ) );
        if (FAILED( hr ))
        {
            ae3d::System::Assert( false, "Unable to create vertex buffer!\n" );
            return;
        }

        perTileLightIndexBuffer->SetName( L"Light index buffer" );
        // TODO: Add to be destroyed on exit
    }

    // Point light buffer
    {
        D3D12_HEAP_PROPERTIES uploadProp = {};
        uploadProp.Type = D3D12_HEAP_TYPE_UPLOAD;
        uploadProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        uploadProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        uploadProp.CreationNodeMask = 1;
        uploadProp.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC bufferProp = {};
        bufferProp.Alignment = 0;
        bufferProp.DepthOrArraySize = 1;
        bufferProp.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferProp.Flags = D3D12_RESOURCE_FLAG_NONE;
        bufferProp.Format = DXGI_FORMAT_UNKNOWN;
        bufferProp.Height = 1;
        bufferProp.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufferProp.MipLevels = 1;
        bufferProp.SampleDesc.Count = 1;
        bufferProp.SampleDesc.Quality = 0;
        bufferProp.Width = pointLightCenterAndRadius.size() * 4 * sizeof( float );

        HRESULT hr = GfxDeviceGlobal::device->CreateCommittedResource(
            &uploadProp,
            D3D12_HEAP_FLAG_NONE,
            &bufferProp,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS( &pointLightCenterAndRadiusBuffer ) );
        if (FAILED( hr ))
        {
            ae3d::System::Assert( false, "Unable to create point light buffer!" );
            return;
        }

        pointLightCenterAndRadiusBuffer->SetName( L"Point light buffer" );
        // TODO: Add to be destroyed on exit
    }
}

void ae3d::LightTiler::SetPointLightPositionAndRadius( int bufferIndex, Vec3& position, float radius )
{
    System::Assert( bufferIndex < MaxLights, "tried to set a too high light index" );

    if (bufferIndex < MaxLights)
    {
        activePointLights = MathUtil::Max( bufferIndex, activePointLights ) + 1;
        pointLightCenterAndRadius[ bufferIndex ] = Vec4( position.x, position.y, position.z, radius );
    }
}

void ae3d::LightTiler::UpdateLightBuffers()
{
    char* pointLightPtr = nullptr;
    HRESULT hr = pointLightCenterAndRadiusBuffer->Map( 0, nullptr, reinterpret_cast<void**>( &pointLightPtr) );

    if (FAILED( hr ))
    {
        ae3d::System::Assert( false, "Unable to light index buffer!\n" );
        return;
    }

    memcpy_s( pointLightPtr, 0, pointLightCenterAndRadius.data(), pointLightCenterAndRadius.size() * 4 * sizeof( float ) );
    pointLightCenterAndRadiusBuffer->Unmap( 0, nullptr );
}

void ae3d::LightTiler::CullLights( ComputeShader& shader, const Matrix44& projection, const Matrix44& view, RenderTexture& depthNormalTarget )
{
    shader.SetRenderTexture( &depthNormalTarget, 0 );
    CullerUniforms uniforms;

    Matrix44::Invert( projection, uniforms.invProjection );

    uniforms.viewMatrix = view;
    uniforms.windowWidth = depthNormalTarget.GetWidth();
    uniforms.windowHeight = depthNormalTarget.GetHeight();
    unsigned activeSpotLights = 0;
    uniforms.numLights = (((unsigned)activeSpotLights & 0xFFFFu) << 16) | ((unsigned)activePointLights & 0xFFFFu);
    uniforms.maxNumLightsPerTile = GetMaxNumLightsPerTile();

    cullerUniformsCreated = true;

    //uint8_t* bufferPointer = (uint8_t *)[ uniformBuffer contents ];
    //memcpy( bufferPointer, &uniforms, sizeof( CullerUniforms ) );

    //shader.SetUniformBuffer( 0, uniformBuffer );
    //shader.SetUniformBuffer( 1, pointLightCenterAndRadiusBuffer );
    //shader.SetUniformBuffer( 2, perTileLightIndexBuffer );

    shader.Dispatch( GetNumTilesX(), GetNumTilesY(), 1 );
}

unsigned ae3d::LightTiler::GetNumTilesX() const
{
    return (unsigned)((GfxDeviceGlobal::backBufferWidth + TileRes - 1) / (float)TileRes);
}

unsigned ae3d::LightTiler::GetNumTilesY() const
{
    return (unsigned)((GfxDeviceGlobal::backBufferHeight + TileRes - 1) / (float)TileRes);
}

unsigned ae3d::LightTiler::GetMaxNumLightsPerTile() const
{
    const unsigned kAdjustmentMultipier = 32;

    // I haven't tested at greater than 1080p, so cap it
    const unsigned uHeight = (GfxDeviceGlobal::backBufferHeight > 1080) ? 1080 : GfxDeviceGlobal::backBufferHeight;

    // adjust max lights per tile down as height increases
    return (MaxLightsPerTile - (kAdjustmentMultipier * (uHeight / 120)));
}
