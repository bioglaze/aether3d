#ifndef DESCRIPTOR_HEAP_MANAGER
#define DESCRIPTOR_HEAP_MANAGER

#include <d3d12.h>

class DescriptorHeapManager
{
public:
    static ID3D12DescriptorHeap* GetCbvSrvUavHeap() { return cbvSrvUavHeap; }
    static ID3D12DescriptorHeap* GetSamplerHeap() { return samplerHeap; }
    static ID3D12DescriptorHeap* GetRTVHeap() { return rtvHeap; }
    static ID3D12DescriptorHeap* GetDSVHeap() { return dsvHeap; }
    static D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE type );
    static void Deinit();

private:
    static ID3D12DescriptorHeap* cbvSrvUavHeap;
    static D3D12_CPU_DESCRIPTOR_HANDLE currentCbvSrvUavHandle;

    static ID3D12DescriptorHeap* samplerHeap;
    static D3D12_CPU_DESCRIPTOR_HANDLE currentSamplerHandle;

    static ID3D12DescriptorHeap* rtvHeap;
    static D3D12_CPU_DESCRIPTOR_HANDLE currentRtvHandle;

    static ID3D12DescriptorHeap* dsvHeap;
    static D3D12_CPU_DESCRIPTOR_HANDLE currentDsvHandle;
};
#endif
