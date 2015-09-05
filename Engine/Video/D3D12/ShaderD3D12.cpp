#include "Shader.hpp"
#include <vector>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include "FileSystem.hpp"
#include "FileWatcher.hpp"
#include "GfxDevice.hpp"
#include "System.hpp"
#include "Texture2D.hpp"
#include "TextureCube.hpp"

#define AE3D_SAFE_RELEASE(x) if (x) { x->Release(); x = nullptr; }

extern ae3d::FileWatcher fileWatcher;

namespace GfxDeviceGlobal
{
    extern ID3D12Device* device;
    extern ID3D12DescriptorHeap* descHeapCbvSrvUav;
}

namespace Global
{
    std::vector< ID3DBlob* > shaders;
    //std::vector< ID3D12DescriptorHeap* > descHeaps;
    std::vector< ID3D12Resource* > constantBuffers;
}

void DestroyShaders()
{
    for (std::size_t i = 0; i < Global::shaders.size(); ++i)
    {
        AE3D_SAFE_RELEASE( Global::shaders[ i ] );
    }

    /*ae3d::System::Assert( Global::descHeaps.size() == Global::constantBuffers.size(), "sizes must equal" );

    for (std::size_t i = 0; i < Global::descHeaps.size(); ++i)
    {
        AE3D_SAFE_RELEASE( Global::descHeaps[ i ] );
        AE3D_SAFE_RELEASE( Global::constantBuffers[ i ] );
    }*/
}

namespace
{
    struct ShaderCacheEntry
    {
        ShaderCacheEntry( const std::string& vp, const std::string& fp, ae3d::Shader* aShader )
            : vertexPath( vp )
            , fragmentPath( fp )
            , shader( aShader ) {}

        std::string vertexPath;
        std::string fragmentPath;
        ae3d::Shader* shader = nullptr;
    };

    std::vector< ShaderCacheEntry > cacheEntries;
}

void ShaderReload( const std::string& path )
{
    for (const auto& entry : cacheEntries)
    {
        if (entry.vertexPath == path || entry.fragmentPath == path)
        {
            const auto vertexData = ae3d::FileSystem::FileContents( entry.vertexPath.c_str() );
            const std::string vertexStr = std::string( std::begin( vertexData.data ), std::end( vertexData.data ) );

            const auto fragmentData = ae3d::FileSystem::FileContents( entry.fragmentPath.c_str() );
            const std::string fragmentStr = std::string( std::begin( fragmentData.data ), std::end( fragmentData.data ) );

            entry.shader->Load( vertexStr.c_str(), fragmentStr.c_str() );
        }
    }
}

void ae3d::Shader::CreateConstantBuffer()
{
    /*D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 100;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NodeMask = 0;
    HRESULT hr = GfxDeviceGlobal::device->CreateDescriptorHeap( &desc, IID_PPV_ARGS( &mDescHeapCbvSrvUav ) );
    if (FAILED( hr ))
    {
        ae3d::System::Print( "Unable to create shader DescriptorHeap!" );
        return;
    }

    Global::descHeaps.push_back( mDescHeapCbvSrvUav );*/

    auto prop = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD );

    HRESULT hr = GfxDeviceGlobal::device->CreateCommittedResource(
        &prop,
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer( D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT ),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS( &constantBuffer ) );
    if (FAILED( hr ))
    {
        ae3d::System::Print( "Unable to create shader constant buffer!" );
        return;
    }

    Global::constantBuffers.push_back( constantBuffer );

    constantBuffer->SetName( L"ConstantBuffer" );
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = constantBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT; // must be a multiple of 256
    GfxDeviceGlobal::device->CreateConstantBufferView(
        &cbvDesc,
        GfxDeviceGlobal::descHeapCbvSrvUav->GetCPUDescriptorHandleForHeapStart() );
    hr = constantBuffer->Map( 0, nullptr, reinterpret_cast<void**>( &constantBufferUpload ) );
    if (FAILED( hr ))
    {
        ae3d::System::Print( "Unable to map shader constant buffer!" );
    }
}

