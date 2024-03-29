// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "Texture2D.hpp"
#include <vector>
#include <map>
#include <d3d12.h>
#define D3DX12_NO_STATE_OBJECT_HELPERS
#include <d3dx12.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.c"
#include "DescriptorHeapManager.hpp"
#include "DDSLoader.hpp"
#include "FileWatcher.hpp"
#include "FileSystem.hpp"
#include "GfxDevice.hpp"
#include "Macros.hpp"
#include "System.hpp"
#include "Statistics.hpp"

extern ae3d::FileWatcher fileWatcher;
bool HasStbExtension( const std::string& path ); // Defined in TextureCommon.cpp
void TexReload( const std::string& path ); // Defined in TextureCommon.cpp
float GetFloatAnisotropy( ae3d::Anisotropy anisotropy );
void TransitionResource( GpuResource& gpuResource, D3D12_RESOURCE_STATES newState );

struct Textures
{
    std::vector< ae3d::Texture2D* > pointers;
};

std::map< std::string, Textures > textures;

namespace MathUtil
{
    int GetMipmapCount( int width, int height );
    int Max( int a, int b );
}

namespace GfxDeviceGlobal
{
    extern ID3D12Device* device;
    extern ID3D12CommandAllocator* commandListAllocator;
    extern ID3D12GraphicsCommandList* graphicsCommandList;
    extern ID3D12CommandQueue* commandQueue;
}

namespace Texture2DGlobal
{
    std::vector< ID3D12Resource* > textures;
    std::vector< ID3D12Resource* > uploadBuffers;
    ae3d::Texture2D defaultTexture;
    int tex2dMemoryUsage = 0;

    std::map< std::string, ae3d::Texture2D > hashToCachedTexture;
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
        
        ae3d::System::Print( "Total texture 2D usage: %d KiB\n", total / 1024 );
    }
#endif
}

void ae3d::Texture2D::DestroyTextures()
{
    for (std::size_t i = 0; i < Texture2DGlobal::textures.size(); ++i)
    {
        AE3D_SAFE_RELEASE( Texture2DGlobal::textures[ i ] );
    }

    for (std::size_t i = 0; i < Texture2DGlobal::uploadBuffers.size(); ++i)
    {
        AE3D_SAFE_RELEASE( Texture2DGlobal::uploadBuffers[ i ] );
    }
}

void InitializeTexture( GpuResource& gpuResource, D3D12_SUBRESOURCE_DATA* subResources, int subResourceCount )
{
    D3D12_HEAP_PROPERTIES heapProps;
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    ID3D12Resource* uploadBuffer = nullptr;
    const UINT64 uploadBufferSize = GetRequiredIntermediateSize( gpuResource.resource, 0, subResourceCount );

    const auto buffer = CD3DX12_RESOURCE_DESC::Buffer( uploadBufferSize );
    HRESULT hr = GfxDeviceGlobal::device->CreateCommittedResource( &heapProps, D3D12_HEAP_FLAG_NONE, &buffer,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS( &uploadBuffer ) );
    AE3D_CHECK_D3D( hr, "Failed to create texture upload resource" );

    if (hr == S_OK)
    {
        uploadBuffer->SetName( L"Texture Upload Buffer" );
        Texture2DGlobal::uploadBuffers.push_back( uploadBuffer );

        hr = GfxDeviceGlobal::graphicsCommandList->Reset( GfxDeviceGlobal::commandListAllocator, nullptr );
        AE3D_CHECK_D3D( hr, "command list reset in InitializeTexture" );

        if (subResources->pData)
        {
            UpdateSubresources( GfxDeviceGlobal::graphicsCommandList, gpuResource.resource, uploadBuffer, 0, 0, subResourceCount, subResources );
        }

        TransitionResource( gpuResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE );

        hr = GfxDeviceGlobal::graphicsCommandList->Close();
        AE3D_CHECK_D3D( hr, "command list close in InitializeTexture" );

        ID3D12CommandList* ppCommandLists[] = { GfxDeviceGlobal::graphicsCommandList };
        GfxDeviceGlobal::commandQueue->ExecuteCommandLists( 1, &ppCommandLists[ 0 ] );
    }
}

