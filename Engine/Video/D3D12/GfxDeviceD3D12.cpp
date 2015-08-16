#include "GfxDevice.hpp"
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <vector>
#include <string>
#include "System.hpp"
#include "RenderTexture.hpp"

#define AE3D_SAFE_RELEASE(x) if (x) { x->Release(); x = nullptr; }

void DestroyVertexBuffers(); // Defined in VertexBufferD3D12.cpp

namespace WindowGlobal
{
    extern HWND hwnd;
}

namespace Global
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
    ID3D12CommandAllocator* commandListAllocator = nullptr;
    ID3D12CommandQueue* commandQueue = nullptr;
    ID3D12RootSignature* rootSignature = nullptr;
    ID3D12PipelineState* pso = nullptr;
    ID3D12GraphicsCommandList* commandList = nullptr;
    // Create a GPU fence that will fire an event once the command list has been executed by the command queue.
    ID3D12Fence* fence = nullptr;
    unsigned frameIndex = 0;
    ID3D12DescriptorHeap* rtvDescriptorHeap2;
    HANDLE fenceEvent;
    float clearColor[ 4 ] = { 0, 0, 0, 1 };
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
    for (int i = 0; i < Global::BufferCount; ++i)
    {
        const HRESULT hr = Global::swapChain->GetBuffer( i, IID_PPV_ARGS( &Global::renderTargets[ i ] ) );
        if (FAILED( hr ))
        {
            OutputDebugStringA( "Failed to create RTV!\n" );
        }

        Global::renderTargets[ i ]->SetName( L"SwapChain_Buffer" );
    }

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    desc.NumDescriptors = 10;
    desc.NodeMask = 0;
    const HRESULT hr = Global::device->CreateDescriptorHeap( &desc, IID_PPV_ARGS( &Global::rtvDescriptorHeap2 ) );
    if (FAILED( hr ))
    {
        OutputDebugStringA( "Failed to create descriptor heap!\n" );
    }

    auto rtvStep = Global::device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
    for (auto i = 0u; i < Global::BufferCount; ++i)
    {
        auto d = Global::rtvDescriptorHeap2->GetCPUDescriptorHandleForHeapStart();
        d.ptr += i * rtvStep;
        Global::device->CreateRenderTargetView( Global::renderTargets[ i ], nullptr, d );
    }
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
    HRESULT hr = D3D12CreateDevice( nullptr, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&Global::device );
    if (hr != S_OK)
    {
        OutputDebugStringA( "Failed to create D3D12 device!\n" );
        ae3d::System::Assert( false, "Failed to create D3D12 device!" );
        return;
    }

    hr = Global::device->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&Global::commandListAllocator );
    if (FAILED( hr ))
    {
        OutputDebugStringA( "Failed to create command allocator!\n" );
        ae3d::System::Assert( false, "Failed to create command allocator!" );
        return;
    }

    hr = Global::device->CreateCommandList( 1, D3D12_COMMAND_LIST_TYPE_DIRECT, Global::commandListAllocator, nullptr, __uuidof(ID3D12CommandList), (void**)&Global::commandList );
    if (FAILED( hr ))
    {
        ae3d::System::Assert( false, "Failed to create command list.\n" );
        return;
    }

    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    hr = Global::device->CreateCommandQueue( &commandQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&Global::commandQueue );
    if (FAILED( hr ))
    {
        OutputDebugStringA( "Failed to create command queue!\n" );
    }

    // Create the swap chain descriptor
    DXGI_SWAP_CHAIN_DESC swapChainDesc{ {},{ 1, 0 }, DXGI_USAGE_RENDER_TARGET_OUTPUT, 2, WindowGlobal::hwnd, TRUE, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH };
    ZeroMemory( &swapChainDesc.BufferDesc, sizeof( swapChainDesc.BufferDesc ) );
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    // Create a DXGI factory to create the swapchain from the command queue
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
    // Create the swap chain using the command queue, NOT using the device.
    hr = dxgiFactory->CreateSwapChain( Global::commandQueue, &swapChainDesc, (IDXGISwapChain**)&Global::swapChain );
    if (FAILED( hr ))
    {
        OutputDebugStringA( "Failed to create DXGI factory!\n" );
    }
    dxgiFactory->Release();

    hr = Global::device->CreateFence( 0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&Global::fence );
    if (FAILED( hr ))
    {
        ae3d::System::Assert( false, "Failed to create fence!\n" );
        return;
    }

    Global::fenceEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr );

    CreateDescriptorHeap();
}

void ae3d::GfxDevice::Init( int /*width*/, int /*height*/ )
{
}

void ae3d::GfxDevice::SetMultiSampling( bool /*enable*/ )
{
}

void ae3d::GfxDevice::IncDrawCalls()
{
    ++Global::drawCalls;
}

void ae3d::GfxDevice::IncTextureBinds()
{
    ++Global::textureBinds;
}

void ae3d::GfxDevice::IncVertexBufferBinds()
{
    ++Global::vaoBinds;
}

