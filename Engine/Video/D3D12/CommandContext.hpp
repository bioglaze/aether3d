#ifndef COMMAND_CONTEXT_HPP
#define COMMAND_CONTEXT_HPP

#include <cstdint>
#include <vector>
#include <memory>
#include <queue>
#include "TextureBase.hpp"

class CommandContext
{
public:
    static CommandContext& Begin();
    void Reset();
    void Initialize( class CommandListManager& commandListManager );
    std::uint64_t CloseAndExecute( bool waitForCompletion );
    struct ID3D12GraphicsCommandList* graphicsCommandList = nullptr;
    void TransitionResource( GpuResource& gpuResource, D3D12_RESOURCE_STATES newState );

private:
    static void FreeContext( CommandContext* context );
  
    static CommandContext* AllocateContext();
    static std::vector< std::unique_ptr< CommandContext > > sContextPool;
    static std::queue< CommandContext* > sFreePool;
    
    std::uint64_t Finish( bool waitForCompletion );

    class CommandListManager* owningManager = nullptr;
    struct ID3D12CommandAllocator* currentAllocator = nullptr;
};
#endif
