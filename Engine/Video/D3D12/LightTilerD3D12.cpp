// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
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

extern int AE3D_CB_SIZE;

using namespace ae3d;

namespace GfxDeviceGlobal
{
    extern unsigned backBufferWidth;
    extern unsigned backBufferHeight;
    extern ID3D12Device* device;
    extern ID3D12Resource* uav0;
    extern ID3D12Resource* uav1;
    extern D3D12_UNORDERED_ACCESS_VIEW_DESC uav0Desc;
    extern D3D12_UNORDERED_ACCESS_VIEW_DESC uav1Desc;
}

void ae3d::LightTiler::DestroyBuffers()
{
    AE3D_SAFE_RELEASE( pointLightCenterAndRadiusBuffer );
    AE3D_SAFE_RELEASE( pointLightColorBuffer );
    AE3D_SAFE_RELEASE( spotLightCenterAndRadiusBuffer );
    AE3D_SAFE_RELEASE( spotLightColorBuffer );
    AE3D_SAFE_RELEASE( spotLightParamsBuffer );
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
        GfxDeviceGlobal::uav0 = perTileLightIndexBuffer;
        GfxDeviceGlobal::uav0Desc.Format = DXGI_FORMAT_UNKNOWN;
        GfxDeviceGlobal::uav0Desc.Buffer.NumElements = maxNumLightsPerTile * numTiles;
        GfxDeviceGlobal::uav0Desc.Buffer.StructureByteStride = 4;
        GfxDeviceGlobal::uav0Desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
        GfxDeviceGlobal::uav0Desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

        // FIXME temp, create from texture
        GfxDeviceGlobal::uav1 = perTileLightIndexBuffer;
        GfxDeviceGlobal::uav1Desc.Format = DXGI_FORMAT_UNKNOWN;
        GfxDeviceGlobal::uav1Desc.Buffer.NumElements = maxNumLightsPerTile * numTiles;
        GfxDeviceGlobal::uav1Desc.Buffer.StructureByteStride = 4;
        GfxDeviceGlobal::uav1Desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
        GfxDeviceGlobal::uav1Desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    }

    // Point light center/radius buffer
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

        pointLightCenterAndRadiusBuffer->SetName( L"LightTiler point light center/radius buffer" );
    }

    // Point light color buffer
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
            IID_PPV_ARGS( &pointLightColorBuffer ) );
        if (FAILED( hr ))
        {
            ae3d::System::Assert( false, "Unable to create point light buffer!" );
            return;
        }

        pointLightColorBuffer->SetName( L"LightTiler point light color buffer" );
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

    // Spot light color buffer
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
            IID_PPV_ARGS( &spotLightColorBuffer ) );
        if (FAILED( hr ))
        {
            ae3d::System::Assert( false, "Unable to create spot light buffer!" );
            return;
        }

        spotLightColorBuffer->SetName( L"LightTiler spot light color buffer" );
    }

    // Spot light params buffer
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
	        IID_PPV_ARGS( &spotLightParamsBuffer ) );
        if ( FAILED( hr ) )
        {
	        ae3d::System::Assert( false, "Unable to create spot light params buffer!" );
	        return;
        }

        spotLightParamsBuffer->SetName( L"LightTiler spot light params buffer" );
    }
}

void ae3d::LightTiler::UpdateLightBuffers()
{
    const std::size_t byteSize = MaxLights * 4 * sizeof( float );

    {
        char* pointLightPtr = nullptr;
        HRESULT hr = pointLightCenterAndRadiusBuffer->Map( 0, nullptr, reinterpret_cast<void**>(&pointLightPtr) );

        if (FAILED( hr ))
        {
            ae3d::System::Assert( false, "Unable to map point light buffer!\n" );
            return;
        }

        memcpy_s( pointLightPtr, byteSize, &pointLightCenterAndRadius[ 0 ], byteSize );
        pointLightCenterAndRadiusBuffer->Unmap( 0, nullptr );
    }

    {
        char* pointLightPtr = nullptr;
        HRESULT hr = pointLightColorBuffer->Map( 0, nullptr, reinterpret_cast<void**>(&pointLightPtr) );

        if (FAILED( hr ))
        {
            ae3d::System::Assert( false, "Unable to map point light buffer!\n" );
            return;
        }

        memcpy_s( pointLightPtr, byteSize, &pointLightColors[ 0 ], byteSize );
        pointLightColorBuffer->Unmap( 0, nullptr );
    }

    {
        char* spotLightPtr = nullptr;
        HRESULT hr = spotLightCenterAndRadiusBuffer->Map( 0, nullptr, reinterpret_cast<void**>(&spotLightPtr) );

        if (FAILED( hr ))
        {
            ae3d::System::Assert( false, "Unable to map spot light buffer!\n" );
            return;
        }

        memcpy_s( spotLightPtr, byteSize, &spotLightCenterAndRadius[ 0 ], byteSize );
        spotLightCenterAndRadiusBuffer->Unmap( 0, nullptr );
    }

    {
        char* spotLightPtr = nullptr;
        HRESULT hr = spotLightColorBuffer->Map( 0, nullptr, reinterpret_cast<void**>( &spotLightPtr ) );

        if ( FAILED( hr ) )
        {
            ae3d::System::Assert( false, "Unable to map spot light buffer!\n" );
            return;
        }

        memcpy_s( spotLightPtr, byteSize, &spotLightColors[ 0 ], byteSize );
        spotLightColorBuffer->Unmap( 0, nullptr );
    }

    {
        char* spotLightPtr = nullptr;
        HRESULT hr = spotLightParamsBuffer->Map( 0, nullptr, reinterpret_cast<void**>( &spotLightPtr ) );

        if ( FAILED( hr ) )
        {
            ae3d::System::Assert( false, "Unable to map spot light buffer!\n" );
            return;
        }

        memcpy_s( spotLightPtr, byteSize, &spotLightParams[ 0 ], byteSize );
        spotLightParamsBuffer->Unmap( 0, nullptr );
    }
}

