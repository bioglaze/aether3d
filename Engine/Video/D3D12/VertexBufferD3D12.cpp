#include "VertexBuffer.hpp"
#include <vector>
#include <d3d12.h>
#include "GfxDevice.hpp"
#include "Vec3.hpp"
#include "System.hpp"
#include "Macros.hpp"
#include "TextureBase.hpp"

void TransitionResource( GpuResource& gpuResource, D3D12_RESOURCE_STATES newState );

namespace GfxDeviceGlobal
{
    extern ID3D12Device* device;
    extern ID3D12GraphicsCommandList* graphicsCommandList;
}

namespace Global
{
    std::vector< ID3D12Resource* > vbs;
    std::vector< ID3D12Resource* > frameVBUploads;
}

void ae3d::VertexBuffer::DestroyBuffers()
{
    for (std::size_t i = 0; i < Global::vbs.size(); ++i)
    {
        AE3D_SAFE_RELEASE( Global::vbs[ i ] );
    }
}

unsigned ae3d::VertexBuffer::GetIBSize() const
{
    return elementCount * 2;
}

unsigned ae3d::VertexBuffer::GetStride() const
{
    if (vertexFormat == VertexFormat::PTC)
    {
        return sizeof( VertexPTC );
    }
    else if (vertexFormat == VertexFormat::PTN)
    {
        return sizeof( VertexPTN );
    }
    else if (vertexFormat == VertexFormat::PTNTC)
    {
        return sizeof( VertexPTNTC );
    }
    else
    {
        System::Assert( false, "unhandled vertex format!" );
        return sizeof( VertexPTC );
    }
}

void ae3d::VertexBuffer::SetDebugName( const char* name )
{
    if (vb)
    {
        wchar_t wname[ 128 ];
        std::mbstowcs( wname, name, 128 );
        vb->SetName( wname );
    }
}

void ae3d::VertexBuffer::UploadVB( void* faces, void* vertices, unsigned ibSize )
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
    bufferProp.Width = ibOffset + ibSize;

    sizeBytes = ibOffset + ibSize;

    HRESULT hr = GfxDeviceGlobal::device->CreateCommittedResource(
        &uploadProp,
        D3D12_HEAP_FLAG_NONE,
        &bufferProp,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS( &vb ) );
    if (FAILED( hr ))
    {
        ae3d::System::Assert( false, "Unable to create vertex buffer!\n" );
        return;
    }

    vb->SetName( L"VertexBuffer" );
    Global::vbs.push_back( vb );

    char* vbUploadPtr = nullptr;
    hr = vb->Map( 0, nullptr, reinterpret_cast<void**>(&vbUploadPtr) );
    if (FAILED( hr ))
    {
        ae3d::System::Assert( false, "Unable to map vertex buffer!\n" );
        return;
    }

    memcpy_s( vbUploadPtr, ibOffset, vertices, ibOffset );
    memcpy_s( vbUploadPtr + ibOffset, ibSize, faces, ibSize );
    vb->Unmap( 0, nullptr );
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTC* vertices, int vertexCount )
{
    vertexFormat = VertexFormat::PTC;
    elementCount = faceCount * 3;

    const int ibSize = elementCount * 2;
    ibOffset = sizeof( VertexPTC ) * vertexCount;

    UploadVB( (void*)faces, (void*)vertices, ibSize );
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTN* vertices, int vertexCount )
{
    vertexFormat = VertexFormat::PTN;
    elementCount = faceCount * 3;

    const int ibSize = elementCount * 2;
    ibOffset = sizeof( VertexPTN ) * vertexCount;

    UploadVB( (void*)faces, (void*)vertices, ibSize );
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTNTC* vertices, int vertexCount )
{
    vertexFormat = VertexFormat::PTNTC;
    elementCount = faceCount * 3;

    const int ibSize = elementCount * 2;
    ibOffset = sizeof( VertexPTNTC ) * vertexCount;

    UploadVB( (void*)faces, (void*)vertices, ibSize );
}

void ae3d::VertexBuffer::Bind() const
{
}
