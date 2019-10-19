// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "DescriptorHeapManager.hpp"
#include "Macros.hpp"
#include "System.hpp"

ID3D12DescriptorHeap* DescriptorHeapManager::cbvSrvUavHeap = nullptr;
D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::currentCbvSrvUavHandle = {};

ID3D12DescriptorHeap* DescriptorHeapManager::samplerHeap = nullptr;
D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::currentSamplerHandle = {};

ID3D12DescriptorHeap* DescriptorHeapManager::rtvHeap = nullptr;
D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::currentRtvHandle = {};

ID3D12DescriptorHeap* DescriptorHeapManager::dsvHeap = nullptr;
D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::currentDsvHandle = {};

namespace GfxDeviceGlobal
{
    extern ID3D12Device* device;
}

void DescriptorHeapManager::Deinit()
{
    AE3D_SAFE_RELEASE( cbvSrvUavHeap );
    AE3D_SAFE_RELEASE( samplerHeap );
    AE3D_SAFE_RELEASE( rtvHeap );
    AE3D_SAFE_RELEASE( dsvHeap );
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetCbvSrvUavGpuHandle( unsigned index )
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle = cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += index * GfxDeviceGlobal::device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetCbvSrvUavCpuHandle( unsigned index )
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += index * GfxDeviceGlobal::device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE type )
{
    D3D12_CPU_DESCRIPTOR_HANDLE outHandle = {};

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = type;
    desc.NumDescriptors = type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 2048 : DescriptorHeapManager::numDescriptors;
    desc.Flags = (type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV || type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV) ? (D3D12_DESCRIPTOR_HEAP_FLAGS)0 : D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NodeMask = 0;

    if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    {
        if (!cbvSrvUavHeap)
        {
            HRESULT hr = GfxDeviceGlobal::device->CreateDescriptorHeap( &desc, IID_PPV_ARGS( &cbvSrvUavHeap ) );
            AE3D_CHECK_D3D( hr, "Failed to create CBV_SRV_UAV descriptor heap" );
            cbvSrvUavHeap->SetName( L"CBV_SRV_UAV Heap" );
            currentCbvSrvUavHandle = cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
        }

        outHandle = currentCbvSrvUavHandle;
        currentCbvSrvUavHandle.ptr += GfxDeviceGlobal::device->GetDescriptorHandleIncrementSize( type );
    }
    else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    {
        if (!samplerHeap)
        {
            HRESULT hr = GfxDeviceGlobal::device->CreateDescriptorHeap( &desc, IID_PPV_ARGS( &samplerHeap ) );
            AE3D_CHECK_D3D( hr, "Failed to create sampler descriptor heap" );
            samplerHeap->SetName( L"Sampler Heap" );
            currentSamplerHandle = samplerHeap->GetCPUDescriptorHandleForHeapStart();
        }

        outHandle = currentSamplerHandle;
        currentSamplerHandle.ptr += GfxDeviceGlobal::device->GetDescriptorHandleIncrementSize( type );
    }
    else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
    {
        if (!rtvHeap)
        {
            HRESULT hr = GfxDeviceGlobal::device->CreateDescriptorHeap( &desc, IID_PPV_ARGS( &rtvHeap ) );
            AE3D_CHECK_D3D( hr, "Failed to create RTV descriptor heap" );
            rtvHeap->SetName( L"RTV Heap" );
            currentRtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        }

        outHandle = currentRtvHandle;
        currentRtvHandle.ptr += GfxDeviceGlobal::device->GetDescriptorHandleIncrementSize( type );
    }
    else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
    {
        if (!dsvHeap)
        {
            HRESULT hr = GfxDeviceGlobal::device->CreateDescriptorHeap( &desc, IID_PPV_ARGS( &dsvHeap ) );
            AE3D_CHECK_D3D( hr, "Failed to create DSV descriptor heap" );
            dsvHeap->SetName( L"DSV Heap" );
            currentDsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
        }

        outHandle = currentDsvHandle;
        currentDsvHandle.ptr += GfxDeviceGlobal::device->GetDescriptorHandleIncrementSize( type );
    }
    else
    {
        ae3d::System::Assert( false, "unhandled descriptor heap type" );
    }

    return outHandle;
}
