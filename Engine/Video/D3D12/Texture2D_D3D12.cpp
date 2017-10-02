#include "Texture2D.hpp"
#include <vector>
#include <map>
#include <d3d12.h>
#include <d3dx12.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.c"
#include "DescriptorHeapManager.hpp"
#include "FileWatcher.hpp"
#include "FileSystem.hpp"
#include "GfxDevice.hpp"
#include "Macros.hpp"
#include "System.hpp"
#include "DDSLoader.hpp"

extern ae3d::FileWatcher fileWatcher;
bool HasStbExtension( const std::string& path ); // Defined in TextureCommon.cpp
void TexReload( const std::string& path ); // Defined in TextureCommon.cpp
float GetFloatAnisotropy( ae3d::Anisotropy anisotropy );
void TransitionResource( GpuResource& gpuResource, D3D12_RESOURCE_STATES newState );

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
        
        ae3d::System::Print( "Total texture usage: %d KiB\n", total / 1024 );
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

        UpdateSubresources( GfxDeviceGlobal::graphicsCommandList, gpuResource.resource, uploadBuffer, 0, 0, subResourceCount, subResources );
        TransitionResource( gpuResource, D3D12_RESOURCE_STATE_GENERIC_READ );

        hr = GfxDeviceGlobal::graphicsCommandList->Close();
        AE3D_CHECK_D3D( hr, "command list close in InitializeTexture" );

        ID3D12CommandList* ppCommandLists[] = { GfxDeviceGlobal::graphicsCommandList };
        GfxDeviceGlobal::commandQueue->ExecuteCommandLists( 1, &ppCommandLists[ 0 ] );
    }
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
        descTex.Flags = D3D12_RESOURCE_FLAG_NONE;

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
    }
    
    return &Texture2DGlobal::defaultTexture;
}

void ae3d::Texture2D::LoadFromData( const void* imageData, int aWidth, int aHeight, int channels, const char* debugName )
{
    width = aWidth;
    height = aHeight;
    wrap = TextureWrap::Repeat;
    filter = TextureFilter::Linear;

    D3D12_RESOURCE_DESC descTex = {};
    descTex.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    descTex.Width = width;
    descTex.Height = static_cast< UINT >(height);
    descTex.DepthOrArraySize = 1;
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
    std::mbstowcs( wstr, debugName, 128 );
    gpuResource.resource->SetName( wstr );
    gpuResource.usageState = D3D12_RESOURCE_STATE_COPY_DEST;
    Texture2DGlobal::textures.push_back( gpuResource.resource );

    D3D12_SUBRESOURCE_DATA texResource = {};
    texResource.pData = imageData;
    texResource.RowPitch = Texture2DGlobal::defaultTexture.width * channels * 4;
    texResource.SlicePitch = texResource.RowPitch * height;
    InitializeTexture( gpuResource, &texResource, 1 );

    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    srv = DescriptorHeapManager::AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
    handle = static_cast< unsigned >(Texture2DGlobal::defaultTexture.srv.ptr);

    GfxDeviceGlobal::device->CreateShaderResourceView( gpuResource.resource, &srvDesc, srv );
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
    
    srv = DescriptorHeapManager::AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
    handle = static_cast< unsigned >( srv.ptr );
    
    GfxDeviceGlobal::device->CreateShaderResourceView( gpuResource.resource, &srvDesc, srv );

    Texture2DGlobal::hashToCachedTexture[ cacheHash ] = *this;
#if DEBUG
    Texture2DGlobal::pathToCachedTextureSizeInBytes[ fileContents.path ] = static_cast< std::size_t >(width * height * 4 * (mipmaps == Mipmaps::Generate ? 1.0f : 1.33333f));
    //Texture2DGlobal::PrintMemoryUsage();
#endif
}

void ae3d::Texture2D::LoadDDS( const char* aPath )
{
    DDSLoader::Output ddsOutput;
    auto fileContents = FileSystem::FileContents( aPath );
    const DDSLoader::LoadResult loadResult = DDSLoader::Load( fileContents, 0, width, height, opaque, ddsOutput );

    if (loadResult != DDSLoader::LoadResult::Success)
    {
        ae3d::System::Print( "DDS Loader could not load %s", aPath );
        return;
    }

    mipLevelCount = static_cast< int >( ddsOutput.dataOffsets.size() );
    int bytesPerPixel = 2;

    dxgiFormat = (colorSpace == ColorSpace::RGB) ? DXGI_FORMAT_BC1_UNORM : DXGI_FORMAT_BC1_UNORM_SRGB;

    if (ddsOutput.format == DDSLoader::Format::BC2)
    {
        dxgiFormat = (colorSpace == ColorSpace::RGB) ? DXGI_FORMAT_BC2_UNORM : DXGI_FORMAT_BC2_UNORM_SRGB;
        bytesPerPixel = 4;
    }
    if (ddsOutput.format == DDSLoader::Format::BC3)
    {
        dxgiFormat = (colorSpace == ColorSpace::RGB) ? DXGI_FORMAT_BC3_UNORM : DXGI_FORMAT_BC3_UNORM_SRGB;
        bytesPerPixel = 4;
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

    wchar_t wstr[ 128 ];
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

        texResources[ i ].pData = &ddsOutput.imageData[ ddsOutput.dataOffsets[ i - 1 ] ];
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
    dxgiFormat = colorSpace == ColorSpace::RGB ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

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

    wchar_t wstr[ 128 ];
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
