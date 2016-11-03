#import <Foundation/Foundation.h>
#include "Shader.hpp"
#include "GfxDevice.hpp"
#include "Texture2D.hpp"
#include "TextureCube.hpp"
#include "RenderTexture.hpp"
#include "System.hpp"

extern id<MTLTexture> texture0;
extern id<MTLTexture> texture1;

namespace GfxDeviceGlobal
{
    void SetSampler( int textureUnit, ae3d::TextureFilter filter, ae3d::TextureWrap );
}

void ae3d::Shader::Load( const char* vertexSource, const char* fragmentSource )
{
}

void ae3d::Shader::Load( const FileSystem::FileContentsData& /*vertexGLSL*/, const FileSystem::FileContentsData& /*fragmentGLSL*/,
                        const char* aMetalVertexShaderName, const char* aMetalFragmentShaderName,
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
    for (MTLArgument *arg in reflection.vertexArguments)
    {
        //if ([arg.name isEqualToString:@"uniforms_t"])
        {
            if (arg.bufferDataType == MTLDataTypeStruct)
            {
                for( MTLStructMember* reflectedUniform in arg.bufferStructType.members )
                {
                    //NSLog(@"uniform: %@ type:%lu, location: %lu", reflectedUniform.name, (unsigned long)reflectedUniform.dataType, (unsigned long)reflectedUniform.offset);
                    
                    Uniform uniform;
                    
                    // FIXME: These values are not needed yet, but can be used for validation etc.
                    if (reflectedUniform.dataType == MTLDataTypeFloat)
                    {
                        uniform.type = UniformType::Float;
                    }
                    else if (reflectedUniform.dataType == MTLDataTypeFloat2)
                    {
                        uniform.type = UniformType::Float2;
                    }
                    else if (reflectedUniform.dataType == MTLDataTypeFloat3)
                    {
                        uniform.type = UniformType::Float3;
                    }
                    else if (reflectedUniform.dataType == MTLDataTypeFloat4)
                    {
                        uniform.type = UniformType::Float4;
                    }
                    else if (reflectedUniform.dataType == MTLDataTypeFloat4x4)
                    {
                        uniform.type = UniformType::Matrix4x4;
                    }
                    else
                    {
                        System::Print( "Shader has unhandled uniform type %d. Uniform name: %d\n", reflectedUniform.dataType, reflectedUniform.name.UTF8String );
                    }
                    
                    uniform.offsetFromBufferStart = reflectedUniform.offset;
                    System::Assert( uniform.offsetFromBufferStart + 16 * 4 < 256, "Uniform buffer is too small" );

                    uniforms[ reflectedUniform.name.UTF8String ] = uniform;
                }
            }
        }
    }
}

void ae3d::Shader::SetMatrix( const char* name, const float* matrix4x4 )
{
    if (uniforms.find( name ) == std::end( uniforms ))
    {
        //System::Print( "SetMatrix: could not find %s\n", name );
        return;
    }
    
    id<MTLBuffer> uniformBuffer = GfxDevice::GetCurrentUniformBuffer();
    uint8_t* bufferPointer = (uint8_t *)[uniformBuffer contents] + uniforms[ name ].offsetFromBufferStart;
    memcpy( bufferPointer, matrix4x4, 16 * 4 );
}

void ae3d::Shader::SetTexture( const char* name, Texture2D* texture, int textureUnit )
{
    if (texture != nullptr)
    {
        if (textureUnit == 0)
        {
            texture0 = texture->GetMetalTexture();
        }
        else if (textureUnit == 1)
        {
            texture1 = texture->GetMetalTexture();
        }
        else
        {
            System::Print( "Shader %s tried to set a texture %s into unit that is not handled\n", metalVertexShaderName.c_str(), name );
        }

        GfxDeviceGlobal::SetSampler( textureUnit, texture->GetFilter(), texture->GetWrap() );
    }
    else
    {
        System::Print( "Shader tried to set null texture\n" );
    }
}

void ae3d::Shader::SetRenderTexture( const char* name, ae3d::RenderTexture* renderTexture, int textureUnit )
{
    if (renderTexture != nullptr)
    {
        if (textureUnit == 0)
        {
            texture0 = renderTexture->GetMetalTexture();
        }
        else if (textureUnit == 1)
        {
            texture1 = renderTexture->GetMetalTexture();
        }
        else
        {
            System::Print( "Shader %s tried to set a texture %s into unit that is not handled\n", metalVertexShaderName.c_str(), name );
        }

        GfxDeviceGlobal::SetSampler( textureUnit, renderTexture->GetFilter(), renderTexture->GetWrap() );
    }
    else
    {
        System::Print( "Shader tried to set null texture\n" );
    }
}

void ae3d::Shader::SetTexture( const char* name, TextureCube* texture, int textureUnit )
{
    if (texture != nullptr)
    {
        if (textureUnit == 0)
        {
            texture0 = texture->GetMetalTexture();
        }
        else if (textureUnit == 1)
        {
            texture1 = texture->GetMetalTexture();
        }
        else
        {
            System::Print( "Shader %s tried to set a texture %s into unit that is not handled\n", metalVertexShaderName.c_str(), name );
        }
        
        GfxDeviceGlobal::SetSampler( textureUnit, texture->GetFilter(), texture->GetWrap() );
    }
    else
    {
        System::Print( "Shader tried to set null texture\n" );
    }
}

void ae3d::Shader::SetInt( const char* name, int value )
{
    System::Print( "Shader::SetInt unimplemented. Tried to set %s to %d\n", name, value );
}

void ae3d::Shader::SetFloat( const char* name, float value )
{
    if (uniforms.find( name ) == std::end( uniforms ))
    {
        return;
    }
    
    id<MTLBuffer> uniformBuffer = GfxDevice::GetCurrentUniformBuffer();
    uint8_t* bufferPointer = (uint8_t *)[uniformBuffer contents] + uniforms[ name ].offsetFromBufferStart;
    memcpy( bufferPointer, &value, 4 );
}

void ae3d::Shader::SetVector3( const char* name, const float* vec3 )
{
    if (uniforms.find( name ) == std::end( uniforms ))
    {
        return;
    }
    
    id<MTLBuffer> uniformBuffer = GfxDevice::GetCurrentUniformBuffer();
    uint8_t* bufferPointer = (uint8_t *)[uniformBuffer contents] + uniforms[ name ].offsetFromBufferStart;
    memcpy( bufferPointer, vec3, 3 * 4 );
}

void ae3d::Shader::SetVector4( const char* name, const float* vec4 )
{
    if (uniforms.find( name ) == std::end( uniforms ))
    {
        return;
    }
    
    id<MTLBuffer> uniformBuffer = GfxDevice::GetCurrentUniformBuffer();
    uint8_t* bufferPointer = (uint8_t *)[uniformBuffer contents] + uniforms[ name ].offsetFromBufferStart;
    memcpy( bufferPointer, vec4, 4 * 4 );
}

