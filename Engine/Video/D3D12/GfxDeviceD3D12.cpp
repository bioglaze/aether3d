#include "GfxDevice.hpp"
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dx12.h>
#include <vector>
#include <unordered_map>
#include <string>
#include "System.hpp"
#include "RenderTexture.hpp"
#include "Shader.hpp"
#include "VertexBuffer.hpp"

#define AE3D_SAFE_RELEASE(x) if (x) { x->Release(); x = nullptr; }

void DestroyVertexBuffers(); // Defined in VertexBufferD3D12.cpp
void DestroyShaders(); // Defined in ShaderD3D12.cpp
void DestroyTextures(); // Defined in Texture2D_D3D12.cpp

namespace WindowGlobal
{
    extern HWND hwnd;
}

namespace GfxDeviceGlobal
{
    const unsigned BufferCount = 2;
    int drawCalls = 0;
    int vaoBinds = 0;
    int textureBinds = 0;
    int backBufferWidth = 640;
    int backBufferHeight = 400;
    ID3D12Device* device = nullptr;
    IDXGISwapChain3* swapChain = nullptr;
    ID3D12Resource* renderTargets[ 2 ] = { nullptr, nullptr };
    ID3D12Resource* depthTexture = nullptr;
    ID3D12CommandAllocator* commandListAllocator = nullptr;
    ID3D12CommandQueue* commandQueue = nullptr;
    ID3D12RootSignature* rootSignature = nullptr;
    ID3D12GraphicsCommandList* commandList = nullptr;
    ID3D12Fence* fence = nullptr;
    unsigned frameIndex = 0;
    ID3D12DescriptorHeap* rtvDescriptorHeap = nullptr;
    ID3D12DescriptorHeap* dsvDescriptorHeap = nullptr;
    ID3D12DescriptorHeap* samplerDescriptorHeap = nullptr;
    HANDLE fenceEvent;
    float clearColor[ 4 ] = { 0, 0, 0, 1 };
    std::unordered_map< std::string, ID3D12PipelineState* > psoCache;

    // FIXME: This is related to texturing and shader constant buffers, so try to move somewhere else.
    ID3D12DescriptorHeap* descHeapCbvSrvUav = nullptr;
}

void setResourceBarrier( ID3D12GraphicsCommandList* commandList, ID3D12Resource* res,
                         D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after )
{
    D3D12_RESOURCE_BARRIER desc = {};
    desc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    desc.Transition.pResource = res;
    desc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    desc.Transition.StateBefore = before;
    desc.Transition.StateAfter = after;
    desc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    commandList->ResourceBarrier( 1, &desc );
}
namespace ae3d
{
    void CreateRenderer( int samples );
}

void CreateDescriptorHeap()
{
    for (int i = 0; i < GfxDeviceGlobal::BufferCount; ++i)
    {
        const HRESULT hr = GfxDeviceGlobal::swapChain->GetBuffer( i, IID_PPV_ARGS( &GfxDeviceGlobal::renderTargets[ i ] ) );
        if (FAILED( hr ))
        {
            OutputDebugStringA( "Failed to create RTV!\n" );
        }

        GfxDeviceGlobal::renderTargets[ i ]->SetName( L"SwapChain_Buffer" );
        GfxDeviceGlobal::backBufferWidth = int( GfxDeviceGlobal::renderTargets[ i ]->GetDesc().Width );
        GfxDeviceGlobal::backBufferHeight = int( GfxDeviceGlobal::renderTargets[ i ]->GetDesc().Height );
    }

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    desc.NumDescriptors = 10;
    desc.NodeMask = 0;
    HRESULT hr = GfxDeviceGlobal::device->CreateDescriptorHeap( &desc, IID_PPV_ARGS( &GfxDeviceGlobal::rtvDescriptorHeap ) );
    if (FAILED( hr ))
    {
        OutputDebugStringA( "Failed to create descriptor heap!\n" );
    }

    auto rtvStep = GfxDeviceGlobal::device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
    for (auto i = 0u; i < GfxDeviceGlobal::BufferCount; ++i)
    {
        auto d = GfxDeviceGlobal::rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        d.ptr += i * rtvStep;
        GfxDeviceGlobal::device->CreateRenderTargetView( GfxDeviceGlobal::renderTargets[ i ], nullptr, d );
    }

    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 100;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NodeMask = 0;
    hr = GfxDeviceGlobal::device->CreateDescriptorHeap( &desc, IID_PPV_ARGS( &GfxDeviceGlobal::descHeapCbvSrvUav ) );
    if (FAILED( hr ))
    {
        ae3d::System::Print( "Unable to create shader DescriptorHeap!" );
        return;
    }
}

