#import <Foundation/Foundation.h>
#include "Shader.hpp"
#include "GfxDevice.hpp"
#include "Texture2D.hpp"
#include "TextureCube.hpp"
#include "System.hpp"

extern id<MTLTexture> texture0;

void ae3d::Shader::Load( const char* vertexSource, const char* fragmentSource )
{
}

void ae3d::Shader::Load( const FileSystem::FileContentsData& /*vertexGLSL*/, const FileSystem::FileContentsData& /*fragmentGLSL*/,
                        const char* metalVertexShaderName, const char* metalFragmentShaderName,
                        const FileSystem::FileContentsData& /*vertexHLSL*/, const FileSystem::FileContentsData& /*fragmentHLSL*/ )
{
    LoadFromLibrary( metalVertexShaderName, metalFragmentShaderName );
}

void ae3d::Shader::LoadFromLibrary( const char* vertexShaderName, const char* fragmentShaderName )
{
    NSString* vertexName = [NSString stringWithUTF8String:vertexShaderName ];
    vertexProgram = [GfxDevice::GetDefaultMetalShaderLibrary() newFunctionWithName:vertexName];

    if (vertexProgram == nullptr)
    {
        NSLog(@"Shader: Could not load %s!\n", vertexShaderName);
        return;
    }
    
    NSString* fragmentName = [NSString stringWithUTF8String:fragmentShaderName ];
    fragmentProgram = [GfxDevice::GetDefaultMetalShaderLibrary() newFunctionWithName:fragmentName];
    
    if (fragmentProgram == nullptr)
    {
        NSLog(@"Shader: Could not load %s!\n", fragmentShaderName);
        return;
    }
    
    id = 1;
    uniformBuffer = [GfxDevice::GetMetalDevice() newBufferWithLength:256 options:MTLResourceOptionCPUCacheModeDefault];
}

void ae3d::Shader::Use()
{
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
        return;
    }
    
    uint8_t* bufferPointer = (uint8_t *)[uniformBuffer contents] + uniforms[ name ].offsetFromBufferStart;
    memcpy( bufferPointer, matrix4x4, 16 * 4 );
}

void ae3d::Shader::SetTexture( const char* name, const Texture2D* texture, int textureUnit )
{
    if (texture != nullptr)
    {
        texture0 = const_cast<Texture2D*>(texture)->GetMetalTexture();
    }
    else
    {
        System::Print("Shader tried to set null texture\n");
    }
}

void ae3d::Shader::SetRenderTexture( const char* name, const ae3d::RenderTexture* renderTexture, int textureUnit )
{

}

void ae3d::Shader::SetTexture( const char* name, const TextureCube* texture, int textureUnit )
{
    if (texture != nullptr)
    {
        texture0 = const_cast<TextureCube*>(texture)->GetMetalTexture();
    }
    else
    {
        System::Print("Shader tried to set null texture\n");
    }
}

void ae3d::Shader::SetInt( const char* name, int value )
{
}

void ae3d::Shader::SetFloat( const char* name, float value )
{
    if (uniforms.find( name ) == std::end( uniforms ))
    {
        return;
    }
    
    uint8_t* bufferPointer = (uint8_t *)[uniformBuffer contents] + uniforms[ name ].offsetFromBufferStart;
    memcpy( bufferPointer, &value, 4 );
}

void ae3d::Shader::SetVector3( const char* name, const float* vec3 )
{
    if (uniforms.find( name ) == std::end( uniforms ))
    {
        return;
    }
    
    uint8_t* bufferPointer = (uint8_t *)[uniformBuffer contents] + uniforms[ name ].offsetFromBufferStart;
    memcpy( bufferPointer, vec3, 3 * 4 );
}

void ae3d::Shader::SetVector4( const char* name, const float* vec4 )
{
    if (uniforms.find( name ) == std::end( uniforms ))
    {
        return;
    }
    
    uint8_t* bufferPointer = (uint8_t *)[uniformBuffer contents] + uniforms[ name ].offsetFromBufferStart;
    memcpy( bufferPointer, vec4, 4 * 4 );
}

