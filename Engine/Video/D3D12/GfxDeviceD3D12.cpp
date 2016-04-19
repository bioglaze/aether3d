#include "GfxDevice.hpp"
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dx12.h>
#include <pix.h>
#include <vector>
#include <unordered_map>
#include <string>
#include "DescriptorHeapManager.hpp"
#include "Macros.hpp"
#include "RenderTexture.hpp"
#include "System.hpp"
#include "Shader.hpp"
#include "TextureBase.hpp"
#include "Texture2D.hpp"
#include "TextureCube.hpp"
#include "VertexBuffer.hpp"

void DestroyVertexBuffers(); // Defined in VertexBufferD3D12.cpp
void DestroyShaders(); // Defined in ShaderD3D12.cpp
void DestroyComputeShaders(); // Defined in ComputeShaderD3D12.cpp

namespace WindowGlobal
{
    extern HWND hwnd;
    extern int windowWidth;
    extern int windowHeight;
}

namespace MathUtil
{
    unsigned GetHash( const char* s, unsigned length );
}

namespace Stats
{
    int drawCalls = 0;
    int vaoBinds = 0;
    int textureBinds = 0;
    int shaderBinds = 0;
    int barrierCalls = 0;
    int fenceCalls = 0;
}

namespace GfxDeviceGlobal
{
    const unsigned BufferCount = 2;
    int backBufferWidth = 640;
    int backBufferHeight = 400;

    ID3D12Device* device = nullptr;
    IDXGISwapChain3* swapChain = nullptr;
    IDXGIAdapter3* adapter = nullptr;

    // Not backbuffer.
    GpuResource* currentRenderTarget = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE currentRenderTargetDSV;
    D3D12_CPU_DESCRIPTOR_HANDLE currentRenderTargetRTV;

    ID3D12Resource* renderTargets[ 2 ] = { nullptr, nullptr };
    GpuResource rtvResources[ 2 ];

    ID3D12Resource* depthTexture = nullptr;
    ID3D12CommandAllocator* commandListAllocator = nullptr;
    ID3D12RootSignature* rootSignature = nullptr;
    ID3D12InfoQueue* infoQueue = nullptr;
    unsigned frameIndex = 0;
    float clearColor[ 4 ] = { 0, 0, 0, 1 };
    std::unordered_map< unsigned, ID3D12PipelineState* > psoCache;
    ae3d::Texture2D* texture2d0 = nullptr;
    ae3d::TextureCube* textureCube0 = nullptr;
    std::vector< ID3D12DescriptorHeap* > frameHeaps;
    std::vector< ID3D12Resource* > frameConstantBuffers;
    void* currentConstantBuffer = nullptr;

    ID3D12GraphicsCommandList* graphicsCommandList = nullptr;
    ID3D12CommandQueue* commandQueue = nullptr;
    ID3D12Fence* fence = nullptr;
    UINT64 fenceValue = 1;
    HANDLE fenceEvent;
}

namespace Global
{
    extern std::vector< ID3D12Resource* > frameVBUploads; // Defined in VertexBufferD3D12.cpp
}

namespace ae3d
{
    void CreateRenderer( int samples );
}

void WaitForPreviousFrame()
{
    const UINT64 fenceValue = GfxDeviceGlobal::fenceValue;
    HRESULT hr = GfxDeviceGlobal::commandQueue->Signal( GfxDeviceGlobal::fence, fenceValue );
    AE3D_CHECK_D3D( hr, "command queue signal" );
    ++GfxDeviceGlobal::fenceValue;
    ++Stats::fenceCalls;

    if (GfxDeviceGlobal::fence->GetCompletedValue() < fenceValue)
    {
        hr = GfxDeviceGlobal::fence->SetEventOnCompletion( fenceValue, GfxDeviceGlobal::fenceEvent );
        AE3D_CHECK_D3D( hr, "fence event" );
        WaitForSingleObject( GfxDeviceGlobal::fenceEvent, INFINITE );
    }
}

void TransitionResource( GpuResource& gpuResource, D3D12_RESOURCE_STATES newState )
{
    D3D12_RESOURCE_STATES oldState = gpuResource.usageState;

    if (oldState == newState)
    {
        return;
    }

    D3D12_RESOURCE_BARRIER BarrierDesc = {};

    BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    BarrierDesc.Transition.pResource = gpuResource.resource;
    BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    BarrierDesc.Transition.StateBefore = oldState;
    BarrierDesc.Transition.StateAfter = newState;

    // Check to see if we already started the transition
    if (newState == gpuResource.transitioningState)
    {
        BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
        gpuResource.transitioningState = (D3D12_RESOURCE_STATES)-1;
    }
    else
    {
        BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    }

    gpuResource.usageState = newState;

    ++Stats::barrierCalls;
    GfxDeviceGlobal::graphicsCommandList->ResourceBarrier( 1, &BarrierDesc );
}

