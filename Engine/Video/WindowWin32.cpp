#include "Window.hpp"
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>
#include <xinput.h>
#include "GfxDevice.hpp"
#include "System.hpp"

typedef DWORD WINAPI x_input_get_state( DWORD dwUserIndex, XINPUT_STATE* pState );

DWORD WINAPI XInputGetStateStub( DWORD /*dwUserIndex*/, XINPUT_STATE* /*pState*/ )
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

static x_input_get_state* XInputGetState_ = XInputGetStateStub;

typedef DWORD WINAPI x_input_set_state( DWORD dwUserIndex, XINPUT_VIBRATION* pVibration );

DWORD WINAPI XInputSetStateStub( DWORD /*dwUserIndex*/, XINPUT_VIBRATION* /*pVibration*/ )
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

static x_input_set_state* XInputSetState_ = XInputSetStateStub;

namespace ae3d
{
    void CreateRenderer();
}

namespace WindowGlobal
{
    int windowWidth = 640;
    int windowHeight = 480;
    const int eventStackSize = 10;
    ae3d::WindowEvent eventStack[ eventStackSize ];
    int eventIndex = -1;

    void IncEventIndex()
    {
        if (eventIndex < eventStackSize - 1)
        {
            ++eventIndex;
        }
    }

    bool isOpen = false;
    HWND hwnd;
    HDC hdc;
    ae3d::KeyCode keyMap[ 256 ] = { ae3d::KeyCode::A };
}

static void InitKeyMap()
{
    WindowGlobal::keyMap[ 13 ] = ae3d::KeyCode::Enter;
    WindowGlobal::keyMap[ 37 ] = ae3d::KeyCode::Left;
    WindowGlobal::keyMap[ 38 ] = ae3d::KeyCode::Up;
    WindowGlobal::keyMap[ 39 ] = ae3d::KeyCode::Right;
    WindowGlobal::keyMap[ 40 ] = ae3d::KeyCode::Down;
    WindowGlobal::keyMap[ 27 ] = ae3d::KeyCode::Escape;
    WindowGlobal::keyMap[ 32 ] = ae3d::KeyCode::Space;

    WindowGlobal::keyMap[ 65 ] = ae3d::KeyCode::A;
    WindowGlobal::keyMap[ 66 ] = ae3d::KeyCode::B;
    WindowGlobal::keyMap[ 67 ] = ae3d::KeyCode::C;
    WindowGlobal::keyMap[ 68 ] = ae3d::KeyCode::D;
    WindowGlobal::keyMap[ 69 ] = ae3d::KeyCode::E;
    WindowGlobal::keyMap[ 70 ] = ae3d::KeyCode::F;
    WindowGlobal::keyMap[ 71 ] = ae3d::KeyCode::G;
    WindowGlobal::keyMap[ 72 ] = ae3d::KeyCode::H;
    WindowGlobal::keyMap[ 73 ] = ae3d::KeyCode::I;
    WindowGlobal::keyMap[ 74 ] = ae3d::KeyCode::J;
    WindowGlobal::keyMap[ 75 ] = ae3d::KeyCode::K;
    WindowGlobal::keyMap[ 76 ] = ae3d::KeyCode::L;
    WindowGlobal::keyMap[ 77 ] = ae3d::KeyCode::M;
    WindowGlobal::keyMap[ 78 ] = ae3d::KeyCode::N;
    WindowGlobal::keyMap[ 79 ] = ae3d::KeyCode::O;
    WindowGlobal::keyMap[ 80 ] = ae3d::KeyCode::P;
    WindowGlobal::keyMap[ 81 ] = ae3d::KeyCode::Q;
    WindowGlobal::keyMap[ 82 ] = ae3d::KeyCode::R;
    WindowGlobal::keyMap[ 83 ] = ae3d::KeyCode::S;
    WindowGlobal::keyMap[ 84 ] = ae3d::KeyCode::T;
    WindowGlobal::keyMap[ 85 ] = ae3d::KeyCode::U;
    WindowGlobal::keyMap[ 86 ] = ae3d::KeyCode::V;
    WindowGlobal::keyMap[ 87 ] = ae3d::KeyCode::W;
    WindowGlobal::keyMap[ 88 ] = ae3d::KeyCode::X;
    WindowGlobal::keyMap[ 89 ] = ae3d::KeyCode::Y;
    WindowGlobal::keyMap[ 90 ] = ae3d::KeyCode::Z;
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
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::KeyDown;
            WindowGlobal::eventStack[WindowGlobal::eventIndex].keyCode = WindowGlobal::keyMap[ (unsigned)wParam ];
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
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[WindowGlobal::eventIndex].type = message == WM_LBUTTONDOWN ? ae3d::WindowEventType::Mouse1Down : ae3d::WindowEventType::Mouse1Up;
            WindowGlobal::eventStack[WindowGlobal::eventIndex].mouseX = LOWORD(lParam);
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = WindowGlobal::windowHeight - HIWORD( lParam );
        }
            break;
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        {
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[WindowGlobal::eventIndex].type = message == WM_RBUTTONDOWN ? ae3d::WindowEventType::Mouse2Down : ae3d::WindowEventType::Mouse2Up;
            WindowGlobal::eventStack[WindowGlobal::eventIndex].mouseX = LOWORD(lParam);
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = WindowGlobal::windowHeight - HIWORD( lParam );
        }
            break;
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = message == WM_MBUTTONDOWN ? ae3d::WindowEventType::MouseMiddleDown : ae3d::WindowEventType::MouseMiddleUp;
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = LOWORD(lParam);
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = WindowGlobal::windowHeight - HIWORD( lParam );
            break;
        case WM_MOUSEWHEEL:
        {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = delta < 0 ? ae3d::WindowEventType::MouseWheelScrollDown : ae3d::WindowEventType::MouseWheelScrollUp;
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = LOWORD(lParam);
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = WindowGlobal::windowHeight - HIWORD( lParam );
        }
            break;
        case WM_MOUSEMOVE:
            // Check to see if the left button is held down:
            //bool leftButtonDown=wParam & MK_LBUTTON;
            
            // Check if right button down:
            //bool rightButtonDown=wParam & MK_RBUTTON;
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::MouseMove;
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = LOWORD( lParam );
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = WindowGlobal::windowHeight - HIWORD( lParam );
            break;
        case WM_CLOSE:
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::Close;
            break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

