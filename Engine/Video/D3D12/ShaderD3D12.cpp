// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
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

extern int AE3D_CB_SIZE;

extern ae3d::FileWatcher fileWatcher;

namespace GfxDeviceGlobal
{
    extern ID3D12Device* device;
    extern ae3d::TextureBase* texture0;
    extern ae3d::TextureBase* texture1;
    extern ae3d::TextureBase* texture3;
    extern ae3d::TextureBase* textureCube;
	extern PerObjectUboStruct perObjectUboStruct;
    extern ae3d::RenderTexture* currentRenderTarget;
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

void ClearPSOCache();

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

            if (entry.vertexPath.find( ".obj" ) != std::string::npos && entry.fragmentPath.find( ".obj" ) != std::string::npos)
            {
                wchar_t wstr[ 256 ];
                std::mbstowcs( wstr, entry.vertexPath.c_str(), 256 );

                HRESULT hr = D3DReadFileToBlob( wstr, &entry.shader->blobShaderVertex );
                AE3D_CHECK_D3D( hr, "Shader bytecode reading failed!" );

                std::mbstowcs( wstr, entry.fragmentPath.c_str(), 256 );
                hr = D3DReadFileToBlob( wstr, &entry.shader->blobShaderPixel );
                AE3D_CHECK_D3D( hr, "Shader bytecode reading failed!" );

                //ReflectVariables();
            }
            else
            {
                entry.shader->Load( vertexStr.c_str(), fragmentStr.c_str() );
            }

            ClearPSOCache();
        }
    }
}

int ae3d::Shader::GetUniformLocation( const char* name )
{
    for (unsigned i = 0; i < uniformLocations.count; ++i)
    {
        if (strcmp( uniformLocations[ i ].uniformName, name ) == 0)
        {
            return uniformLocations[ i ].offset;
        }
    }

    return -1;
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
    HRESULT hr = D3DCompile( vertexSource, vertexSourceLength, "main", nullptr /*defines*/, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_1", flags, 0, &blobShaderVertex, &blobError );

    if (FAILED( hr ))
    {
        ae3d::System::Print( "Unable to compile vertex shader %s: %s!\n", vertexPath.c_str(), blobError->GetBufferPointer() );
        ae3d::System::Assert( false, "");
        return;
    }

    const std::size_t pixelSourceLength = std::string( fragmentSource ).size();

    hr = D3DCompile( fragmentSource, pixelSourceLength, "main", nullptr /*defines*/, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_1", flags, 0, &blobShaderPixel, &blobError );

    if (FAILED( hr ))
    {
        ae3d::System::Print( "Unable to compile pixel shader %s: %s!\n", fragmentPath.c_str(), blobError->GetBufferPointer() );
        ae3d::System::Assert( false, "" );
        return;
    }

    Global::shaders.push_back( blobShaderVertex );
    Global::shaders.push_back( blobShaderPixel );

    //ReflectVariables();
}

void ae3d::Shader::Load( const char* /*metalVertexShaderName*/, const char* /*metalFragmentShaderName*/,
                         const FileSystem::FileContentsData& vertexDataHLSL, const FileSystem::FileContentsData& fragmentDataHLSL,
                         const FileSystem::FileContentsData& /*spirvData*/, const FileSystem::FileContentsData& /*spirvData*/ )
{
    const std::string vertexStr = std::string( std::begin( vertexDataHLSL.data ), std::end( vertexDataHLSL.data ) );
    const std::string fragmentStr = std::string( std::begin( fragmentDataHLSL.data ), std::end( fragmentDataHLSL.data ) );

    vertexPath = vertexDataHLSL.path;
    fragmentPath = fragmentDataHLSL.path;

    if (vertexPath.find( ".obj" ) != std::string::npos && fragmentPath.find( ".obj" ) != std::string::npos)
    {
        wchar_t wstr[ 256 ];
        std::mbstowcs( wstr, vertexPath.c_str(), 256 );

        HRESULT hr = D3DReadFileToBlob( wstr, &blobShaderVertex );
        AE3D_CHECK_D3D( hr, "Shader bytecode reading failed!" );

        std::mbstowcs( wstr, fragmentPath.c_str(), 256 );
        hr = D3DReadFileToBlob( wstr, &blobShaderPixel );
        AE3D_CHECK_D3D( hr, "Shader bytecode reading failed!" );

        Global::shaders.push_back( blobShaderVertex );
        Global::shaders.push_back( blobShaderPixel );

        //ReflectVariables();
    }
    else
    {
        Load( vertexStr.c_str(), fragmentStr.c_str() );
    }

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
        fileWatcher.AddFile( vertexDataHLSL.path, ShaderReload );
        fileWatcher.AddFile( fragmentDataHLSL.path, ShaderReload );
        cacheEntries.push_back( { vertexDataHLSL.path, fragmentDataHLSL.path, this } );
    }
}

void ae3d::Shader::ReflectVariables()
{
    ID3D12ShaderReflection* reflector = nullptr;

    HRESULT hr = D3DReflect( blobShaderVertex->GetBufferPointer(), blobShaderVertex->GetBufferSize(), IID_PPV_ARGS( &reflector ) );
    AE3D_CHECK_D3D( hr, "Shader reflection failed" );

    D3D12_SHADER_DESC descShader;
    reflector->GetDesc( &descShader );
    AE3D_CHECK_D3D( hr, "Shader desc reflection failed" );

    int uniformCount = 0;

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
            
            ++uniformCount;

            ae3d::System::Assert( descVar.StartOffset + descVar.Size < (unsigned)AE3D_CB_SIZE, "too big constant buffer" );
        }
    }

    uniformLocations.Allocate( uniformCount );
    uniformCount = 0;

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

            uniformLocations[ uniformCount ].offset = descVar.StartOffset;
            strncpy( uniformLocations[ uniformCount ].uniformName, descVar.Name, 128 );

            ++uniformCount;

            ae3d::System::Assert( descVar.StartOffset + descVar.Size < (unsigned)AE3D_CB_SIZE, "too big constant buffer" );
        }
    }
}

