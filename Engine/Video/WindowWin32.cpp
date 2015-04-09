#include "Window.hpp"
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>
#include <GL/glxw.h>
#include <GL/wglext.h>

// Testing hotloading
#include "FileWatcher.hpp"
#include "System.hpp"
extern ae3d::FileWatcher fileWatcher;

// TODO: Separate all GL stuff from this source file so it can be used also with D3D.

void APIENTRY DebugCallbackARB(GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar *message, const GLvoid *userParam);

namespace WindowGlobal
{
    bool isOpen = false;
    const int eventStackSize = 10;
    ae3d::WindowEvent eventStack[eventStackSize];
    int eventIndex = -1;
    HWND hwnd;
    HDC hdc;
}

static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_ACTIVATEAPP:
            //RendererImpl::Instance().SetFocus((wParam == WA_INACTIVE) ? false : true);
            break;
        case WM_SIZE:
        case WM_SETCURSOR:
        case WM_DESTROY:
        case WM_SYSKEYUP:
        case WM_KEYUP:
            break;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            ++WindowGlobal::eventIndex;
            WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::KeyDown;
            ae3d::System::Print("Got key down: %d", wParam);
            //WindowGlobal::eventStack[WindowGlobal::eventIndex].keyCode = static_cast<int>( wParam );
            if (wParam == 65) // A
            {
                WindowGlobal::eventStack[WindowGlobal::eventIndex].keyCode = ae3d::KeyCode::A;
            }
            if (wParam == 38) // arrow up
            {
                WindowGlobal::eventStack[WindowGlobal::eventIndex].keyCode = ae3d::KeyCode::Up;
            }
            if (wParam == 40) // arrow down
            {
                WindowGlobal::eventStack[WindowGlobal::eventIndex].keyCode = ae3d::KeyCode::Down;
            }
            if (wParam == 37) // arrow left
            {
                WindowGlobal::eventStack[WindowGlobal::eventIndex].keyCode = ae3d::KeyCode::Left;
            }
            if (wParam == 39) // arrow right
            {
                WindowGlobal::eventStack[WindowGlobal::eventIndex].keyCode = ae3d::KeyCode::Right;
            }
            if (wParam == 66) // B
            {
                WindowGlobal::eventStack[WindowGlobal::eventIndex].keyCode = ae3d::KeyCode::B;
            }
            if (wParam == 27) // escape
            {
                WindowGlobal::eventStack[WindowGlobal::eventIndex].keyCode = ae3d::KeyCode::Escape;
            }
        }
        break;
        case WM_SYSCOMMAND:
            if (wParam == SC_MINIMIZE)
            {
                // Handled by WM_SIZE.
            }
            else if (wParam == SC_RESTORE)
            {
            }
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        {
            ++WindowGlobal::eventIndex;
            WindowGlobal::eventStack[WindowGlobal::eventIndex].type = message == WM_LBUTTONDOWN ? ae3d::WindowEventType::Mouse1Down : ae3d::WindowEventType::Mouse1Up;
            WindowGlobal::eventStack[WindowGlobal::eventIndex].mouseX = LOWORD(lParam);
            WindowGlobal::eventStack[WindowGlobal::eventIndex].mouseY = HIWORD(lParam);
        }
            break;
        case WM_RBUTTONDOWN:
            break;
        case WM_MBUTTONDOWN:
            break;
        case WM_MOUSEWHEEL:
            break;
        case WM_MOUSEMOVE:
            //gMousePosition[0] = (short)LOWORD(lParam);
            //gMousePosition[1] = (short)HIWORD(lParam);
            
            // Check to see if the left button is held down:
            //bool leftButtonDown=wParam & MK_LBUTTON;
            
            // Check if right button down:
            //bool rightButtonDown=wParam & MK_RBUTTON;
            break;
        case WM_CLOSE:
            ++WindowGlobal::eventIndex;
            WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::Close;
            break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

