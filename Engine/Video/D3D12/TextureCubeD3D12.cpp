// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "TextureCube.hpp"
#include <string>
#include <vector>
#define D3DX12_NO_STATE_OBJECT_HELPERS
#include <d3dx12.h>
#include "stb_image.c"
#include "DescriptorHeapManager.hpp"
#include "DDSLoader.hpp"
#include "FileSystem.hpp"
#include "GfxDevice.hpp"
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
	ae3d::TextureCube defaultTexture;
}

void ae3d::TextureCube::DestroyTextures()
{
    for (std::size_t i = 0; i < TextureCubeGlobal::textures.size(); ++i)
    {
        AE3D_SAFE_RELEASE( TextureCubeGlobal::textures[ i ] );
    }
}

ae3d::TextureCube* ae3d::TextureCube::GetDefaultTexture()
{
    if( TextureCubeGlobal::defaultTexture.srv.ptr == 0 )
    {
        struct TGAHeader
        {
            std::uint8_t idLength;
            std::uint8_t colorMap;
            std::uint8_t imageType;
            std::uint8_t colorMapSpec[ 5 ];
            std::uint16_t xOrg;
            std::uint16_t yOrg;
            std::uint16_t width;
            std::uint16_t height;
            std::uint8_t pixelDepth;
            std::uint8_t descriptor;
        };

        TGAHeader header = {};
        header.imageType = 2;
        header.width = 32;
        header.height = 32;
        header.pixelDepth = 32;
        
        std::vector< unsigned char > imageData( sizeof( TGAHeader ) + 32 * 32 * 4 );
        std::memcpy( imageData.data(), &header, sizeof( TGAHeader ) );
        
        for (int i = 0; i < 32 * 32 * 4; ++i)
        {
            imageData[ i + sizeof( TGAHeader ) ] = 0x00;
        }

        FileSystem::FileContentsData tgaData;
        tgaData.isLoaded = true;
        tgaData.path = "none.tga";
        tgaData.data = imageData;
        
        TextureCubeGlobal::defaultTexture.Load( tgaData, tgaData, tgaData, tgaData, tgaData, tgaData, TextureWrap::Clamp, TextureFilter::Linear, Mipmaps::None, ColorSpace::SRGB );
    }

    return &TextureCubeGlobal::defaultTexture;
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
    path = negX.path;
    isCube = true;
    dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    int bytesPerPixel = 2;

    // Gets dimension
    {
        const bool isDDS = posX.path.find( ".dds" ) != std::string::npos || posX.path.find( ".DDS" ) != std::string::npos;

        if (isDDS)
        {
            DDSLoader::Output ddsOutput;
            auto fileContents = FileSystem::FileContents( posX.path.c_str() );
            const DDSLoader::LoadResult loadResult = DDSLoader::Load( fileContents, width, height, opaque, ddsOutput );
            mipLevelCount = mipmaps == Mipmaps::Generate ? ddsOutput.dataOffsets.count : 1;

            if (loadResult != DDSLoader::LoadResult::Success)
            {
                ae3d::System::Print( "DDS Loader could not load %s", posX.path.c_str() );
                return;
            }

            if (ddsOutput.format == DDSLoader::Format::BC1)
            {
                dxgiFormat = (colorSpace == ColorSpace::Linear) ? DXGI_FORMAT_BC1_UNORM : DXGI_FORMAT_BC1_UNORM_SRGB;
            }
            else if (ddsOutput.format == DDSLoader::Format::BC2)
            {
                dxgiFormat = (colorSpace == ColorSpace::Linear) ? DXGI_FORMAT_BC2_UNORM : DXGI_FORMAT_BC2_UNORM_SRGB;
                bytesPerPixel = 4;
            }
            else if (ddsOutput.format == DDSLoader::Format::BC3)
            {
                dxgiFormat = (colorSpace == ColorSpace::Linear) ? DXGI_FORMAT_BC3_UNORM : DXGI_FORMAT_BC3_UNORM_SRGB;
                bytesPerPixel = 4;
            }
            else if (ddsOutput.format == DDSLoader::Format::BC4U)
            {
                dxgiFormat = DXGI_FORMAT_BC4_UNORM;
                bytesPerPixel = 2;
            }
            else if (ddsOutput.format == DDSLoader::Format::BC4S)
            {
                dxgiFormat = DXGI_FORMAT_BC4_SNORM;
                bytesPerPixel = 2;
            }
            else if (ddsOutput.format == DDSLoader::Format::BC5S)
            {
                dxgiFormat = DXGI_FORMAT_BC5_SNORM;
                bytesPerPixel = 4;
            }
            else if (ddsOutput.format == DDSLoader::Format::BC5U)
            {
                dxgiFormat = DXGI_FORMAT_BC5_UNORM;
                bytesPerPixel = 4;
            }
            else
            {
                System::Assert( false, "Unhandled DDS format!" );
            }
        }
        else
        {
            int tempComp;
            unsigned char* tempData = stbi_load_from_memory( &posX.data[ 0 ], static_cast<int>(posX.data.size()), &width, &height, &tempComp, 4 );
            stbi_image_free( tempData );
        }
    }

    const std::string paths[] = { posX.path, negX.path, negY.path, posY.path, negZ.path, posZ.path };
    const std::vector< unsigned char >* datas[] = { &posX.data, &negX.data, &negY.data, &posY.data, &negZ.data, &posZ.data };

    D3D12_RESOURCE_DESC descTex = {};
    descTex.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    descTex.Width = width;
    descTex.Height = (UINT)height;
    descTex.DepthOrArraySize = 6;
    descTex.MipLevels = (UINT16)mipLevelCount;
    descTex.Format = dxgiFormat;
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
	wchar_t wstr[ 128 ] = {};
    std::mbstowcs( wstr, posX.path.c_str(), 128 );
    gpuResource.resource->SetName( wstr );
    gpuResource.usageState = D3D12_RESOURCE_STATE_COPY_DEST;
    TextureCubeGlobal::textures.push_back( gpuResource.resource );

    unsigned char* faceData[ 6 ] = {};
    D3D12_SUBRESOURCE_DATA texResources[ 6 * 15 ] = {};

    bool isStbImage[ 6 ] = {};
    DDSLoader::Output ddsOutput[ 6 ];
    int resCounter = 0;

    for (int face = 0; face < 6; ++face)
    {
        const bool isDDS = paths[ face ].find( ".dds" ) != std::string::npos || paths[ face ].find( ".DDS" ) != std::string::npos;
        
        if( isDDS )
        {
            isStbImage[ face ] = false;

            auto fileContents = FileSystem::FileContents( paths[ face ].c_str() );
            const DDSLoader::LoadResult loadResult = DDSLoader::Load( fileContents, width, height, opaque, ddsOutput[ face ] );

            if( loadResult != DDSLoader::LoadResult::Success )
            {
                ae3d::System::Print( "DDS Loader could not load %s", paths[ face ].c_str() );
                *this = TextureCubeGlobal::defaultTexture;
                return;
            }

            opaque = true;

            unsigned mipWidth = width;
            unsigned mipHeight = height;

            for( int i = 0; i < mipLevelCount; ++i )
            {
                faceData[ face ] = &ddsOutput[ face ].imageData[ ddsOutput[ face ].dataOffsets[ i ] ];

                texResources[ resCounter ].pData = faceData[ face ];
                texResources[ resCounter ].RowPitch = mipWidth * bytesPerPixel;
                texResources[ resCounter ].SlicePitch = texResources[ resCounter ].RowPitch * mipHeight;
                ++resCounter;

                mipWidth = ( mipWidth + 1 ) >> 1;
                mipHeight = ( mipHeight + 1 ) >> 1;
           }
        }
        else if (HasStbExtension( paths[ face ] ))
        {
            isStbImage[ face ] = true;

            int components;
            faceData[ face ] = stbi_load_from_memory( datas[ face ]->data(), static_cast<int>(datas[ face ]->size()), &width, &height, &components, 4 );
            
            if (faceData[ face ] == nullptr)
            {
                const std::string reason( stbi_failure_reason() );
                System::Print( "%s failed to load. stb_image's reason: %s\n", paths[ face ].c_str(), reason.c_str() );
                *this = TextureCubeGlobal::defaultTexture;
                return;
            }
            
            opaque = (components == 3 || components == 1);

            bytesPerPixel = 4;
            texResources[ face ].pData = faceData[ face ];
            texResources[ face ].RowPitch = width * bytesPerPixel;
            texResources[ face ].SlicePitch = texResources[ face ].RowPitch * height;
        }
        else
        {
            System::Print( "Cube map face has unsupported texture extension in file: %s\n", paths[ face ].c_str() );
        }
    }

    InitializeTexture( gpuResource, &texResources[ 0 ], 6 * mipLevelCount );

    srvDesc.Format = dxgiFormat;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = mipLevelCount;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    srv = DescriptorHeapManager::AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
    handle = static_cast< unsigned >(srv.ptr);

    GfxDeviceGlobal::device->CreateShaderResourceView( gpuResource.resource, &srvDesc, srv );

    for (int face = 0; face < 6; ++face)
    {
        if (isStbImage[ face ])
        {
            stbi_image_free( faceData[ face ] );
        }
    }
}

