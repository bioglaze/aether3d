#include "Window.hpp"
#include <GL/glxw.h>
#include <GL/wglext.h>

#include "System.hpp"

void APIENTRY DebugCallbackARB(GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar *message, const GLvoid *userParam);

namespace WindowGlobal
{
    extern HWND hwnd;
    extern HDC hdc;
}

namespace Dummy
{
    HWND hwnd;
    HDC hdc;
    HGLRC hrc;
    WNDCLASSEX wcl;
}

LRESULT CALLBACK DummyGLWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch (msg)
    {
    case WM_CREATE:
    {
        Dummy::hdc = GetDC( hWnd );
        
        if (!Dummy::hdc)
        {
            return -1;
        }
    }
        break;

    case WM_DESTROY:
        if (Dummy::hdc)
        {
            if (Dummy::hrc)
            {
                wglMakeCurrent( Dummy::hdc, 0 );
                wglDeleteContext( Dummy::hrc );
                Dummy::hrc = 0;
            }

            ReleaseDC( hWnd, Dummy::hdc );
            Dummy::hdc = 0;
        }

        PostQuitMessage( 0 );
        return 0;

    default:
        break;
    }

    return DefWindowProc( hWnd, msg, wParam, lParam );
}

namespace ae3d
{
    bool CreateDummyGLWindow()
    {
        Dummy::wcl.cbSize = sizeof( Dummy::wcl );
        Dummy::wcl.style = CS_OWNDC;
        Dummy::wcl.lpfnWndProc = DummyGLWndProc;
        Dummy::wcl.hInstance = reinterpret_cast<HINSTANCE>(GetModuleHandle( nullptr ));
        Dummy::wcl.lpszClassName = "DummyGLWindowClass";

        if (!RegisterClassEx( &Dummy::wcl ))
        {
            OutputDebugStringA( "Failed to register dummy window class!\n" );
            return false;
        }

        Dummy::hwnd = CreateWindow( Dummy::wcl.lpszClassName, "", WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, 0, 0, Dummy::wcl.hInstance, 0 );

        if (!Dummy::hwnd)
        {
            OutputDebugStringA( "Failed create dummy window!\n" );
            return false;
        }

        Dummy::hdc = GetDC( Dummy::hwnd );

        PIXELFORMATDESCRIPTOR pfd = { 0 };

        pfd.nSize = sizeof( pfd );
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 24;
        pfd.cDepthBits = 24;
        pfd.iLayerType = PFD_MAIN_PLANE;

        int pf = ChoosePixelFormat( Dummy::hdc, &pfd );

        if (!SetPixelFormat( Dummy::hdc, pf, &pfd ))
        {
            OutputDebugStringA( "Failed to set dummy window pixel format!\n" );
            return false;
        }

        Dummy::hrc = wglCreateContext( Dummy::hdc );

        if (!Dummy::hrc)
        {
            OutputDebugStringA( "Failed create dummy OpenGL context!\n" );
            return false;
        }

        if (!wglMakeCurrent( Dummy::hdc, Dummy::hrc ))
        {
            OutputDebugStringA( "Failed to make current dummy OpenGL context!\n" );
            return false;
        }

        return true;
    }

    void DestroyDummyGLWindow()
    {
        if (Dummy::hwnd)
        {
            PostMessage( Dummy::hwnd, WM_CLOSE, 0, 0 );

            BOOL bRet;
            MSG msg;

            while ((bRet = GetMessage( &msg, 0, 0, 0 )) != 0)
            {
                TranslateMessage( &msg );
                DispatchMessage( &msg );
            }
        }

        UnregisterClass( Dummy::wcl.lpszClassName, Dummy::wcl.hInstance );
    }

    void ChooseAntiAliasingPixelFormat( int& outPixelFormat, int samples )
    {
        const int attributes[] =
        {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
            WGL_COLOR_BITS_ARB, 24,
            WGL_ALPHA_BITS_ARB, 8,
            WGL_DEPTH_BITS_ARB, 24,
            WGL_STENCIL_BITS_ARB, 8,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            WGL_SAMPLE_BUFFERS_ARB, samples > 0 ? 1 : 0,
            WGL_SAMPLES_ARB, samples,
            WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
            0, 0
        };

        PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress( "wglChoosePixelFormatARB" );
        int returnedPixelFormat = 0;
        UINT numFormats = 0;
        BOOL status = wglChoosePixelFormatARB( Dummy::hdc, attributes, 0, 1, &returnedPixelFormat, &numFormats );

        if (status != FALSE && numFormats > 0)
        {
            outPixelFormat = returnedPixelFormat;
        }
        else
        {
            OutputDebugStringA( "Failed to choose a pixel format!\n" );
        }
    }

    void CreateRenderer( int samples )
    {
        PIXELFORMATDESCRIPTOR pfd = { 0 };
        pfd.nSize = sizeof( pfd );
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_COMPOSITION;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 24;
        pfd.cDepthBits = 24;
        pfd.iLayerType = PFD_MAIN_PLANE;

        CreateDummyGLWindow();
        int pixelFormat = 0;
        ChooseAntiAliasingPixelFormat( pixelFormat, samples );
        PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress( "wglCreateContextAttribsARB" );
        DestroyDummyGLWindow();

        if (!SetPixelFormat( WindowGlobal::hdc, pixelFormat, &pfd ))
        {
            OutputDebugStringA( "Failed to set pixel format!\n" );
            return;
        }

        int otherBits = 0;
#if DEBUG
        otherBits = WGL_CONTEXT_DEBUG_BIT_ARB;
#endif
        const int contextAttribs[] =
        {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 1,
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | otherBits,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0
        };

        if (!wglCreateContextAttribsARB)
        {
            OutputDebugStringA( "Failed to find pointer to wglCreateContextAttribsARB function!\n" );
            return;
        }

        HANDLE context = wglCreateContextAttribsARB( WindowGlobal::hdc, 0, contextAttribs );

        if (context == nullptr)
        {
            OutputDebugStringA( "Failed to create OpenGL context!\n" );
            return;
        }

        if (!wglMakeCurrent( WindowGlobal::hdc, (HGLRC)context ))
        {
            OutputDebugStringA( "Failed to activate forward compatible context!\n" );
            return;
        }

        if (glxwInit() != 0)
        {
            OutputDebugStringA( "Failed to load OpenGL function pointers using GLXW!\n" );
            return;
        }

        GLint v;
        glGetIntegerv( GL_CONTEXT_FLAGS, &v );

        if (v & GL_CONTEXT_FLAG_DEBUG_BIT)
        {
            glDebugMessageCallback( DebugCallbackARB, nullptr );
            glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
        }

		PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress( "wglSwapIntervalEXT" );
		wglSwapIntervalEXT( 1 );
    }
}