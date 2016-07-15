#include "RenderTexture.hpp"
#include <vector>
#include "DescriptorHeapManager.hpp"
#include "GfxDevice.hpp"
#include "Macros.hpp"
#include "System.hpp"

namespace GfxDeviceGlobal
{
    extern ID3D12Device* device;
}

namespace RenderTextureGlobal
{
    std::vector< ID3D12Resource* > renderTextures;
}

void ae3d::RenderTexture::DestroyTextures()
{
    for (std::size_t i = 0; i < RenderTextureGlobal::renderTextures.size(); ++i)
    {
        AE3D_SAFE_RELEASE( RenderTextureGlobal::renderTextures[ i ] );
    }
}

void ae3d::RenderTexture::Create2D( int aWidth, int aHeight, DataType aDataType, TextureWrap aWrap, TextureFilter aFilter )
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

    D3D12_RESOURCE_DESC descTex = {};
    descTex.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    descTex.Width = width;
    descTex.Height = (UINT)height;
    descTex.DepthOrArraySize = 1;
    descTex.MipLevels = 1;
    descTex.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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

    HRESULT hr = GfxDeviceGlobal::device->CreateCommittedResource( &heapProps, D3D12_HEAP_FLAG_NONE, &descTex,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS( &gpuResource.resource ) );
    AE3D_CHECK_D3D( hr, "Unable to create texture resource" );

    gpuResource.resource->SetName( L"Render Texture 2D" );
    RenderTextureGlobal::renderTextures.push_back( gpuResource.resource );

    {
        rtv = DescriptorHeapManager::AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );

        D3D12_RENDER_TARGET_VIEW_DESC descRtv = {};
        descRtv.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        descRtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        GfxDeviceGlobal::device->CreateRenderTargetView( gpuResource.resource, &descRtv, rtv );
    }

    {
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
        descDepth.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

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
        descDsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        GfxDeviceGlobal::device->CreateDepthStencilView( gpuResourceDepth.resource, &descDsv, dsv );
        RenderTextureGlobal::renderTextures.push_back( gpuResourceDepth.resource );
    }
}

void ae3d::RenderTexture::CreateCube( int aDimension, DataType aDataType, TextureWrap aWrap, TextureFilter aFilter )
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

    // Base resources
    {
        D3D12_RESOURCE_DESC descTex = {};
        descTex.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        descTex.Width = width;
        descTex.Height = (UINT)height;
        descTex.DepthOrArraySize = 6;
        descTex.MipLevels = 1;
        descTex.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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

        HRESULT hr = GfxDeviceGlobal::device->CreateCommittedResource( &heapProps, D3D12_HEAP_FLAG_NONE, &descTex,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS( &gpuResource.resource ) );
        AE3D_CHECK_D3D( hr, "Unable to create texture resource" );
        gpuResource.resource->SetName( L"Cube map RT base" );
        RenderTextureGlobal::renderTextures.push_back( gpuResource.resource );

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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
        descRtv.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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
        descDepth.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

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