void ae3d::Shader::Load( const char* vertexSource, const char* fragmentSource )
{
    const std::size_t vertexSourceLength = std::string( vertexSource ).size();
    ID3DBlob* blobError = nullptr;
    HRESULT hr = D3DCompile( vertexSource, vertexSourceLength, "VSMain", nullptr /*defines*/, nullptr, "VSMain", "vs_5_0", 0, 0, &blobShaderVertex, &blobError );
    if (FAILED( hr ))
    {
        ae3d::System::Print( "Unable to compile vertex shader: %s!\n", blobError->GetBufferPointer() );
        return;
    }

    const std::size_t pixelSourceLength = std::string( fragmentSource ).size();

    hr = D3DCompile( fragmentSource, pixelSourceLength, "PSMain", nullptr /*defines*/, nullptr, "PSMain", "ps_5_0", 0, 0, &blobShaderPixel, &blobError );
    if (FAILED( hr ))
    {
        ae3d::System::Print( "Unable to compile pixel shader: %s!\n", blobError->GetBufferPointer() );
        return;
    }

    Global::shaders.push_back( blobShaderVertex );
    Global::shaders.push_back( blobShaderPixel );

    CreateConstantBuffer();
    id = 1;
}

void ae3d::Shader::Load( const FileSystem::FileContentsData& /*vertexDataGLSL*/, const FileSystem::FileContentsData& /*fragmentDataGLSL*/,
                         const char* /*metalVertexShaderName*/, const char* /*metalFragmentShaderName*/,
                         const FileSystem::FileContentsData& vertexDataHLSL, const FileSystem::FileContentsData& fragmentDataHLSL )
{
    const std::string vertexStr = std::string( std::begin( vertexDataHLSL.data ), std::end( vertexDataHLSL.data ) );
    const std::string fragmentStr = std::string( std::begin( fragmentDataHLSL.data ), std::end( fragmentDataHLSL.data ) );

    Load( vertexStr.c_str(), fragmentStr.c_str() );

    bool isInCache = false;
    
    for (const auto& entry : cacheEntries)
    {
        if (entry.shader == this)
        {
            isInCache = true;
        }
    }
    
    if (!isInCache && !vertexDataHLSL.path.empty() && !fragmentDataHLSL.path.empty() && IsValid())
    {
        fileWatcher.AddFile( vertexDataHLSL.path.c_str(), ShaderReload );
        fileWatcher.AddFile( fragmentDataHLSL.path.c_str(), ShaderReload );
        cacheEntries.push_back( { vertexDataHLSL.path, fragmentDataHLSL.path, this } );
    }
}

void ae3d::Shader::Use()
{
}

void ae3d::Shader::SetMatrix( const char* /*name*/, const float* matrix4x4 )
{
    System::Assert( constantBufferUpload != nullptr, "CreateConstantBuffer probably not called!" );
    memcpy_s( constantBufferUpload, 64, matrix4x4, 64 );
}

void ae3d::Shader::SetTexture( const char* name, const ae3d::Texture2D* texture, int textureUnit )
{
    GfxDevice::IncTextureBinds();

    const std::string scaleOffsetName = std::string( name ) + std::string( "_ST" );
    SetVector4( scaleOffsetName.c_str(), &texture->GetScaleOffset().x );
}

void ae3d::Shader::SetTexture( const char* name, const ae3d::TextureCube* texture, int textureUnit )
{
    GfxDevice::IncTextureBinds();
    SetInt( name, textureUnit );
    
    const std::string scaleOffsetName = std::string( name ) + std::string( "_ST" );
    SetVector4( scaleOffsetName.c_str(), &texture->GetScaleOffset().x );
}

void ae3d::Shader::SetInt( const char* name, int value )
{
}

void ae3d::Shader::SetFloat( const char* name, float value )
{
}

void ae3d::Shader::SetVector3( const char* name, const float* vec3 )
{
}

void ae3d::Shader::SetVector4( const char* name, const float* vec4 )
{
}
