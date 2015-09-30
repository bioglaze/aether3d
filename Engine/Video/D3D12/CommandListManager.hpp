#ifndef COMMAND_LIST_MANAGER
#define COMMAND_LIST_MANAGER

#include <cstdint>
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>

// Based on Microsoft's MiniEngine's command list manager.
class CommandListManager
{
public:
    void Create( struct ID3D12Device* device );
    void Destroy();
    struct ID3D12CommandQueue* GetCommandQueue() { return commandQueue; }

    void CreateNewCommandList( struct ID3D12GraphicsCommandList** outList, struct ID3D12CommandAllocator** outAllocator );
    std::uint64_t ExecuteCommandList( struct ID3D12CommandList* list );

    std::uint64_t IncrementFence();
    bool IsFenceComplete( std::uint64_t fenceValue );
    void WaitForFence( std::uint64_t fenceValue );

    void DiscardAllocator( std::uint64_t fenceValue, ID3D12CommandAllocator* allocator );
    ID3D12CommandAllocator* RequestAllocator();

private:
    struct ID3D12Fence* fence = nullptr;
    ID3D12Device* device = nullptr;
    ID3D12CommandQueue* commandQueue = nullptr;
    ID3D12CommandAllocator* commandListAllocator = nullptr;
    HANDLE fenceEvent;
    std::uint64_t lastCompletedFenceValue = 0;
    std::uint64_t nextFenceValue = 0;
};

#endif