void CreateBackBuffer()
{
    for (int i = 0; i < GfxDeviceGlobal::BufferCount; ++i)
    {
        const HRESULT hr = GfxDeviceGlobal::swapChain->GetBuffer( i, IID_PPV_ARGS( &GfxDeviceGlobal::renderTargets[ i ] ) );
        AE3D_CHECK_D3D( hr, "Failed to create RTV" );

        GfxDeviceGlobal::renderTargets[ i ]->SetName( L"SwapChain_Buffer" );
        GfxDeviceGlobal::rtvResources[ i ].usageState = D3D12_RESOURCE_STATE_COMMON;
        GfxDeviceGlobal::backBufferWidth = int( GfxDeviceGlobal::renderTargets[ i ]->GetDesc().Width );
        GfxDeviceGlobal::backBufferHeight = int( GfxDeviceGlobal::renderTargets[ i ]->GetDesc().Height );
    }

    for (auto i = 0u; i < GfxDeviceGlobal::BufferCount; ++i)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = DescriptorHeapManager::AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );

        D3D12_RENDER_TARGET_VIEW_DESC descRtv = {};
        descRtv.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        descRtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        GfxDeviceGlobal::device->CreateRenderTargetView( GfxDeviceGlobal::renderTargets[ i ], &descRtv, handle );
    }
}

void CreateSampler()
{
    D3D12_SAMPLER_DESC descSampler = {};
    descSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    descSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    descSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    descSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    descSampler.MinLOD = 0;
    descSampler.MaxLOD = FLT_MAX;
    descSampler.MipLODBias = 0;
    descSampler.MaxAnisotropy = 0;
    descSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    GfxDeviceGlobal::device->CreateSampler( &descSampler, DescriptorHeapManager::GetSamplerHeap()->GetCPUDescriptorHandleForHeapStart() );
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

    HRESULT hr = D3D12SerializeRootSignature( &descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &pOutBlob, &pErrorBlob );
    AE3D_CHECK_D3D( hr, "Failed to serialize root signature" );

    hr = GfxDeviceGlobal::device->CreateRootSignature( 0, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(), IID_PPV_ARGS( &GfxDeviceGlobal::rootSignature ) );
    AE3D_CHECK_D3D( hr, "Failed to create root signature" );
    GfxDeviceGlobal::rootSignature->SetName( L"Root Signature" );
}

unsigned GetPSOHash( ae3d::VertexBuffer::VertexFormat vertexFormat, ae3d::Shader& shader, ae3d::GfxDevice::BlendMode blendMode,
                     ae3d::GfxDevice::DepthFunc depthFunc, ae3d::GfxDevice::CullMode cullMode, DXGI_FORMAT rtvFormat )
{
    std::string hashString;
    hashString += std::to_string( (unsigned)vertexFormat );
    hashString += std::to_string( (ptrdiff_t)&shader.blobShaderVertex );
    hashString += std::to_string( (ptrdiff_t)&shader.blobShaderPixel );
    hashString += std::to_string( (unsigned)blendMode );
    hashString += std::to_string( ((unsigned)depthFunc) );
    hashString += std::to_string( ((unsigned)cullMode) );
    hashString += std::to_string( ((unsigned)rtvFormat) );

    return MathUtil::GetHash( hashString.c_str(), static_cast< unsigned >( hashString.length() ) );
}

