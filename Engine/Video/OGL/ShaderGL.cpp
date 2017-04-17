#include "Shader.hpp"
#include <GL/glxw.h>
#include <vector>
#include "FileSystem.hpp"
#include "FileWatcher.hpp"
#include "GfxDevice.hpp"
#include "System.hpp"
#include "Statistics.hpp"
#include "Texture2D.hpp"
#include "TextureCube.hpp"
#include "RenderTexture.hpp"

//#define WARN_ON_MISSING_BINDINGS

extern ae3d::FileWatcher fileWatcher;

namespace MathUtil
{
    bool IsFinite( float f );
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

    enum class InfoLogType
    {
        Program,
        Shader
    };

    void PrintInfoLog( GLuint shader, InfoLogType logType, GLenum programParam )
    {
        GLint logLength = 0;
        
        if (logType == InfoLogType::Program)
        {
            glGetProgramiv( shader, programParam, &logLength );
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
            PrintInfoLog( shader, InfoLogType::Shader, GL_INFO_LOG_LENGTH );
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

void ae3d::Shader::Validate()
{
    glValidateProgram( handle );
    PrintInfoLog( handle, InfoLogType::Program, GL_VALIDATE_STATUS );
}

void ae3d::Shader::Load( const char* vertexSource, const char* fragmentSource )
{
    GLuint vertexShader = CompileShader( vertexSource, GL_VERTEX_SHADER );
    GLuint fragmentShader = CompileShader( fragmentSource, GL_FRAGMENT_SHADER );

    GLuint program = GfxDevice::CreateProgramId();

    if (GfxDevice::HasExtension( "GL_KHR_debug" ))
    {
        glObjectLabel( GL_PROGRAM, program, -1, "shader" );
    }

    glAttachShader( program, vertexShader );
    glAttachShader( program, fragmentShader );

    glLinkProgram( program );
    
    glDeleteShader( vertexShader );
    glDeleteShader( fragmentShader );
    
    GLint wasLinked;
    glGetProgramiv( program, GL_LINK_STATUS, &wasLinked );
    
    if (!wasLinked)
    {
        ae3d::System::Print("Shader linking failed.\n");
        PrintInfoLog( program, InfoLogType::Program, GL_INFO_LOG_LENGTH );
        return;
    }

    handle = program;
    uniformLocations = GetUniformLocations( program );
    
    glUseProgram( program );
    GfxDevice::ErrorCheck( "UBO begin" );
    uboLoc = glGetUniformBlockIndex( program, "PerObject" );

    if (uboLoc != 4294967295)
    {
        glUniformBlockBinding( program, uboLoc, 0 );
    }
    
    GfxDevice::ErrorCheck( "UBO end" );
}

void ae3d::Shader::Load( const FileSystem::FileContentsData& vertexGLSL, const FileSystem::FileContentsData& fragmentGLSL,
                         const char* /*metalVertexShaderName*/, const char* /*metalFragmentShaderName*/,
                         const FileSystem::FileContentsData& /*vertexHLSL*/, const FileSystem::FileContentsData& /*fragmentHLSL*/,
                         const FileSystem::FileContentsData& /* spirvData */, const FileSystem::FileContentsData& /*spirvData*/ )
{
    const std::string vertexStr = std::string( std::begin( vertexGLSL.data ), std::end( vertexGLSL.data ) );
    const std::string fragmentStr = std::string( std::begin( fragmentGLSL.data ), std::end( fragmentGLSL.data ) );

    Load( vertexStr.c_str(), fragmentStr.c_str() );
    
    fragmentPath = fragmentGLSL.path;
    
    bool isInCache = false;
    
    for (const auto& entry : cacheEntries)
    {
        if (entry.shader == this)
        {
            isInCache = true;
        }
    }
    
    if (!isInCache && !vertexGLSL.path.empty() && !fragmentGLSL.path.empty())
    {
        fileWatcher.AddFile( vertexGLSL.path.c_str(), ShaderReload );
        fileWatcher.AddFile( fragmentGLSL.path.c_str(), ShaderReload );
        cacheEntries.push_back( { vertexGLSL.path, fragmentGLSL.path, this } );
    }
}

void ae3d::Shader::Use()
{
    static unsigned boundHandle = 0;
    
    if (boundHandle != handle)
    {
        Statistics::IncShaderBinds();
        glUseProgram( handle );
        boundHandle = handle;
    }
}

void ae3d::Shader::SetMatrix( const char* name, const float* matrix4x4 )
{
#ifdef WARN_ON_MISSING_BINDINGS
    if (uniformLocations[ name ].i == -1) { System::Print( "Missing uniform matrix binding %s in vertex or fragment shader %s\n", name, fragmentPath.c_str() ); }
#endif
    glProgramUniformMatrix4fv( handle, uniformLocations[ name ].i, 1, GL_FALSE, matrix4x4 );
}

void ae3d::Shader::SetTexture( const char* name, ae3d::Texture2D* texture, int textureUnit )
{
    glActiveTexture( GL_TEXTURE0 + textureUnit );
    glBindTexture( GL_TEXTURE_2D, texture->GetID() );
    Statistics::IncTextureBinds();
    SetInt( name, textureUnit );

    const std::string scaleOffsetName = std::string( name ) + std::string( "_ST" );
    SetVector4( scaleOffsetName.c_str(), &texture->GetScaleOffset().x );
}

void ae3d::Shader::SetTexture( const char* name, ae3d::TextureCube* texture, int textureUnit )
{
    glActiveTexture( GL_TEXTURE0 + textureUnit );
    glBindTexture( GL_TEXTURE_CUBE_MAP, texture->GetID() );
    Statistics::IncTextureBinds();
    SetInt( name, textureUnit );
    
    const std::string scaleOffsetName = std::string( name ) + std::string( "_ST" );
    SetVector4( scaleOffsetName.c_str(), &texture->GetScaleOffset().x );
}

void ae3d::Shader::SetRenderTexture( const char* name, ae3d::RenderTexture* texture, int textureUnit )
{
    glActiveTexture( GL_TEXTURE0 + textureUnit );
    glBindTexture( texture->IsCube() ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, texture->GetID() );
    Statistics::IncTextureBinds();
    SetInt( name, textureUnit );

    const std::string scaleOffsetName = std::string( name ) + std::string( "_ST" );
    SetVector4( scaleOffsetName.c_str(), &texture->GetScaleOffset().x );
}

void ae3d::Shader::SetInt( const char* name, int value )
{
#ifdef WARN_ON_MISSING_BINDINGS
    if (uniformLocations[ name ].i == -1) { System::Print( "Missing uniform int binding %s in vertex or fragment shader %s\n", name, fragmentPath.c_str() ); }
#endif
    glProgramUniform1i( handle, uniformLocations[ name ].i, value );
}

void ae3d::Shader::SetFloat( const char* name, float value )
{
#if DEBUG
    System::Assert( MathUtil::IsFinite( value ), "Shader::SetFloat got an invalid value" );
#endif
#ifdef WARN_ON_MISSING_BINDINGS
    if (uniformLocations[ name ].i == -1) { System::Print( "Missing uniform float binding %s in vertex or fragment shader %s\n", name, fragmentPath.c_str() ); }
#endif
    glProgramUniform1f( handle, uniformLocations[ name ].i, value );
}

void ae3d::Shader::SetVector3( const char* name, const float* vec3 )
{
#if DEBUG
    for (int i = 0; i < 3; ++i)
    {
        System::Assert( MathUtil::IsFinite( vec3[ i ] ), "Shader::SetVector got an invalid value" );
    }
#endif
#ifdef WARN_ON_MISSING_BINDINGS
    if (uniformLocations[ name ].i == -1) { System::Print( "Missing uniform vec3 binding %s in vertex or fragment shader %s\n", name, fragmentPath.c_str() ); }
#endif
    glProgramUniform3fv( handle, uniformLocations[ name ].i, 1, vec3 );
}

void ae3d::Shader::SetVector4( const char* name, const float* vec4 )
{
#if DEBUG
    for (int i = 0; i < 4; ++i)
    {
        System::Assert( MathUtil::IsFinite( vec4[ i ] ), "Shader::SetVector got an invalid value" );
    }
#endif
#ifdef WARN_ON_MISSING_BINDINGS
    if (uniformLocations[ name ].i == -1) { System::Print( "Missing uniform vec4 binding %s in vertex or fragment shader %s\n", name, fragmentPath.c_str() ); }
#endif
    glProgramUniform4fv( handle, uniformLocations[ name ].i, 1, vec4 );
}
