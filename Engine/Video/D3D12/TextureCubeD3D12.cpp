#include "TextureCube.hpp"
#include <string>
#include <vector>
#include <d3dx12.h>
#include "stb_image.c"
#include "DescriptorHeapManager.hpp"
#include "GfxDevice.hpp"
#include "FileSystem.hpp"
#include "Macros.hpp"
#include "System.hpp"

bool HasStbExtension( const std::string& path ); // Defined in TextureCommon.cpp
void TransitionResource( GpuResource& gpuResource, D3D12_RESOURCE_STATES newState );
void InitializeTexture( GpuResource& gpuResource, D3D12_SUBRESOURCE_DATA* subResources, int subResourceCount );

namespace GfxDeviceGlobal
{
    extern ID3D12Device* device;
    extern ID3D12CommandAllocator* commandListAllocator;
    extern ID3D12GraphicsCommandList* graphicsCommandList;
    extern ID3D12CommandQueue* commandQueue;
}

namespace TextureCubeGlobal
{
    std::vector< ID3D12Resource* > textures;
}

void ae3d::TextureCube::DestroyTextures()
{
    for (std::size_t i = 0; i < TextureCubeGlobal::textures.size(); ++i)
    {
        AE3D_SAFE_RELEASE( TextureCubeGlobal::textures[ i ] );
    }
}

void ae3d::TextureCube::Load( const FileSystem::FileContentsData& negX, const FileSystem::FileContentsData& posX,
          const FileSystem::FileContentsData& negY, const FileSystem::FileContentsData& posY,
          const FileSystem::FileContentsData& negZ, const FileSystem::FileContentsData& posZ,
          TextureWrap aWrap, TextureFilter aFilter, Mipmaps aMipmaps, ColorSpace aColorSpace )
{
    filter = aFilter;
    wrap = aWrap;
    mipmaps = aMipmaps;
    colorSpace = aColorSpace;

    // Gets dimension
    {
        int tempComp;
        unsigned char* tempData = stbi_load_from_memory( &posX.data[ 0 ], static_cast<int>(posX.data.size()), &width, &height, &tempComp, 4 );
        stbi_image_free( tempData );
    }

    const std::string paths[] = { posX.path, negX.path, negY.path, posY.path, negZ.path, posZ.path };
    const std::vector< unsigned char >* datas[] = { &posX.data, &negX.data, &negY.data, &posY.data, &negZ.data, &posZ.data };

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
    descTex.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    HRESULT hr = GfxDeviceGlobal::device->CreateCommittedResource( &heapProps, D3D12_HEAP_FLAG_NONE, &descTex,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS( &gpuResource.resource ) );
    AE3D_CHECK_D3D( hr, "Unable to create texture resource" );
    wchar_t wstr[ 128 ];
    std::mbstowcs( wstr, posX.path.c_str(), 128 );
    gpuResource.resource->SetName( wstr );
    gpuResource.usageState = D3D12_RESOURCE_STATE_COPY_DEST;
    TextureCubeGlobal::textures.push_back( gpuResource.resource );

    unsigned char* faceData[ 6 ] = {};
    D3D12_SUBRESOURCE_DATA texResources[ 6 ] = {};

    for (int face = 0; face < 6; ++face)
    {
        const bool isDDS = paths[ face ].find( ".dds" ) != std::string::npos || paths[ face ].find( ".DDS" ) != std::string::npos;
        
        if (HasStbExtension( paths[ face ] ))
        {
            int components;
            faceData[ face ] = stbi_load_from_memory( datas[ face ]->data(), static_cast<int>(datas[ face ]->size()), &width, &height, &components, 4 );
            
            if (faceData[ face ] == nullptr)
            {
                const std::string reason( stbi_failure_reason() );
                System::Print( "%s failed to load. stb_image's reason: %s\n", paths[ face ].c_str(), reason.c_str() );
                return;
            }
            
            opaque = (components == 3 || components == 1);

            const int bytesPerPixel = 4;
            texResources[ face ].pData = faceData[ face ];
            texResources[ face ].RowPitch = width * bytesPerPixel;
            texResources[ face ].SlicePitch = texResources[ face ].RowPitch * height;
        }
        else if (isDDS)
        {
            //LoadDDS( fileContents.path.c_str() );
        }
        else
        {
            System::Print( "Cube map face has unsupported texture extension in file: %s\n", paths[ face ].c_str() );
        }
    }

    InitializeTexture( gpuResource, &texResources[ 0 ], 6 );

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

    for (int face = 0; face < 6; ++face)
    {
        stbi_image_free( faceData[ face ] );
    }
}