void CreatePSO( ae3d::VertexBuffer::VertexFormat vertexFormat, ae3d::Shader& shader, ae3d::GfxDevice::BlendMode blendMode, ae3d::GfxDevice::DepthFunc depthFunc,
                ae3d::GfxDevice::CullMode cullMode, DXGI_FORMAT rtvFormat )
{
    D3D12_RASTERIZER_DESC descRaster = {};

    if (cullMode == ae3d::GfxDevice::CullMode::Off)
    {
        descRaster.CullMode = D3D12_CULL_MODE_NONE;
    }
    else if (cullMode == ae3d::GfxDevice::CullMode::Front)
    {
        descRaster.CullMode = D3D12_CULL_MODE_FRONT;
    }
    else if (cullMode == ae3d::GfxDevice::CullMode::Back)
    {
        descRaster.CullMode = D3D12_CULL_MODE_BACK;
    }
    else
    {
        ae3d::System::Assert( false, "unhandled cull mode" );
    }

    descRaster.DepthBias = 0;
    descRaster.DepthBiasClamp = 0;
    descRaster.DepthClipEnable = TRUE;
    descRaster.FillMode = D3D12_FILL_MODE_SOLID;
    descRaster.FrontCounterClockwise = TRUE;
    descRaster.MultisampleEnable = FALSE;
    descRaster.SlopeScaledDepthBias = 0;

    D3D12_BLEND_DESC descBlend = {};
    descBlend.AlphaToCoverageEnable = FALSE;
    descBlend.IndependentBlendEnable = FALSE;

    const D3D12_RENDER_TARGET_BLEND_DESC blendOff =
    {
        FALSE, FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };

    D3D12_RENDER_TARGET_BLEND_DESC blendAlpha;
    ZeroMemory( &blendAlpha, sizeof( D3D12_RENDER_TARGET_BLEND_DESC ) );
    blendAlpha.BlendEnable = TRUE;
    blendAlpha.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendAlpha.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blendAlpha.SrcBlendAlpha = D3D12_BLEND_ONE;
    blendAlpha.DestBlendAlpha = D3D12_BLEND_ZERO;
    blendAlpha.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendAlpha.BlendOp = D3D12_BLEND_OP_ADD;
    blendAlpha.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_RENDER_TARGET_BLEND_DESC blendAdd;
    ZeroMemory( &blendAdd, sizeof( D3D12_RENDER_TARGET_BLEND_DESC ) );
    blendAdd.BlendEnable = TRUE;
    blendAdd.SrcBlend = D3D12_BLEND_ONE;
    blendAdd.DestBlend = D3D12_BLEND_ONE;
    blendAdd.SrcBlendAlpha = D3D12_BLEND_ONE;
    blendAdd.DestBlendAlpha = D3D12_BLEND_ONE;
    blendAdd.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendAdd.BlendOp = D3D12_BLEND_OP_ADD;
    blendAdd.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    if (blendMode == ae3d::GfxDevice::BlendMode::Off)
    {
        descBlend.RenderTarget[ 0 ] = blendOff;
    }
    else if (blendMode == ae3d::GfxDevice::BlendMode::AlphaBlend)
    {
        descBlend.RenderTarget[ 0 ] = blendAlpha;
    }
    else if (blendMode == ae3d::GfxDevice::BlendMode::Additive)
    {
        descBlend.RenderTarget[ 0 ] = blendAdd;
    }
    else
    {
        ae3d::System::Assert( false, "unhandled blend mode" );
    }

    D3D12_INPUT_ELEMENT_DESC layoutPTC[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_INPUT_ELEMENT_DESC layoutPTN[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_INPUT_ELEMENT_DESC layoutPTNTC[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    UINT numElements = 0;
    D3D12_INPUT_ELEMENT_DESC* layout = nullptr;
    if (vertexFormat == ae3d::VertexBuffer::VertexFormat::PTC)
    {
        layout = layoutPTC;
        numElements = 3;
    }
    else if (vertexFormat == ae3d::VertexBuffer::VertexFormat::PTN)
    {
        layout = layoutPTN;
        numElements = 3;
    }
    else if (vertexFormat == ae3d::VertexBuffer::VertexFormat::PTNTC)
    {
        layout = layoutPTNTC;
        numElements = 5;
    }
    else
    {
        ae3d::System::Assert( false, "unhandled vertex format" );
        layout = layoutPTC;
        numElements = 3;
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC descPso = {};
    descPso.InputLayout = { layout, numElements };
    descPso.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    descPso.pRootSignature = GfxDeviceGlobal::rootSignature;
    descPso.VS = { reinterpret_cast<BYTE*>(shader.blobShaderVertex->GetBufferPointer()), shader.blobShaderVertex->GetBufferSize() };
    descPso.PS = { reinterpret_cast<BYTE*>(shader.blobShaderPixel->GetBufferPointer()), shader.blobShaderPixel->GetBufferSize() };
    descPso.RasterizerState = descRaster;
    descPso.BlendState = descBlend;
    descPso.DepthStencilState.StencilEnable = FALSE;
    descPso.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    
    if (depthFunc == ae3d::GfxDevice::DepthFunc::LessOrEqualWriteOff)
    {
        descPso.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        descPso.DepthStencilState.DepthEnable = TRUE;
    }
    else if (depthFunc == ae3d::GfxDevice::DepthFunc::LessOrEqualWriteOn)
    {
        descPso.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        descPso.DepthStencilState.DepthEnable = TRUE;
    }
    else if (depthFunc == ae3d::GfxDevice::DepthFunc::NoneWriteOff)
    {
        descPso.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        descPso.DepthStencilState.DepthEnable = FALSE;
    }
    else
    {
        ae3d::System::Assert( false, "unhandled depth mode" );
    }

    descPso.SampleMask = UINT_MAX;
    descPso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    descPso.NumRenderTargets = 1;
    descPso.RTVFormats[ 0 ] = rtvFormat;
    descPso.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    descPso.SampleDesc.Count = 1;

    ID3D12PipelineState* pso;
    HRESULT hr = GfxDeviceGlobal::device->CreateGraphicsPipelineState( &descPso, IID_PPV_ARGS( &pso ) );
    AE3D_CHECK_D3D( hr, "Failed to create PSO" );
    pso->SetName( L"PSO" );

    const unsigned hash = GetPSOHash( vertexFormat, shader, blendMode, depthFunc, cullMode, rtvFormat );
    GfxDeviceGlobal::psoCache[ hash ] = pso;
}

void CreateDepthStencilView()
{
    auto descResource = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R32_TYPELESS, GfxDeviceGlobal::backBufferWidth, GfxDeviceGlobal::backBufferHeight, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
        D3D12_TEXTURE_LAYOUT_UNKNOWN, 0 );

    auto prop = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT );

    D3D12_CLEAR_VALUE dsvClearValue;
    dsvClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    dsvClearValue.DepthStencil.Depth = 1.0f;
    dsvClearValue.DepthStencil.Stencil = 0;
    HRESULT hr = GfxDeviceGlobal::device->CreateCommittedResource(
        &prop, // No need to read/write by CPU
        D3D12_HEAP_FLAG_NONE,
        &descResource,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &dsvClearValue,
        IID_PPV_ARGS( &GfxDeviceGlobal::depthTexture ) );
    AE3D_CHECK_D3D( hr, "Failed to create depth texture" );

    GfxDeviceGlobal::depthTexture->SetName( L"DepthTexture" );

    D3D12_DEPTH_STENCIL_VIEW_DESC descDsv = {};
    descDsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    descDsv.Format = DXGI_FORMAT_D32_FLOAT;
    descDsv.Texture2D.MipSlice = 0;
    descDsv.Flags = D3D12_DSV_FLAG_NONE;
    GfxDeviceGlobal::device->CreateDepthStencilView( GfxDeviceGlobal::depthTexture, &descDsv, DescriptorHeapManager::GetDSVHeap()->GetCPUDescriptorHandleForHeapStart() );
}

void ae3d::CreateRenderer( int /*samples*/ )
{
#ifdef DEBUG
    ID3D12Debug* debugController;
    const HRESULT dhr = D3D12GetDebugInterface( IID_PPV_ARGS( &debugController ) );
    if (dhr == S_OK)
    {
        debugController->EnableDebugLayer();
        debugController->Release();
    }
    else
    {
        OutputDebugStringA( "Failed to create debug layer!\n" );
    }
#endif
    //IDXGIAdapter* adapter = nullptr;
    IDXGIFactory4* factory = nullptr;
    HRESULT hr = CreateDXGIFactory1( IID_PPV_ARGS( &factory ) );
    AE3D_CHECK_D3D( hr, "Failed to create D3D12 WARP factory" );

    factory->EnumAdapters( 0, reinterpret_cast<IDXGIAdapter**>(&GfxDeviceGlobal::adapter) );

    bool useWarp = false;

    if (useWarp)
    {
        hr = factory->EnumWarpAdapter( IID_PPV_ARGS( &GfxDeviceGlobal::adapter ) );
        AE3D_CHECK_D3D( hr, "Failed to create D3D12 WARP adapter" );
    }

    hr = D3D12CreateDevice( GfxDeviceGlobal::adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS( &GfxDeviceGlobal::device ) );
    AE3D_CHECK_D3D( hr, "Failed to create D3D12 device with feature level 11.0" );
#ifdef DEBUG
    // Prevents GPU from over/underclocking to get consistent timing information.
    GfxDeviceGlobal::device->SetStablePowerState( TRUE );

    hr = GfxDeviceGlobal::device->QueryInterface( IID_PPV_ARGS( &GfxDeviceGlobal::infoQueue ) );
    AE3D_CHECK_D3D( hr, "Infoqueue failed" );
    GfxDeviceGlobal::infoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_ERROR, TRUE );
#endif

    hr = GfxDeviceGlobal::device->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &GfxDeviceGlobal::commandListAllocator ) );
    AE3D_CHECK_D3D( hr, "Failed to create command allocator" );
    GfxDeviceGlobal::commandListAllocator->SetName( L"Command Allocator" );

    hr = GfxDeviceGlobal::device->CreateCommandList( 1, D3D12_COMMAND_LIST_TYPE_DIRECT, GfxDeviceGlobal::commandListAllocator, nullptr, IID_PPV_ARGS( &GfxDeviceGlobal::graphicsCommandList ) );
    AE3D_CHECK_D3D( hr, "Failed to create command list" );
    GfxDeviceGlobal::graphicsCommandList->SetName( L"Graphics Command List" );

    hr = GfxDeviceGlobal::graphicsCommandList->Close();
    AE3D_CHECK_D3D( hr, "command list close" );

    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    hr = GfxDeviceGlobal::device->CreateCommandQueue( &commandQueueDesc, IID_PPV_ARGS( &GfxDeviceGlobal::commandQueue ) );
    AE3D_CHECK_D3D( hr, "Failed to create command queue" );
    GfxDeviceGlobal::commandQueue->SetName( L"Command Queue" );

    hr = GfxDeviceGlobal::device->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &GfxDeviceGlobal::fence ) );
    AE3D_CHECK_D3D( hr, "Failed to create fence" );
    GfxDeviceGlobal::fence->SetName( L"Fence" );
    GfxDeviceGlobal::fence->Signal( 0 );

    GfxDeviceGlobal::fenceEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr );
    ae3d::System::Assert( GfxDeviceGlobal::fenceEvent != INVALID_HANDLE_VALUE, "Invalid fence event value!" );

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc1 = {};
    swapChainDesc1.BufferCount = 2;
    swapChainDesc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc1.Flags = 0;
    swapChainDesc1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc1.Width = WindowGlobal::windowWidth;
    swapChainDesc1.Height = WindowGlobal::windowHeight;
    swapChainDesc1.SampleDesc.Count = 1;
    swapChainDesc1.SampleDesc.Quality = 0;
    swapChainDesc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

    hr = factory->CreateSwapChainForHwnd( GfxDeviceGlobal::commandQueue, WindowGlobal::hwnd, &swapChainDesc1, nullptr, nullptr, (IDXGISwapChain1**)&GfxDeviceGlobal::swapChain );
    AE3D_CHECK_D3D( hr, "Failed to create swap chain" );
    factory->Release();

    D3D12_CPU_DESCRIPTOR_HANDLE initSamplerHeapTemp = DescriptorHeapManager::AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER );
    D3D12_CPU_DESCRIPTOR_HANDLE initDsvHeapTemp = DescriptorHeapManager::AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE_DSV );
    D3D12_CPU_DESCRIPTOR_HANDLE initCbvSrvUavHeapTemp = DescriptorHeapManager::AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

    CreateBackBuffer();
    CreateRootSignature();
    CreateDepthStencilView();
    CreateSampler();
}

