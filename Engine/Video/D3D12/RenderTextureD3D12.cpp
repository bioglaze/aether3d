// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "RenderTexture.hpp"
#include <map>
#include <vector>
#include "DescriptorHeapManager.hpp"
#include "GfxDevice.hpp"
#include "Macros.hpp"
#include "System.hpp"

DXGI_FORMAT FormatToDXGIFormat( ae3d::DataType format );
int GetTextureMemoryUsageBytes( int width, int height, DXGI_FORMAT format, bool hasMips );
void TransitionResource( GpuResource& gpuResource, D3D12_RESOURCE_STATES newState );

namespace GfxDeviceGlobal
{
    extern ID3D12Device* device;
    extern ID3D12GraphicsCommandList* graphicsCommandList;
}

namespace RenderTextureGlobal
{
    std::vector< ID3D12Resource* > renderTextures;

#if DEBUG
    std::map< std::string, std::size_t > pathToCachedTextureSizeInBytes;

    void PrintMemoryUsage()
    {
        std::size_t total = 0;

        for (const auto& path : pathToCachedTextureSizeInBytes)
        {
            ae3d::System::Print( "%s: %d B\n", path.first.c_str(), path.second );
            total += path.second;
        }

        ae3d::System::Print( "Total render texture usage: %d KiB\n", total / 1024 );
    }
#endif
}

void ae3d::RenderTexture::DestroyTextures()
{
    for (std::size_t i = 0; i < RenderTextureGlobal::renderTextures.size(); ++i)
    {
        AE3D_SAFE_RELEASE( RenderTextureGlobal::renderTextures[ i ] );
    }
}

void ae3d::RenderTexture::Create2D( int aWidth, int aHeight, DataType aDataType, TextureWrap aWrap, TextureFilter aFilter, const char* debugName, bool isMultisampled, RenderTexture::UavFlag uavFlag )
{
    if (aWidth <= 0 || aHeight <= 0)
    {
        System::Print( "Render texture has invalid dimension!\n" );
        return;
    }

    width = aWidth;
    height = aHeight;
    wrap = aWrap;
    filter = aFilter;
    isRenderTexture = true;
    dataType = aDataType;
    isCube = false;
    handle = 1;
	dxgiFormat = FormatToDXGIFormat( dataType );
    sampleCount = isMultisampled ? 4 : 1;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = dxgiFormat;

    D3D12_RESOURCE_DESC descTex = {};
    descTex.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    descTex.Width = width;
    descTex.Height = (UINT)height;
    descTex.DepthOrArraySize = 1;
    descTex.MipLevels = 1;
    descTex.Format = dxgiFormat;
    descTex.SampleDesc.Count = isMultisampled ? 4 : 1;
    descTex.SampleDesc.Quality = isMultisampled ? DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN : 0;
    descTex.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    descTex.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    if (uavFlag == UavFlag::Enabled)
    {
        descTex.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    HRESULT hr = GfxDeviceGlobal::device->CreateCommittedResource( &heapProps, D3D12_HEAP_FLAG_NONE, &descTex,
        D3D12_RESOURCE_STATE_COPY_DEST, &clearValue, IID_PPV_ARGS( &gpuResource.resource ) );
    AE3D_CHECK_D3D( hr, "Unable to create texture resource" );

	wchar_t wstr[ 128 ] = {};
    std::mbstowcs( wstr, debugName, 128 );

    gpuResource.resource->SetName( wstr );
    gpuResource.usageState = D3D12_RESOURCE_STATE_COPY_DEST;
    RenderTextureGlobal::renderTextures.push_back( gpuResource.resource );

    {
        rtv = DescriptorHeapManager::AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );

        D3D12_RENDER_TARGET_VIEW_DESC descRtv = {};
        descRtv.Format = dxgiFormat;
        descRtv.ViewDimension = isMultisampled ? D3D12_RTV_DIMENSION_TEXTURE2DMS : D3D12_RTV_DIMENSION_TEXTURE2D;
        GfxDeviceGlobal::device->CreateRenderTargetView( gpuResource.resource, &descRtv, rtv );
    }

    {
        D3D12_RESOURCE_DESC descDepth = {};
        descDepth.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        descDepth.Width = width;
        descDepth.Height = height;
        descDepth.DepthOrArraySize = isMultisampled ? 4 : 1;
        descDepth.MipLevels = 1;
        descDepth.Format = DXGI_FORMAT_D32_FLOAT;
        descDepth.SampleDesc.Count = isMultisampled ? 4 : 1;
        descDepth.SampleDesc.Quality = isMultisampled ? DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN : 0;
        descDepth.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        descDepth.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

        D3D12_CLEAR_VALUE dsClear = {};
        dsClear.Format = descDepth.Format;
        dsClear.DepthStencil.Depth = 1;
        dsClear.DepthStencil.Stencil = 0;

        hr = GfxDeviceGlobal::device->CreateCommittedResource( &heapProps, D3D12_HEAP_FLAG_NONE, &descDepth,
            D3D12_RESOURCE_STATE_DEPTH_WRITE, &dsClear, IID_PPV_ARGS( &gpuResourceDepth.resource ) );
        AE3D_CHECK_D3D( hr, "Unable to create texture resource" );

        gpuResourceDepth.resource->SetName( L"Render Texture Depth 2D" );
        
        dsv = DescriptorHeapManager::AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE_DSV );

        D3D12_DEPTH_STENCIL_VIEW_DESC descDsv = {};
        descDsv.Format = DXGI_FORMAT_D32_FLOAT;
        descDsv.ViewDimension = isMultisampled ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;
        GfxDeviceGlobal::device->CreateDepthStencilView( gpuResourceDepth.resource, &descDsv, dsv );
        RenderTextureGlobal::renderTextures.push_back( gpuResourceDepth.resource );

        srvDesc.Format = dxgiFormat;
        srvDesc.ViewDimension = isMultisampled ? D3D12_SRV_DIMENSION_TEXTURE2DMS : D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.PlaneSlice = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

        srv = DescriptorHeapManager::AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
        handle = static_cast< unsigned >(srv.ptr);

        GfxDeviceGlobal::device->CreateShaderResourceView( gpuResource.resource, &srvDesc, srv );
    }

