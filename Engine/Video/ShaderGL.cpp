#include "Shader.hpp"
#include <GL/glxw.h>
#include "GfxDevice.hpp"
#include "System.hpp"
#include "Texture2D.hpp"

enum class InfoLogType
{
    Program,
    Shader
};

static std::map<std::string, GLint> GetUniformLocations(GLuint program)
{
    int numUni = -1;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numUni);

    std::map<std::string, GLint> outUniforms;

    for (int i = 0; i < numUni; ++i)
    {
        int namelen;
        int num;
        GLenum type;
        char name[128];

        glGetActiveUniform(program, static_cast<GLuint>(i), sizeof(name) - 1, &namelen, &num, &type, name);

        name[namelen] = 0;

        outUniforms[ name ] = glGetUniformLocation( program, name );
    }

    return outUniforms;
}

static void PrintInfoLog( GLuint shader, InfoLogType logType )
{
    GLint logLength = 0;
    
    if (logType == InfoLogType::Program)
    {
        glGetProgramiv( shader, GL_INFO_LOG_LENGTH, &logLength );
    }
    else
    {
        glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &logLength );
    }

    if (logLength > 0)
    {
        GLchar* log = new GLchar[ (std::size_t)logLength + 1 ];
        GLsizei charsWritten = 0;
        
        if (logType == InfoLogType::Program)
        {
            glGetProgramInfoLog( shader, logLength, &charsWritten, log );
        }
        else
        {
            glGetShaderInfoLog( shader, logLength, &charsWritten, log );
        }
        
        if (charsWritten > 0)
        {
            ae3d::System::Print( log );
        }
        
        delete[] log;
    }
}

static GLuint CompileShader( const char* source, GLenum shaderType )
{
    const GLuint shader = glCreateShader( shaderType );
    glShaderSource( shader, 1, &source, nullptr );
    glCompileShader( shader );
    
    GLint wasCompiled;
    
    glGetShaderiv( shader, GL_COMPILE_STATUS, &wasCompiled );
    
    if (!wasCompiled)
    {
        ae3d::System::Print( "Shader compile error in source: \n%s", source );
        PrintInfoLog( shader, InfoLogType::Shader );
        return 0;
    }

    return shader;
}

void ae3d::Shader::Load( const char* vertexSource, const char* fragmentSource )
{
    GLuint vertexShader = CompileShader( vertexSource, GL_VERTEX_SHADER );
    GLuint fragmentShader = CompileShader( fragmentSource, GL_FRAGMENT_SHADER );

    GLuint program = glCreateProgram();

    if (GfxDevice::HasExtension( "KHR_debug" ))
    {
        glObjectLabel( GL_PROGRAM, id, (GLsizei)std::string( "shader" ).size(), "shader" );
    }

    glAttachShader( program, vertexShader );
    glAttachShader( program, fragmentShader );

    glLinkProgram( program );
    GLint wasLinked;
    glGetProgramiv( program, GL_LINK_STATUS, &wasLinked );
    
    if (!wasLinked)
    {
        ae3d::System::Print("Link failed.");
        PrintInfoLog( program, InfoLogType::Program );
        return;
    }

    id = program;
    uniformLocations = GetUniformLocations(id);
}

void ae3d::Shader::Use()
{
    glUseProgram( id );
}

void ae3d::Shader::SetMatrix( const char* name, const float* matrix4x4 )
{
    glUniformMatrix4fv( uniformLocations[ name ], 1, GL_FALSE, matrix4x4 );
}

void ae3d::Shader::SetTexture( const char* name, const ae3d::Texture2D* texture, int textureUnit )
{
    glActiveTexture( GL_TEXTURE0 + textureUnit );
    glBindTexture( GL_TEXTURE_2D, texture->GetID() );
    SetInt( name, textureUnit );
}

void ae3d::Shader::SetInt( const char* name, int value )
{
    glUniform1i( uniformLocations[ name ], value );
}

void ae3d::Shader::SetFloat( const char* name, float value )
{
    glUniform1f( uniformLocations[ name ], value );
}

void ae3d::Shader::SetVector3( const char* name, const float* vec3 )
{
    glUniform3fv( uniformLocations[ name ], 1, vec3 );
}

void ae3d::Shader::SetVector4( const char* name, const float* vec4 )
{
    glUniform4fv( uniformLocations[ name ], 1, vec4 );
}
