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

#define AE3D_SAFE_RELEASE(x) if (x) { x->Release(); x = nullptr; }

extern ae3d::FileWatcher fileWatcher;
bool HasStbExtension( const std::string& path ); // Defined in TextureCommon.cpp

namespace GfxDeviceGlobal
{
    extern ID3D12Device* device;
    extern ID3D12DescriptorHeap* descHeapCbvSrvUav;
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

    auto cbvSrcUavDescHandle = GfxDeviceGlobal::descHeapCbvSrvUav->GetCPUDescriptorHandleForHeapStart();
    auto cbvSrvUavDescStep = GfxDeviceGlobal::device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
    cbvSrcUavDescHandle.ptr += cbvSrvUavDescStep;

    GfxDeviceGlobal::device->CreateShaderResourceView( resource, &srvDesc, cbvSrcUavDescHandle );

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

   D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
       DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_NONE,
        D3D12_TEXTURE_LAYOUT_UNKNOWN, 0 );

    auto prop = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD );

    HRESULT hr = GfxDeviceGlobal::device->CreateCommittedResource(
        &prop,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS( &resource ) );
    if (FAILED( hr ))
    {
        ae3d::System::Assert( false, "Unable to create texture resource!\n" );
        return;
    }

    Global::textures.push_back( resource );

    resource->SetName( L"Texture2D" );
    D3D12_BOX box = {};
    box.right = width;
    box.bottom = height;
    box.back = 1;
    hr = resource->WriteToSubresource( 0, &box, data, 4 * width, 4 * width * height );
    if (FAILED( hr ))
    {
        ae3d::System::Assert( false, "Unable write texture resource!" );
        return;
    }

    stbi_image_free( data );
}