void ae3d::Texture2D::SetLayout( ae3d::TextureLayout layout )
{
    if (layout == TextureLayout::ShaderRead)
    {
        TransitionResource( gpuResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE );
    }
    else if (layout == TextureLayout::ShaderReadWrite)
    {
        TransitionResource( gpuResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS );
    }
    else if (layout == TextureLayout::General)
    {
        TransitionResource( gpuResource, D3D12_RESOURCE_STATE_COMMON );
    }
    else if (layout == TextureLayout::UAVBarrier)
    {
        D3D12_RESOURCE_BARRIER barrierDesc = {};
        barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrierDesc.UAV.pResource = gpuResource.resource;

        GfxDeviceGlobal::graphicsCommandList->ResourceBarrier( 1, &barrierDesc );
    }
}

void ae3d::Texture2D::SetLayouts( Texture2D* texture2ds[], TextureLayout layouts[], int count )
{
    ae3d::System::Assert( count < 5, "Count too large!" );

    int realCount = 0;
    D3D12_RESOURCE_BARRIER barriers[ 5 ] = {};

    for (int i = 0; i < count; ++i)
    {
        D3D12_RESOURCE_STATES oldState = texture2ds[ i ]->GetGpuResource()->usageState;
        D3D12_RESOURCE_STATES newState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        
        if (layouts[ i ] == TextureLayout::ShaderReadWrite)
        {
            newState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        }
        else if (layouts[ i ] == TextureLayout::General)
        {
            newState = D3D12_RESOURCE_STATE_COMMON;
        }

        if (oldState == newState)
        {
            continue;
        }

        texture2ds[ i ]->GetGpuResource()->usageState = newState;

        barriers[ realCount ].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[ realCount ].Transition.pResource = texture2ds[ i ]->GetGpuResource()->resource;
        barriers[ realCount ].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barriers[ realCount ].Transition.StateBefore = oldState;
        barriers[ realCount ].Transition.StateAfter = newState;
        barriers[ realCount ].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        
        ++realCount;
    }

    Statistics::IncBarrierCalls();
    GfxDeviceGlobal::graphicsCommandList->ResourceBarrier( realCount, &barriers[ 0 ] );
}

ae3d::Texture2D* ae3d::Texture2D::GetDefaultTexture()
{
    if (Texture2DGlobal::defaultTexture.GetWidth() == 0)
    {
        Texture2DGlobal::defaultTexture.width = 128;
        Texture2DGlobal::defaultTexture.height = 128;

        D3D12_RESOURCE_DESC descTex = {};
        descTex.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        descTex.Width = Texture2DGlobal::defaultTexture.width;
        descTex.Height = static_cast< UINT >(Texture2DGlobal::defaultTexture.height);
        descTex.DepthOrArraySize = 1;
        descTex.MipLevels = 1;
        descTex.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        descTex.SampleDesc.Count = 1;
        descTex.SampleDesc.Quality = 0;
        descTex.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        descTex.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        HRESULT hr = GfxDeviceGlobal::device->CreateCommittedResource( &heapProps, D3D12_HEAP_FLAG_NONE, &descTex,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS( &Texture2DGlobal::defaultTexture.gpuResource.resource ) );
        AE3D_CHECK_D3D( hr, "Unable to create texture resource" );

        Texture2DGlobal::defaultTexture.gpuResource.resource->SetName( L"default texture 2D" );
        Texture2DGlobal::defaultTexture.gpuResource.usageState = D3D12_RESOURCE_STATE_COPY_DEST;
        Texture2DGlobal::textures.push_back( Texture2DGlobal::defaultTexture.gpuResource.resource );

        const int bytesPerPixel = 4;
        std::vector< std::uint8_t > data( Texture2DGlobal::defaultTexture.width * Texture2DGlobal::defaultTexture.height * bytesPerPixel );
        for (std::size_t i = 0; i < data.size() / 4; ++i)
        {
            data[ i * 4 + 0 ] = 255;
            data[ i * 4 + 1 ] = 0;
            data[ i * 4 + 2 ] = 255;
            data[ i * 4 + 3 ] = 255;
        }

        D3D12_SUBRESOURCE_DATA texResource = {};
        texResource.pData = data.data();
        texResource.RowPitch = Texture2DGlobal::defaultTexture.width * bytesPerPixel;
        texResource.SlicePitch = texResource.RowPitch * Texture2DGlobal::defaultTexture.height;
        InitializeTexture( Texture2DGlobal::defaultTexture.gpuResource, &texResource, 1 );

        Texture2DGlobal::defaultTexture.srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        Texture2DGlobal::defaultTexture.srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        Texture2DGlobal::defaultTexture.srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        Texture2DGlobal::defaultTexture.srvDesc.Texture2D.MipLevels = 1;
        Texture2DGlobal::defaultTexture.srvDesc.Texture2D.MostDetailedMip = 0;
        Texture2DGlobal::defaultTexture.srvDesc.Texture2D.PlaneSlice = 0;
        Texture2DGlobal::defaultTexture.srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

        Texture2DGlobal::defaultTexture.srv = DescriptorHeapManager::AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
        Texture2DGlobal::defaultTexture.handle = static_cast< unsigned >(Texture2DGlobal::defaultTexture.srv.ptr);

        GfxDeviceGlobal::device->CreateShaderResourceView( Texture2DGlobal::defaultTexture.gpuResource.resource, &Texture2DGlobal::defaultTexture.srvDesc, Texture2DGlobal::defaultTexture.srv );

        Texture2DGlobal::defaultTexture.uav = DescriptorHeapManager::AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

        Texture2DGlobal::defaultTexture.uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        Texture2DGlobal::defaultTexture.uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        Texture2DGlobal::defaultTexture.uavDesc.Texture2D.MipSlice = 0;
        Texture2DGlobal::defaultTexture.uavDesc.Texture2D.PlaneSlice = 0;

        GfxDeviceGlobal::device->CreateUnorderedAccessView( Texture2DGlobal::defaultTexture.gpuResource.resource, nullptr, &Texture2DGlobal::defaultTexture.uavDesc, Texture2DGlobal::defaultTexture.uav );
    }
    
    return &Texture2DGlobal::defaultTexture;
}