namespace ae3d
{
    void Window::Create(int width, int height, WindowCreateFlags flags)
    {
        const int finalWidth = width == 0 ? GetSystemMetrics(SM_CXSCREEN) : width;
        const int finalHeight = height == 0 ? GetSystemMetrics(SM_CYSCREEN) : height;

        const HINSTANCE hInstance = GetModuleHandle(nullptr);
        const bool fullscreen = (flags & WindowCreateFlags::Fullscreen) != 0;

        WNDCLASSEX wc;
        ZeroMemory(&wc, sizeof(WNDCLASSEX));

        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // OWNDC is needed for OpenGL.
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
        wc.lpszClassName = "WindowClass1";

        wc.hIcon = (HICON)LoadImage(nullptr,
            "glider.ico",
            IMAGE_ICON,
            32,
            32,
            LR_LOADFROMFILE);

        RegisterClassEx(&wc);

        WindowGlobal::hwnd = CreateWindowExA(fullscreen ? WS_EX_TOOLWINDOW | WS_EX_TOPMOST : 0,
            "WindowClass1",    // name of the window class
            "Window",   // title of the window
            fullscreen ? WS_POPUP : (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU),    // window style
            CW_USEDEFAULT,    // x-position of the window
            CW_USEDEFAULT,    // y-position of the window
            finalWidth,    // width of the window
            finalHeight,    // height of the window
            nullptr,    // we have no parent window
            nullptr,    // we aren't using menus
            hInstance,    // application handle
            nullptr);    // used with multiple windows    
        
        ShowWindow(WindowGlobal::hwnd, SW_SHOW);
        CreateRenderer();
        WindowGlobal::isOpen = true;
    }

    bool Window::PollEvent(WindowEvent& outEvent)
    {
        if (WindowGlobal::eventIndex == -1)
        {
            return false;
        }

        outEvent = WindowGlobal::eventStack[WindowGlobal::eventIndex];
        --WindowGlobal::eventIndex;
        return true;
    }

    void Window::PumpEvents()
    {
        MSG msg;

        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    bool Window::IsOpen()
    {
        return WindowGlobal::isOpen;
    }

    void Window::CreateRenderer()
    {
        // Choose pixel format
        PIXELFORMATDESCRIPTOR pfd;
        ZeroMemory( &pfd, sizeof(pfd) );
        pfd.nSize = sizeof(pfd);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cDepthBits = 32;
        pfd.iLayerType = PFD_MAIN_PLANE;

        WindowGlobal::hdc = GetDC(WindowGlobal::hwnd);
        const int pixelFormat = ChoosePixelFormat( WindowGlobal::hdc, &pfd );
        
        if (pixelFormat == 0)
        {
            OutputDebugStringA( "Failed to find suitable pixel format! ");
            return;
        }

        if (!SetPixelFormat(WindowGlobal::hdc, pixelFormat, &pfd))
        {
            OutputDebugStringA( "Failed to set pixel format!" );
            return;
        }

        // Create temporary context and make sure we have support
        HGLRC tempContext = wglCreateContext( WindowGlobal::hdc );
        if (!tempContext)
        {
            OutputDebugStringA("Failed to create temporary context!");
            return;
        }

        if (!wglMakeCurrent( WindowGlobal::hdc, tempContext ))
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
        wglMakeCurrent( nullptr, nullptr );
        wglDeleteContext( tempContext );

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
        glGetIntegerv( GL_CONTEXT_FLAGS, &v );

        if (v & GL_CONTEXT_FLAG_DEBUG_BIT)
        {
            glDebugMessageCallback( DebugCallbackARB, nullptr );
            glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
        }

        glEnable( GL_DEPTH_TEST);
    }

    void Window::SwapBuffers() const
    {
        ::SwapBuffers( WindowGlobal::hdc );

        static int frame = 0;
        ++frame;

        if ((frame % 60) == 0)
        {
            fileWatcher.Poll();
        }
    }
}