void ae3d::GfxDevice::ResetFrameStatistics()
{
    Global::drawCalls = 0;
    Global::vaoBinds = 0;
    Global::textureBinds = 0;

    // TODO: Create BeginFrame() etc.
    ++Global::frameIndex;
}

int ae3d::GfxDevice::GetDrawCalls()
{
    return Global::drawCalls;
}

int ae3d::GfxDevice::GetTextureBinds()
{
    return Global::textureBinds;
}

int ae3d::GfxDevice::GetVertexBufferBinds()
{
    return Global::vaoBinds;
}

void ae3d::GfxDevice::ReleaseGPUObjects()
{
    CloseHandle( Global::fenceEvent );
    //Global::rtvDescriptorHeap.Destroy();
    AE3D_SAFE_RELEASE( Global::rtvDescriptorHeap2 );
    AE3D_SAFE_RELEASE( Global::commandList );
    AE3D_SAFE_RELEASE( Global::commandListAllocator );
    AE3D_SAFE_RELEASE( Global::commandQueue );
    AE3D_SAFE_RELEASE( Global::fence );
    AE3D_SAFE_RELEASE( Global::pso );
    AE3D_SAFE_RELEASE( Global::rootSignature );
    AE3D_SAFE_RELEASE( Global::device );
    AE3D_SAFE_RELEASE( Global::renderTargets[ 0 ] );
    AE3D_SAFE_RELEASE( Global::renderTargets[ 1 ] );
    AE3D_SAFE_RELEASE( Global::swapChain );
    DestroyVertexBuffers();
}

void ae3d::GfxDevice::ClearScreen( unsigned clearFlags )
{
    if (clearFlags == 0) // TODO: replace 0 with enum
    {
        return;
    }

    // Barrier Present -> RenderTarget
    ID3D12Resource* d3dBuffer = Global::renderTargets[ (Global::frameIndex - 1) % Global::BufferCount ];
    setResourceBarrier( Global::commandList, d3dBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET );

    // Viewport
    D3D12_VIEWPORT viewport = {};
    viewport.Width = (float)Global::backBufferWidth;
    viewport.Height = (float)Global::backBufferHeight;
    Global::commandList->RSSetViewports( 1, &viewport );

    D3D12_CPU_DESCRIPTOR_HANDLE descHandleRtv = Global::rtvDescriptorHeap2->GetCPUDescriptorHandleForHeapStart();
    auto descHandleRtvStep = Global::device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
    descHandleRtv.ptr += ((Global::frameIndex - 1) % Global::BufferCount) * descHandleRtvStep;
    Global::commandList->ClearRenderTargetView( descHandleRtv, Global::clearColor, 0, nullptr );
}

void ae3d::GfxDevice::Present()
{
    // Barrier RenderTarget -> Present
    ID3D12Resource* d3dBuffer = Global::renderTargets[ (Global::frameIndex - 1) % Global::BufferCount ];
    setResourceBarrier( Global::commandList, d3dBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT );

    HRESULT hr = Global::commandList->Close();
    if (hr != S_OK)
    {
        OutputDebugStringA( "Failed to close command list!\n" );
    }

    ID3D12CommandList* const cmdList = Global::commandList;
    Global::commandQueue->ExecuteCommandLists( 1, &cmdList );

    hr = Global::swapChain->Present( 1, 0 );
    if (FAILED( hr ))
    {
        OutputDebugStringA( "Present() failed\n" );
    }

    Global::fence->SetEventOnCompletion( Global::frameIndex, Global::fenceEvent );
    Global::commandQueue->Signal( Global::fence, Global::frameIndex );
    DWORD wait = WaitForSingleObject( Global::fenceEvent, 10000 );
    if (wait != WAIT_OBJECT_0)
    {
        OutputDebugStringA( "Present() failed in WaitForSingleObject\n" );
    }

    Global::commandListAllocator->Reset();
    Global::commandList->Reset( Global::commandListAllocator, nullptr );
}

void ae3d::GfxDevice::SetBackFaceCulling( bool /*enable*/ )
{
}

void ae3d::GfxDevice::SetClearColor( float red, float green, float blue )
{
    Global::clearColor[ 0 ] = red;
    Global::clearColor[ 1 ] = green;
    Global::clearColor[ 2 ] = blue;
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

void ae3d::GfxDevice::WaitForCommandQueueFence()
{
    // Reset the fence signal
    HRESULT hr = Global::fence->Signal( 0 );
    if (FAILED( hr ))
    {
        return;
    }
    // Set the event to be fired once the signal value is 1
    hr = Global::fence->SetEventOnCompletion( 1, Global::fenceEvent );
    if (FAILED( hr ))
    {
        return;
    }

    // After the command list has executed, tell the GPU to signal the fence
    hr = Global::commandQueue->Signal( Global::fence, 1 );
    if (FAILED( hr ))
    {
        return;
    }

    // Wait for the event to be fired by the fence
    WaitForSingleObject( Global::fenceEvent, INFINITE );
};
