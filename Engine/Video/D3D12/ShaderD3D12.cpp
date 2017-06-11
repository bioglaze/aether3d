#include "Shader.hpp"
#include <vector>
#include <string>
#include <d3d12.h>
#include <d3dcompiler.h>
#include "FileSystem.hpp"
#include "FileWatcher.hpp"
#include "GfxDevice.hpp"
#include "System.hpp"
#include "Texture2D.hpp"
#include "TextureCube.hpp"
#include "RenderTexture.hpp"
#include "Macros.hpp"

#define AE3D_CB_SIZE (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT * 3)

extern ae3d::FileWatcher fileWatcher;

namespace GfxDeviceGlobal
{
    extern ID3D12Device* device;
    extern ae3d::TextureBase* texture0;
    extern ae3d::TextureBase* texture1;
}

namespace Global
{
    std::vector< ID3DBlob* > shaders;
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

void ae3d::Shader::Load( const char* vertexSource, const char* fragmentSource )
{
    const std::size_t vertexSourceLength = std::string( vertexSource ).size();
    ID3DBlob* blobError = nullptr;
#if DEBUG
    const UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_ALL_RESOURCES_BOUND | D3DCOMPILE_WARNINGS_ARE_ERRORS;
#else
    const UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_ALL_RESOURCES_BOUND | D3DCOMPILE_WARNINGS_ARE_ERRORS;
#endif
    HRESULT hr = D3DCompile( vertexSource, vertexSourceLength, "main", nullptr /*defines*/, nullptr, "main", "vs_5_0", flags, 0, &blobShaderVertex, &blobError );

    if (FAILED( hr ))
    {
        ae3d::System::Print( "Unable to compile vertex shader %s: %s!\n", vertexPath.c_str(), blobError->GetBufferPointer() );
        ae3d::System::Assert( false, "");
        return;
    }

    const std::size_t pixelSourceLength = std::string( fragmentSource ).size();

    hr = D3DCompile( fragmentSource, pixelSourceLength, "main", nullptr /*defines*/, nullptr, "main", "ps_5_0", flags, 0, &blobShaderPixel, &blobError );

    if (FAILED( hr ))
    {
        ae3d::System::Print( "Unable to compile pixel shader %s: %s!\n", fragmentPath.c_str(), blobError->GetBufferPointer() );
        ae3d::System::Assert( false, "" );
        return;
    }

    Global::shaders.push_back( blobShaderVertex );
    Global::shaders.push_back( blobShaderPixel );

    ReflectVariables();
}

void ae3d::Shader::Load( const FileSystem::FileContentsData& /*vertexDataGLSL*/, const FileSystem::FileContentsData& /*fragmentDataGLSL*/,
                         const char* /*metalVertexShaderName*/, const char* /*metalFragmentShaderName*/,
                         const FileSystem::FileContentsData& vertexDataHLSL, const FileSystem::FileContentsData& fragmentDataHLSL,
                         const FileSystem::FileContentsData& /*spirvData*/, const FileSystem::FileContentsData& /*spirvData*/ )
{
    const std::string vertexStr = std::string( std::begin( vertexDataHLSL.data ), std::end( vertexDataHLSL.data ) );
    const std::string fragmentStr = std::string( std::begin( fragmentDataHLSL.data ), std::end( fragmentDataHLSL.data ) );

    vertexPath = vertexDataHLSL.path;
    fragmentPath = fragmentDataHLSL.path;

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

    for (UINT i = 0; i < descShader.ConstantBuffers; ++i)
    {
        auto buffer = reflector->GetConstantBufferByIndex( i );

        D3D12_SHADER_BUFFER_DESC desc;
        hr = buffer->GetDesc( &desc );
        AE3D_CHECK_D3D( hr, "Shader desc reflection failed" );

        for (UINT j = 0; j < desc.Variables; ++j)
        {
            auto var = buffer->GetVariableByIndex( j );
            D3D12_SHADER_VARIABLE_DESC descVar;
            hr = var->GetDesc( &descVar );
            AE3D_CHECK_D3D( hr, "Shader desc reflection failed" );

            uniformLocations[ std::string( descVar.Name ) ].i = descVar.StartOffset;
            ae3d::System::Assert( descVar.StartOffset + descVar.Size < AE3D_CB_SIZE, "too big constant buffer" );
        }
    }
}

void ae3d::Shader::Use()
{
    System::Assert( IsValid(), "Shader not loaded" );
    GfxDevice::CreateNewUniformBuffer();
}

void ae3d::Shader::SetMatrix( const char* name, const float* matrix4x4 )
{
    System::Assert( GfxDevice::GetCurrentUniformBuffer() != nullptr, "CreateNewUniformBuffer probably not called!" );
    
    const int offset = uniformLocations[ name ].i;

    if (offset != -1)
    {
        memcpy_s( (char*)GfxDevice::GetCurrentUniformBuffer() + offset, AE3D_CB_SIZE, matrix4x4, 64 );
    }
}

void ae3d::Shader::SetTexture( const char* name, ae3d::Texture2D* texture, int textureUnit )
{
    if (textureUnit == 0)
    {
        GfxDeviceGlobal::texture0 = texture;
    }
    else if (textureUnit == 1)
    {
        GfxDeviceGlobal::texture1 = texture;
    }

    const std::string scaleOffsetName = std::string( name ) + std::string( "_ST" );
    SetVector4( scaleOffsetName.c_str(), &texture->GetScaleOffset().x );
}

void ae3d::Shader::SetTexture( const char* name, ae3d::TextureCube* texture, int textureUnit )
{
    if (textureUnit == 0)
    {
        GfxDeviceGlobal::texture0 = texture;
    }
    else if (textureUnit == 1)
    {
        GfxDeviceGlobal::texture1 = texture;
    }

    const std::string scaleOffsetName = std::string( name ) + std::string( "_ST" );
    SetVector4( scaleOffsetName.c_str(), &texture->GetScaleOffset().x );
}

namespace GfxDeviceGlobal
{
    extern D3D12_CPU_DESCRIPTOR_HANDLE currentRenderTargetRTV;
}

//void TransitionResource( GpuResource& gpuResource, D3D12_RESOURCE_STATES newState );

void ae3d::Shader::SetRenderTexture( const char* name, ae3d::RenderTexture* texture, int textureUnit )
{
    if (textureUnit == 0)
    {
        GfxDeviceGlobal::texture0 = texture;
    }
    else if (textureUnit == 1)
    {
        GfxDeviceGlobal::texture1 = texture;
    }

    //TransitionResource( *texture->GetGpuResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE );

    const std::string scaleOffsetName = std::string( name ) + std::string( "_ST" );
    SetVector4( scaleOffsetName.c_str(), &texture->GetScaleOffset().x );
}

void ae3d::Shader::SetInt( const char* name, int value )
{
    System::Assert( GfxDevice::GetCurrentUniformBuffer() != nullptr, "CreateNewUniformBuffer probably not called!" );

    const int offset = uniformLocations[ name ].i;

    if (offset != -1)
    {
        memcpy_s( (char*)GfxDevice::GetCurrentUniformBuffer() + offset, AE3D_CB_SIZE, &value, 4 );
    }
}

void ae3d::Shader::SetFloat( const char* name, float value )
{
    System::Assert( GfxDevice::GetCurrentUniformBuffer() != nullptr, "CreateNewUniformBuffer probably not called!" );

    const int offset = uniformLocations[ name ].i;

    if (offset != -1)
    {
        memcpy_s( (char*)GfxDevice::GetCurrentUniformBuffer() + offset, AE3D_CB_SIZE, &value, 4 );
    }
}

void ae3d::Shader::SetVector3( const char* name, const float* vec3 )
{
    System::Assert( GfxDevice::GetCurrentUniformBuffer() != nullptr, "CreateNewUniformBuffer probably not called!" );

    const int offset = uniformLocations[ name ].i;

    if (offset != -1)
    {
        memcpy_s( (char*)GfxDevice::GetCurrentUniformBuffer() + offset, AE3D_CB_SIZE, vec3, 3 * 4 );
    }
}

void ae3d::Shader::SetVector4( const char* name, const float* vec4 )
{
    System::Assert( GfxDevice::GetCurrentUniformBuffer() != nullptr, "CreateNewUniformBuffer probably not called!" );

    const int offset = uniformLocations[ name ].i;

    if (offset != -1)
    {
        memcpy_s( (char*)GfxDevice::GetCurrentUniformBuffer() + offset, AE3D_CB_SIZE, vec4, 4 * 4 );
    }
}
