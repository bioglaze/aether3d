#include "Shader.hpp"
#include <vector>
#include <d3d12.h>
#include <d3dcompiler.h>
#include "FileSystem.hpp"
#include "FileWatcher.hpp"
#include "GfxDevice.hpp"
#include "System.hpp"
#include "Texture2D.hpp"
#include "TextureCube.hpp"

#define AE3D_SAFE_RELEASE(x) if (x) { x->Release(); x = nullptr; }

extern ae3d::FileWatcher fileWatcher;

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
    ID3DBlob* blobShaderVert = nullptr;
    ID3DBlob* blobError = nullptr;
    HRESULT hr = D3DCompile( vertexSource, vertexSourceLength, "VSMain", nullptr /*defines*/, nullptr, "VSMain", "vs_5_0", 0, 0, &blobShaderVert, &blobError );
    if (FAILED( hr ))
    {
        ae3d::System::Print( "Unable to compile vertex shader: %s!\n", blobError->GetBufferPointer() );
        return;
    }

    ID3DBlob* blobShaderPixel = nullptr;
    const std::size_t pixelSourceLength = std::string( fragmentSource ).size();

    hr = D3DCompile( fragmentSource, pixelSourceLength, "PSMain", nullptr /*defines*/, nullptr, "PSMain", "ps_5_0", 0, 0, &blobShaderPixel, &blobError );
    if (FAILED( hr ))
    {
        ae3d::System::Print( "Unable to compile pixel shader: %s!\n", blobError->GetBufferPointer() );
        return;
    }

    D3D12_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    UINT numElements = sizeof( layout ) / sizeof( layout[ 0 ] );

    AE3D_SAFE_RELEASE( blobShaderVert );
    AE3D_SAFE_RELEASE( blobShaderPixel );
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
    
    if (!isInCache && !vertexDataHLSL.path.empty() && !fragmentDataHLSL.path.empty())
    {
        fileWatcher.AddFile( vertexDataHLSL.path.c_str(), ShaderReload );
        fileWatcher.AddFile( fragmentDataHLSL.path.c_str(), ShaderReload );
        cacheEntries.push_back( { vertexDataHLSL.path, fragmentDataHLSL.path, this } );
    }
}

void ae3d::Shader::Use()
{
}

void ae3d::Shader::SetMatrix( const char* /*name*/, const float* /*matrix4x4*/ )
{
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
