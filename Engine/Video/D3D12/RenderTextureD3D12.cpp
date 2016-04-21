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

void ae3d::RenderTexture::Create2D( int aWidth, int aHeight, DataType /*dataType*/, TextureWrap aWrap, TextureFilter aFilter )
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
        descDepth.Height = (UINT)height;
        descDepth.DepthOrArraySize = 1;
        descDepth.MipLevels = 1;
        descDepth.Format = DXGI_FORMAT_D32_FLOAT;
        descDepth.SampleDesc.Count = 1;
        descDepth.SampleDesc.Quality = 0;
        descDepth.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        descDepth.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        hr = GfxDeviceGlobal::device->CreateCommittedResource( &heapProps, D3D12_HEAP_FLAG_NONE, &descDepth,
            D3D12_RESOURCE_STATE_DEPTH_WRITE, nullptr, IID_PPV_ARGS( &gpuResourceDepth.resource ) );
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

void ae3d::RenderTexture::CreateCube( int aDimension, DataType /*dataType*/, TextureWrap aWrap, TextureFilter aFilter )
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
}