void ae3d::GfxDevice::CreateNewUniformBuffer()
{
    D3D12_HEAP_PROPERTIES prop = {};
    prop.Type = D3D12_HEAP_TYPE_UPLOAD;
    prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    prop.CreationNodeMask = 1;
    prop.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC buf = {};
    buf.Alignment = 0;
    buf.DepthOrArraySize = 1;
    buf.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    buf.Flags = D3D12_RESOURCE_FLAG_NONE;
    buf.Format = DXGI_FORMAT_UNKNOWN;
    buf.Height = 1;
    buf.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    buf.MipLevels = 1;
    buf.SampleDesc.Count = 1;
    buf.SampleDesc.Quality = 0;
    buf.Width = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;

    ID3D12Resource* constantBuffer;
    HRESULT hr = GfxDeviceGlobal::device->CreateCommittedResource(
        &prop,
        D3D12_HEAP_FLAG_NONE,
        &buf,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS( &constantBuffer ) );
    if (FAILED( hr ))
    {
        ae3d::System::Print( "Unable to create shader constant buffer!" );
        return;
    }

    GfxDeviceGlobal::frameConstantBuffers.push_back( constantBuffer );

    constantBuffer->SetName( L"ConstantBuffer" );
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = constantBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT; // must be a multiple of 256
    GfxDeviceGlobal::device->CreateConstantBufferView(
        &cbvDesc,
        DescriptorHeapManager::GetCbvSrvUavHeap()->GetCPUDescriptorHandleForHeapStart() );
    hr = constantBuffer->Map( 0, nullptr, reinterpret_cast<void**>(&GfxDeviceGlobal::currentConstantBuffer) );
    if (FAILED( hr ))
    {
        ae3d::System::Print( "Unable to map shader constant buffer!" );
    }
}

