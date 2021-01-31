#include "Window.hpp"
#include <map>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_ewmh.h>
#include <X11/keysym.h>
#include <X11/Xlib-xcb.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <dirent.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "GfxDevice.hpp"

struct GamePad
{
    bool isActive = false;
    int fd;
    int buttonA;
    int buttonB;
    int buttonX;
    int buttonY;
    int buttonBack;
    int buttonStart;
    int buttonLeftShoulder;
    int buttonRightShoulder;
    int dpadXaxis;
    int dpadYaxis;
    int leftThumbX;
    int leftThumbY;
    int rightThumbX;
    int rightThumbY;
    short deadZone;
};

namespace ae3d
{
    void CreateRenderer( int samples );
}

namespace
{
    // TODO: DRY. Duplicated in WindowWin32.cpp
    float ProcessGamePadStickValue( short value, short deadZoneThreshold )
    {
        float result = 0;
        
        if (value < -deadZoneThreshold)
        {
            result = (value + deadZoneThreshold) / (32768.0f - deadZoneThreshold);
        }
        else if (value > deadZoneThreshold)
        {
            result = (value - deadZoneThreshold) / (32767.0f - deadZoneThreshold);
        }

        return result;
    }
}

namespace WindowGlobal
{
    bool isWayland = false;
    bool isOpen = false;
    const int eventStackSize = 15;
    ae3d::WindowEvent eventStack[eventStackSize];
    int eventIndex = -1;
    int presentInterval = 1;

    void IncEventIndex()
    {
        if (eventIndex < eventStackSize - 1)
        {
            ++eventIndex;
        }
    }
    
    Display* display = nullptr;
    xcb_connection_t* connection = nullptr;
    xcb_window_t window;
    xcb_key_symbols_t* key_symbols = nullptr;
    xcb_atom_t wm_protocols;
    xcb_atom_t wm_delete_window;

    xcb_ewmh_connection_t EWMH;
    xcb_intern_atom_cookie_t* EWMHCookie = nullptr;
    
    int windowWidth = 640;
    int windowHeight = 480;
    GamePad gamePad;
    float lastLeftThumbX = 0;
    float lastLeftThumbY = 0;
    float lastRightThumbX = 0;
    float lastRightThumbY = 0;
    bool keyDown[ 256 ] = {};
    
    std::map< xcb_keysym_t, ae3d::KeyCode > keyMap =
    {
        { XK_a, ae3d::KeyCode::A },
        { XK_b, ae3d::KeyCode::B },
        { XK_c, ae3d::KeyCode::C },
        { XK_d, ae3d::KeyCode::D },
        { XK_e, ae3d::KeyCode::E },
        { XK_f, ae3d::KeyCode::F },
        { XK_g, ae3d::KeyCode::G },
        { XK_h, ae3d::KeyCode::H },
        { XK_i, ae3d::KeyCode::I },
        { XK_j, ae3d::KeyCode::J },
        { XK_k, ae3d::KeyCode::K },
        { XK_l, ae3d::KeyCode::L },
        { XK_m, ae3d::KeyCode::M },
        { XK_n, ae3d::KeyCode::N },
        { XK_o, ae3d::KeyCode::O },
        { XK_p, ae3d::KeyCode::P },
        { XK_q, ae3d::KeyCode::Q },
        { XK_r, ae3d::KeyCode::R },
        { XK_s, ae3d::KeyCode::S },
        { XK_t, ae3d::KeyCode::T },
        { XK_u, ae3d::KeyCode::U },
        { XK_v, ae3d::KeyCode::V },
        { XK_w, ae3d::KeyCode::W },
        { XK_x, ae3d::KeyCode::X },
        { XK_y, ae3d::KeyCode::Y },
        { XK_z, ae3d::KeyCode::Z },
        { XK_Left, ae3d::KeyCode::Left },
        { XK_Right, ae3d::KeyCode::Right },
        { XK_Up, ae3d::KeyCode::Up },
        { XK_Down, ae3d::KeyCode::Down },
        { XK_Escape, ae3d::KeyCode::Escape },
        { XK_Return, ae3d::KeyCode::Enter },
        { XK_Delete, ae3d::KeyCode::Delete },
        { XK_space, ae3d::KeyCode::Space },
        { XK_BackSpace, ae3d::KeyCode::Backspace },
        { XK_0, ae3d::KeyCode::N0 },
        { XK_1, ae3d::KeyCode::N1 },
        { XK_2, ae3d::KeyCode::N2 },
        { XK_3, ae3d::KeyCode::N3 },
        { XK_4, ae3d::KeyCode::N4 },
        { XK_5, ae3d::KeyCode::N5 },
        { XK_6, ae3d::KeyCode::N6 },
        { XK_7, ae3d::KeyCode::N7 },
        { XK_8, ae3d::KeyCode::N8 },
        { XK_9, ae3d::KeyCode::N9 },
        { 46, ae3d::KeyCode::Dot },
        { 43, ae3d::KeyCode::Plus },
        { 45, ae3d::KeyCode::Minus },
        { 65365, ae3d::KeyCode::PageUp },
        { 65366, ae3d::KeyCode::PageDown }        
    };
}

