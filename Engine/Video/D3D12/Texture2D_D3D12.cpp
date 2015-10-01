#include "Texture2D.hpp"
#include <algorithm>
#include <vector>
#include <map>
#include <d3d12.h>
#include <d3dx12.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.c"
#include "GfxDevice.hpp"
#include "FileWatcher.hpp"
#include "FileSystem.hpp"
#include "System.hpp"
#include "CommandListManager.hpp"
#include "CommandContext.hpp"

#define AE3D_SAFE_RELEASE(x) if (x) { x->Release(); x = nullptr; }
#define AE3D_CHECK_D3D(x, msg) if (x != S_OK) { ae3d::System::Assert( false, msg ); }

extern ae3d::FileWatcher fileWatcher;
bool HasStbExtension( const std::string& path ); // Defined in TextureCommon.cpp

namespace GfxDeviceGlobal
{
    extern ID3D12Device* device;
    extern ID3D12DescriptorHeap* descHeapCbvSrvUav;
    extern CommandListManager commandListManager;
    extern ID3D12CommandAllocator* commandListAllocator;
}

namespace Global
{
    std::vector< ID3D12Resource* > textures;
}

void DestroyTextures()
{
    for (std::size_t i = 0; i < Global::textures.size(); ++i)
    {
        AE3D_SAFE_RELEASE( Global::textures[ i ] );
    }
}

namespace Texture2DGlobal
{
    ae3d::Texture2D defaultTexture;
    
    std::map< std::string, ae3d::Texture2D > pathToCachedTexture;
#if DEBUG
    std::map< std::string, std::size_t > pathToCachedTextureSizeInBytes;
    
    void PrintMemoryUsage()
    {
        std::size_t total = 0;
        
        for (const auto& path : pathToCachedTextureSizeInBytes)
        {
            ae3d::System::Print("%s: %d B\n", path.first.c_str(), path.second);
            total += path.second;
        }
        
        ae3d::System::Print( "Total texture usage: %d KiB\n", total / 1024 );
    }
#endif
}

void InitializeTexture( GpuResource& gpuResource, D3D12_SUBRESOURCE_DATA* data, unsigned dataSize )
{
    CommandContext& initContext = CommandContext::Begin();

    D3D12_HEAP_PROPERTIES heapProps;
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC bufferDesc;
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = dataSize;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ID3D12Resource* uploadBuffer = nullptr;

    HRESULT hr = GfxDeviceGlobal::device->CreateCommittedResource( &heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS( &uploadBuffer ) );
    AE3D_CHECK_D3D( hr, "Failed to create texture upload resource" );
    uploadBuffer->SetName( L"Texture2D Upload Buffer" );

    // copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
    initContext.TransitionResource( gpuResource, D3D12_RESOURCE_STATE_COPY_DEST );
    UpdateSubresources( initContext.graphicsCommandList, gpuResource.resource, uploadBuffer, 0, 0, 1, data );
    initContext.TransitionResource( gpuResource, D3D12_RESOURCE_STATE_GENERIC_READ );

    initContext.CloseAndExecute( true );
    //uploadBuffer->Release();
}

void TexReload( const std::string& path )
{
    auto& tex = Texture2DGlobal::pathToCachedTexture[ path ];

    tex.Load( ae3d::FileSystem::FileContents( path.c_str() ), tex.GetWrap(), tex.GetFilter(), tex.GetMipmaps(), tex.GetAnisotropy() );
}

const ae3d::Texture2D* ae3d::Texture2D::GetDefaultTexture()
{
    if (Texture2DGlobal::defaultTexture.GetWidth() == 0)
    {
        Texture2DGlobal::defaultTexture.width = 32;
        Texture2DGlobal::defaultTexture.height = 32;
// TODO: Init default texture.
    }
    
    return &Texture2DGlobal::defaultTexture;
}

void ae3d::Texture2D::Load( const FileSystem::FileContentsData& fileContents, TextureWrap aWrap, TextureFilter aFilter, Mipmaps aMipmaps, float aAnisotropy )
{
    filter = aFilter;
    wrap = aWrap;
    mipmaps = aMipmaps;
    anisotropy = aAnisotropy;
    
    if (!fileContents.isLoaded)
    {
        *this = Texture2DGlobal::defaultTexture;
        return;
    }
    
    const bool isCached = Texture2DGlobal::pathToCachedTexture.find( fileContents.path ) != Texture2DGlobal::pathToCachedTexture.end();

    if (isCached && handle == 0)
    {
        *this = Texture2DGlobal::pathToCachedTexture[ fileContents.path ];
        return;
    }
    
    // First load
    //    fileWatcher.AddFile( fileContents.path, TexReload );

    const bool isDDS = fileContents.path.find( ".dds" ) != std::string::npos || fileContents.path.find( ".DDS" ) != std::string::npos;
    
    if (HasStbExtension( fileContents.path ))
    {
        LoadSTB( fileContents );
    }
    else if (isDDS)
    {
        ae3d::System::Assert( false, ".dds loading not implemented in d3d12!\n" );
        LoadDDS( fileContents.path.c_str() );
    }
    else
    {
        ae3d::System::Print("Unknown texture file extension: %s\n", fileContents.path.c_str() );
    }
    
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1; 
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    srv = GfxDeviceGlobal::descHeapCbvSrvUav->GetCPUDescriptorHandleForHeapStart();
    auto cbvSrvUavDescStep = GfxDeviceGlobal::device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
    srv.ptr += cbvSrvUavDescStep;

    GfxDeviceGlobal::device->CreateShaderResourceView( gpuResource.resource, &srvDesc, srv );

    if (mipmaps == Mipmaps::Generate)
    {
    }

    Texture2DGlobal::pathToCachedTexture[ fileContents.path ] = *this;
#if DEBUG
    Texture2DGlobal::pathToCachedTextureSizeInBytes[ fileContents.path ] = static_cast< std::size_t >(width * height * 4 * (mipmaps == Mipmaps::Generate ? 1.0f : 1.33333f));
    //Texture2DGlobal::PrintMemoryUsage();
#endif
}

void ae3d::Texture2D::LoadDDS( const char* path )
{
}

void ae3d::Texture2D::LoadSTB( const FileSystem::FileContentsData& fileContents )
{
    int components;
    unsigned char* data = stbi_load_from_memory( &fileContents.data[ 0 ], static_cast<int>(fileContents.data.size()), &width, &height, &components, 4 );
    System::Assert( width > 0 && height > 0, "Invalid texture dimension" );

    if (data == nullptr)
    {
        const std::string reason( stbi_failure_reason() );
        System::Print( "%s failed to load. stb_image's reason: %s\n", fileContents.path.c_str(), reason.c_str() );
        return;
    }
  
    opaque = (components == 3 || components == 1);

    gpuResource.usageState = D3D12_RESOURCE_STATE_COMMON;

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = width;
    texDesc.Height = (UINT)height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES heapProps;
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT; // was upload
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    HRESULT hr = GfxDeviceGlobal::device->CreateCommittedResource( &heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
                 D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS( &gpuResource.resource ) );
    AE3D_CHECK_D3D( hr, "Unable to create texture resource" );
    
    gpuResource.resource->SetName( L"Texture2D" );
    Global::textures.push_back( gpuResource.resource );

    const int bytesPerPixel = 4;
    D3D12_SUBRESOURCE_DATA texResource;
    texResource.pData = data;
    texResource.RowPitch = width * bytesPerPixel;
    texResource.SlicePitch = texResource.RowPitch * height;

    InitializeTexture( gpuResource, &texResource, width * height * 4 );

    stbi_image_free( data );
}
