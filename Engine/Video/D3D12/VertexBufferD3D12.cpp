// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "VertexBuffer.hpp"
#include <vector>
#include <d3d12.h>
#include "GfxDevice.hpp"
#include "Vec3.hpp"
#include "System.hpp"
#include "Macros.hpp"
#include "TextureBase.hpp"

namespace GfxDeviceGlobal
{
    extern ID3D12Device* device;
    extern ID3D12GraphicsCommandList* commandList;
    extern ID3D12CommandAllocator* commandListAllocator;
    extern ID3D12CommandQueue* commandQueue;
}

namespace Global
{
    std::vector< ID3D12Resource* > vbs;
    unsigned totalBufferMemoryUsageBytes = 0;
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
    else if (vertexFormat == VertexFormat::PTNTC_Skinned)
    {
        return sizeof( VertexPTNTC_Skinned );
    }
    else
    {
        System::Assert( false, "unhandled vertex format!" );
        return sizeof( VertexPTC );
    }
}

void ae3d::VertexBuffer::SetDebugName( const char* name )
{
    if (vbUpload)
    {
		wchar_t wname[ 128 ] = {};
        std::mbstowcs( wname, name, 128 );
        vbUpload->SetName( wname );
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
        IID_PPV_ARGS( &vbUpload ) );
    if (FAILED( hr ))
    {
        ae3d::System::Assert( false, "Unable to create vertex buffer!\n" );
        return;
    }

    vbUpload->SetName( L"UploadVertexBuffer" );
    Global::vbs.push_back( vbUpload );
    Global::totalBufferMemoryUsageBytes += sizeBytes;

    D3D12_RANGE emptyRange{};
    char* vbUploadPtr = nullptr;
    hr = vbUpload->Map( 0, &emptyRange, reinterpret_cast<void**>(&vbUploadPtr) );
    if (FAILED( hr ))
    {
        ae3d::System::Assert( false, "Unable to map upload vertex buffer!\n" );
        return;
    }

    memcpy_s( vbUploadPtr, ibOffset, vertices, ibOffset );
    memcpy_s( vbUploadPtr + ibOffset, ibSize, faces, ibSize );
    vbUpload->Unmap( 0, nullptr );

    vertexBufferView.BufferLocation = vbUpload->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes = GetStride();
    vertexBufferView.SizeInBytes = GetIBOffset();

    indexBufferView.BufferLocation = vbUpload->GetGPUVirtualAddress() + GetIBOffset();
    indexBufferView.SizeInBytes = GetIBSize();
    indexBufferView.Format = DXGI_FORMAT_R16_UINT;
}

void ae3d::VertexBuffer::GenerateDynamic( int faceCount, int vertexCount )
{
    vertexFormat = VertexFormat::PTNTC;
    elementCount = faceCount * 3;

    const int ibSize = elementCount * 2;
    ibOffset = sizeof( VertexPTNTC ) * vertexCount;

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
        IID_PPV_ARGS( &vbUpload ) );
    if (FAILED( hr ))
    {
        ae3d::System::Assert( false, "Unable to create vertex buffer!\n" );
        return;
    }

    vbUpload->SetName( L"VertexBuffer" );
    Global::vbs.push_back( vbUpload );
    Global::totalBufferMemoryUsageBytes += sizeBytes;

    mappedDynamic = nullptr;
    D3D12_RANGE emptyRange{};
    hr = vbUpload->Map( 0, &emptyRange, reinterpret_cast<void**>(&mappedDynamic) );
    if (FAILED( hr ))
    {
        ae3d::System::Assert( false, "Unable to map vertex buffer!\n" );
        return;
    }

    vertexBufferView.BufferLocation = vbUpload->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes = GetStride();
    vertexBufferView.SizeInBytes = GetIBOffset();

    indexBufferView.BufferLocation = vbUpload->GetGPUVirtualAddress() + GetIBOffset();
    indexBufferView.SizeInBytes = GetIBSize();
    indexBufferView.Format = DXGI_FORMAT_R16_UINT;
}