void* ae3d::GfxDevice::GetCurrentUniformBuffer()
{
    return GfxDeviceGlobal::currentConstantBuffer;
}

void ae3d::GfxDevice::PushGroupMarker( const char* name )
{
    wchar_t wstr[ 128 ];
    std::mbstowcs( wstr, name, 128 );
    const UINT size = static_cast<UINT>((wcslen( wstr ) + 1) * sizeof( wstr[ 0 ] ));
    GfxDeviceGlobal::graphicsCommandList->BeginEvent( DirectX::Detail::PIX_EVENT_UNICODE_VERSION, wstr, size );
}

void ae3d::GfxDevice::PopGroupMarker()
{
    GfxDeviceGlobal::graphicsCommandList->EndEvent();
}

void ae3d::GfxDevice::Draw( VertexBuffer& vertexBuffer, int startFace, int endFace, Shader& shader, BlendMode blendMode, DepthFunc depthFunc,
                            CullMode cullMode )
{   
    const DXGI_FORMAT rtvFormat = GfxDeviceGlobal::currentRenderTarget ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    const unsigned psoHash = GetPSOHash( vertexBuffer.GetVertexFormat(), shader, blendMode, depthFunc, cullMode, rtvFormat );

    if (GfxDeviceGlobal::psoCache.find( psoHash ) == std::end( GfxDeviceGlobal::psoCache ))
    {
        CreatePSO( vertexBuffer.GetVertexFormat(), shader, blendMode, depthFunc, cullMode, rtvFormat );
    }
    
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 100;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NodeMask = 1;

    ID3D12DescriptorHeap* tempHeap = nullptr;
    HRESULT hr = GfxDeviceGlobal::device->CreateDescriptorHeap( &desc, IID_PPV_ARGS( &tempHeap ) );
    AE3D_CHECK_D3D( hr, "Failed to create CBV_SRV_UAV descriptor heap" );
    GfxDeviceGlobal::frameHeaps.push_back( tempHeap );
    tempHeap->SetName( L"tempHeap" );

    D3D12_CPU_DESCRIPTOR_HANDLE handle = tempHeap->GetCPUDescriptorHandleForHeapStart();

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = GfxDeviceGlobal::frameConstantBuffers.back()->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT; // must be a multiple of 256
    GfxDeviceGlobal::device->CreateConstantBufferView( &cbvDesc, handle );

    // TODO: Get from texture object
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = GfxDeviceGlobal::texture2d0 != nullptr ? D3D12_SRV_DIMENSION_TEXTURE2D : D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if (srvDesc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURE2D)
    {
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.PlaneSlice = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    }
    else if (srvDesc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURECUBE)
    {
        srvDesc.TextureCube.MipLevels = 1;
        srvDesc.TextureCube.MostDetailedMip = 0;
        srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
    }
    else
    {
        System::Assert( false, "unhandled texture dimension" );
    }

    handle.ptr += GfxDeviceGlobal::device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

    ID3D12Resource* texResource = nullptr;

    if (GfxDeviceGlobal::texture2d0 != nullptr)
    {
        texResource = GfxDeviceGlobal::texture2d0->GetGpuResource()->resource;
    }
    else if (GfxDeviceGlobal::textureCube0 != nullptr)
    {
        texResource = GfxDeviceGlobal::textureCube0->GetGpuResource()->resource;
    }
    else if (!GfxDeviceGlobal::texture2d0)
    {
        texResource = const_cast< Texture2D*>(Texture2D::GetDefaultTexture())->GetGpuResource()->resource;
    }

    GfxDeviceGlobal::device->CreateShaderResourceView( texResource, &srvDesc, handle );

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    vertexBufferView.BufferLocation = vertexBuffer.GetVBResource()->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes = vertexBuffer.GetStride();
    vertexBufferView.SizeInBytes = vertexBuffer.GetIBOffset();

    D3D12_INDEX_BUFFER_VIEW indexBufferView;
    indexBufferView.BufferLocation = vertexBuffer.GetVBResource()->GetGPUVirtualAddress() + vertexBuffer.GetIBOffset();
    indexBufferView.SizeInBytes = vertexBuffer.GetIBSize();
    indexBufferView.Format = DXGI_FORMAT_R16_UINT;

    ID3D12DescriptorHeap* descHeaps[] = { tempHeap, DescriptorHeapManager::GetSamplerHeap() };
    GfxDeviceGlobal::graphicsCommandList->SetDescriptorHeaps( 2, &descHeaps[ 0 ] );
    GfxDeviceGlobal::graphicsCommandList->SetGraphicsRootSignature( GfxDeviceGlobal::rootSignature );
    GfxDeviceGlobal::graphicsCommandList->SetGraphicsRootDescriptorTable( 0, tempHeap->GetGPUDescriptorHandleForHeapStart() );
    GfxDeviceGlobal::graphicsCommandList->SetGraphicsRootDescriptorTable( 1, DescriptorHeapManager::GetSamplerHeap()->GetGPUDescriptorHandleForHeapStart() );
    GfxDeviceGlobal::graphicsCommandList->SetPipelineState( GfxDeviceGlobal::psoCache[ psoHash ] );
    GfxDeviceGlobal::graphicsCommandList->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    GfxDeviceGlobal::graphicsCommandList->IASetVertexBuffers( 0, 1, &vertexBufferView );
    GfxDeviceGlobal::graphicsCommandList->IASetIndexBuffer( &indexBufferView );
    GfxDeviceGlobal::graphicsCommandList->DrawIndexedInstanced( endFace * 3 - startFace * 3, 1, startFace * 3, 0, 0 );

    GfxDevice::IncDrawCalls();
}