void PlatformInitGamePad()
{
    DIR* dir = opendir( "/dev/input" );
    dirent* result = readdir( dir );

    while (result != nullptr)
    {
        dirent& entry = *result;
        
        if ((entry.d_name[0] == 'j') && (entry.d_name[1] == 's'))
        {
            char full_device_path[ 267 ];
            snprintf( full_device_path, sizeof( full_device_path ), "%s/%s", "/dev/input", entry.d_name );
            int fd = open( full_device_path, O_RDONLY );

            if (fd < 0)
            {
                // Permissions could cause this code path.
                continue;
            }

            char name[ 128 ];
            ioctl( fd, JSIOCGNAME( 128 ), name);
            /*if (!strstr(name, "Microsoft X-Box 360 pad"))
            {
                //close(fd);
                std::cerr << "Not an xbox controller: " << std::string( name ) << std::endl;;
                continue;
                }*/

            int version;
            ioctl( fd, JSIOCGVERSION, &version );
            uint8_t axes;
            ioctl( fd, JSIOCGAXES, &axes );
            uint8_t buttons;
            ioctl( fd, JSIOCGBUTTONS, &buttons );
            WindowGlobal::gamePad.fd = fd;
            WindowGlobal::gamePad.isActive = true;
            // XBox One Controller values. Should also work for 360 Controller.
            WindowGlobal::gamePad.buttonA = 0;
            WindowGlobal::gamePad.buttonB = 1;
            WindowGlobal::gamePad.buttonX = 2;
            WindowGlobal::gamePad.buttonY = 3;
            WindowGlobal::gamePad.buttonStart = 7;
            WindowGlobal::gamePad.buttonBack = 6;
            WindowGlobal::gamePad.buttonLeftShoulder = 4;
            WindowGlobal::gamePad.buttonRightShoulder = 5;
            WindowGlobal::gamePad.dpadXaxis = 6;
            WindowGlobal::gamePad.dpadYaxis = 7;
            WindowGlobal::gamePad.leftThumbX = 0;
            WindowGlobal::gamePad.leftThumbY = 1;
            WindowGlobal::gamePad.rightThumbX = 3;
            WindowGlobal::gamePad.rightThumbY = 4;
            WindowGlobal::gamePad.deadZone = 7849;
            
            fcntl( fd, F_SETFL, O_NONBLOCK );
        }

        result = readdir( dir );
    }

    closedir( dir );
}