void ae3d::VertexBuffer::UpdateDynamic( const Face* faces, int /*faceCount*/, const VertexPTC* vertices, int vertexCount )
{
    System::Assert( mappedDynamic != nullptr, "Must call GenerateDynamic before UpdateDynamic!" );

    std::vector< VertexPTNTC > verticesPTNTC( vertexCount );

    for (std::size_t vertexInd = 0; vertexInd < verticesPTNTC.size(); ++vertexInd)
    {
        verticesPTNTC[ vertexInd ].position = vertices[ vertexInd ].position;
        verticesPTNTC[ vertexInd ].u = vertices[ vertexInd ].u;
        verticesPTNTC[ vertexInd ].v = vertices[ vertexInd ].v;
        verticesPTNTC[ vertexInd ].normal = Vec3( 0, 0, 1 );
        verticesPTNTC[ vertexInd ].tangent = Vec4( 1, 0, 0, 0 );
        verticesPTNTC[ vertexInd ].color = vertices[ vertexInd ].color;
    }

    const int ibSize = elementCount * 2;
    std::memcpy( mappedDynamic, verticesPTNTC.data(), vertexCount * sizeof( VertexPTNTC ) );
    memcpy_s( mappedDynamic + ibOffset, ibSize, faces, ibSize );
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTC* vertices, int vertexCount, Storage /*storage*/ )
{
    vertexFormat = VertexFormat::PTNTC;
    elementCount = faceCount * 3;

    const int ibSize = elementCount * 2;
    ibOffset = sizeof( VertexPTNTC ) * vertexCount;

    std::vector< VertexPTNTC > verticesPTNTC( vertexCount );

    for (std::size_t vertexInd = 0; vertexInd < verticesPTNTC.size(); ++vertexInd)
    {
        verticesPTNTC[ vertexInd ].position = vertices[ vertexInd ].position;
        verticesPTNTC[ vertexInd ].u = vertices[ vertexInd ].u;
        verticesPTNTC[ vertexInd ].v = vertices[ vertexInd ].v;
        verticesPTNTC[ vertexInd ].normal = Vec3( 0, 0, 1 );
        verticesPTNTC[ vertexInd ].tangent = Vec4( 1, 0, 0, 0 );
        verticesPTNTC[ vertexInd ].color = vertices[ vertexInd ].color;
    }

    UploadVB( (void*)faces, verticesPTNTC.data(), ibSize );
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTN* vertices, int vertexCount )
{
    vertexFormat = VertexFormat::PTNTC;
    elementCount = faceCount * 3;

    const int ibSize = elementCount * 2;
    ibOffset = sizeof( VertexPTNTC ) * vertexCount;

    std::vector< VertexPTNTC > verticesPTNTC( vertexCount );

    for (std::size_t vertexInd = 0; vertexInd < verticesPTNTC.size(); ++vertexInd)
    {
        verticesPTNTC[ vertexInd ].position = vertices[ vertexInd ].position;
        verticesPTNTC[ vertexInd ].u = vertices[ vertexInd ].u;
        verticesPTNTC[ vertexInd ].v = vertices[ vertexInd ].v;
        verticesPTNTC[ vertexInd ].normal = vertices[ vertexInd ].normal;
        verticesPTNTC[ vertexInd ].tangent = Vec4( 1, 0, 0, 0 );
        verticesPTNTC[ vertexInd ].color = Vec4( 1, 1, 1, 1 );
    }

    UploadVB( (void*)faces, verticesPTNTC.data(), ibSize );
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTNTC* vertices, int vertexCount )
{
    vertexFormat = VertexFormat::PTNTC;
    elementCount = faceCount * 3;

    const int ibSize = elementCount * 2;
    ibOffset = sizeof( VertexPTNTC ) * vertexCount;

    UploadVB( (void*)faces, (void*)vertices, ibSize );
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTNTC_Skinned* vertices, int vertexCount )
{
    vertexFormat = VertexFormat::PTNTC_Skinned;
    elementCount = faceCount * 3;

    const int ibSize = elementCount * 2;
    ibOffset = sizeof( VertexPTNTC_Skinned ) * vertexCount;

    UploadVB( (void*)faces, (void*)vertices, ibSize );
}

void ae3d::VertexBuffer::Bind() const
{
}