ae3d::Texture2D* ae3d::Texture2D::GetDefaultTextureUAV()
{
    return GetDefaultTexture();
}

DXGI_FORMAT FormatToDXGIFormat( ae3d::DataType format )
{
	DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN;

	if( format == ae3d::DataType::Float )
	{
		dxgiFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
	}
	else if( format == ae3d::DataType::Float16 )
	{
		dxgiFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	}
	else if( format == ae3d::DataType::R32F )
	{
		dxgiFormat = DXGI_FORMAT_R32_FLOAT;
	}
	else if( format == ae3d::DataType::R32G32 )
	{
		dxgiFormat = DXGI_FORMAT_R32G32_FLOAT;
	}
	else if( format == ae3d::DataType::UByte )
	{
		dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	}
	else
	{
		ae3d::System::Assert( false, "unhandled format!" );
	}

	return dxgiFormat;
}

void ae3d::Texture2D::CreateUAV( int aWidth, int aHeight, const char* debugName, DataType format, const void* imageData )
{
    createUAV = true;
    LoadFromData( imageData, aWidth, aHeight, debugName, format );

    uav = DescriptorHeapManager::AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

    uavDesc.Format = FormatToDXGIFormat( format );
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;
    uavDesc.Texture2D.PlaneSlice = 0;

    GfxDeviceGlobal::device->CreateUnorderedAccessView( gpuResource.resource, nullptr, &uavDesc, uav );
}

