#import <Foundation/Foundation.h>
#include "Shader.hpp"
#include "GfxDevice.hpp"
#include "Texture2D.hpp"
#include "TextureCube.hpp"
#include "System.hpp"

uint8_t constantDataBufferIndex = 0;
extern id <MTLBuffer> uniformBuffer;
extern id<MTLTexture> texture0;

void ae3d::Shader::Load( const char* vertexSource, const char* fragmentSource )
{
}

void ae3d::Shader::LoadFromLibrary( const char* vertexShaderName, const char* fragmentShaderName )
{
    NSString* vertexName = [NSString stringWithUTF8String:vertexShaderName ];
    vertexProgram = [GfxDevice::GetDefaultMetalShaderLibrary() newFunctionWithName:vertexName];

    if (vertexProgram == nullptr)
    {
        NSLog(@"Shader: Could not load vertexProgram!\n");
    }
    
    NSString* fragmentName = [NSString stringWithUTF8String:fragmentShaderName ];
    fragmentProgram = [GfxDevice::GetDefaultMetalShaderLibrary() newFunctionWithName:fragmentName];
    
    if (fragmentProgram == nullptr)
    {
        NSLog(@"Shader: Could not load fragmentProgram!\n");
    }
}

void ae3d::Shader::Use()
{
}

void ae3d::Shader::SetMatrix( const char* name, const float* matrix4x4 )
{
    const int uniformBufferSize = (16 * 4);

    uint8_t* bufferPointer = (uint8_t *)[uniformBuffer contents] + (uniformBufferSize * constantDataBufferIndex);
    memcpy( bufferPointer, matrix4x4, uniformBufferSize );
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
}

void ae3d::Shader::SetVector3( const char* name, const float* vec3 )
{
}

void ae3d::Shader::SetVector4( const char* name, const float* vec4 )
{
}