void CreateSampler()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    desc.NumDescriptors = 100;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NodeMask = 0;
    HRESULT hr = GfxDeviceGlobal::device->CreateDescriptorHeap( &desc, IID_PPV_ARGS( &GfxDeviceGlobal::samplerDescriptorHeap ) );
    if (FAILED( hr ))
    {
        ae3d::System::Print( "Unable to create sampler DescriptorHeap!" );
        return;
    }

    D3D12_SAMPLER_DESC samplerDesc;
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.MinLOD = -FLT_MAX;
    samplerDesc.MaxLOD = FLT_MAX;
    samplerDesc.MipLODBias = 0;
    samplerDesc.MaxAnisotropy = 0;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    GfxDeviceGlobal::device->CreateSampler( &samplerDesc, GfxDeviceGlobal::samplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart() );
}

void CreateRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE descRange1[ 2 ];
    descRange1[ 0 ].Init( D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0 );
    descRange1[ 1 ].Init( D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0 );

    CD3DX12_DESCRIPTOR_RANGE descRange2[ 1 ];
    descRange2[ 0 ].Init( D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0 );

    CD3DX12_ROOT_PARAMETER rootParam[ 2 ];
    rootParam[ 0 ].InitAsDescriptorTable( 2, descRange1 );
    rootParam[ 1 ].InitAsDescriptorTable( 1, descRange2, D3D12_SHADER_VISIBILITY_PIXEL );

    ID3DBlob* pOutBlob = nullptr;
    ID3DBlob* pErrorBlob = nullptr;
    D3D12_ROOT_SIGNATURE_DESC descRootSignature;
    descRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    descRootSignature.NumParameters = 2;
    descRootSignature.NumStaticSamplers = 0;
    descRootSignature.pParameters = rootParam;
    descRootSignature.pStaticSamplers = nullptr;
    D3D12SerializeRootSignature( &descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &pOutBlob, &pErrorBlob );
    HRESULT hr = GfxDeviceGlobal::device->CreateRootSignature( 0, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(), IID_PPV_ARGS( &GfxDeviceGlobal::rootSignature ) );
    if (FAILED( hr ))
    {
        OutputDebugStringA( "Unable to create root signature!\n" );
    }
}

std::string GetPSOHash( ae3d::VertexBuffer& vertexBuffer, ae3d::Shader& shader, ae3d::GfxDevice::BlendMode blendMode, ae3d::GfxDevice::DepthFunc depthFunc )
{
    std::string hashString;
    hashString += std::to_string( (ptrdiff_t)&vertexBuffer );
    hashString += std::to_string( (ptrdiff_t)&shader.blobShaderVertex );
    hashString += std::to_string( (ptrdiff_t)&shader.blobShaderPixel );
    hashString += std::to_string( (unsigned)blendMode );
    hashString += std::to_string( ((unsigned)depthFunc) + 4 );
    return hashString;
}

