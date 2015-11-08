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
#include "CommandListManager.hpp"
#include "CommandContext.hpp"
#include "DescriptorHeapManager.hpp"
#include "Macros.hpp"
#include "TextureBase.hpp"
#include "Texture2D.hpp"

void DestroyVertexBuffers(); // Defined in VertexBufferD3D12.cpp
void DestroyShaders(); // Defined in ShaderD3D12.cpp
void DestroyTextures(); // Defined in Texture2D_D3D12.cpp

namespace WindowGlobal
{
    extern HWND hwnd;
    extern int windowWidth;
    extern int windowHeight;
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
    ID3D12RootSignature* rootSignature = nullptr;
    ID3D12InfoQueue* infoQueue = nullptr;
    CommandContext graphicsContext;
    unsigned frameIndex = 0;
    float clearColor[ 4 ] = { 0, 0, 0, 1 };
    std::unordered_map< std::string, ID3D12PipelineState* > psoCache;
    CommandListManager commandListManager;
    ae3d::Texture2D* texture0 = nullptr;
    std::vector< ID3D12DescriptorHeap* > frameHeaps;
}

namespace ae3d
{
    void CreateRenderer( int samples );
}

void CreateBackBuffer()
{
    for (int i = 0; i < GfxDeviceGlobal::BufferCount; ++i)
    {
        const HRESULT hr = GfxDeviceGlobal::swapChain->GetBuffer( i, IID_PPV_ARGS( &GfxDeviceGlobal::renderTargets[ i ] ) );
        AE3D_CHECK_D3D( hr, "Failed to create RTV" );

        GfxDeviceGlobal::renderTargets[ i ]->SetName( L"SwapChain_Buffer" );
        GfxDeviceGlobal::backBufferWidth = int( GfxDeviceGlobal::renderTargets[ i ]->GetDesc().Width );
        GfxDeviceGlobal::backBufferHeight = int( GfxDeviceGlobal::renderTargets[ i ]->GetDesc().Height );
    }

    auto rtvStep = GfxDeviceGlobal::device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );

    for (auto i = 0u; i < GfxDeviceGlobal::BufferCount; ++i)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = DescriptorHeapManager::GetRTVHeap()->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += i * rtvStep;
        GfxDeviceGlobal::device->CreateRenderTargetView( GfxDeviceGlobal::renderTargets[ i ], nullptr, handle );
    }
}

void CreateSampler()
{
    D3D12_SAMPLER_DESC descSampler;
    descSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    descSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    descSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    descSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    descSampler.MinLOD = -FLT_MAX;
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
    descRaster.CullMode = D3D12_CULL_MODE_BACK;
    descRaster.DepthBias = 0;
    descRaster.DepthBiasClamp = 0;
    descRaster.DepthClipEnable = TRUE;
    descRaster.FillMode = D3D12_FILL_MODE_SOLID;
    descRaster.FrontCounterClockwise = FALSE;
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
    descPso.RTVFormats[ 0 ] = DXGI_FORMAT_R8G8B8A8_UNORM;
    descPso.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    descPso.SampleDesc.Count = 1;

    const std::string hash = GetPSOHash( vertexBuffer, shader, blendMode, depthFunc );
    ID3D12PipelineState* pso;
    HRESULT hr = GfxDeviceGlobal::device->CreateGraphicsPipelineState( &descPso, IID_PPV_ARGS( &pso ) );
    AE3D_CHECK_D3D( hr, "Failed to create PSO" );
    pso->SetName( L"PSO" );

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
    HRESULT hr = D3D12CreateDevice( nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS( &GfxDeviceGlobal::device ) );
    AE3D_CHECK_D3D( hr, "Failed to create D3D12 device with feature level 11.0" );
#ifdef DEBUG
    // Prevents GPU from over/underclocking to get consistent timing information.
    GfxDeviceGlobal::device->SetStablePowerState( TRUE );

    hr = GfxDeviceGlobal::device->QueryInterface( IID_PPV_ARGS( &GfxDeviceGlobal::infoQueue ) );
    AE3D_CHECK_D3D( hr, "Infoqueue failed" );
    GfxDeviceGlobal::infoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_ERROR, TRUE );
#endif

    GfxDeviceGlobal::commandListManager.Create( GfxDeviceGlobal::device );
    GfxDeviceGlobal::graphicsContext.Initialize( GfxDeviceGlobal::commandListManager );

    IDXGIFactory2 *dxgiFactory = nullptr;
    unsigned factoryFlags = 0;
