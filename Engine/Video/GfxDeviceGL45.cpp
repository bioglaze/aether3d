#include "GfxDevice.hpp"
#include <algorithm>
//#include <string>
#include <vector>
#include <GL/glxw.h>
#include "System.hpp"

void PrintOpenGLDebugOutput( GLenum source, GLenum type, GLuint id, GLenum severity, const char *msg)
{
    const char *sourceFmt = "UNDEFINED(0x%04X)";

    switch (source)
    {
    case GL_DEBUG_SOURCE_API_ARB:             sourceFmt = "API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:   sourceFmt = "WINDOW_SYSTEM"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: sourceFmt = "SHADER_COMPILER"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:     sourceFmt = "THIRD_PARTY"; break;
    case GL_DEBUG_SOURCE_APPLICATION_ARB:     sourceFmt = "APPLICATION"; break;
    case GL_DEBUG_SOURCE_OTHER_ARB:           sourceFmt = "OTHER"; break;
    }

    const char *typeFmt = "UNDEFINED(0x%04X)";

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR_ARB:               typeFmt = "ERROR"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: typeFmt = "DEPRECATED_BEHAVIOR"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:  typeFmt = "UNDEFINED_BEHAVIOR"; break;
    case GL_DEBUG_TYPE_PORTABILITY_ARB:         typeFmt = "PORTABILITY"; break;
    case GL_DEBUG_TYPE_PERFORMANCE_ARB:         typeFmt = "PERFORMANCE"; break;
    case GL_DEBUG_TYPE_OTHER_ARB:               typeFmt = "OTHER"; break;
    }

    const char *severityFmt = "UNDEFINED";

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH_ARB:   severityFmt = "HIGH";   break;
    case GL_DEBUG_SEVERITY_MEDIUM_ARB: severityFmt = "MEDIUM"; break;
    case GL_DEBUG_SEVERITY_LOW_ARB:    severityFmt = "LOW"; break;
    }

    ae3d::System::Print( "OpenGL: %s [source=%s type=%s severity=%s id=%u]",
        msg, sourceFmt, typeFmt, severityFmt, id );
}

void APIENTRY DebugCallbackARB( GLenum source, GLenum type, GLuint id, GLenum severity,
                                GLsizei length, const GLchar *message, const GLvoid *userParam )
{
    const int undefinedSeverity = 33387;
    // Old engine (untested on current): NVIDIA driver spams buffer creation messages at 'undefined' severity, this silences them.
    if (severity != undefinedSeverity)
    {
        (void)length;
        (void)userParam;
        PrintOpenGLDebugOutput( source, type, id, severity, message);
        ae3d::System::Assert(false, "OpenGL error");
    }
}

namespace GfxDeviceGlobal
{
    std::vector< GLuint > vaoIds;
    std::vector< GLuint > bufferIds;
    std::vector< GLuint > textureIds;
    std::vector< GLuint > shaderIds;
    std::vector< GLuint > programIds;
    int drawCalls = 0;
}

void ae3d::GfxDevice::IncDrawCalls()
{
    ++GfxDeviceGlobal::drawCalls;
}

void ae3d::GfxDevice::ResetFrameStatistics()
{
    GfxDeviceGlobal::drawCalls = 0;
}

int ae3d::GfxDevice::GetDrawCalls()
{
    return GfxDeviceGlobal::drawCalls;
}

void ae3d::GfxDevice::ReleaseGPUObjects()
{
    if (!GfxDeviceGlobal::vaoIds.empty())
    {
        glDeleteVertexArrays( static_cast<GLsizei>(GfxDeviceGlobal::vaoIds.size()), GfxDeviceGlobal::vaoIds.data() );
    }
    if (!GfxDeviceGlobal::bufferIds.empty())
    {
        glDeleteBuffers( static_cast<GLsizei>(GfxDeviceGlobal::bufferIds.size()), GfxDeviceGlobal::bufferIds.data() );
    }
    if (!GfxDeviceGlobal::textureIds.empty())
    {
        glDeleteTextures( static_cast<GLsizei>(GfxDeviceGlobal::textureIds.size()), GfxDeviceGlobal::textureIds.data() );
    }

    for (auto i : GfxDeviceGlobal::shaderIds)
    {
        glDeleteShader( i );
    }

    for (auto i : GfxDeviceGlobal::programIds)
    {
        glDeleteProgram( i );
    }
}

