// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "Window.hpp"
#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOHELP
#define NOSERVICE
#define NOKANJI
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
    void CreateRenderer( int samples, bool apiValidation );
}

namespace WindowGlobal
{
    int windowWidth = 640;
    int windowHeight = 480;
    int windowHeightWithoutTitleBar = 640;
    int presentInterval = 1;
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
    bool isGamePadConnected = false;
    int gamePadIndex = 0;
    HWND hwnd;
    ae3d::KeyCode keyMap[ 256 ];
}

namespace GfxDeviceGlobal
{
    extern unsigned frameIndex;
}

static void InitKeyMap()
{
    for (int keyIndex = 0; keyIndex < 256; ++keyIndex)
    {
        WindowGlobal::keyMap[ keyIndex ] = ae3d::KeyCode::Empty;
    }

    WindowGlobal::keyMap[ 33 ] = ae3d::KeyCode::PageUp;
    WindowGlobal::keyMap[ 34 ] = ae3d::KeyCode::PageDown;
    WindowGlobal::keyMap[ 13 ] = ae3d::KeyCode::Enter;
    WindowGlobal::keyMap[ 37 ] = ae3d::KeyCode::Left;
    WindowGlobal::keyMap[ 38 ] = ae3d::KeyCode::Up;
    WindowGlobal::keyMap[ 39 ] = ae3d::KeyCode::Right;
    WindowGlobal::keyMap[ 40 ] = ae3d::KeyCode::Down;
    WindowGlobal::keyMap[ 27 ] = ae3d::KeyCode::Escape;
    WindowGlobal::keyMap[ 32 ] = ae3d::KeyCode::Space;
    WindowGlobal::keyMap[ 46 ] = ae3d::KeyCode::Delete;
    WindowGlobal::keyMap[ 8 ] = ae3d::KeyCode::Backspace;
    WindowGlobal::keyMap[ 189 ] = ae3d::KeyCode::Minus;
    WindowGlobal::keyMap[ 187 ] = ae3d::KeyCode::Plus;

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
    
    WindowGlobal::keyMap[ 48 ] = ae3d::KeyCode::N0;
    WindowGlobal::keyMap[ 49 ] = ae3d::KeyCode::N1;
    WindowGlobal::keyMap[ 50 ] = ae3d::KeyCode::N2;
    WindowGlobal::keyMap[ 51 ] = ae3d::KeyCode::N3;
    WindowGlobal::keyMap[ 52 ] = ae3d::KeyCode::N4;
    WindowGlobal::keyMap[ 53 ] = ae3d::KeyCode::N5;
    WindowGlobal::keyMap[ 54 ] = ae3d::KeyCode::N6;
    WindowGlobal::keyMap[ 55 ] = ae3d::KeyCode::N7;
    WindowGlobal::keyMap[ 56 ] = ae3d::KeyCode::N8;
    WindowGlobal::keyMap[ 57 ] = ae3d::KeyCode::N9;
        
    WindowGlobal::keyMap[ 190 ] = ae3d::KeyCode::Dot;
    WindowGlobal::keyMap[ 36 ] = ae3d::KeyCode::Home;
    WindowGlobal::keyMap[ 35 ] = ae3d::KeyCode::End;
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
            break;
        case WM_SYSKEYUP:
        case WM_KEYUP:
            ae3d::System::Assert( wParam < 256, "too high keycode" );
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::KeyUp;
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].keyCode = WindowGlobal::keyMap[ (unsigned)wParam ];

            if ((GetKeyState( VK_CONTROL ) & 0x8000) != 0)
            {
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].keyModifier = ae3d::KeyModifier::Control;
            }
            if ((GetKeyState( VK_SHIFT ) & 0x8000) != 0)
            {
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].keyModifier = ae3d::KeyModifier::Shift;
            }
            if ((GetKeyState( VK_MENU ) & 0x8000) != 0) // alt
            {
                //WindowGlobal::eventStack[ WindowGlobal::eventIndex ].keyModifier = ae3d::KeyModifier::A;
            }

            break;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            ae3d::System::Assert( wParam < 256, "too high keycode" );
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::KeyDown;
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].keyCode = WindowGlobal::keyMap[ (unsigned)wParam ];
            //ae3d::System::Print( "%d\n", wParam  );
            if ((GetKeyState( VK_CONTROL ) & 0x8000) != 0)
            {
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].keyModifier = ae3d::KeyModifier::Control;
            }
            if ((GetKeyState( VK_SHIFT ) & 0x8000) != 0)
            {
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].keyModifier = ae3d::KeyModifier::Shift;
            }
            if ((GetKeyState( VK_MENU ) & 0x8000) != 0) // alt
            {
                //WindowGlobal::eventStack[ WindowGlobal::eventIndex ].keyModifier = ae3d::KeyModifier::A;
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
            break;
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        {
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = message == WM_LBUTTONDOWN ? ae3d::WindowEventType::Mouse1Down : ae3d::WindowEventType::Mouse1Up;
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = LOWORD(lParam);
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = WindowGlobal::windowHeightWithoutTitleBar - HIWORD( lParam );
        }
            break;
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        {
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = message == WM_RBUTTONDOWN ? ae3d::WindowEventType::Mouse2Down : ae3d::WindowEventType::Mouse2Up;
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = LOWORD(lParam);
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = WindowGlobal::windowHeightWithoutTitleBar - HIWORD( lParam );
        }
            break;
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = message == WM_MBUTTONDOWN ? ae3d::WindowEventType::MouseMiddleDown : ae3d::WindowEventType::MouseMiddleUp;
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = LOWORD(lParam);
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = WindowGlobal::windowHeightWithoutTitleBar - HIWORD( lParam );
            break;
        case WM_MOUSEWHEEL:
        {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = delta < 0 ? ae3d::WindowEventType::MouseWheelScrollDown : ae3d::WindowEventType::MouseWheelScrollUp;
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = LOWORD(lParam);
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = WindowGlobal::windowHeightWithoutTitleBar - HIWORD( lParam );
			WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseWheel = delta < 0 ? -1 : 1;
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
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = WindowGlobal::windowHeightWithoutTitleBar - HIWORD( lParam );
            break;
        case WM_CLOSE:
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::Close;
            break;
    }

    return DefWindowProc( hWnd, message, wParam, lParam );
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

        XINPUT_STATE controllerState;

        WindowGlobal::isGamePadConnected = XInputGetState_( 0, &controllerState ) == ERROR_SUCCESS;

        if (!WindowGlobal::isGamePadConnected)
        {
            WindowGlobal::isGamePadConnected = XInputGetState_( 1, &controllerState ) == ERROR_SUCCESS;
            WindowGlobal::gamePadIndex = 1;
        }
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
        XINPUT_STATE controllerState;

        if (XInputGetState_( WindowGlobal::gamePadIndex, &controllerState ) == ERROR_SUCCESS)
        {
            XINPUT_GAMEPAD* pad = &controllerState.Gamepad;

            float avgX = ProcessGamePadStickValue( pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE );
            float avgY = ProcessGamePadStickValue( pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE );
            
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::GamePadLeftThumbState;
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].gamePadThumbX = avgX;
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].gamePadThumbY = avgY;

            avgX = ProcessGamePadStickValue( pad->sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE );
            avgY = ProcessGamePadStickValue( pad->sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE );

            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::GamePadRightThumbState;
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].gamePadThumbX = avgX;
            WindowGlobal::eventStack[ WindowGlobal::eventIndex ].gamePadThumbY = avgY;

            if ((pad->wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0)
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::GamePadButtonDPadUp;
            }
            if ((pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0)
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::GamePadButtonDPadDown;
            }
            if ((pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0)
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::GamePadButtonDPadLeft;
            }
            if ((pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0)
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::GamePadButtonDPadRight;
            }
            if ((pad->wButtons & XINPUT_GAMEPAD_A) != 0)
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::GamePadButtonA;
            }
            if ((pad->wButtons & XINPUT_GAMEPAD_B) != 0)
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::GamePadButtonB;
            }
            if ((pad->wButtons & XINPUT_GAMEPAD_X) != 0)
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::GamePadButtonX;
            }
            if ((pad->wButtons & XINPUT_GAMEPAD_Y) != 0)
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::GamePadButtonY;
            }
            if ((pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0)
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::GamePadButtonLeftShoulder;
            }
            if ((pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)!= 0)
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::GamePadButtonRightShoulder;

            }
            if ((pad->wButtons & XINPUT_GAMEPAD_START) != 0)
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::GamePadButtonStart;
            }
            if ((pad->wButtons & XINPUT_GAMEPAD_BACK) != 0)
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::GamePadButtonBack;
            }
        }
    }

    bool Window::IsKeyDown( KeyCode keyCode )
    {
        for (int i = 0; i < 256; ++i)
        {
            if (WindowGlobal::keyMap[ i ] == keyCode)
            {
                return (GetKeyState( i ) & 0x8000) != 0;
            }
        }

        return false;
    }

    void Window::Create(int width, int height, WindowCreateFlags flags)
    {
        SetEnvironmentVariable( "DISABLE_VK_LAYER_VALVE_steam_overlay_1", "1" );

        InitKeyMap();
        WindowGlobal::windowWidth = width == 0 ? GetSystemMetrics( SM_CXSCREEN ) : width;
        WindowGlobal::windowHeight = height == 0 ? GetSystemMetrics( SM_CYSCREEN ) : height;
        WindowGlobal::presentInterval = (flags & WindowCreateFlags::No_vsync) != 0 ? 0 : 1;

        const HINSTANCE hInstance = GetModuleHandle( nullptr );
        const bool fullscreen = (flags & WindowCreateFlags::Fullscreen) != 0;

        WNDCLASSEX wc = {};

        wc.cbSize = sizeof( WNDCLASSEX );
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
        wc.lpszClassName = "WindowClass1";

        wc.hIcon = static_cast< HICON >(LoadImage(nullptr,
            "glider.ico",
            IMAGE_ICON,
            32,
            32,
            LR_LOADFROMFILE));

        auto ret = RegisterClassEx(&wc);
		
        if (ret == 0)
        {
            System::Assert( false, "failed to register window class" );
        }

        const int xPos = (GetSystemMetrics( SM_CXSCREEN ) - WindowGlobal::windowWidth) / 2;
        const int yPos = (GetSystemMetrics( SM_CYSCREEN ) - WindowGlobal::windowHeight) / 2;

        WindowGlobal::hwnd = CreateWindowExA( fullscreen ? WS_EX_TOOLWINDOW | WS_EX_TOPMOST : 0,
            "WindowClass1",    // name of the window class
            "Window",   // title of the window
            fullscreen ? WS_POPUP : (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU),    // window style
            xPos,    // x-position of the window
            yPos,    // y-position of the window
            WindowGlobal::windowWidth,    // width of the window
            WindowGlobal::windowHeight,    // height of the window
            nullptr,    // we have no parent window
            nullptr,    // we aren't using menus
            hInstance,    // application handle
            nullptr );    // used with multiple windows    

        ShowWindow( WindowGlobal::hwnd, SW_SHOW );
        
        int samples = 1;

        if (flags & WindowCreateFlags::MSAA4)
        {
            samples = 4;
        }
        else if (flags & WindowCreateFlags::MSAA8)
        {
            samples = 8;
        }
        else if (flags & WindowCreateFlags::MSAA16)
        {
            samples = 16;
        }

        CreateRenderer( samples, (flags & ae3d::WindowCreateFlags::ApiValidation) != 0 ? true : false );
        WindowGlobal::isOpen = true;
        RECT rect = {};
        GetClientRect( WindowGlobal::hwnd, &rect );
        GfxDevice::Init( rect.right, rect.bottom );
        WindowGlobal::windowHeightWithoutTitleBar = rect.bottom;
    }

    void Window::SetTitle( const char* title )
    {
        SetWindowTextA( WindowGlobal::hwnd, title );
    }

    void Window::GetSize( int& outWidth, int& outHeight )
    {
        outWidth = WindowGlobal::windowWidth;
        outHeight = WindowGlobal::windowHeightWithoutTitleBar;
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

    bool Window::IsWayland()
    {
        return false;
    }

    void Window::PumpEvents()
    {
        MSG msg;

        while (PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ))
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }

        if (WindowGlobal::isGamePadConnected)
        {
            PumpGamePadEvents();
        }
    }

    bool Window::IsOpen()
    {
        return WindowGlobal::isOpen;
    }

    void Window::SwapBuffers()
    {
        GfxDevice::Present();
#if RENDERER_D3D12
        ++GfxDeviceGlobal::frameIndex;
#endif
    }
}
