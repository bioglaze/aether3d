#include "ComputeShader.hpp"
#include <d3d12.h>
#include <d3dcompiler.h>
#include "FileSystem.hpp"
#include "GfxDevice.hpp"
#include "Macros.hpp"
#include "System.hpp"
#include "TextureBase.hpp"
#include "Texture2D.hpp"

extern int AE3D_CB_SIZE;

void TransitionResource( GpuResource& gpuResource, D3D12_RESOURCE_STATES newState );

namespace GfxDeviceGlobal
{
    extern ID3D12Device* device;
    extern ID3D12GraphicsCommandList* graphicsCommandList;
    extern ID3D12RootSignature* rootSignatureTileCuller;
    extern ID3D12DescriptorHeap* computeCbvSrvUavHeap;
    extern D3D12_UNORDERED_ACCESS_VIEW_DESC uav1Desc;
    extern ID3D12PipelineState* cachedPSO;
	extern PerObjectUboStruct perObjectUboStruct;
}

namespace Global
{
    std::vector< ID3DBlob* > computeShaders;
    std::vector< ID3D12PipelineState* > psos;
}

void DestroyComputeShaders()
{
    for (std::size_t i = 0; i < Global::computeShaders.size(); ++i)
    {
        AE3D_SAFE_RELEASE( Global::computeShaders[ i ] );
    }

    for (std::size_t i = 0; i < Global::psos.size(); ++i)
    {
        AE3D_SAFE_RELEASE( Global::psos[ i ] );
    }
}

void ae3d::ComputeShader::Begin()
{
}

void ae3d::ComputeShader::End()
{
}

void ae3d::ComputeShader::SetTexture2D( unsigned slot, Texture2D* texture )
{
    // FIXME: This is a hack
    SetUAVBuffer( slot, texture->GetGpuResource()->resource );
}

void ae3d::ComputeShader::SetBlurDirection( float x, float y )
{
    GfxDeviceGlobal::perObjectUboStruct.tilesXY.z = x;
    GfxDeviceGlobal::perObjectUboStruct.tilesXY.w = y;
}

void ae3d::ComputeShader::Dispatch( unsigned groupCountX, unsigned groupCountY, unsigned groupCountZ )
{
    System::Assert( GfxDeviceGlobal::graphicsCommandList != nullptr, "graphics command list not initialized" );
    System::Assert( GfxDeviceGlobal::computeCbvSrvUavHeap != nullptr, "heap not initialized" );

    if (!pso)
    {
        D3D12_COMPUTE_PIPELINE_STATE_DESC descPso = {};
        descPso.CS = { reinterpret_cast<BYTE*>(blobShader->GetBufferPointer()), blobShader->GetBufferSize() };
        descPso.pRootSignature = GfxDeviceGlobal::rootSignatureTileCuller;

        HRESULT hr = GfxDeviceGlobal::device->CreateComputePipelineState( &descPso, IID_PPV_ARGS( &pso ) );
        AE3D_CHECK_D3D( hr, "Failed to create compute PSO" );
        pso->SetName( L"PSO" );
        Global::psos.push_back( pso );
    }

    SetUniformBuffer( 0, (ID3D12Resource*)GfxDevice::GetCurrentConstantBuffer() );

    D3D12_CPU_DESCRIPTOR_HANDLE handle = GfxDeviceGlobal::computeCbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = uniformBuffers[ 0 ]->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = AE3D_CB_SIZE;
    GfxDeviceGlobal::device->CreateConstantBufferView( &cbvDesc, handle );

    handle.ptr += GfxDeviceGlobal::device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc0 = {};
    srvDesc0.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srvDesc0.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc0.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc0.Buffer.FirstElement = 0;
    srvDesc0.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    srvDesc0.Buffer.NumElements = 2048; // FIXME: Sync with LightTiler
    srvDesc0.Buffer.StructureByteStride = 0;

    GfxDeviceGlobal::device->CreateShaderResourceView( textureBuffers[ 0 ], &srvDesc0, handle );

    handle.ptr += GfxDeviceGlobal::device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc1 = {};
    srvDesc1.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srvDesc1.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc1.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc1.Texture2D.MipLevels = 1;
    srvDesc1.Texture2D.MostDetailedMip = 0;
    srvDesc1.Texture2D.PlaneSlice = 0;
    srvDesc1.Texture2D.ResourceMinLODClamp = 0.0f;

    GfxDeviceGlobal::device->CreateShaderResourceView( textureBuffers[ 1 ], &srvDesc1, handle );

    handle.ptr += GfxDeviceGlobal::device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2 = {};
    srvDesc2.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc2.Buffer.FirstElement = 0;
    srvDesc2.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    srvDesc2.Buffer.NumElements = 2048; // FIXME: Sync with LightTiler
    srvDesc2.Buffer.StructureByteStride = 0;

    GfxDeviceGlobal::device->CreateShaderResourceView( textureBuffers[ 2 ], &srvDesc2, handle );

    handle.ptr += GfxDeviceGlobal::device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

    GfxDeviceGlobal::device->CreateUnorderedAccessView( uavBuffers[ 0 ], nullptr, &GfxDeviceGlobal::uav1Desc, handle );

    GpuResource depthNormals = {};
    depthNormals.resource = textureBuffers[ 1 ];
    depthNormals.usageState = D3D12_RESOURCE_STATE_RENDER_TARGET;

    if (depthNormals.resource != nullptr)
    {
        TransitionResource( depthNormals, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE );
    }

    GfxDeviceGlobal::cachedPSO = pso;
    GfxDeviceGlobal::graphicsCommandList->SetPipelineState( pso );
    GfxDeviceGlobal::graphicsCommandList->SetDescriptorHeaps( 1, &GfxDeviceGlobal::computeCbvSrvUavHeap );
    GfxDeviceGlobal::graphicsCommandList->SetComputeRootSignature( GfxDeviceGlobal::rootSignatureTileCuller );
    GfxDeviceGlobal::graphicsCommandList->SetComputeRootDescriptorTable( 0, GfxDeviceGlobal::computeCbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart() );
    GfxDeviceGlobal::graphicsCommandList->Dispatch( groupCountX, groupCountY, groupCountZ );

    if (depthNormals.resource != nullptr)
    {
        TransitionResource( depthNormals, D3D12_RESOURCE_STATE_RENDER_TARGET );
    }
}

