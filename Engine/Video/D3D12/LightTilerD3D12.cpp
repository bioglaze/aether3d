#include "LightTiler.hpp"
#include <d3d12.h>
#include "ComputeShader.hpp"
#include "DescriptorHeapManager.hpp"
#include "GfxDevice.hpp"
#include "Macros.hpp"
#include "Matrix.hpp"
#include "RenderTexture.hpp"
#include "System.hpp"
#include "Vec3.hpp"

#define AE3D_CB_SIZE (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT * 3 + 80 * 64)

using namespace ae3d;

namespace GfxDeviceGlobal
{
    extern int backBufferWidth;
    extern int backBufferHeight;
    extern ID3D12Device* device;
    extern ID3D12Resource* uav1;
    extern D3D12_UNORDERED_ACCESS_VIEW_DESC uav1Desc;
}

namespace MathUtil
{
    int Max( int x, int y );
}

void ae3d::LightTiler::DestroyBuffers()
{
    AE3D_SAFE_RELEASE( pointLightCenterAndRadiusBuffer );
    AE3D_SAFE_RELEASE( spotLightCenterAndRadiusBuffer );
    AE3D_SAFE_RELEASE( perTileLightIndexBuffer );
}

void ae3d::LightTiler::Init()
{
    // Light index buffer
    {
        const unsigned numTiles = GetNumTilesX() * GetNumTilesY();
        const unsigned maxNumLightsPerTile = GetMaxNumLightsPerTile();

        D3D12_HEAP_PROPERTIES heapProp = {};
        heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProp.CreationNodeMask = 1;
        heapProp.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC bufferProp = {};
        bufferProp.Alignment = 0;
        bufferProp.DepthOrArraySize = 1;
        bufferProp.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferProp.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        bufferProp.Format = DXGI_FORMAT_UNKNOWN;
        bufferProp.Height = 1;
        bufferProp.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufferProp.MipLevels = 1;
        bufferProp.SampleDesc.Count = 1;
        bufferProp.SampleDesc.Quality = 0;
        bufferProp.Width = maxNumLightsPerTile * numTiles * sizeof( unsigned );

        HRESULT hr = GfxDeviceGlobal::device->CreateCommittedResource(
            &heapProp,
            D3D12_HEAP_FLAG_NONE,
            &bufferProp,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr,
            IID_PPV_ARGS( &perTileLightIndexBuffer ) );
        if (FAILED( hr ))
        {
            ae3d::System::Assert( false, "Unable to create light index buffer!" );
            return;
        }

        perTileLightIndexBuffer->SetName( L"LightTiler light index buffer" );
        GfxDeviceGlobal::uav1 = perTileLightIndexBuffer;
        GfxDeviceGlobal::uav1Desc.Format = DXGI_FORMAT_UNKNOWN;
        GfxDeviceGlobal::uav1Desc.Buffer.NumElements = maxNumLightsPerTile * numTiles;
        GfxDeviceGlobal::uav1Desc.Buffer.StructureByteStride = 4;
        GfxDeviceGlobal::uav1Desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
        GfxDeviceGlobal::uav1Desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
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
        bufferProp.Width = MaxLights * 4 * sizeof( float );

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

        pointLightCenterAndRadiusBuffer->SetName( L"LightTiler point light buffer" );
    }

    // Spot light buffer
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
        bufferProp.Width = MaxLights * 4 * sizeof( float );

        HRESULT hr = GfxDeviceGlobal::device->CreateCommittedResource(
	        &uploadProp,
	        D3D12_HEAP_FLAG_NONE,
	        &bufferProp,
	        D3D12_RESOURCE_STATE_GENERIC_READ,
	        nullptr,
	        IID_PPV_ARGS( &spotLightCenterAndRadiusBuffer ) );
        if( FAILED( hr ) )
        {
	        ae3d::System::Assert( false, "Unable to create spot light buffer!" );
	        return;
        }

        spotLightCenterAndRadiusBuffer->SetName( L"LightTiler spot light buffer" );
    }
}

void ae3d::LightTiler::SetPointLightPositionAndRadius( int bufferIndex, Vec3& position, float radius )
{
    System::Assert( bufferIndex < MaxLights, "tried to set a too high light index" );

    if (bufferIndex < MaxLights)
    {
        activePointLights = MathUtil::Max( bufferIndex + 1, activePointLights );
        pointLightCenterAndRadius[ bufferIndex ] = Vec4( position.x, position.y, position.z, radius );
    }
}

void ae3d::LightTiler::SetSpotLightPositionAndRadius( int bufferIndex, Vec3& position, float radius )
{
    System::Assert( bufferIndex < MaxLights, "tried to set a too high light index" );

    if (bufferIndex < MaxLights)
    {
        activeSpotLights = MathUtil::Max( bufferIndex + 1, activeSpotLights );
        spotLightCenterAndRadius[ bufferIndex ] = Vec4( position.x, position.y, position.z, radius );
    }
}

void ae3d::LightTiler::UpdateLightBuffers()
{
    char* pointLightPtr = nullptr;
    HRESULT hr = pointLightCenterAndRadiusBuffer->Map( 0, nullptr, reinterpret_cast<void**>( &pointLightPtr) );

    if (FAILED( hr ))
    {
        ae3d::System::Assert( false, "Unable to map point light buffer!\n" );
        return;
    }

    const std::size_t byteSize = MaxLights * 4 * sizeof( float );
    memcpy_s( pointLightPtr, byteSize, &pointLightCenterAndRadius[ 0 ], byteSize );
    pointLightCenterAndRadiusBuffer->Unmap( 0, nullptr );

    char* spotLightPtr = nullptr;
    hr = spotLightCenterAndRadiusBuffer->Map( 0, nullptr, reinterpret_cast<void**>(&spotLightPtr) );

    if( FAILED( hr ) )
    {
        ae3d::System::Assert( false, "Unable to map spot light buffer!\n" );
        return;
    }

    memcpy_s( spotLightPtr, byteSize, &spotLightCenterAndRadius[ 0 ], byteSize );
    spotLightCenterAndRadiusBuffer->Unmap( 0, nullptr );
}

void ae3d::LightTiler::CullLights( ComputeShader& shader, const Matrix44& projection, const Matrix44& localToView, RenderTexture& depthNormalTarget )
{
    GfxDevice::CreateNewUniformBuffer();

    PerObjectUboStruct uniforms;

    Matrix44::Invert( projection, uniforms.clipToView );

    uniforms.localToView = localToView;
    uniforms.windowWidth = depthNormalTarget.GetWidth();
    uniforms.windowHeight = depthNormalTarget.GetHeight();
    uniforms.numLights = (((unsigned)activeSpotLights & 0xFFFFu) << 16) | ((unsigned)activePointLights & 0xFFFFu);
    uniforms.maxNumLightsPerTile = GetMaxNumLightsPerTile();

    cullerUniformsCreated = true;

    memcpy_s( ae3d::GfxDevice::GetCurrentMappedConstantBuffer(), AE3D_CB_SIZE, &uniforms, sizeof( PerObjectUboStruct ) );

    shader.SetUniformBuffer( 0, (ID3D12Resource*)GfxDevice::GetCurrentConstantBuffer() );
    shader.SetTextureBuffer( 0, pointLightCenterAndRadiusBuffer );
    shader.SetTextureBuffer( 1, depthNormalTarget.GetGpuResource()->resource );
    shader.SetTextureBuffer( 2, spotLightCenterAndRadiusBuffer );
    shader.SetUAVBuffer( 0, perTileLightIndexBuffer );

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
