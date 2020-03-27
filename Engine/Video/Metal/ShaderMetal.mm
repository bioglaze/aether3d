#import <Foundation/Foundation.h>
#include <string.h>
#include "Shader.hpp"
#include "GfxDevice.hpp"
#include "Texture2D.hpp"
#include "TextureCube.hpp"
#include "RenderTexture.hpp"
#include "System.hpp"

extern id<MTLTexture> textures[ 13 ];

namespace GfxDeviceGlobal
{
    void SetSampler( int textureUnit, ae3d::TextureFilter filter, ae3d::TextureWrap wrap, ae3d::Anisotropy anisotropy );
    extern PerObjectUboStruct perObjectUboStruct;
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
    NSLog( @"Shader: Unimplemented method Load()\n" );
}

void ae3d::Shader::Load( const char* aMetalVertexShaderName, const char* aMetalFragmentShaderName,
                        const FileSystem::FileContentsData& /*vertexHLSL*/, const FileSystem::FileContentsData& /*fragmentHLSL*/,
                        const FileSystem::FileContentsData& /*vertexSPIRV*/, const FileSystem::FileContentsData& /*fragmentSPIRV*/)
{
    LoadFromLibrary( aMetalVertexShaderName, aMetalFragmentShaderName );
}

void ae3d::Shader::LoadFromLibrary( const char* vertexShaderName, const char* fragmentShaderName )
{
    NSString* vertexName = [NSString stringWithUTF8String:vertexShaderName ];
    vertexProgram = [GfxDevice::GetDefaultMetalShaderLibrary() newFunctionWithName:vertexName];
    metalVertexShaderName = std::string( vertexShaderName );

    if (vertexProgram == nullptr)
    {
        NSLog( @"Shader: Could not load %s!\n", vertexShaderName );
        return;
    }
    
    NSString* fragmentName = [NSString stringWithUTF8String:fragmentShaderName ];
    fragmentProgram = [GfxDevice::GetDefaultMetalShaderLibrary() newFunctionWithName:fragmentName];
    
    if (fragmentProgram == nullptr)
    {
        NSLog( @"Shader: Could not load %s!\n", fragmentShaderName );
        return;
    }
}

void ae3d::Shader::Use()
{
    System::Assert( IsValid(), "Shader not loaded" );
    GfxDevice::GetNewUniformBuffer();
}

void ae3d::Shader::LoadUniforms( MTLRenderPipelineReflection* reflection )
{
    int count = 0;
    
    for (MTLArgument *arg in reflection.vertexArguments)
    {
        if (arg.bufferDataType == MTLDataTypeStruct)
        {
            for( MTLStructMember* reflectedUniform in arg.bufferStructType.members )
            {
                System::Assert( reflectedUniform.offset + 16 * 4 < GfxDevice::UNIFORM_BUFFER_SIZE, "Uniform buffer is too small" );
                ++count;
            }
        }
    }
    
    uniformLocations.Allocate( count );
    count = 0;
    
    for (MTLArgument *arg in reflection.vertexArguments)
    {
        if (arg.bufferDataType == MTLDataTypeStruct)
        {
            for( MTLStructMember* reflectedUniform in arg.bufferStructType.members )
            {
                uniformLocations[ count ].offset = (int)reflectedUniform.offset;
                strncpy( uniformLocations[ count ].uniformName, reflectedUniform.name.UTF8String, 128 );
                ++count;
            }
        }
    }
}

void ae3d::Shader::SetTexture( Texture2D* texture, int textureUnit )
{
    if (texture != nullptr)
    {
        if (textureUnit == 0)
        {
            textures[ textureUnit ] = texture->GetMetalTexture();
            GfxDeviceGlobal::perObjectUboStruct.tex0scaleOffset = texture->GetScaleOffset();
        }
        else if (textureUnit < 5)
        {
            textures[ textureUnit ] = texture->GetMetalTexture();
        }
        else
        {
            System::Print( "Shader %s tried to set a texture2D into unit %d that is not handled\n", metalVertexShaderName.c_str(), textureUnit );
        }

        GfxDeviceGlobal::SetSampler( textureUnit, texture->GetFilter(), texture->GetWrap(), texture->GetAnisotropy() );
    }
}

void ae3d::Shader::SetRenderTexture( ae3d::RenderTexture* renderTexture, int textureUnit )
{
    if (renderTexture != nullptr)
    {
        if (textureUnit == 0)
        {
            textures[ 0 ] = renderTexture->GetMetalTexture();
            GfxDeviceGlobal::perObjectUboStruct.tex0scaleOffset = renderTexture->GetScaleOffset();
        }
        else if (textureUnit == 1)
        {
            textures[ 1 ] = renderTexture->GetMetalTexture();
        }
        else if (textureUnit == 2)
        {
            textures[ 2 ] = renderTexture->GetMetalTexture();
        }
        else if (textureUnit == 3)
        {
            textures[ 3 ] = renderTexture->GetMetalTexture();
        }
        else if (textureUnit == 4)
        {
            textures[ 4 ] = renderTexture->GetMetalTexture();
        }
        else
        {
            System::Print( "Shader %s tried to set a render texture into unit %d that is not handled\n", metalVertexShaderName.c_str(), textureUnit );
        }

        GfxDeviceGlobal::SetSampler( textureUnit, renderTexture->GetFilter(), renderTexture->GetWrap(), renderTexture->GetAnisotropy() );
    }
}

void ae3d::Shader::SetTexture( TextureCube* texture, int textureUnit )
{
    if (texture != nullptr)
    {
        if (textureUnit == 0)
        {
            textures[ 0 ] = texture->GetMetalTexture();
            GfxDeviceGlobal::perObjectUboStruct.tex0scaleOffset = texture->GetScaleOffset();
        }
        else if (textureUnit < 13)
        {
            textures[ textureUnit ] = texture->GetMetalTexture();
        }
        else
        {
            System::Print( "Shader %s tried to set a cube texture into unit %d that is not handled\n", metalVertexShaderName.c_str(), textureUnit );
        }
        
        GfxDeviceGlobal::SetSampler( textureUnit, texture->GetFilter(), texture->GetWrap(), texture->GetAnisotropy() );
    }
}

