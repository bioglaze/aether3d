#include "CommandContext.hpp"
#include <d3d12.h>
#include "CommandListManager.hpp"
#include "System.hpp"
#include "Macros.hpp"

namespace GfxDeviceGlobal
{
    extern CommandListManager commandListManager;
}

std::vector< std::unique_ptr< CommandContext > > CommandContext::sContextPool;
std::queue< CommandContext* > CommandContext::sFreePool;

CommandContext* CommandContext::AllocateContext()
{
    CommandContext* ret = nullptr;

    if (sFreePool.empty())
    {
        ret = new CommandContext;
        sContextPool.emplace_back( ret );
        ret->Initialize( GfxDeviceGlobal::commandListManager );
    }
    else
    {
        ret = sFreePool.front();
        sFreePool.pop();
        ret->Reset();
    }

    return ret;
}

void CommandContext::Destroy()
{
    for (auto& ctx : sContextPool)
    {
        AE3D_SAFE_RELEASE( ctx->graphicsCommandList );
        AE3D_SAFE_RELEASE( ctx->currentAllocator );
    }
    
    while (!sFreePool.empty())
    {
        auto fctx = sFreePool.front();
        AE3D_SAFE_RELEASE( fctx->graphicsCommandList );
        AE3D_SAFE_RELEASE( fctx->currentAllocator );
        sFreePool.pop();
    }
}

void CommandContext::Reset()
{
    ae3d::System::Assert( graphicsCommandList != nullptr && currentAllocator == nullptr, "CommandContext was not initialized" );
    currentAllocator = owningManager->RequestAllocator();
    graphicsCommandList->Reset( currentAllocator, nullptr );
}

void CommandContext::Initialize( CommandListManager& commandListManager )
{
    owningManager = &commandListManager;
    
    if (!graphicsCommandList)
    {
        owningManager->CreateNewCommandList( &graphicsCommandList, &currentAllocator );
        graphicsCommandList->SetName( L"Command context graphics command list" );
    }
}

void CommandContext::TransitionResource( GpuResource& gpuResource, D3D12_RESOURCE_STATES newState )
{
    D3D12_RESOURCE_STATES oldState = gpuResource.usageState;

    if (oldState != newState)
    {
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

        graphicsCommandList->ResourceBarrier( 1, &BarrierDesc );
    }
    //else if (newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    //    InsertUAVBarrier( gpuResource, flushImmediate );
    //GfxDeviceGlobal::graphicsCommandList->ResourceBarrier( 1, &BarrierDesc );
}

CommandContext& CommandContext::Begin()
{
    CommandContext* outContext = CommandContext::AllocateContext();
    return *outContext;
}

void CommandContext::FreeContext( CommandContext* UsedContext )
{
    sFreePool.push( UsedContext );
}

std::uint64_t CommandContext::Finish( bool waitForCompletion )
{
    auto hr = graphicsCommandList->Close();
    AE3D_CHECK_D3D( hr, "command context command list close" );

    std::uint64_t fenceValue = owningManager->ExecuteCommandList( graphicsCommandList );
    owningManager->DiscardAllocator( fenceValue, currentAllocator );
    currentAllocator = nullptr;

    if (waitForCompletion)
    {
        owningManager->WaitForFence( fenceValue );
    }

    return fenceValue;
}

std::uint64_t CommandContext::CloseAndExecute( bool waitForCompletion )
{
    std::uint64_t fence = Finish( waitForCompletion );
    FreeContext( this );
    return fence;
}