void CreatePSO( ae3d::VertexBuffer& vertexBuffer, ae3d::Shader& shader, ae3d::GfxDevice::BlendMode blendMode, ae3d::GfxDevice::DepthFunc depthFunc )
{
    D3D12_RASTERIZER_DESC descRaster;
    ZeroMemory( &descRaster, sizeof( descRaster ) );
    descRaster.CullMode = D3D12_CULL_MODE_NONE;
    descRaster.DepthBias = 0;
    descRaster.DepthBiasClamp = 0;
    descRaster.DepthClipEnable = true;
    descRaster.FillMode = D3D12_FILL_MODE_SOLID;
    descRaster.FrontCounterClockwise = FALSE;
    descRaster.MultisampleEnable = FALSE;
    descRaster.SlopeScaledDepthBias = 0;

    D3D12_BLEND_DESC descBlend = {};
    descBlend.AlphaToCoverageEnable = FALSE;
    descBlend.IndependentBlendEnable = FALSE;

    const D3D12_RENDER_TARGET_BLEND_DESC rtBlend =
    {
        FALSE, FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    descBlend.RenderTarget[ 0 ] = rtBlend;

    D3D12_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    
    const UINT numElements = sizeof( layout ) / sizeof( layout[ 0 ] );

    D3D12_GRAPHICS_PIPELINE_STATE_DESC descPso;
    ZeroMemory( &descPso, sizeof( descPso ) );
    descPso.InputLayout = { layout, numElements };
    descPso.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    descPso.pRootSignature = GfxDeviceGlobal::rootSignature;
    descPso.VS = { reinterpret_cast<BYTE*>(shader.blobShaderVertex->GetBufferPointer()), shader.blobShaderVertex->GetBufferSize() };
    descPso.PS = { reinterpret_cast<BYTE*>(shader.blobShaderPixel->GetBufferPointer()), shader.blobShaderPixel->GetBufferSize() };
    descPso.RasterizerState = descRaster;
    descPso.BlendState = descBlend;
    descPso.DepthStencilState.DepthEnable = FALSE;
    descPso.DepthStencilState.StencilEnable = FALSE;
    descPso.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    descPso.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    descPso.SampleMask = UINT_MAX;
    descPso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    descPso.NumRenderTargets = 1;
    descPso.RTVFormats[ 0 ] = DXGI_FORMAT_R8G8B8A8_UNORM;
    descPso.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    descPso.SampleDesc.Count = 1;

    const std::string hash = GetPSOHash( vertexBuffer, shader, blendMode, depthFunc );
    ID3D12PipelineState* pso;
    HRESULT hr = GfxDeviceGlobal::device->CreateGraphicsPipelineState( &descPso, IID_PPV_ARGS( &pso ) );

    if (FAILED( hr ))
    {
        ae3d::System::Assert( false, "Unable to create PSO!\n" );
        return;
    }

    GfxDeviceGlobal::psoCache[ hash ] = pso;
}

void CreateDepthStencilView()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    desc.NumDescriptors = 10;
    desc.NodeMask = 0;
    HRESULT hr = GfxDeviceGlobal::device->CreateDescriptorHeap( &desc, IID_PPV_ARGS( &GfxDeviceGlobal::dsvDescriptorHeap ) );
    if (FAILED( hr ))
    {
        ae3d::System::Assert( false, "Unable to create depth-stencil descriptor!\n" );
        return;
    }

    auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R32_TYPELESS, GfxDeviceGlobal::backBufferWidth, GfxDeviceGlobal::backBufferHeight, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
        D3D12_TEXTURE_LAYOUT_UNKNOWN, 0 );

    D3D12_CLEAR_VALUE dsvClearValue;
    dsvClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    dsvClearValue.DepthStencil.Depth = 1.0f;
    dsvClearValue.DepthStencil.Stencil = 0;
    hr = GfxDeviceGlobal::device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT ), // No need to read/write by CPU
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &dsvClearValue,
        IID_PPV_ARGS( &GfxDeviceGlobal::depthTexture ) );
    if (FAILED( hr ))
    {
        ae3d::System::Assert( false, "Unable to create depth-stencil texture!\n" );
        return;
    }
    GfxDeviceGlobal::depthTexture->SetName( L"DepthTexture" );

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.Texture2D.MipSlice = 0;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    GfxDeviceGlobal::device->CreateDepthStencilView( GfxDeviceGlobal::depthTexture, &dsvDesc, GfxDeviceGlobal::dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart() );
}

