#include "VertexBuffer.hpp"
#include <vector>
#include <d3d12.h>
#include <d3dx12.h>
#include "GfxDevice.hpp"
#include "Vec3.hpp"

#define AE3D_SAFE_RELEASE(x) if (x) { x->Release(); x = nullptr; }

namespace Global
{
    extern ID3D12Device* device;
    extern ID3D12GraphicsCommandList* commandList;
    std::vector< ID3D12Resource* > vbs;
    std::vector< ID3D12Resource* > vbUploads;
}

void DestroyVertexBuffers()
{
    for (std::size_t i = 0; i < Global::vbs.size(); ++i)
    {
        AE3D_SAFE_RELEASE( Global::vbs[ i ] );
    }

    for (std::size_t i = 0; i < Global::vbUploads.size(); ++i)
    {
        AE3D_SAFE_RELEASE( Global::vbUploads[ i ] );
    }
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTC* vertices, int vertexCount )
{
    vertexFormat = VertexFormat::PTC;
    elementCount = faceCount * 3;

    HRESULT hr = Global::device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT ),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer( sizeof( VertexPTC ) * vertexCount ),
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS( &vb ) );
    if (FAILED( hr ))
    {
        OutputDebugStringA( "Unable to create vertex buffer!\n" );
    }
    
    vb->SetName( L"Vertex Buffer Resource" );
    Global::vbs.push_back( vb );

    hr = Global::device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT ),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer( sizeof( VertexPTC ) * vertexCount ),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS( &vbUpload ) );
    if (FAILED( hr ))
    {
        OutputDebugStringA( "Unable to create vertex buffer upload resource!\n" );
    }

    vbUpload->SetName( L"Vertex Buffer Upload Resource" );
    Global::vbUploads.push_back( vbUpload );

    // Upload the vertex buffer to the GPU.
    {
        D3D12_SUBRESOURCE_DATA vertexData = {};
        vertexData.pData = reinterpret_cast<const BYTE*>(vertices);
        vertexData.RowPitch = sizeof( VertexPTC ) * vertexCount;
        vertexData.SlicePitch = vertexData.RowPitch;

        UpdateSubresources( Global::commandList, vb, vbUpload, 0, 0, 1, &vertexData );
        Global::commandList->ResourceBarrier( 1, &CD3DX12_RESOURCE_BARRIER::Transition( vb, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER ) );
    }

    GfxDevice::WaitForCommandQueueFence();
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTN* vertices, int vertexCount )
{
    vertexFormat = VertexFormat::PTN;
    elementCount = faceCount * 3;
 
    HRESULT hr = Global::device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT ),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer( sizeof( VertexPTN ) * vertexCount ),
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS( &vb ) );
    if (FAILED( hr ))
    {
        OutputDebugStringA( "Unable to create vertex buffer!\n" );
    }

    vb->SetName( L"Vertex Buffer Resource" );
    Global::vbs.push_back( vb );

    hr = Global::device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT ),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer( sizeof( VertexPTN ) * vertexCount ),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS( &vbUpload ) );
    if (FAILED( hr ))
    {
        OutputDebugStringA( "Unable to create vertex buffer upload resource!\n" );
    }

    vbUpload->SetName( L"Vertex Buffer Upload Resource" );
    Global::vbUploads.push_back( vbUpload );

    // Upload the vertex buffer to the GPU.
    {
        D3D12_SUBRESOURCE_DATA vertexData = {};
        vertexData.pData = reinterpret_cast<const BYTE*>(vertices);
        vertexData.RowPitch = sizeof( VertexPTN ) * vertexCount;
        vertexData.SlicePitch = vertexData.RowPitch;

        UpdateSubresources( Global::commandList, vb, vbUpload, 0, 0, 1, &vertexData );
        Global::commandList->ResourceBarrier( 1, &CD3DX12_RESOURCE_BARRIER::Transition( vb, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER ) );
    }

    GfxDevice::WaitForCommandQueueFence();
}

void ae3d::VertexBuffer::Generate( const Face* faces, int faceCount, const VertexPTNTC* vertices, int vertexCount )
{
    vertexFormat = VertexFormat::PTNTC;
    elementCount = faceCount * 3;

    HRESULT hr = Global::device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT ),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer( sizeof( VertexPTNTC ) * vertexCount ),
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS( &vb ) );
    if (FAILED( hr ))
    {
        OutputDebugStringA( "Unable to create vertex buffer!\n" );
    }

    vb->SetName( L"Vertex Buffer Resource" );
    Global::vbs.push_back( vb );

    hr = Global::device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT ),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer( sizeof( VertexPTNTC ) * vertexCount ),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS( &vbUpload ) );
    if (FAILED( hr ))
    {
        OutputDebugStringA( "Unable to create vertex buffer upload resource!\n" );
    }

    vbUpload->SetName( L"Vertex Buffer Upload Resource" );
    Global::vbUploads.push_back( vbUpload );

    // Upload the vertex buffer to the GPU.
    {
        D3D12_SUBRESOURCE_DATA vertexData = {};
        vertexData.pData = reinterpret_cast<const BYTE*>(vertices);
        vertexData.RowPitch = sizeof( VertexPTNTC ) * vertexCount;
        vertexData.SlicePitch = vertexData.RowPitch;

        UpdateSubresources( Global::commandList, vb, vbUpload, 0, 0, 1, &vertexData );
        Global::commandList->ResourceBarrier( 1, &CD3DX12_RESOURCE_BARRIER::Transition( vb, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER ) );
    }

    GfxDevice::WaitForCommandQueueFence();
}

void ae3d::VertexBuffer::Bind() const
{

}

void ae3d::VertexBuffer::Draw() const
{
    GfxDevice::IncDrawCalls();
}

void ae3d::VertexBuffer::DrawRange( int start, int end ) const
{
    GfxDevice::IncDrawCalls();
}