void ae3d::Texture2D::LoadFromData( const void* imageData, int aWidth, int aHeight, const char* debugName, DataType format )
{
	width = aWidth;
	height = aHeight;
	wrap = TextureWrap::Repeat;
	filter = TextureFilter::Linear;

	D3D12_RESOURCE_DESC descTex = {};
	descTex.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	descTex.Width = width;
	descTex.Height = static_cast< UINT >( height );
	descTex.DepthOrArraySize = 1;
	descTex.MipLevels = 1;
	descTex.Format = FormatToDXGIFormat( format );
	descTex.SampleDesc.Count = 1;
	descTex.SampleDesc.Quality = 0;
	descTex.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	descTex.Flags = createUAV ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;

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
    std::mbstowcs( wstr, debugName, 128 );
    gpuResource.resource->SetName( wstr );
    gpuResource.usageState = D3D12_RESOURCE_STATE_COPY_DEST;
    Texture2DGlobal::textures.push_back( gpuResource.resource );

	int rowPitch = 0;
	if( format == DataType::Float)
	{
		rowPitch = width * 4 * sizeof( float );
	}
	else if( format == DataType::R32G32 )
	{
		rowPitch = width * 2 * sizeof( float );
	}
	else if( format == DataType::Float16 )
	{
		rowPitch = width * 4 * sizeof( float ) / 2;
	}
	else if( format == DataType::UByte )
	{
		rowPitch = width * 4;
	}
	else if( format == DataType::R32F )
	{
		rowPitch = width * 1 * sizeof( float );
	}
	else
	{
		System::Assert( false, "unhandled format!" );
	}

    D3D12_SUBRESOURCE_DATA texResource = {};
    texResource.pData = imageData;
    texResource.RowPitch = rowPitch;
    texResource.SlicePitch = texResource.RowPitch * height;
    InitializeTexture( gpuResource, &texResource, 1 );

    srvDesc.Format = descTex.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    srv = DescriptorHeapManager::AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
    handle = static_cast< unsigned >(srv.ptr);

    GfxDeviceGlobal::device->CreateShaderResourceView( gpuResource.resource, &srvDesc, srv );
}

int GetTextureMemoryUsageBytes( int width, int height, DXGI_FORMAT format, bool hasMips )
{
    int bytesPerPixel = 2;

    if (format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB || format == DXGI_FORMAT_R8G8B8A8_UNORM)
    {
        bytesPerPixel = 4;
    }
    else if (format == DXGI_FORMAT_R32G32B32A32_FLOAT)
    {
        bytesPerPixel = 4 * 3;
    }
    else if (format == DXGI_FORMAT_BC1_UNORM || format == DXGI_FORMAT_BC1_UNORM_SRGB)
    {
        bytesPerPixel = 2;
    }
    else if (format == DXGI_FORMAT_BC2_UNORM || format == DXGI_FORMAT_BC2_UNORM_SRGB)
    {
        bytesPerPixel = 4;
    }
    else if (format == DXGI_FORMAT_BC3_UNORM || format == DXGI_FORMAT_BC3_UNORM_SRGB)
    {
        bytesPerPixel = 4;
    }
    else if (format == DXGI_FORMAT_BC4_UNORM || format == DXGI_FORMAT_BC4_SNORM)
    {
        bytesPerPixel = 2;
    }
    else if (format == DXGI_FORMAT_BC5_UNORM || format == DXGI_FORMAT_BC5_SNORM)
    {
        bytesPerPixel = 4;
    }
    else if (format == DXGI_FORMAT_R32G32_FLOAT)
    {
        bytesPerPixel = 4 * 2; // TODO: verify
    }
    else
    {
        ae3d::System::Assert( false, "Unhandled DDS format!" );
    }

    return (int)(width * height * bytesPerPixel * (hasMips ? 1.0f : 1.33333f));
}