void ae3d::ComputeShader::Load( const char* source )
{
    const std::size_t sourceLength = std::string( source ).size();
    ID3DBlob* blobError = nullptr;
    HRESULT hr = D3DCompile( source, sourceLength, "CSMain", nullptr /*defines*/, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CSMain", "cs_5_1",
#if DEBUG
        D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_ALL_RESOURCES_BOUND | D3DCOMPILE_WARNINGS_ARE_ERRORS, 0, &blobShader, &blobError );
#else
        D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_ALL_RESOURCES_BOUND | D3DCOMPILE_WARNINGS_ARE_ERRORS, 0, &blobShader, &blobError );
#endif
    if (FAILED( hr ))
    {
        ae3d::System::Print( "Unable to compile compute shader: %s!\n", blobError->GetBufferPointer() );
        blobError->Release();
        return;
    }

    Global::computeShaders.push_back( blobShader );
}

void ae3d::ComputeShader::Load( const char* /*metalShaderName*/, const FileSystem::FileContentsData& dataHLSL, const FileSystem::FileContentsData& /*dataSPIRV*/ )
{
    for (int slotIndex = 0; slotIndex < SLOT_COUNT; ++slotIndex)
    {
        uniformBuffers[ slotIndex ] = nullptr;
        textureBuffers[ slotIndex ] = nullptr;
        uavBuffers[ slotIndex ] = nullptr;
    }

    if (dataHLSL.path.find( ".obj" ) != std::string::npos)
    {
        wchar_t wstr[ 256 ];
        std::mbstowcs( wstr, dataHLSL.path.c_str(), 256 );

        HRESULT hr = D3DReadFileToBlob( wstr, &blobShader );
        AE3D_CHECK_D3D( hr, "Shader bytecode reading failed!" );
        
        Global::computeShaders.push_back( blobShader );
    }
    else
    {
        const std::string dataStr = std::string( std::begin( dataHLSL.data ), std::end( dataHLSL.data ) );
        Load( dataStr.c_str() );
    }
}

void ae3d::ComputeShader::SetRenderTexture( unsigned slot, RenderTexture* renderTexture )
{
    if (slot < SLOT_COUNT)
    {
        renderTextures[ slot ] = renderTexture;
    }
    else
    {
        System::Print( "SetRenderTexture: too high slot, max is 3\n" );
    }
}

void ae3d::ComputeShader::SetUniformBuffer( unsigned slot, ID3D12Resource* buffer )
{
    if (slot < SLOT_COUNT)
    {
        uniformBuffers[ slot ] = buffer;
    }
    else
    {
        System::Print( "SetUniformBuffer: too high slot, max is 3\n" );
    }
}

void ae3d::ComputeShader::SetTextureBuffer( unsigned slot, ID3D12Resource* buffer )
{
    if (slot < SLOT_COUNT)
    {
        textureBuffers[ slot ] = buffer;
    }
    else
    {
        System::Print( "SetTextureBuffer: too high slot, max is 3\n" );
    }
}

void ae3d::ComputeShader::SetUAVBuffer( unsigned slot, ID3D12Resource* buffer )
{
    if (slot < SLOT_COUNT)
    {
        uavBuffers[ slot ] = buffer;
    }
    else
    {
        System::Print( "SetUAVBuffer: too high slot, max is 3\n" );
    }
}
