#include "Shader.hpp"
#if __APPLE__
#include <OpenGL/GL.h>
#endif
#if _MSC_VER
#include <GL/glxw.h>
#endif
#include "System.hpp"
#include "Texture2D.hpp"

// TODO [2015-03-21] Find a better place for this, avoid global scope.
ae3d::BuiltinShaders builtinShaders;

void ae3d::BuiltinShaders::Load()
{
    const char* vertexSource =
        "#version 410 core\n \
            \
                layout (location = 0) in vec3 aPosition;\
                    layout (location = 1) in vec2 aTexCoord;\
                        uniform mat4 _ProjectionMatrix;\
                            uniform vec4 textureMap_ST;\
                                \
                                    out vec2 vTexCoord;\
                                        \
                                            void main()\
                                                {\
                                                        gl_Position = _ProjectionMatrix * vec4(aPosition.xyz, 1.0);\
                                                                vTexCoord = aTexCoord;\
    }";
    
    const char* fragmentSource =
    "#version 410 core\n\
    \
    in vec2 vTexCoord;\
    out vec4 fragColor;\
    \
    uniform sampler2D textureMap;\
    \
    void main()\
    {\
        fragColor = texture( textureMap, vTexCoord );\
    }";
    
    spriteRendererShader.Load( vertexSource, fragmentSource );
}

enum class InfoLogType
{
    Program,
    Shader
};

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
    
    GLint compiled;
    
    glGetShaderiv( shader, GL_COMPILE_STATUS, &compiled );
    
    if (!compiled)
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
}

void ae3d::Shader::Use()
{
    glUseProgram( id );
}

void ae3d::Shader::SetMatrix( const char* name, const float* matrix4x4 )
{
    // TODO [2014-03-22] Uniform location cache.
    glUniformMatrix4fv( glGetUniformLocation( id, name ), 1, GL_FALSE, matrix4x4 );
}

void ae3d::Shader::SetTexture( const char* name, const ae3d::Texture2D* texture, int textureUnit )
{
    glActiveTexture( GL_TEXTURE0 + textureUnit );
    glBindTexture( GL_TEXTURE_2D, texture->GetID() );
    SetInt( name, textureUnit );
}

void ae3d::Shader::SetInt( const char* name, int value )
{
    glUniform1i( glGetUniformLocation( id, name ), value );
}

void ae3d::Shader::SetFloat(const char* name, float value)
{
    glUniform1f(glGetUniformLocation(id, name), value);
}