void ae3d::CreateRenderer( int /*samples*/ )
{
#ifndef NDEBUG
    ID3D12Debug* debugController;
    const HRESULT dhr = D3D12GetDebugInterface( IID_PPV_ARGS( &debugController ) );
    if (dhr == S_OK)
    {
        debugController->EnableDebugLayer();
    }
    else
    {
        OutputDebugStringA( "Failed to create debug layer!\n" );
    }
#endif
    HRESULT hr = D3D12CreateDevice( nullptr, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&GfxDeviceGlobal::device );
    if (hr != S_OK)
    {
        OutputDebugStringA( "Failed to create D3D12 device!\n" );
        ae3d::System::Assert( false, "Failed to create D3D12 device!" );
        return;
    }

    hr = GfxDeviceGlobal::device->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&GfxDeviceGlobal::commandListAllocator );
    if (FAILED( hr ))
    {
        OutputDebugStringA( "Failed to create command allocator!\n" );
        ae3d::System::Assert( false, "Failed to create command allocator!" );
        return;
    }

    hr = GfxDeviceGlobal::device->CreateCommandList( 1, D3D12_COMMAND_LIST_TYPE_DIRECT, GfxDeviceGlobal::commandListAllocator, nullptr, __uuidof(ID3D12CommandList), (void**)&GfxDeviceGlobal::commandList );
    if (FAILED( hr ))
    {
        ae3d::System::Assert( false, "Failed to create command list.\n" );
        return;
    }

    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    hr = GfxDeviceGlobal::device->CreateCommandQueue( &commandQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&GfxDeviceGlobal::commandQueue );
    if (FAILED( hr ))
    {
        OutputDebugStringA( "Failed to create command queue!\n" );
    }

    DXGI_SWAP_CHAIN_DESC swapChainDesc{ {},{ 1, 0 }, DXGI_USAGE_RENDER_TARGET_OUTPUT, 2, WindowGlobal::hwnd, TRUE, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH };
    ZeroMemory( &swapChainDesc.BufferDesc, sizeof( swapChainDesc.BufferDesc ) );
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    IDXGIFactory2 *dxgiFactory = nullptr;
    unsigned factoryFlags = 0;
#if DEBUG
    factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
    hr = CreateDXGIFactory2( factoryFlags, __uuidof(IDXGIFactory2), (void**)&dxgiFactory );
    if (FAILED( hr ))
    {
        OutputDebugStringA( "Failed to create DXGI factory!\n" );
    }

    hr = dxgiFactory->CreateSwapChain( GfxDeviceGlobal::commandQueue, &swapChainDesc, (IDXGISwapChain**)&GfxDeviceGlobal::swapChain );
    if (FAILED( hr ))
    {
        OutputDebugStringA( "Failed to create DXGI factory!\n" );
    }
    dxgiFactory->Release();

    hr = GfxDeviceGlobal::device->CreateFence( 0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&GfxDeviceGlobal::fence );
    if (FAILED( hr ))
    {
        ae3d::System::Assert( false, "Failed to create fence!\n" );
        return;
    }

    GfxDeviceGlobal::fenceEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr );

    CreateDescriptorHeap();
    CreateRootSignature();
    CreateDepthStencilView();
    CreateSampler();
}

void ae3d::GfxDevice::Draw( VertexBuffer& vertexBuffer, Shader& shader, BlendMode blendMode, DepthFunc depthFunc )
{
    const std::string psoHash = GetPSOHash( vertexBuffer, shader, blendMode, depthFunc );
    
    if (GfxDeviceGlobal::psoCache.find( psoHash ) == std::end( GfxDeviceGlobal::psoCache ))
    {
        CreatePSO( vertexBuffer, shader, blendMode, depthFunc );
    }
    
    GfxDeviceGlobal::commandList->SetGraphicsRootSignature( GfxDeviceGlobal::rootSignature );
    ID3D12DescriptorHeap* descHeaps[] = { GfxDeviceGlobal::descHeapCbvSrvUav, GfxDeviceGlobal::samplerDescriptorHeap };
    GfxDeviceGlobal::commandList->SetDescriptorHeaps( 2, descHeaps );
    GfxDeviceGlobal::commandList->SetGraphicsRootDescriptorTable( 0, GfxDeviceGlobal::descHeapCbvSrvUav->GetGPUDescriptorHandleForHeapStart() );
    GfxDeviceGlobal::commandList->SetGraphicsRootDescriptorTable( 1, GfxDeviceGlobal::samplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart() );
    GfxDeviceGlobal::commandList->SetPipelineState( GfxDeviceGlobal::psoCache[ psoHash ] );

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    vertexBufferView.BufferLocation = vertexBuffer.GetVBResource()->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes = vertexBuffer.GetStride();
    vertexBufferView.SizeInBytes = vertexBuffer.GetIBOffset();

    D3D12_INDEX_BUFFER_VIEW indexBufferView;
    indexBufferView.BufferLocation = vertexBuffer.GetVBResource()->GetGPUVirtualAddress() + vertexBuffer.GetIBOffset();
    indexBufferView.SizeInBytes = vertexBuffer.GetIBSize();
    indexBufferView.Format = DXGI_FORMAT_R16_UINT;

    GfxDeviceGlobal::commandList->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    GfxDeviceGlobal::commandList->IASetVertexBuffers( 0, 1, &vertexBufferView );
    GfxDeviceGlobal::commandList->IASetIndexBuffer( &indexBufferView );
    GfxDeviceGlobal::commandList->DrawIndexedInstanced( vertexBuffer.GetIndexCount(), 1, 0, 0, 0 );
}