void ae3d::GfxDevice::Init( int /*width*/, int /*height*/ )
{
}

void ae3d::GfxDevice::SetMultiSampling( bool /*enable*/ )
{
}

int ae3d::GfxDevice::GetRenderTargetBinds()
{
    return 0;
}

void ae3d::GfxDevice::IncDrawCalls()
{
    ++Stats::drawCalls;
}

void ae3d::GfxDevice::IncTextureBinds()
{
    ++Stats::textureBinds;
}

void ae3d::GfxDevice::IncVertexBufferBinds()
{
    ++Stats::vaoBinds;
}

void ae3d::GfxDevice::ResetFrameStatistics()
{
    Stats::drawCalls = 0;
    Stats::vaoBinds = 0;
    Stats::textureBinds = 0;
    Stats::barrierCalls = 0;
    Stats::fenceCalls = 0;

    // TODO: Figure out a better place for this.

    HRESULT hr = GfxDeviceGlobal::graphicsCommandList->Reset( GfxDeviceGlobal::commandListAllocator, nullptr );
    AE3D_CHECK_D3D( hr, "graphicsCommandList Reset" );
}

int ae3d::GfxDevice::GetDrawCalls()
{
    return Stats::drawCalls;
}

int ae3d::GfxDevice::GetTextureBinds()
{
    return Stats::textureBinds;
}