unsigned ae3d::GfxDevice::CreateTextureId()
{
    GLuint outId;
    glGenTextures(1, &outId);
    GfxDeviceGlobal::textureIds.push_back( outId );
    return outId;
}

unsigned ae3d::GfxDevice::CreateVaoId()
{
    GLuint outId;
    glGenVertexArrays(1, &outId);
    GfxDeviceGlobal::vaoIds.push_back(outId);
    return outId;
}

unsigned ae3d::GfxDevice::CreateBufferId()
{
    GLuint outId;
    glGenBuffers(1, &outId);
    GfxDeviceGlobal::bufferIds.push_back(outId);
    return outId;
}

unsigned ae3d::GfxDevice::CreateShaderId( unsigned shaderType )
{
    GLuint outId = glCreateShader( shaderType );
    GfxDeviceGlobal::shaderIds.push_back(outId);
    return outId;
}

unsigned ae3d::GfxDevice::CreateProgramId()
{
    GLuint outId = glCreateProgram();
    GfxDeviceGlobal::programIds.push_back(outId);
    return outId;
}

void ae3d::GfxDevice::ClearScreen(unsigned clearFlags)
{
    GLbitfield mask = 0;

    if ((clearFlags & ClearFlags::Color) != 0)
    {
        mask |= GL_COLOR_BUFFER_BIT;
    }
    if ((clearFlags & ClearFlags::Depth) != 0)
    {
        mask |= GL_DEPTH_BUFFER_BIT;
    }

    glClear( mask );
}

void ae3d::GfxDevice::SetClearColor(float red, float green, float blue)
{
    glClearColor( red, green, blue, 1 );
}

void ae3d::GfxDevice::SetBlendMode( BlendMode blendMode )
{
    if (blendMode == BlendMode::Off)
    {
        glDisable( GL_BLEND );
    }
    else if (blendMode == BlendMode::AlphaBlend)
    {
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glEnable( GL_BLEND );
    }
    else if (blendMode == BlendMode::Additive)
    {
        glBlendFunc( GL_ONE, GL_ONE );
        glEnable( GL_BLEND );
    }
    else
    {
        ae3d::System::Assert( false, "Unhandled blend mode." );
    }
}

const char* GetGLErrorString(GLenum code)
{
    if (code == GL_OUT_OF_MEMORY)
    {
        return "GL_OUT_OF_MEMORY";
    }
    if (code == GL_INVALID_ENUM)
    {
        return "GL_INVALID_ENUM";
    }
    if (code == GL_INVALID_VALUE)
    {
        return "GL_INVALID_VALUE";
    }
    if (code == GL_INVALID_OPERATION)
    {
        return "GL_INVALID_OPERATION";
    }
    if (code == GL_INVALID_FRAMEBUFFER_OPERATION)
    {
        return "GL_INVALID_FRAMEBUFFER_OPERATION";
    }

    return "other GL error";
}

void ae3d::GfxDevice::ErrorCheck(const char* info)
{
        (void)info;
#if defined _DEBUG || defined DEBUG
        GLenum errorCode = GL_INVALID_ENUM;

        while ((errorCode = glGetError()) != GL_NO_ERROR)
        {
            ae3d::System::Print("%s caused an OpenGL error: %s\n", info, GetGLErrorString( errorCode ) );
            ae3d::System::Assert(false, "OpenGL error!");
        }
#endif
}

bool ae3d::GfxDevice::HasExtension( const char* glExtension )
{
    static std::vector< std::string > sExtensions;
    
    if (sExtensions.empty())
    {
        int count;
        glGetIntegerv( GL_NUM_EXTENSIONS, &count );
        sExtensions.resize( (std::size_t)count );
        
        for (int i = 0; i < count; ++i)
        {
            sExtensions[ i ] = std::string( (const char*)glGetStringi( GL_EXTENSIONS, i ) );
        }
    }
    
    return std::find( std::begin( sExtensions ), std::end( sExtensions ), glExtension ) != std::end( sExtensions );
}