void ae3d::GfxDevice::Init( int /*width*/, int /*height*/ )
{
}

void ae3d::GfxDevice::SetMultiSampling( bool /*enable*/ )
{
}

void ae3d::GfxDevice::IncDrawCalls()
{
    ++GfxDeviceGlobal::drawCalls;
}

void ae3d::GfxDevice::IncTextureBinds()
{
    ++GfxDeviceGlobal::textureBinds;
}

void ae3d::GfxDevice::IncVertexBufferBinds()
{
    ++GfxDeviceGlobal::vaoBinds;
}

void ae3d::GfxDevice::ResetFrameStatistics()
{
    GfxDeviceGlobal::drawCalls = 0;
    GfxDeviceGlobal::vaoBinds = 0;
    GfxDeviceGlobal::textureBinds = 0;

    // TODO: Create BeginFrame() etc.
    ++GfxDeviceGlobal::frameIndex;
}

int ae3d::GfxDevice::GetDrawCalls()
{
    return GfxDeviceGlobal::drawCalls;
}

int ae3d::GfxDevice::GetTextureBinds()
{
    return GfxDeviceGlobal::textureBinds;
}

int ae3d::GfxDevice::GetVertexBufferBinds()
{
    return GfxDeviceGlobal::vaoBinds;
}

void ae3d::GfxDevice::ReleaseGPUObjects()
{
    DestroyVertexBuffers();
    DestroyShaders();
    DestroyTextures();
    CloseHandle( GfxDeviceGlobal::fenceEvent );
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::rtvDescriptorHeap );
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::dsvDescriptorHeap );
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::depthTexture );
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::commandList );
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::commandListAllocator );
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::commandQueue );
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::fence );
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::descHeapCbvSrvUav );

    for (auto& pso : GfxDeviceGlobal::psoCache)
    {
        AE3D_SAFE_RELEASE( pso.second );
    }

    AE3D_SAFE_RELEASE( GfxDeviceGlobal::rootSignature );
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::device );
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::renderTargets[ 0 ] );
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::renderTargets[ 1 ] );
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::swapChain );
}

void ae3d::GfxDevice::ClearScreen( unsigned clearFlags )
{
    if (clearFlags == 0) // TODO: replace 0 with enum
    {
        return;
    }
    
    // Barrier Present -> RenderTarget
    ID3D12Resource* d3dBuffer = GfxDeviceGlobal::renderTargets[ (GfxDeviceGlobal::frameIndex - 1) % GfxDeviceGlobal::BufferCount ];
    setResourceBarrier( GfxDeviceGlobal::commandList, d3dBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET );

    // Viewport
    D3D12_VIEWPORT mViewPort{ 0, 0, static_cast<float>(GfxDeviceGlobal::backBufferWidth), static_cast<float>(GfxDeviceGlobal::backBufferHeight), 0, 1 };
    GfxDeviceGlobal::commandList->RSSetViewports( 1, &mViewPort );

    D3D12_RECT scissor = {};
    scissor.right = (LONG)GfxDeviceGlobal::backBufferWidth;
    scissor.bottom = (LONG)GfxDeviceGlobal::backBufferHeight;
    GfxDeviceGlobal::commandList->RSSetScissorRects( 1, &scissor );

    D3D12_CPU_DESCRIPTOR_HANDLE descHandleRtv = GfxDeviceGlobal::rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    auto descHandleRtvStep = GfxDeviceGlobal::device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
    descHandleRtv.ptr += ((GfxDeviceGlobal::frameIndex - 1) % GfxDeviceGlobal::BufferCount) * descHandleRtvStep;
    GfxDeviceGlobal::commandList->ClearRenderTargetView( descHandleRtv, GfxDeviceGlobal::clearColor, 0, nullptr );

    D3D12_CPU_DESCRIPTOR_HANDLE descHandleDsv = GfxDeviceGlobal::dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    GfxDeviceGlobal::commandList->ClearDepthStencilView( descHandleDsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr );
    GfxDeviceGlobal::commandList->OMSetRenderTargets( 1, &descHandleRtv, TRUE, &descHandleDsv );
}