void ae3d::Shader::Use()
{
    System::Assert( IsValid(), "Shader not loaded" );
    GfxDevice::GetNewUniformBuffer();
}

void ae3d::Shader::SetUniform( int offset, void* data, int dataBytes )
{
    memcpy_s( (char*)GfxDevice::GetCurrentMappedConstantBuffer() + offset, AE3D_CB_SIZE, data, dataBytes );
}

void ae3d::Shader::SetTexture( ae3d::Texture2D* texture, int textureUnit )
{
    if (textureUnit == 0)
    {
        if( texture != nullptr )
        {
            GfxDeviceGlobal::perObjectUboStruct.tex0scaleOffset = texture->GetScaleOffset();
        }

        GfxDeviceGlobal::texture0 = texture ? texture : Texture2D::GetDefaultTexture();
    }
    else if (textureUnit == 1)
    {
        GfxDeviceGlobal::texture1 = texture ? texture : Texture2D::GetDefaultTexture();
    }
    else
    {
        //System::Assert( false, "unhandled texture unit!" );
    }
}

void ae3d::Shader::SetTexture( ae3d::TextureCube* texture, int textureUnit )
{
    GfxDeviceGlobal::textureCube = texture;

    if (texture != nullptr && textureUnit == 0)
    {
        GfxDeviceGlobal::perObjectUboStruct.tex0scaleOffset = texture->GetScaleOffset();
    }
}

void TransitionResource( GpuResource& gpuResource, D3D12_RESOURCE_STATES newState );

void ae3d::Shader::SetRenderTexture( ae3d::RenderTexture* texture, int textureUnit )
{
    // Prevents feedback.
    if (texture != nullptr && texture == GfxDeviceGlobal::currentRenderTarget)
    {
        return;
    }

    if (textureUnit == 0)
    {
        if( texture != nullptr )
        {
            GfxDeviceGlobal::perObjectUboStruct.tex0scaleOffset = texture->GetScaleOffset();
        }

        GfxDeviceGlobal::texture0 = texture;
    }
    else if (textureUnit == 1)
    {
        GfxDeviceGlobal::texture1 = texture;
    }
    else if (textureUnit == 3)
    {
        GfxDeviceGlobal::texture3 = texture;
    }
    else if (textureUnit == 4)
    {
        if (!texture->IsCube())
        {
            System::Print( "Shader::SetRenderTexture: texture in slot 4 must be a cube map!\n" );
            return;
        }

        GfxDeviceGlobal::textureCube = texture;
    }
    else
    {
        System::Assert( false, "unhandled texture unit!" );
    }

    if (texture != nullptr)
    {
        TransitionResource( *texture->GetGpuResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE );
    }
}
