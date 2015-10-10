#include "CommandListManager.hpp"
#include <d3d12.h>
#include "System.hpp"
#include "Macros.hpp"

#define MY_MAX( a, b ) (((a) > (b)) ? a : b)

void CommandListManager::Create( ID3D12Device* aDevice )
{
    device = aDevice;
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    HRESULT hr = device->CreateCommandQueue( &commandQueueDesc, IID_PPV_ARGS( &commandQueue ) );
    AE3D_CHECK_D3D( hr, "Failed to create command queue" );

    hr = device->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &fence ) );
    AE3D_CHECK_D3D( hr, "Failed to create fence" );
    fence->Signal( 0 );

    fenceEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr );
    ae3d::System::Assert( fenceEvent != INVALID_HANDLE_VALUE, "Invalid fence event value!" );
}

void CommandListManager::Destroy()
{
    AE3D_SAFE_RELEASE( fence );
    AE3D_SAFE_RELEASE( commandQueue );
}

void CommandListManager::CreateNewCommandList( ID3D12GraphicsCommandList** outList, ID3D12CommandAllocator** outAllocator )
{
    HRESULT hr = device->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( outAllocator ) );
    AE3D_CHECK_D3D( hr, "Failed to create command allocator" );

    hr = device->CreateCommandList( 1, D3D12_COMMAND_LIST_TYPE_DIRECT, *outAllocator, nullptr, IID_PPV_ARGS( outList ) );
    AE3D_CHECK_D3D( hr, "Failed to create command list" );
}

ID3D12CommandAllocator* CommandListManager::RequestAllocator()
{
    // TODO: allocator pool
    ID3D12CommandAllocator* outAllocator = nullptr;
    HRESULT hr = device->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &outAllocator ) );
    AE3D_CHECK_D3D( hr, "Failed to create command allocator" );
    return outAllocator;
}

void CommandListManager::DiscardAllocator( uint64_t /*fenceValue*/, ID3D12CommandAllocator* allocator )
{
    // TODO: allocator pool
    AE3D_SAFE_RELEASE( allocator );
}

std::uint64_t CommandListManager::ExecuteCommandList( ID3D12CommandList* list )
{
    commandQueue->ExecuteCommandLists( 1, &list );
    return IncrementFence();
}

std::uint64_t CommandListManager::IncrementFence()
{
    HRESULT hr = commandQueue->Signal( fence, nextFenceValue );
    AE3D_CHECK_D3D( hr, "Failed to increment fence" );

    return nextFenceValue++;
}

bool CommandListManager::IsFenceComplete( std::uint64_t fenceValue )
{
    if (fenceValue > lastCompletedFenceValue)
    {
        lastCompletedFenceValue = MY_MAX( lastCompletedFenceValue, fence->GetCompletedValue() );
    }

    return fenceValue <= lastCompletedFenceValue;
}

void CommandListManager::WaitForFence( std::uint64_t fenceValue )
{
    if (IsFenceComplete( fenceValue ))
    {
        return;
    }

    HRESULT hr = fence->SetEventOnCompletion( fenceValue, fenceEvent );
    AE3D_CHECK_D3D( hr, "Failed to set fence completion event" );

    const DWORD wait = WaitForSingleObject( fenceEvent, INFINITE );

    if (wait != WAIT_OBJECT_0)
    {
        ae3d::System::Assert( false, "WaitForSingleObject" );
    }

    lastCompletedFenceValue = fenceValue;
}