int ae3d::GfxDevice::GetShaderBinds()
{
    return Stats::shaderBinds;
}

int ae3d::GfxDevice::GetVertexBufferBinds()
{
    return Stats::vaoBinds;
}

int ae3d::GfxDevice::GetBarrierCalls()
{
    return Stats::barrierCalls;
}

int ae3d::GfxDevice::GetFenceCalls()
{
    return Stats::fenceCalls;
}

void ae3d::GfxDevice::GetGpuMemoryUsage( unsigned& outUsedMBytes, unsigned& outBudgetMBytes )
{
    DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo;
    GfxDeviceGlobal::adapter->QueryVideoMemoryInfo( 0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemoryInfo );

    outUsedMBytes = (unsigned)videoMemoryInfo.CurrentUsage / (1024 * 1024);
    outBudgetMBytes = (unsigned)videoMemoryInfo.Budget / (1024 * 1024);
}

void ae3d::GfxDevice::ReleaseGPUObjects()
{
    DestroyVertexBuffers();
    DestroyShaders();
    DestroyComputeShaders();
    Texture2D::DestroyTextures();
    TextureCube::DestroyTextures();
    RenderTexture::DestroyTextures();
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::depthTexture );
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::commandListAllocator );
    DescriptorHeapManager::Deinit();

    for (auto& pso : GfxDeviceGlobal::psoCache)
    {
        AE3D_SAFE_RELEASE( pso.second );
    }

    AE3D_SAFE_RELEASE( GfxDeviceGlobal::infoQueue );
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::renderTargets[ 0 ] );
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::renderTargets[ 1 ] );
    GfxDeviceGlobal::swapChain->SetFullscreenState( FALSE, nullptr );
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::swapChain );
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::rootSignature );

    AE3D_SAFE_RELEASE( GfxDeviceGlobal::fence );
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::graphicsCommandList );
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::commandQueue );

/*
    ID3D12DebugDevice* d3dDebug = nullptr;
    GfxDeviceGlobal::device->QueryInterface(__uuidof(ID3D12DebugDevice), reinterpret_cast<void**>(&d3dDebug));
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::device );
    d3dDebug->ReportLiveDeviceObjects( D3D12_RLDO_DETAIL );
    AE3D_SAFE_RELEASE( d3dDebug );
*/
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::device );
}

