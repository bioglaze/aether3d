#include "DescriptorHeapManager.hpp"
#include "Macros.hpp"
#include "System.hpp"

ID3D12DescriptorHeap* DescriptorHeapManager::cbvSrvUavHeap = nullptr;
D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::currentCbvSrvUavHandle;

ID3D12DescriptorHeap* DescriptorHeapManager::samplerHeap = nullptr;
D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::currentSamplerHandle;

ID3D12DescriptorHeap* DescriptorHeapManager::rtvHeap = nullptr;
D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::currentRtvHandle;

namespace GfxDeviceGlobal
{
    extern ID3D12Device* device;
}

void DescriptorHeapManager::Deinit()
{
    AE3D_SAFE_RELEASE( cbvSrvUavHeap );
    AE3D_SAFE_RELEASE( samplerHeap );
    AE3D_SAFE_RELEASE( rtvHeap );
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE type )
{
    D3D12_CPU_DESCRIPTOR_HANDLE outHandle = {};

    if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    {
        if (!cbvSrvUavHeap)
        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            desc.NumDescriptors = 100;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            desc.NodeMask = 1;
            HRESULT hr = GfxDeviceGlobal::device->CreateDescriptorHeap( &desc, IID_PPV_ARGS( &cbvSrvUavHeap ) );
            AE3D_CHECK_D3D( hr, "Failed to create CBV_SRV_UAV descriptor heap" );
            currentCbvSrvUavHandle = cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
        }

        outHandle = currentCbvSrvUavHandle;
        currentCbvSrvUavHandle.ptr += GfxDeviceGlobal::device->GetDescriptorHandleIncrementSize( type );
    }
    else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    {
        if (!samplerHeap)
        {
            D3D12_DESCRIPTOR_HEAP_DESC desc;
            desc.Type = type;
            desc.NumDescriptors = 100;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            desc.NodeMask = 0;
            HRESULT hr = GfxDeviceGlobal::device->CreateDescriptorHeap( &desc, IID_PPV_ARGS( &samplerHeap ) );
            AE3D_CHECK_D3D( hr, "Failed to create sampler descriptor heap" );
            currentSamplerHandle = samplerHeap->GetCPUDescriptorHandleForHeapStart();
        }

        outHandle = currentSamplerHandle;
        currentSamplerHandle.ptr += GfxDeviceGlobal::device->GetDescriptorHandleIncrementSize( type );
    }
    else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
    {
        if (!rtvHeap)
        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.Type = type;
            desc.NumDescriptors = 10;
            desc.NodeMask = 1;
            HRESULT hr = GfxDeviceGlobal::device->CreateDescriptorHeap( &desc, IID_PPV_ARGS( &rtvHeap ) );
            AE3D_CHECK_D3D( hr, "Failed to create RTV descriptor heap" );
            currentRtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        }

        outHandle = currentRtvHandle;
        currentRtvHandle.ptr += GfxDeviceGlobal::device->GetDescriptorHandleIncrementSize( type );
    }
    else
    {
        ae3d::System::Assert( false, "unhandled descriptor heap type" );
    }

    return outHandle;
}
