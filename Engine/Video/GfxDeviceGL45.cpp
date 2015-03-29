#include "GfxDevice.hpp"
#include <vector>
#include <GL/glxw.h>
#include "System.hpp"

namespace GfxDeviceGlobal
{
    std::vector< GLuint > vaoIds;
    std::vector< GLuint > bufferIds;
    std::vector< GLuint > textureIds;
}

void ae3d::GfxDevice::ReleaseGPUObjects()
{
    glDeleteVertexArrays(static_cast<GLsizei>(GfxDeviceGlobal::vaoIds.size()), GfxDeviceGlobal::vaoIds.data());
    glDeleteBuffers(static_cast<GLsizei>(GfxDeviceGlobal::bufferIds.size()), GfxDeviceGlobal::bufferIds.data());
    glDeleteTextures(static_cast<GLsizei>(GfxDeviceGlobal::textureIds.size()), GfxDeviceGlobal::textureIds.data());
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
            ae3d::System::Print("%s caused an OpenGL error: %s", info, GetGLErrorString( errorCode ) );
            ae3d::System::Assert(false, "OpenGL error!");
        }
#endif

}
