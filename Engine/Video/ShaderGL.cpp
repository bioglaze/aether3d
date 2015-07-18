#include "Shader.hpp"
#include <GL/glxw.h>
#include <vector>
#include "FileSystem.hpp"
#include "FileWatcher.hpp"
#include "GfxDevice.hpp"
#include "System.hpp"
#include "Texture2D.hpp"
#include "TextureCube.hpp"

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

    enum class InfoLogType
    {
        Program,
        Shader
    };

    void PrintInfoLog( GLuint shader, InfoLogType logType )
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

    GLuint CompileShader( const char* source, GLenum shaderType )
    {
        const GLuint shader = ae3d::GfxDevice::CreateShaderId( static_cast<unsigned>(shaderType) );
        glShaderSource( shader, 1, &source, nullptr );
        glCompileShader( shader );
        
        GLint wasCompiled;
        
        glGetShaderiv( shader, GL_COMPILE_STATUS, &wasCompiled );
        
        if (!wasCompiled)
        {
            ae3d::System::Print( "Shader compile error.\n" );
            PrintInfoLog( shader, InfoLogType::Shader );
            return 0;
        }
        
        return shader;
    }
    
    std::map< std::string, ae3d::Shader::IntDefaultedToMinusOne > GetUniformLocations( GLuint program )
    {
        int numUni = -1;
        glGetProgramiv( program, GL_ACTIVE_UNIFORMS, &numUni );
        
        std::map< std::string, ae3d::Shader::IntDefaultedToMinusOne > outUniforms;
        
        for (int i = 0; i < numUni; ++i)
        {
            int namelen;
            int num;
            GLenum type;
            char name[128];
            
            glGetActiveUniform( program, static_cast<GLuint>(i), sizeof(name) - 1, &namelen, &num, &type, name );
            
            name[ namelen ] = 0;
            
            outUniforms[ name ].i = glGetUniformLocation( program, name );
        }
        
        return outUniforms;
    }
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
    GLuint vertexShader = CompileShader( vertexSource, GL_VERTEX_SHADER );
    GLuint fragmentShader = CompileShader( fragmentSource, GL_FRAGMENT_SHADER );

    GLuint program = GfxDevice::CreateProgramId();

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
        ae3d::System::Print("Shader linking failed.\n");
        PrintInfoLog( program, InfoLogType::Program );
        return;
    }

    id = program;
    uniformLocations = GetUniformLocations( program );
}

void ae3d::Shader::Load( const FileSystem::FileContentsData& vertexData, const FileSystem::FileContentsData& fragmentData,
                         const char* /*metalVertexShaderName*/, const char* /*metalFragmentShaderName*/ )
{
    const std::string vertexStr = std::string( std::begin( vertexData.data ), std::end( vertexData.data ) );
    const std::string fragmentStr = std::string( std::begin( fragmentData.data ), std::end( fragmentData.data ) );

    Load( vertexStr.c_str(), fragmentStr.c_str() );

    bool isInCache = false;
    
    for (const auto& entry : cacheEntries)
    {
        if (entry.shader == this)
        {
            isInCache = true;
        }
    }
    
    if (!isInCache && !vertexData.path.empty() && !fragmentData.path.empty())
    {
        fileWatcher.AddFile( vertexData.path.c_str(), ShaderReload );
        fileWatcher.AddFile( fragmentData.path.c_str(), ShaderReload );
        cacheEntries.push_back( { vertexData.path, fragmentData.path, this } );
    }
}

void ae3d::Shader::Use()
{
    glUseProgram( id );
}

void ae3d::Shader::SetMatrix( const char* name, const float* matrix4x4 )
{
    glProgramUniformMatrix4fv( id, uniformLocations[ name ].i, 1, GL_FALSE, matrix4x4 );
}

void ae3d::Shader::SetTexture( const char* name, const ae3d::Texture2D* texture, int textureUnit )
{
    glActiveTexture( GL_TEXTURE0 + textureUnit );
    glBindTexture( GL_TEXTURE_2D, texture->GetID() );
    GfxDevice::IncTextureBinds();
    SetInt( name, textureUnit );

    const std::string scaleOffsetName = std::string( name ) + std::string( "_ST" );
    SetVector4( scaleOffsetName.c_str(), &texture->GetScaleOffset().x );
}

void ae3d::Shader::SetTexture( const char* name, const ae3d::TextureCube* texture, int textureUnit )
{
    glActiveTexture( GL_TEXTURE0 + textureUnit );
    glBindTexture( GL_TEXTURE_CUBE_MAP, texture->GetID() );
    GfxDevice::IncTextureBinds();
    SetInt( name, textureUnit );
    
    const std::string scaleOffsetName = std::string( name ) + std::string( "_ST" );
    SetVector4( scaleOffsetName.c_str(), &texture->GetScaleOffset().x );
}

void ae3d::Shader::SetInt( const char* name, int value )
{
    glProgramUniform1i( id, uniformLocations[ name ].i, value );
}

void ae3d::Shader::SetFloat( const char* name, float value )
{
    glProgramUniform1f( id, uniformLocations[ name ].i, value );
}

void ae3d::Shader::SetVector3( const char* name, const float* vec3 )
{
    glProgramUniform3fv( id, uniformLocations[ name ].i, 1, vec3 );
}

void ae3d::Shader::SetVector4( const char* name, const float* vec4 )
{
    glProgramUniform4fv( id, uniformLocations[ name ].i, 1, vec4 );
}