void LoadAtoms()
{
    xcb_intern_atom_cookie_t wm_delete_window_cookie = xcb_intern_atom( WindowGlobal::connection, 0, 16, "WM_DELETE_WINDOW" );
    xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom(WindowGlobal::connection, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS" );

    xcb_flush( WindowGlobal::connection );
    xcb_intern_atom_reply_t* wm_delete_window_cookie_reply = xcb_intern_atom_reply( WindowGlobal::connection, wm_delete_window_cookie, nullptr );
    xcb_intern_atom_reply_t* wm_protocols_cookie_reply = xcb_intern_atom_reply( WindowGlobal::connection, wm_protocols_cookie, nullptr );

    WindowGlobal::wm_protocols = wm_protocols_cookie_reply->atom;
    WindowGlobal::wm_delete_window = wm_delete_window_cookie_reply->atom;
}

static int CreateWindowAndContext( Display* display, xcb_connection_t* connection, xcb_screen_t* screen, int width, int height, ae3d::WindowCreateFlags flags )
{
    WindowGlobal::presentInterval = (flags & ae3d::WindowCreateFlags::No_vsync) ? 0 : 1;
    WindowGlobal::isWayland = strstr( getenv( "XDG_SESSION_TYPE" ), "wayland" ) != 0;
    
    int visualID = screen->root_visual;
    
    xcb_colormap_t colormap = xcb_generate_id( connection );
    WindowGlobal::window = xcb_generate_id( connection );

    WindowGlobal::windowWidth = width == 0 ? screen->width_in_pixels : width;
    WindowGlobal::windowHeight = height == 0 ? screen->height_in_pixels : height;

    xcb_create_colormap( connection, XCB_COLORMAP_ALLOC_NONE, colormap, screen->root, visualID );
    
    const uint32_t eventmask = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
                               XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION;
    const uint32_t valuelist[] = { eventmask, colormap, 0 };
    const uint32_t valuemask = XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
    
    xcb_create_window(
                      connection,
                      XCB_COPY_FROM_PARENT,
                      WindowGlobal::window,
                      screen->root,
                      0, 0,
                      WindowGlobal::windowWidth, WindowGlobal::windowHeight,
                      0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      visualID,
                      valuemask,
                      valuelist
                      );

    xcb_map_window( connection, WindowGlobal::window );

    xcb_size_hints_t hints = {};

    xcb_icccm_size_hints_set_min_size( &hints, WindowGlobal::windowWidth, WindowGlobal::windowHeight );
    xcb_icccm_size_hints_set_max_size( &hints, WindowGlobal::windowWidth, WindowGlobal::windowHeight );

    xcb_icccm_set_wm_size_hints( WindowGlobal::connection, WindowGlobal::window, XCB_ATOM_WM_NORMAL_HINTS, &hints );

    xcb_atom_t protocols[] =
    {
        WindowGlobal::wm_delete_window
    };
    xcb_icccm_set_wm_protocols( WindowGlobal::connection, WindowGlobal::window,
                                WindowGlobal::wm_protocols, 1, protocols );
    
    if ((flags & ae3d::WindowCreateFlags::Fullscreen) != 0)
    {
        WindowGlobal::EWMHCookie = xcb_ewmh_init_atoms( WindowGlobal::connection, &WindowGlobal::EWMH );

        if (!xcb_ewmh_init_atoms_replies( &WindowGlobal::EWMH, WindowGlobal::EWMHCookie, nullptr ))
        {
            printf( "Fullscreen not supported.\n" );
        }

        xcb_ewmh_request_change_wm_state( &WindowGlobal::EWMH, XDefaultScreen( display ), WindowGlobal::window,
                                          XCB_EWMH_WM_STATE_ADD, WindowGlobal::EWMH._NET_WM_STATE_FULLSCREEN, 0,
                                          XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL
                                          );
        xcb_ewmh_request_change_active_window( &WindowGlobal::EWMH, XDefaultScreen( display ), WindowGlobal::window,
                                               XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL, XCB_CURRENT_TIME, XCB_WINDOW_NONE
                                               );
    
        xcb_generic_error_t* error;
        xcb_get_window_attributes_reply_t* reply = xcb_get_window_attributes_reply( WindowGlobal::connection, xcb_get_window_attributes( WindowGlobal::connection, WindowGlobal::window ), &error );

        if (!reply)
        {
            printf( "Full screen reply failed\n" );
        }
    }
    
    return 0;
}

bool ae3d::Window::IsWayland()
{
    return WindowGlobal::isWayland;
}

bool ae3d::Window::IsOpen()
{
    return WindowGlobal::isOpen;
}

void ae3d::Window::Create( int width, int height, WindowCreateFlags flags )
{
    WindowGlobal::display = XOpenDisplay( nullptr );

    if (!WindowGlobal::display)
    {
        printf( "Can't open display\n" );
        return;
    }
    
    const int default_screen = DefaultScreen( WindowGlobal::display );

    WindowGlobal::connection = XGetXCBConnection( WindowGlobal::display );
    LoadAtoms();

    if (!WindowGlobal::connection)
    {
        XCloseDisplay( WindowGlobal::display );
        printf( "Can't get xcb connection from display\n" );
        return;
    }
    
    XSetEventQueueOwner( WindowGlobal::display, XCBOwnsEventQueue );

    xcb_screen_iterator_t screen_iter = xcb_setup_roots_iterator( xcb_get_setup( WindowGlobal::connection ) );
    for (int screen_num = default_screen; screen_iter.rem && screen_num > 0; --screen_num, xcb_screen_next( &screen_iter ));
    xcb_screen_t* screen = screen_iter.data;
    
    WindowGlobal::key_symbols = xcb_key_symbols_alloc( WindowGlobal::connection );

    if (CreateWindowAndContext( WindowGlobal::display, WindowGlobal::connection, screen, width, height, flags ) == -1)
    {
        return;
    }

    GfxDevice::Init( WindowGlobal::windowWidth, WindowGlobal::windowHeight );

    int samples = 1;

    if (flags & ae3d::WindowCreateFlags::MSAA4)
    {
        samples = 4;
    }
    else if (flags & ae3d::WindowCreateFlags::MSAA8)
    {
        samples = 8;
    }
    else if (flags & ae3d::WindowCreateFlags::MSAA16)
    {
        samples = 16;
    }

    ae3d::CreateRenderer( samples );
    SetTitle( "Aether3D Game Engine" );
    WindowGlobal::isOpen = true;
}

void ae3d::Window::SetTitle( const char* title )
{
    xcb_change_property(WindowGlobal::connection,
                        XCB_PROP_MODE_REPLACE,
                        WindowGlobal::window,
                        XCB_ATOM_WM_NAME,
                        XCB_ATOM_STRING,
                        8,
                        strlen( title ),
                        title );

    xcb_change_property(WindowGlobal::connection,
                    XCB_PROP_MODE_REPLACE,
                    WindowGlobal::window,
                    XCB_ATOM_WM_CLASS,
                    XCB_ATOM_STRING,
                    8,
                    sizeof("aether3d""\0""Aether3D"),
                    "aether3d\0Aether3D" );
}

void ae3d::Window::GetSize( int& outWidth, int& outHeight )
{
    outWidth = WindowGlobal::windowWidth;
    outHeight = WindowGlobal::windowHeight;
}

void ae3d::Window::PumpEvents()
{
    xcb_generic_event_t* event;
    
    while ((event = xcb_poll_for_event( WindowGlobal::connection )))
    {
        const uint8_t response_type = event->response_type & ~0x80;
        
        switch (response_type)
        {
            case 5: // XCB_EVENT_MASK_BUTTON_RELEASE doesn't work for some reason
            {
                const xcb_button_release_event_t* be = (xcb_button_release_event_t*)event;

                WindowGlobal::IncEventIndex();
                
                if (be->detail == 1)
                {
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::Mouse1Up;
                }
                else if (be->detail == 3)
                {
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::Mouse2Up;
                }
                else if (be->detail == 2)
                {
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::MouseMiddleUp;
                }
                else if (be->detail == 4)
                {
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::MouseWheelScrollUp;
                }
                else if (be->detail == 5)
                {
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::MouseWheelScrollDown;
                }
                else
                {
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::Mouse1Up;
                }
                
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = be->event_x;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = WindowGlobal::windowHeight - be->event_y;

                break;
            }
            case XCB_EVENT_MASK_BUTTON_PRESS:
            {
                const xcb_query_pointer_reply_t* pointer = xcb_query_pointer_reply( WindowGlobal::connection, xcb_query_pointer(WindowGlobal::connection, XDefaultRootWindow(WindowGlobal::display)), nullptr );
                const bool newb1 = (pointer->mask & XCB_BUTTON_MASK_1) != 0;
                const bool newb2 = (pointer->mask & XCB_BUTTON_MASK_2) != 0;
                const bool newb3 = (pointer->mask & XCB_BUTTON_MASK_3) != 0;
                const xcb_button_press_event_t* be = (xcb_button_press_event_t*)event;

                WindowGlobal::IncEventIndex();
                
                if (newb1)
                {
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::Mouse1Down;
                }
                else if (newb3)
                {
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::Mouse2Down;
                }
                else if (newb2)
                {
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::MouseMiddleDown;
                }
                else if (be->detail == 4)
                {
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::MouseWheelScrollUp;
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseWheel = -1;
                }
                else if (be->detail == 5)
                {
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::MouseWheelScrollDown;
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseWheel = 1;
                }
                else
                {
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::Mouse1Down;
                }
                
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = be->event_x;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = WindowGlobal::windowHeight - be->event_y;
                break;
            }
            case XCB_KEY_PRESS:
            case XCB_KEY_RELEASE:
            {
                const xcb_key_press_event_t* e = (xcb_key_press_event_t *)event;
                const bool isDown = (response_type == XCB_KEY_PRESS);
                const auto type = isDown ? ae3d::WindowEventType::KeyDown : ae3d::WindowEventType::KeyUp;
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = type;

                if (e->state == 1)
                {
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].keyModifier = KeyModifier::Shift;
                }
                else if (e->state == 4)
                {
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].keyModifier = KeyModifier::Control;
                }
                else
                {
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].keyModifier = KeyModifier::Empty;
                }

                const xcb_keysym_t keysym = xcb_key_symbols_get_keysym(WindowGlobal::key_symbols, e->detail, 0);

                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].keyCode = (WindowGlobal::keyMap.find( keysym ) == WindowGlobal::keyMap.end() ) ? ae3d::KeyCode::Empty : WindowGlobal::keyMap[ keysym ];
                WindowGlobal::keyDown[ (int)WindowGlobal::eventStack[ WindowGlobal::eventIndex ].keyCode ] = isDown;
                
                break;
            }
            case XCB_MOTION_NOTIFY:
            {
                const xcb_motion_notify_event_t* e = (xcb_motion_notify_event_t*)event;
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::MouseMove;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = e->event_x;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = WindowGlobal::windowHeight - e->event_y;
                break;
            }
            case XCB_CLIENT_MESSAGE:
            {
                const xcb_client_message_event_t* client_message_event = (xcb_client_message_event_t*)event;

                //if (client_message_event->type == WindowGlobal::wm_protocols)
                {
                    if (client_message_event->data.data32[0] == WindowGlobal::wm_delete_window)
                    {
                        WindowGlobal::IncEventIndex();
                        WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::Close;
                    }
                    else
                    {
                        printf( "got client message\n" );
                    }
                }
                //else
                {
                    //std::cerr << "got client message" << std::endl;
                }
                break;
            }
            case XCB_EXPOSE:
            {
                break;
            }
        default:
            break;
        }

        free( event );
    }

    if (!WindowGlobal::gamePad.isActive)
    {
        return;
    }

    js_event j;

    while (read( WindowGlobal::gamePad.fd, &j, sizeof(js_event) ) == sizeof(js_event))
    {
        // Don't care if init or afterwards
        j.type &= ~JS_EVENT_INIT;
            
        if (j.type == JS_EVENT_BUTTON)
        {
            if (j.number == WindowGlobal::gamePad.buttonA)
            {
                if (j.value > 0)
                {
                    WindowGlobal::IncEventIndex();
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = WindowEventType::GamePadButtonA;
                }
            }
            else if (j.number == WindowGlobal::gamePad.buttonB)
            {
                if (j.value > 0)
                {
                    WindowGlobal::IncEventIndex();
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = WindowEventType::GamePadButtonB;
                }
            }
            else if (j.number == WindowGlobal::gamePad.buttonX)
            {
                if (j.value > 0)
                {
                    WindowGlobal::IncEventIndex();
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = WindowEventType::GamePadButtonX;
                }
            }
            else if (j.number == WindowGlobal::gamePad.buttonY)
            {
                if (j.value > 0)
                {
                    WindowGlobal::IncEventIndex();
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = WindowEventType::GamePadButtonY;
                }
            }
            else if (j.number == WindowGlobal::gamePad.buttonStart)
            {
                if (j.value > 0)
                {
                    WindowGlobal::IncEventIndex();
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = WindowEventType::GamePadButtonStart;
                }
            }
            else if (j.number == WindowGlobal::gamePad.buttonBack)
            {
                if (j.value > 0)
                {
                    WindowGlobal::IncEventIndex();
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = WindowEventType::GamePadButtonBack;
                }
            }
            else
            {
            }
        }
        else if (j.type == JS_EVENT_AXIS)
        {
            if (j.number == WindowGlobal::gamePad.leftThumbX)
            {
                const float x = ProcessGamePadStickValue( j.value, WindowGlobal::gamePad.deadZone );
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = WindowEventType::GamePadLeftThumbState;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].gamePadThumbX = x;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].gamePadThumbY = WindowGlobal::lastLeftThumbY;
                WindowGlobal::lastLeftThumbX = x;
            }
            else if (j.number == WindowGlobal::gamePad.leftThumbY)
            {
                const float y = ProcessGamePadStickValue( j.value, WindowGlobal::gamePad.deadZone );
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = WindowEventType::GamePadLeftThumbState;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].gamePadThumbX = WindowGlobal::lastLeftThumbX;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].gamePadThumbY = -y;
                WindowGlobal::lastLeftThumbY = -y;
            }
            else if (j.number == WindowGlobal::gamePad.rightThumbX)
            {
                const float x = ProcessGamePadStickValue( j.value, WindowGlobal::gamePad.deadZone );
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = WindowEventType::GamePadRightThumbState;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].gamePadThumbX = x;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].gamePadThumbY = WindowGlobal::lastRightThumbY;
                WindowGlobal::lastRightThumbX = x;
            }
            else if (j.number == WindowGlobal::gamePad.rightThumbY)
            {
                const float y = ProcessGamePadStickValue( j.value, WindowGlobal::gamePad.deadZone );
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = WindowEventType::GamePadRightThumbState;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].gamePadThumbX = WindowGlobal::lastRightThumbX;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].gamePadThumbY = -y;
                WindowGlobal::lastRightThumbY = -y;
            }
            else if (j.number == WindowGlobal::gamePad.dpadXaxis)
            {
                if (j.value > 0)
                {
                    WindowGlobal::IncEventIndex();
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = WindowEventType::GamePadButtonDPadRight;
                }
                if (j.value < 0)
                {
                    WindowGlobal::IncEventIndex();
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = WindowEventType::GamePadButtonDPadLeft;
                }
            }
            else if (j.number == WindowGlobal::gamePad.dpadYaxis)
            {
                if (j.value < 0)
                {
                    WindowGlobal::IncEventIndex();
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = WindowEventType::GamePadButtonDPadUp;
                }
                if (j.value > 0)
                {
                    WindowGlobal::IncEventIndex();
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = WindowEventType::GamePadButtonDPadDown;
                }
            }
        }
    }
}

bool ae3d::Window::IsKeyDown( KeyCode keyCode )
{
    return WindowGlobal::keyDown[ (int)keyCode ];
}

void ae3d::Window::SwapBuffers()
{
    GfxDevice::Present();
}

bool ae3d::Window::PollEvent( WindowEvent& outEvent )
{
    if (WindowGlobal::eventIndex == -1)
    {
        return false;
    }
    
    outEvent = WindowGlobal::eventStack[ WindowGlobal::eventIndex ];
    --WindowGlobal::eventIndex;
    return true;
}