void ae3d::LightTiler::CullLights( ComputeShader& shader, const Matrix44& projection, const Matrix44& localToView, RenderTexture& depthNormalTarget )
{
    GfxDevice::GetNewUniformBuffer();

    PerObjectUboStruct uniforms;

    Matrix44::Invert( projection, uniforms.clipToView );

    uniforms.localToView = localToView;
    uniforms.windowWidth = depthNormalTarget.GetWidth();
    uniforms.windowHeight = depthNormalTarget.GetHeight();
    uniforms.numLights = (((unsigned)activeSpotLights & 0xFFFFu) << 16) | ((unsigned)activePointLights & 0xFFFFu);
    uniforms.maxNumLightsPerTile = GetMaxNumLightsPerTile();

    memcpy_s( ae3d::GfxDevice::GetCurrentMappedConstantBuffer(), AE3D_CB_SIZE, &uniforms, sizeof( PerObjectUboStruct ) );

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc0 = {};
    srvDesc0.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srvDesc0.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc0.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc0.Buffer.FirstElement = 0;
    srvDesc0.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    srvDesc0.Buffer.NumElements = 2048;
    srvDesc0.Buffer.StructureByteStride = 0;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc1 = {};
    srvDesc1.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srvDesc1.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc1.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc1.Texture2D.MipLevels = 1;
    srvDesc1.Texture2D.MostDetailedMip = 0;
    srvDesc1.Texture2D.PlaneSlice = 0;
    srvDesc1.Texture2D.ResourceMinLODClamp = 0.0f;

    shader.SetSRV( 0, depthNormalTarget.GetGpuResource()->resource, srvDesc1 );
    shader.SetSRV( 1, depthNormalTarget.GetGpuResource()->resource, srvDesc1 ); // Unused, but something must be bound.
    shader.SetSRV( 2, depthNormalTarget.GetGpuResource()->resource, srvDesc1 ); // Unused, but something must be bound.
    shader.SetSRV( 3, depthNormalTarget.GetGpuResource()->resource, srvDesc1 ); // Unused, but something must be bound.
    shader.SetSRV( 4, depthNormalTarget.GetGpuResource()->resource, srvDesc1 ); // Unused, but something must be bound.
    shader.SetSRV( 5, pointLightCenterAndRadiusBuffer, srvDesc0 );
    shader.SetSRV( 6, depthNormalTarget.GetGpuResource()->resource, srvDesc1 ); // Unused, but something must be bound.
    shader.SetSRV( 7, spotLightCenterAndRadiusBuffer, srvDesc0 );
    shader.SetSRV( 8, depthNormalTarget.GetGpuResource()->resource, srvDesc1 ); // Unused, but something must be bound.
    shader.SetSRV( 9, depthNormalTarget.GetGpuResource()->resource, srvDesc1 ); // Unused, but something must be bound.


    shader.SetUAV( 0, perTileLightIndexBuffer, GfxDeviceGlobal::uav0Desc );
    shader.SetUAV( 1, perTileLightIndexBuffer, GfxDeviceGlobal::uav0Desc ); // Unused, but something must be bound.

    shader.Dispatch( GetNumTilesX(), GetNumTilesY(), 1, "LightCuller" );
}

unsigned ae3d::LightTiler::GetNumTilesX() const
{
    return (unsigned)((GfxDeviceGlobal::backBufferWidth + TileRes - 1) / (float)TileRes);
}

unsigned ae3d::LightTiler::GetNumTilesY() const
{
    return (unsigned)((GfxDeviceGlobal::backBufferHeight + TileRes - 1) / (float)TileRes);
}