void ae3d::GfxDevice::Present()
{
    // Barrier RenderTarget -> Present
    ID3D12Resource* d3dBuffer = GfxDeviceGlobal::renderTargets[ (GfxDeviceGlobal::frameIndex - 1) % GfxDeviceGlobal::BufferCount ];
    setResourceBarrier( GfxDeviceGlobal::commandList, d3dBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT );

    HRESULT hr = GfxDeviceGlobal::commandList->Close();
    if (hr != S_OK)
    {
        OutputDebugStringA( "Failed to close command list!\n" );
    }

    ID3D12CommandList* const cmdList = GfxDeviceGlobal::commandList;
    GfxDeviceGlobal::commandQueue->ExecuteCommandLists( 1, &cmdList );

    hr = GfxDeviceGlobal::swapChain->Present( 1, 0 );
    if (FAILED( hr ))
    {
        if (hr == DXGI_ERROR_DEVICE_REMOVED)
        {
            ae3d::System::Assert( false, "Present failed. Reason: device removed." );
        }
        else if (hr == DXGI_ERROR_DEVICE_RESET)
        {
            ae3d::System::Assert( false, "Present failed. Reason: device reset." );
        }
        else
        {
            ae3d::System::Assert( false, "Present failed. Reason: unknown." );
        }
    }

    GfxDeviceGlobal::fence->SetEventOnCompletion( GfxDeviceGlobal::frameIndex, GfxDeviceGlobal::fenceEvent );
    GfxDeviceGlobal::commandQueue->Signal( GfxDeviceGlobal::fence, GfxDeviceGlobal::frameIndex );
    DWORD wait = WaitForSingleObject( GfxDeviceGlobal::fenceEvent, 10000 );
    if (wait != WAIT_OBJECT_0)
    {
        OutputDebugStringA( "Present() failed in WaitForSingleObject\n" );
    }

    GfxDeviceGlobal::commandListAllocator->Reset();
    GfxDeviceGlobal::commandList->Reset( GfxDeviceGlobal::commandListAllocator, nullptr );
}

void ae3d::GfxDevice::SetBackFaceCulling( bool /*enable*/ )
{
}

void ae3d::GfxDevice::SetClearColor( float red, float green, float blue )
{
    GfxDeviceGlobal::clearColor[ 0 ] = red;
    GfxDeviceGlobal::clearColor[ 1 ] = green;
    GfxDeviceGlobal::clearColor[ 2 ] = blue;
}

void ae3d::GfxDevice::SetBlendMode( BlendMode blendMode )
{
    if (blendMode == BlendMode::Off)
    {
    }
    else if (blendMode == BlendMode::AlphaBlend)
    {
    }
    else if (blendMode == BlendMode::Additive)
    {
    }
    else
    {
        ae3d::System::Assert( false, "Unhandled blend mode." );
    }
}

void ae3d::GfxDevice::ErrorCheck(const char* info)
{
        (void)info;
#if defined _DEBUG || defined DEBUG

#endif
}

void ae3d::GfxDevice::SetRenderTarget( RenderTexture2D* /*target*/ )
{

}

void ae3d::GfxDevice::SetDepthFunc( DepthFunc depthFunc )
{
    if (depthFunc == DepthFunc::LessOrEqualWriteOn)
    {
    }
    else if (depthFunc == DepthFunc::LessOrEqualWriteOff)
    {
    }
}
