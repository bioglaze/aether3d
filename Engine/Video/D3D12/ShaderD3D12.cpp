#include "Shader.hpp"
#include <vector>
#include <string>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include "FileSystem.hpp"
#include "FileWatcher.hpp"
#include "DescriptorHeapManager.hpp"
#include "GfxDevice.hpp"
#include "System.hpp"
#include "Texture2D.hpp"
#include "TextureCube.hpp"
#include "RenderTexture.hpp"
#include "Macros.hpp"

extern ae3d::FileWatcher fileWatcher;

namespace GfxDeviceGlobal
{
    extern ID3D12Device* device;
}

namespace Global
{
    std::vector< ID3DBlob* > shaders;
    std::vector< ID3D12Resource* > constantBuffers;
}

void DestroyShaders()
{
    for (std::size_t i = 0; i < Global::shaders.size(); ++i)
    {
        AE3D_SAFE_RELEASE( Global::shaders[ i ] );
    }
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
    auto prop = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD );
    auto buf = CD3DX12_RESOURCE_DESC::Buffer( D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT );

    HRESULT hr = GfxDeviceGlobal::device->CreateCommittedResource(
        &prop,
        D3D12_HEAP_FLAG_NONE,
        &buf,
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
        DescriptorHeapManager::GetCbvSrvUavHeap()->GetCPUDescriptorHandleForHeapStart() );
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
    ReflectVariables();
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

void ae3d::Shader::ReflectVariables()
{
    HRESULT hr = D3DReflect( blobShaderVertex->GetBufferPointer(), blobShaderVertex->GetBufferSize(), IID_PPV_ARGS( &reflector ) );
    AE3D_CHECK_D3D( hr, "Shader reflection failed" );

    D3D12_SHADER_DESC descShader;
    reflector->GetDesc( &descShader );
    AE3D_CHECK_D3D( hr, "Shader desc reflection failed" );

    // FIXME: This doesn't differentiate between constant buffers, but currently I only have one [TimoW, 2015-10-11]
    for (UINT i = 0; i < descShader.ConstantBuffers; ++i)
    {
        auto buffer = reflector->GetConstantBufferByIndex( i );

        D3D12_SHADER_BUFFER_DESC desc;
        hr = buffer->GetDesc( &desc );
        AE3D_CHECK_D3D( hr, "Shader desc reflection failed" );

        auto var = buffer->GetVariableByIndex( 0 );
        D3D12_SHADER_VARIABLE_DESC descVar;
        hr = var->GetDesc( &descVar );

        uniformLocations[ descVar.Name ].i = descVar.StartOffset;
    }
}

void ae3d::Shader::Use()
{
}

void ae3d::Shader::SetMatrix( const char* name, const float* matrix4x4 )
{
    System::Assert( constantBufferUpload != nullptr, "CreateConstantBuffer probably not called!" );
    
    if (uniformLocations.find( name ) != std::end( uniformLocations ))
    {
        memcpy_s( (char*)constantBufferUpload + uniformLocations[ name ].i, 64, matrix4x4, 64 );
    }
}

void ae3d::Shader::SetTexture( const char* name, const ae3d::Texture2D* texture, int /*textureUnit*/ )
{
    const std::string scaleOffsetName = std::string( name ) + std::string( "_ST" );
    SetVector4( scaleOffsetName.c_str(), &texture->GetScaleOffset().x );
}

void ae3d::Shader::SetTexture( const char* name, const ae3d::TextureCube* texture, int textureUnit )
{
    SetInt( name, textureUnit );
    
    const std::string scaleOffsetName = std::string( name ) + std::string( "_ST" );
    SetVector4( scaleOffsetName.c_str(), &texture->GetScaleOffset().x );
}

void ae3d::Shader::SetRenderTexture( const char* name, const ae3d::RenderTexture* texture, int textureUnit )
{
    SetInt( name, textureUnit );

    const std::string scaleOffsetName = std::string( name ) + std::string( "_ST" );
    SetVector4( scaleOffsetName.c_str(), &texture->GetScaleOffset().x );
}

void ae3d::Shader::SetInt( const char* name, int value )
{
    if (uniformLocations.find( name ) != std::end( uniformLocations ))
    {
        memcpy_s( (char*)constantBufferUpload + uniformLocations[ name ].i, 4, &value, 4 );
    }
}

void ae3d::Shader::SetFloat( const char* name, float value )
{
    if (uniformLocations.find( name ) != std::end( uniformLocations ))
    {
        memcpy_s( (char*)constantBufferUpload + uniformLocations[ name ].i, 4, &value, 4 );
    }
}

void ae3d::Shader::SetVector3( const char* name, const float* vec3 )
{
    if (uniformLocations.find( name ) != std::end( uniformLocations ))
    {
        memcpy_s( (char*)constantBufferUpload + uniformLocations[ name ].i, 3 * 4, vec3, 3 * 4 );
    }
}

void ae3d::Shader::SetVector4( const char* name, const float* vec4 )
{
    if (uniformLocations.find( name ) != std::end( uniformLocations ))
    {
        memcpy_s( (char*)constantBufferUpload + uniformLocations[ name ].i, 4 * 4, vec4, 4 * 4 );
    }
}
