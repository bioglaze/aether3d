#include "Window.hpp"
#include <GL/glxw.h>
#include <GL/wglext.h>

void APIENTRY DebugCallbackARB(GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar *message, const GLvoid *userParam);

namespace WindowGlobal
{
    extern HWND hwnd;
    extern HDC hdc;
}

namespace ae3d
{
    void Window::CreateRenderer()
    {
        // Choose pixel format
        PIXELFORMATDESCRIPTOR pfd;
        ZeroMemory(&pfd, sizeof(pfd));
        pfd.nSize = sizeof(pfd);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cDepthBits = 32;
        pfd.iLayerType = PFD_MAIN_PLANE;

        WindowGlobal::hdc = GetDC(WindowGlobal::hwnd);
        const int pixelFormat = ChoosePixelFormat(WindowGlobal::hdc, &pfd);

        if (pixelFormat == 0)
        {
            OutputDebugStringA("Failed to find suitable pixel format! ");
            return;
        }

        if (!SetPixelFormat(WindowGlobal::hdc, pixelFormat, &pfd))
        {
            OutputDebugStringA("Failed to set pixel format!");
            return;
        }

        // Create temporary context and make sure we have support
        HGLRC tempContext = wglCreateContext(WindowGlobal::hdc);
        if (!tempContext)
        {
            OutputDebugStringA("Failed to create temporary context!");
            return;
        }

        if (!wglMakeCurrent(WindowGlobal::hdc, tempContext))
        {
            OutputDebugStringA("Failed to activate temporary context!");
            return;
        }

        int otherBits = 0;
#if _DEBUG
        otherBits = WGL_CONTEXT_DEBUG_BIT_ARB;
#endif

        const int attribs[] =
        {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3, // Not using 4.5 yet because I'm developing also on non-supported GPUs.
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | otherBits,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0
        };
#if _DEBUG

#endif
        PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

        if (!wglCreateContextAttribsARB)
        {
            OutputDebugStringA("Failed to find pointer to wglCreateContextAttribsARB function!");
            return;
        }

        HANDLE context = wglCreateContextAttribsARB(WindowGlobal::hdc, 0, attribs);

        if (context == 0)
        {
            OutputDebugStringA("Failed to create OpenGL context!");
            return;
        }

        // Remove temporary context and activate forward compatible context
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(tempContext);

        if (!wglMakeCurrent(WindowGlobal::hdc, (HGLRC)context))
        {
            OutputDebugStringA("Failed to activate forward compatible context!");
            return;
        }

        if (glxwInit() != 0)
        {
            OutputDebugStringA("Failed to load OpenGL function pointers using GLXW!");
        }

        //MessageBoxA(0, (char*)glGetString(GL_VERSION), "OPENGL VERSION", 0);

        GLint v;
        glGetIntegerv(GL_CONTEXT_FLAGS, &v);

        if (v & GL_CONTEXT_FLAG_DEBUG_BIT)
        {
            glDebugMessageCallback(DebugCallbackARB, nullptr);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        }

        glEnable(GL_DEPTH_TEST);
    }
}