#if DEBUG
    factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
    hr = CreateDXGIFactory2( factoryFlags, IID_PPV_ARGS( &dxgiFactory ) );
    AE3D_CHECK_D3D( hr, "Failed to create DXGI factory" );

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

    hr = dxgiFactory->CreateSwapChainForHwnd( GfxDeviceGlobal::commandListManager.GetCommandQueue(), WindowGlobal::hwnd, &swapChainDesc1, nullptr, nullptr, (IDXGISwapChain1**)&GfxDeviceGlobal::swapChain );
    AE3D_CHECK_D3D( hr, "Failed to create swap chain" );
    dxgiFactory->Release();

    D3D12_CPU_DESCRIPTOR_HANDLE initRtvHeapTemp = DescriptorHeapManager::AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
    D3D12_CPU_DESCRIPTOR_HANDLE initSamplerHeapTemp = DescriptorHeapManager::AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER );
    D3D12_CPU_DESCRIPTOR_HANDLE initDsvHeapTemp = DescriptorHeapManager::AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE_DSV );
    D3D12_CPU_DESCRIPTOR_HANDLE initCbvSrvUavHeapTemp = DescriptorHeapManager::AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

    CreateBackBuffer();
    CreateRootSignature();
    CreateDepthStencilView();
    CreateSampler();
    GfxDeviceGlobal::graphicsContext.Initialize( GfxDeviceGlobal::commandListManager );
}

void ae3d::GfxDevice::Draw( VertexBuffer& vertexBuffer, int startFace, int endFace, Shader& shader, BlendMode blendMode, DepthFunc depthFunc )
{
    const std::string psoHash = GetPSOHash( vertexBuffer, shader, blendMode, depthFunc );
    
    if (GfxDeviceGlobal::psoCache.find( psoHash ) == std::end( GfxDeviceGlobal::psoCache ))
    {
        CreatePSO( vertexBuffer, shader, blendMode, depthFunc );
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

    D3D12_CPU_DESCRIPTOR_HANDLE handle = tempHeap->GetCPUDescriptorHandleForHeapStart();

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = shader.GetConstantBuffer()->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT; // must be a multiple of 256
    GfxDeviceGlobal::device->CreateConstantBufferView(
        &cbvDesc,
        handle );

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    
    handle.ptr += GfxDeviceGlobal::device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

    GfxDeviceGlobal::device->CreateShaderResourceView( GfxDeviceGlobal::texture0->GetGpuResource()->resource, &srvDesc, handle );

    GfxDeviceGlobal::graphicsContext.graphicsCommandList->SetGraphicsRootSignature( GfxDeviceGlobal::rootSignature );
    ID3D12DescriptorHeap* descHeaps[] = { tempHeap, DescriptorHeapManager::GetSamplerHeap() };
    GfxDeviceGlobal::graphicsContext.graphicsCommandList->SetDescriptorHeaps( 2, descHeaps );
    GfxDeviceGlobal::graphicsContext.graphicsCommandList->SetGraphicsRootDescriptorTable( 0, tempHeap->GetGPUDescriptorHandleForHeapStart() );
    GfxDeviceGlobal::graphicsContext.graphicsCommandList->SetGraphicsRootDescriptorTable( 1, DescriptorHeapManager::GetSamplerHeap()->GetGPUDescriptorHandleForHeapStart() );
    GfxDeviceGlobal::graphicsContext.graphicsCommandList->SetPipelineState( GfxDeviceGlobal::psoCache[ psoHash ] );

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    vertexBufferView.BufferLocation = vertexBuffer.GetVBResource()->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes = vertexBuffer.GetStride();
    vertexBufferView.SizeInBytes = vertexBuffer.GetIBOffset();

    D3D12_INDEX_BUFFER_VIEW indexBufferView;
    indexBufferView.BufferLocation = vertexBuffer.GetVBResource()->GetGPUVirtualAddress() + vertexBuffer.GetIBOffset();
    indexBufferView.SizeInBytes = vertexBuffer.GetIBSize();
    indexBufferView.Format = DXGI_FORMAT_R16_UINT;

    GfxDeviceGlobal::graphicsContext.graphicsCommandList->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    GfxDeviceGlobal::graphicsContext.graphicsCommandList->IASetVertexBuffers( 0, 1, &vertexBufferView );
    GfxDeviceGlobal::graphicsContext.graphicsCommandList->IASetIndexBuffer( &indexBufferView );
    GfxDeviceGlobal::graphicsContext.graphicsCommandList->DrawIndexedInstanced( endFace * 3 - startFace, 1, startFace, 0, 0 );
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
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::swapChain );
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::rootSignature );

    CommandContext::Destroy();
    GfxDeviceGlobal::commandListManager.Destroy();