void ae3d::GfxDevice::ClearScreen( unsigned clearFlags )
{
    if (clearFlags == ClearFlags::DontClear)
    {
        return;
    }

    auto i = GfxDeviceGlobal::swapChain->GetCurrentBackBufferIndex();
    GfxDeviceGlobal::rtvResources[ i ].resource = GfxDeviceGlobal::renderTargets[ i ];

    GpuResource* resource = GfxDeviceGlobal::currentRenderTarget ? GfxDeviceGlobal::currentRenderTarget : &GfxDeviceGlobal::rtvResources[ i ];

    TransitionResource( *resource, D3D12_RESOURCE_STATE_RENDER_TARGET );
    
    D3D12_VIEWPORT viewPort{ 0, 0, static_cast<float>(GfxDeviceGlobal::backBufferWidth), static_cast<float>(GfxDeviceGlobal::backBufferHeight), 0, 1 };
    GfxDeviceGlobal::graphicsCommandList->RSSetViewports( 1, &viewPort );

    D3D12_RECT scissor = {};
    scissor.right = (LONG)GfxDeviceGlobal::backBufferWidth;
    scissor.bottom = (LONG)GfxDeviceGlobal::backBufferHeight;
    GfxDeviceGlobal::graphicsCommandList->RSSetScissorRects( 1, &scissor );

    auto descHandleRtvStep = GfxDeviceGlobal::device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );

    D3D12_CPU_DESCRIPTOR_HANDLE descHandleRtv;

    if (GfxDeviceGlobal::currentRenderTarget)
    {
        descHandleRtv = GfxDeviceGlobal::currentRenderTargetRTV;
    }
    else
    {
        descHandleRtv = DescriptorHeapManager::GetRTVHeap()->GetCPUDescriptorHandleForHeapStart();
        descHandleRtv.ptr += GfxDeviceGlobal::swapChain->GetCurrentBackBufferIndex() * descHandleRtvStep;
    }

    if ((clearFlags & ClearFlags::Color) != 0)
    {
        GfxDeviceGlobal::graphicsCommandList->ClearRenderTargetView( descHandleRtv, GfxDeviceGlobal::clearColor, 0, nullptr );
    }

    D3D12_CPU_DESCRIPTOR_HANDLE descHandleDsv = GfxDeviceGlobal::currentRenderTarget ? GfxDeviceGlobal::currentRenderTargetDSV : 
                                                       DescriptorHeapManager::GetDSVHeap()->GetCPUDescriptorHandleForHeapStart();

    if ((clearFlags & ClearFlags::Depth) != 0)
    {
        GfxDeviceGlobal::graphicsCommandList->ClearDepthStencilView( descHandleDsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr );
    }

    GfxDeviceGlobal::graphicsCommandList->OMSetRenderTargets( 1, &descHandleRtv, TRUE, &descHandleDsv );
}

void ae3d::GfxDevice::Present()
{
    TransitionResource( GfxDeviceGlobal::rtvResources[ GfxDeviceGlobal::swapChain->GetCurrentBackBufferIndex() ], D3D12_RESOURCE_STATE_PRESENT );

    HRESULT hr = GfxDeviceGlobal::graphicsCommandList->Close();
    AE3D_CHECK_D3D( hr, "command list close" );

    ID3D12CommandList* ppCommandLists[] = { GfxDeviceGlobal::graphicsCommandList };
    GfxDeviceGlobal::commandQueue->ExecuteCommandLists( 1, &ppCommandLists[ 0 ] );

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
        else if (hr == DXGI_ERROR_DEVICE_HUNG)
        {
            ae3d::System::Assert( false, "Present failed. Reason: device hung." );
        }
        else
        {
            ae3d::System::Assert( false, "Present failed. Reason: unknown." );
        }
    }

    WaitForPreviousFrame();

    hr = GfxDeviceGlobal::commandListAllocator->Reset();
    AE3D_CHECK_D3D( hr, "commandListAllocator Reset" );

    for (std::size_t i = 0; i < GfxDeviceGlobal::frameHeaps.size(); ++i)
    {
        AE3D_SAFE_RELEASE( GfxDeviceGlobal::frameHeaps[ i ] );
    }
    
    GfxDeviceGlobal::frameHeaps.clear();

    for (std::size_t i = 0; i < Global::frameVBUploads.size(); ++i)
    {
        AE3D_SAFE_RELEASE( Global::frameVBUploads[ i ] );
    }

    for (std::size_t i = 0; i < GfxDeviceGlobal::frameConstantBuffers.size(); ++i)
    {
        AE3D_SAFE_RELEASE( GfxDeviceGlobal::frameConstantBuffers[ i ] );
    }
    
    GfxDeviceGlobal::frameConstantBuffers.clear();
    Global::frameVBUploads.clear();
}

void ae3d::GfxDevice::SetClearColor( float red, float green, float blue )
{
    GfxDeviceGlobal::clearColor[ 0 ] = red;
    GfxDeviceGlobal::clearColor[ 1 ] = green;
    GfxDeviceGlobal::clearColor[ 2 ] = blue;
}

void ae3d::GfxDevice::ErrorCheck( const char* /*info*/ )
{
}

void ae3d::GfxDevice::SetRenderTarget( RenderTexture* target, unsigned /*cubeMapFace*/ )
{
    GfxDeviceGlobal::currentRenderTarget = !target ? nullptr : target->GetGpuResource();
    
    if (target)
    {
        GfxDeviceGlobal::currentRenderTargetDSV = target->GetDSV();
        GfxDeviceGlobal::currentRenderTargetRTV = target->GetRTV();
    }
}