void ae3d::Texture2D::Load( const FileSystem::FileContentsData& fileContents, TextureWrap aWrap, TextureFilter aFilter, Mipmaps aMipmaps, ColorSpace aColorSpace, Anisotropy aAnisotropy )
{
    filter = aFilter;
    wrap = aWrap;
    mipmaps = aMipmaps;
    anisotropy = aAnisotropy;
    colorSpace = aColorSpace;
    path = fileContents.path;

    if (!fileContents.isLoaded)
    {
        *this = *Texture2D::GetDefaultTexture();
        return;
    }
    
    const std::string cacheHash = GetCacheHash( fileContents.path, aWrap, aFilter, aMipmaps, aColorSpace, aAnisotropy );
    const bool isCached = Texture2DGlobal::hashToCachedTexture.find( cacheHash ) != Texture2DGlobal::hashToCachedTexture.end();

    if (isCached && handle == 0)
    {
        *this = Texture2DGlobal::hashToCachedTexture[ cacheHash ];
        textures[ cacheHash ].pointers.push_back( this );
        return;
    }
    
    // First load
    if (handle == 0)
    {
        fileWatcher.AddFile( fileContents.path, TexReload );
    }

    const bool isDDS = fileContents.path.find( ".dds" ) != std::string::npos || fileContents.path.find( ".DDS" ) != std::string::npos;
    
    if (HasStbExtension( fileContents.path ))
    {
        LoadSTB( fileContents );
    }
    else if (isDDS)
    {
        LoadDDS( fileContents.path.c_str() );
    }
    else
    {
        ae3d::System::Print("Unknown texture file extension: %s\n", fileContents.path.c_str() );
    }
    
    srvDesc.Format = dxgiFormat;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = mipLevelCount; 
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    
    const int oldHandle = handle;

    srv = DescriptorHeapManager::AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
    handle = static_cast< unsigned >( srv.ptr );
    
    GfxDeviceGlobal::device->CreateShaderResourceView( gpuResource.resource, &srvDesc, srv );

    Texture2DGlobal::hashToCachedTexture[ cacheHash ] = *this;

    if (oldHandle != 0)
    {
        for (std::size_t i = 0; i < textures[ cacheHash ].pointers.size(); ++i)
        {
            *textures[ cacheHash ].pointers[ i ] = *this;
        }
    }
    else
    {
        textures[ cacheHash ].pointers.push_back( this );
    }

#if DEBUG
    Texture2DGlobal::pathToCachedTextureSizeInBytes[ fileContents.path ] = (size_t)GetTextureMemoryUsageBytes( width, height, dxgiFormat, mipLevelCount > 1 );
    Texture2DGlobal::PrintMemoryUsage();
#endif
}

void ae3d::Texture2D::LoadDDS( const char* aPath )
{
    DDSLoader::Output ddsOutput;
    auto fileContents = FileSystem::FileContents( aPath );
    const DDSLoader::LoadResult loadResult = DDSLoader::Load( fileContents, width, height, opaque, ddsOutput );

    if (loadResult != DDSLoader::LoadResult::Success)
    {
        ae3d::System::Print( "DDS Loader could not load %s", aPath );
        return;
    }

	mipLevelCount = ddsOutput.dataOffsets.count;
    int bytesPerPixel = 2;

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

    D3D12_RESOURCE_DESC descTex = {};
    descTex.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    descTex.Width = width;
    descTex.Height = static_cast< UINT >(height);
    descTex.DepthOrArraySize = 1;
    descTex.MipLevels = static_cast< UINT16 >( mipLevelCount );
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
    const D3D12_RESOURCE_ALLOCATION_INFO info = GfxDeviceGlobal::device->GetResourceAllocationInfo( 0, 1, &descTex );
    Texture2DGlobal::tex2dMemoryUsage += (int)info.SizeInBytes;

	wchar_t wstr[ 128 ] = {};
    std::mbstowcs( wstr, aPath, 128 );
    gpuResource.resource->SetName( wstr );
    gpuResource.usageState = D3D12_RESOURCE_STATE_COPY_DEST;
    Texture2DGlobal::textures.push_back( gpuResource.resource );

    std::vector< D3D12_SUBRESOURCE_DATA > texResources( mipLevelCount );
    texResources[ 0 ].pData = &ddsOutput.imageData[ ddsOutput.dataOffsets[ 0 ] ];
    texResources[ 0 ].RowPitch = width * bytesPerPixel;
    texResources[ 0 ].SlicePitch = texResources[ 0 ].RowPitch * height;

    unsigned mipWidth = width;
    unsigned mipHeight = height;

    for (int i = 1; i < mipLevelCount; ++i)
    {
        mipWidth = (mipWidth + 1) >> 1;
        mipHeight = (mipHeight + 1) >> 1;

        texResources[ i ].pData = &ddsOutput.imageData[ ddsOutput.dataOffsets[ i ] ];
        texResources[ i ].RowPitch = mipWidth * bytesPerPixel;
        texResources[ i ].SlicePitch = texResources[ i ].RowPitch * mipHeight;
    }

    InitializeTexture( gpuResource, texResources.data(), mipLevelCount );
}