#if DEBUG
    RenderTextureGlobal::pathToCachedTextureSizeInBytes[ debugName ] = (size_t)GetTextureMemoryUsageBytes( width, height, dxgiFormat, mipLevelCount > 1 );
    //RenderTextureGlobal::PrintMemoryUsage();
#endif
}

void ae3d::RenderTexture::CreateCube( int aDimension, DataType aDataType, TextureWrap aWrap, TextureFilter aFilter, const char* debugName )
{
    if (aDimension <= 0)
    {
        System::Print( "Render texture has invalid dimension!\n" );
        return;
    }
    
    width = height = aDimension;
    wrap = aWrap;
    filter = aFilter;
    isRenderTexture = true;
    dataType = aDataType;
    isCube = true;
	dxgiFormat = FormatToDXGIFormat( dataType );

    // Base resources
    {
        D3D12_RESOURCE_DESC descTex = {};
        descTex.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        descTex.Width = width;
        descTex.Height = (UINT)height;
        descTex.DepthOrArraySize = 6;
        descTex.MipLevels = 1;
        descTex.Format = dxgiFormat;
        descTex.SampleDesc.Count = 1;
        descTex.SampleDesc.Quality = 0;
        descTex.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        descTex.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = dxgiFormat;

        HRESULT hr = GfxDeviceGlobal::device->CreateCommittedResource( &heapProps, D3D12_HEAP_FLAG_NONE, &descTex,
            D3D12_RESOURCE_STATE_COPY_DEST, &clearValue, IID_PPV_ARGS( &gpuResource.resource ) );
        AE3D_CHECK_D3D( hr, "Unable to create texture resource" );

		wchar_t wstr[ 128 ] = {};
        std::mbstowcs( wstr, debugName, 128 );
        gpuResource.resource->SetName( wstr );
        gpuResource.usageState = D3D12_RESOURCE_STATE_COPY_DEST;
        RenderTextureGlobal::renderTextures.push_back( gpuResource.resource );

        srvDesc.Format = dxgiFormat;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.PlaneSlice = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

        srv = DescriptorHeapManager::AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
        handle = static_cast< unsigned >(srv.ptr);

        GfxDeviceGlobal::device->CreateShaderResourceView( gpuResource.resource, &srvDesc, srv );
    }

    for (int i = 0; i < 6; ++i)
    {
        cubeRtvs[ i ] = DescriptorHeapManager::AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );

        D3D12_RENDER_TARGET_VIEW_DESC descRtv = {};
        descRtv.Format = dxgiFormat;
        descRtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		descRtv.Texture2DArray.FirstArraySlice = i;
		descRtv.Texture2DArray.ArraySize = 1;
        GfxDeviceGlobal::device->CreateRenderTargetView( gpuResource.resource, &descRtv, cubeRtvs[ i ] );
    }

    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC descDepth = {};
        descDepth.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        descDepth.Width = width;
        descDepth.Height = height;
        descDepth.DepthOrArraySize = 1;
        descDepth.MipLevels = 1;
        descDepth.Format = DXGI_FORMAT_D32_FLOAT;
        descDepth.SampleDesc.Count = 1;
        descDepth.SampleDesc.Quality = 0;
        descDepth.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        descDepth.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

        D3D12_CLEAR_VALUE dsClear = {};
        dsClear.Format = descDepth.Format;
        dsClear.DepthStencil.Depth = 1;
        dsClear.DepthStencil.Stencil = 0;

        HRESULT hr = GfxDeviceGlobal::device->CreateCommittedResource( &heapProps, D3D12_HEAP_FLAG_NONE, &descDepth,
            D3D12_RESOURCE_STATE_DEPTH_WRITE, &dsClear, IID_PPV_ARGS( &gpuResourceDepth.resource ) );
        AE3D_CHECK_D3D( hr, "Unable to create depth resource" );

        gpuResourceDepth.resource->SetName( L"Render Texture Depth Cube" );
        RenderTextureGlobal::renderTextures.push_back( gpuResourceDepth.resource );

        for (int i = 0; i < 6; ++i)
        {
            cubeDsvs[ i ] = DescriptorHeapManager::AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE_DSV );

            D3D12_DEPTH_STENCIL_VIEW_DESC descDsv = {};
            descDsv.Format = DXGI_FORMAT_D32_FLOAT;
            descDsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            GfxDeviceGlobal::device->CreateDepthStencilView( gpuResourceDepth.resource, &descDsv, cubeDsvs[ i ] );
        }
    }
}

void ae3d::RenderTexture::ResolveTo( ae3d::RenderTexture* target )
{
    TransitionResource( gpuResource, D3D12_RESOURCE_STATE_RESOLVE_SOURCE );
    TransitionResource( *target->GetGpuResource(), D3D12_RESOURCE_STATE_RESOLVE_DEST );
    GfxDeviceGlobal::graphicsCommandList->ResolveSubresource( target->GetGpuResource()->resource, 0, gpuResource.resource, 0, FormatToDXGIFormat( dataType ) );
}