/*#if _DEBUG
    ID3D12DebugDevice* d3dDebug = nullptr;
    GfxDeviceGlobal::device->QueryInterface(__uuidof(ID3D12DebugDevice), reinterpret_cast<void**>(&d3dDebug));
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::device );
    d3dDebug->ReportLiveDeviceObjects( D3D12_RLDO_DETAIL );
    AE3D_SAFE_RELEASE( d3dDebug );
#else*/
    AE3D_SAFE_RELEASE( GfxDeviceGlobal::device );
//#endif
}

void ae3d::GfxDevice::ClearScreen( unsigned clearFlags )
{
    if (clearFlags == 0) // TODO: replace 0 with enum
    {
        return;
    }
    
    // Barrier Present -> RenderTarget
    GpuResource rtvResource;
    rtvResource.resource = GfxDeviceGlobal::renderTargets[ (GfxDeviceGlobal::frameIndex - 1) % GfxDeviceGlobal::BufferCount ];
    rtvResource.usageState = D3D12_RESOURCE_STATE_RENDER_TARGET;
    GfxDeviceGlobal::graphicsContext.TransitionResource( rtvResource, D3D12_RESOURCE_STATE_RENDER_TARGET );
    
    // Viewport
    D3D12_VIEWPORT mViewPort{ 0, 0, static_cast<float>(GfxDeviceGlobal::backBufferWidth), static_cast<float>(GfxDeviceGlobal::backBufferHeight), 0, 1 };
    GfxDeviceGlobal::graphicsContext.graphicsCommandList->RSSetViewports( 1, &mViewPort );

    D3D12_RECT scissor = {};
    scissor.right = (LONG)GfxDeviceGlobal::backBufferWidth;
    scissor.bottom = (LONG)GfxDeviceGlobal::backBufferHeight;
    GfxDeviceGlobal::graphicsContext.graphicsCommandList->RSSetScissorRects( 1, &scissor );

    D3D12_CPU_DESCRIPTOR_HANDLE descHandleRtv = DescriptorHeapManager::GetRTVHeap()->GetCPUDescriptorHandleForHeapStart();
    auto descHandleRtvStep = GfxDeviceGlobal::device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
    descHandleRtv.ptr += ((GfxDeviceGlobal::frameIndex - 1) % GfxDeviceGlobal::BufferCount) * descHandleRtvStep;
    GfxDeviceGlobal::graphicsContext.graphicsCommandList->ClearRenderTargetView( descHandleRtv, GfxDeviceGlobal::clearColor, 0, nullptr );

    D3D12_CPU_DESCRIPTOR_HANDLE descHandleDsv = DescriptorHeapManager::GetDSVHeap()->GetCPUDescriptorHandleForHeapStart();
    GfxDeviceGlobal::graphicsContext.graphicsCommandList->ClearDepthStencilView( descHandleDsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr );
    GfxDeviceGlobal::graphicsContext.graphicsCommandList->OMSetRenderTargets( 1, &descHandleRtv, TRUE, &descHandleDsv );
}

void ae3d::GfxDevice::Present()
{
    // Barrier RenderTarget -> Present
    GpuResource presentResource;
    presentResource.resource = GfxDeviceGlobal::renderTargets[ (GfxDeviceGlobal::frameIndex - 1) % GfxDeviceGlobal::BufferCount ];
    presentResource.usageState = D3D12_RESOURCE_STATE_RENDER_TARGET;
    GfxDeviceGlobal::graphicsContext.TransitionResource( presentResource, D3D12_RESOURCE_STATE_PRESENT );

    GfxDeviceGlobal::graphicsContext.CloseAndExecute( true );

    auto hr = GfxDeviceGlobal::swapChain->Present( 1, 0 );

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

    GfxDeviceGlobal::graphicsContext.Reset();

    for (std::size_t i = 0; i < GfxDeviceGlobal::frameHeaps.size(); ++i)
    {
        AE3D_SAFE_RELEASE( GfxDeviceGlobal::frameHeaps[ i ] );
    }
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

void ae3d::GfxDevice::ErrorCheck(const char* info)
{
        (void)info;
#if defined _DEBUG || defined DEBUG

#endif
}

void ae3d::GfxDevice::SetRenderTarget( RenderTexture* /*target*/, unsigned cubeMapFace )
{
    cubeMapFace;
}