void PlatformInitGamePad()
{
    HMODULE XInputLibrary = LoadLibraryA( "xinput1_4.dll" );
    
    if (!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA( "xinput9_1_0.dll" );
    }

    if (!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA( "xinput1_3.dll" );
    }

    if (XInputLibrary)
    {
        XInputGetState_ = (x_input_get_state*)GetProcAddress( XInputLibrary, "XInputGetState" );
        XInputSetState_ = (x_input_set_state*)GetProcAddress( XInputLibrary, "XInputSetState" );
    }
    else
    {
        ae3d::System::Print( "Could not init gamepad.\n" );
    }
}

namespace ae3d
{
    float ProcessGamePadStickValue( SHORT value, SHORT deadZoneThreshold )
    {
        float result = 0;
        
        if (value < -deadZoneThreshold)
        {
            result = (float)((value + deadZoneThreshold) / (32768.0f - deadZoneThreshold));
        }
        else if (value > deadZoneThreshold)
        {
            result = (float)((value - deadZoneThreshold) / (32767.0f - deadZoneThreshold));
        }

        return result;
    }

    void PumpGamePadEvents()
    {
        XINPUT_STATE ControllerState;

        if (XInputGetState_( 0, &ControllerState ) == ERROR_SUCCESS)
        {
            XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;

            float avgX = ProcessGamePadStickValue( Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE );
            float avgY = ProcessGamePadStickValue( Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE );
            
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadLeftThumbState;
            WindowGlobal::eventStack[WindowGlobal::eventIndex].gamePadThumbX = avgX;
            WindowGlobal::eventStack[WindowGlobal::eventIndex].gamePadThumbY = avgY;

            avgX = ProcessGamePadStickValue( Pad->sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE );
            avgY = ProcessGamePadStickValue( Pad->sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE );

            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadRightThumbState;
            WindowGlobal::eventStack[WindowGlobal::eventIndex].gamePadThumbX = avgX;
            WindowGlobal::eventStack[WindowGlobal::eventIndex].gamePadThumbY = avgY;

            if ((Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0)
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadButtonDPadUp;
            }
            if ((Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0)
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadButtonDPadDown;
            }
            if ((Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0)
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadButtonDPadLeft;
            }
            if ((Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0)
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadButtonDPadRight;
            }
            if ((Pad->wButtons & XINPUT_GAMEPAD_A) != 0)
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadButtonA;
            }
            if ((Pad->wButtons & XINPUT_GAMEPAD_B) != 0)
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadButtonB;
            }
            if ((Pad->wButtons & XINPUT_GAMEPAD_X) != 0)
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadButtonX;
            }
            if ((Pad->wButtons & XINPUT_GAMEPAD_Y) != 0)
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadButtonY;
            }
            if ((Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0)
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadButtonLeftShoulder;
            }
            if ((Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)!= 0)
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadButtonRightShoulder;

            }
            if ((Pad->wButtons & XINPUT_GAMEPAD_START) != 0)
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadButtonStart;
            }
            if ((Pad->wButtons & XINPUT_GAMEPAD_BACK) != 0)
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadButtonBack;
            }
        }
    }

    void Window::Create(int width, int height, WindowCreateFlags flags)
    {
        InitKeyMap();
        WindowGlobal::windowWidth = width == 0 ? GetSystemMetrics(SM_CXSCREEN) : width;
        WindowGlobal::windowHeight = height == 0 ? GetSystemMetrics( SM_CYSCREEN ) : height;

        const HINSTANCE hInstance = GetModuleHandle(nullptr);
        const bool fullscreen = (flags & WindowCreateFlags::Fullscreen) != 0;

        WNDCLASSEX wc;
        ZeroMemory( &wc, sizeof( WNDCLASSEX ) );

        wc.cbSize = sizeof( WNDCLASSEX );
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
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

        WindowGlobal::hwnd = CreateWindowExA( fullscreen ? WS_EX_TOOLWINDOW | WS_EX_TOPMOST : 0,
            "WindowClass1",    // name of the window class
            "Window",   // title of the window
            fullscreen ? WS_POPUP : (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU),    // window style
            CW_USEDEFAULT,    // x-position of the window
            CW_USEDEFAULT,    // y-position of the window
            WindowGlobal::windowWidth,    // width of the window
            WindowGlobal::windowHeight,    // height of the window
            nullptr,    // we have no parent window
            nullptr,    // we aren't using menus
            hInstance,    // application handle
            nullptr );    // used with multiple windows    
        
        ShowWindow( WindowGlobal::hwnd, SW_SHOW );
        CreateRenderer();
        WindowGlobal::isOpen = true;
        GfxDevice::Init( WindowGlobal::windowWidth, WindowGlobal::windowHeight );
    }

    void Window::SetTitle( const char* title )
    {
        SetWindowTextA( WindowGlobal::hwnd, title );
    }

    void Window::GetSize( int& outWidth, int& outHeight )
    {
        outWidth = WindowGlobal::windowWidth;
        outHeight = WindowGlobal::windowHeight;
    }

    bool Window::PollEvent(WindowEvent& outEvent)
    {
        if (WindowGlobal::eventIndex == -1)
        {
            return false;
        }

        outEvent = WindowGlobal::eventStack[ WindowGlobal::eventIndex ];
        --WindowGlobal::eventIndex;
        return true;
    }

    void Window::PumpEvents()
    {
        MSG msg;

        if (PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ))
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }

        PumpGamePadEvents();
    }

    bool Window::IsOpen()
    {
        return WindowGlobal::isOpen;
    }

    void Window::SwapBuffers()
    {
        ::SwapBuffers( WindowGlobal::hdc );
    }
}