void ae3d::Texture2D::LoadSTB( const FileSystem::FileContentsData& fileContents )
{
    int components;
    unsigned char* data = stbi_load_from_memory( fileContents.data.data(), static_cast<int>(fileContents.data.size()), &width, &height, &components, 4 );
    System::Assert( width > 0 && height > 0, "Invalid texture dimension" );

    if (data == nullptr)
    {
        const std::string reason( stbi_failure_reason() );
        System::Print( "%s failed to load. stb_image's reason: %s\n", fileContents.path.c_str(), reason.c_str() );
        return;
    }

    opaque = (components == 3 || components == 1);
    mipLevelCount = mipmaps == Mipmaps::Generate ? MathUtil::GetMipmapCount( width, height ) : 1;
    dxgiFormat = colorSpace == ColorSpace::Linear ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

    D3D12_RESOURCE_DESC descTex = {};
    descTex.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    descTex.Width = width;
    descTex.Height = static_cast< UINT >( height );
    descTex.DepthOrArraySize = 1;
    descTex.MipLevels = static_cast< UINT16 >( mipLevelCount );
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
    const D3D12_RESOURCE_ALLOCATION_INFO info = GfxDeviceGlobal::device->GetResourceAllocationInfo( 0, 1, &descTex );
    Texture2DGlobal::tex2dMemoryUsage += (int)info.SizeInBytes;

	wchar_t wstr[ 128 ] = {};
    std::mbstowcs( wstr, fileContents.path.c_str(), 128 );
    gpuResource.resource->SetName( wstr );
    gpuResource.usageState = D3D12_RESOURCE_STATE_COPY_DEST;
    Texture2DGlobal::textures.push_back( gpuResource.resource );

    if (mipLevelCount == 1)
    {
        const int bytesPerPixel = 4;
        D3D12_SUBRESOURCE_DATA texResource = {};
        texResource.pData = data;
        texResource.RowPitch = width * bytesPerPixel;
        texResource.SlicePitch = texResource.RowPitch * height;
        InitializeTexture( gpuResource, &texResource, 1 );
    }
    else
    {
        const int bytesPerPixel = 4;
        std::vector< D3D12_SUBRESOURCE_DATA > texResources( mipLevelCount );
        texResources[ 0 ].pData = data;
        texResources[ 0 ].RowPitch = width * bytesPerPixel;
        texResources[ 0 ].SlicePitch = texResources[ 0 ].RowPitch * height;

        std::vector< std::vector< std::uint8_t > > mipLevels( mipLevelCount - 1 );
        for (std::size_t i = 1; i < mipLevelCount; ++i)
        {
            const std::int32_t mipWidth = MathUtil::Max( width >> i, 1 );
            const std::int32_t mipHeight = MathUtil::Max( height >> i, 1 );
            mipLevels[ i-1 ].resize( mipWidth * mipHeight * 4 );
        }

        for (std::size_t i = 1; i < mipLevelCount; ++i)
        {
            const std::int32_t mipWidth = MathUtil::Max( width >> i, 1 );
            const std::int32_t mipHeight = MathUtil::Max( height >> i, 1 );

            for (int mipY = 0; mipY < mipHeight; ++mipY)
            {
                for (int mipX = 0; mipX < mipWidth; ++mipX)
                {
                    const int yInBase = mipY << i;
                    const int xInBase = mipX << i;

                    const std::uint8_t red = data[ (yInBase * width + xInBase) * 4 + 0 ];
                    const std::uint8_t green = data[ (yInBase * width + xInBase) * 4 + 1 ];
                    const std::uint8_t blue = data[ (yInBase * width + xInBase) * 4 + 2 ];
                    const std::uint8_t alpha = data[ (yInBase * width + xInBase) * 4 + 3 ];

                    mipLevels[ i - 1 ][ (mipY * mipWidth + mipX) * 4 + 0 ] = red;
                    mipLevels[ i - 1 ][ (mipY * mipWidth + mipX) * 4 + 1 ] = green;
                    mipLevels[ i - 1 ][ (mipY * mipWidth + mipX) * 4 + 2 ] = blue;
                    mipLevels[ i - 1 ][ (mipY * mipWidth + mipX) * 4 + 3 ] = alpha;
                }
            }

            texResources[ i ].pData = mipLevels[ i - 1 ].data();
            texResources[ i ].RowPitch = mipWidth * bytesPerPixel;
            texResources[ i ].SlicePitch = texResources[ i ].RowPitch * mipHeight;
        }

        InitializeTexture( gpuResource, texResources.data(), mipLevelCount );
    }

    stbi_image_free( data );
}
