#include "Window.hpp"
#include <iostream>
#include <map>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_ewmh.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <GL/glxw.h>
#include <GL/glx.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <dirent.h>
#include <cstring>
#include "GfxDevice.hpp"

// Based on https://github.com/nxsy/xcb_handmade/blob/master/src/xcb_handmade.cpp
// Reference to setting up OpenGL in XCB: http://xcb.freedesktop.org/opengl/
// Event tutorial: http://xcb.freedesktop.org/tutorial/events/

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

namespace
{
    // TODO: DRY. Duplicated in WindowWin32.cpp
    float ProcessGamePadStickValue( short value, short deadZoneThreshold )
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
}

namespace WindowGlobal
{
    bool isOpen = false;
    const int eventStackSize = 15;
    ae3d::WindowEvent eventStack[eventStackSize];
    int eventIndex = -1;
    
    void IncEventIndex()
    {
        if (eventIndex < eventStackSize - 1)
        {
            ++eventIndex;
        }
    }
    
    Display* display;
    xcb_connection_t *connection = nullptr;
    xcb_window_t window;
    xcb_key_symbols_t *key_symbols;
    xcb_atom_t wm_protocols;
    xcb_atom_t wm_delete_window;

    // Testing fullscreen.
    xcb_ewmh_connection_t EWMH;
    xcb_intern_atom_cookie_t* EWMHCookie = nullptr;
    
    int windowWidth = 640;
    int windowHeight = 480;
    GLXDrawable drawable = 0;
    GamePad gamePad;
    float lastLeftThumbX = 0;
    float lastLeftThumbY = 0;
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
        { XK_space, ae3d::KeyCode::Space }
    };
}

void PlatformInitGamePad()
{
    DIR* dir = opendir( "/dev/input" );
    dirent entry;
    dirent* result;

    while (!readdir_r( dir, &entry, &result ) && result)
    {
        if ((entry.d_name[0] == 'j') && (entry.d_name[1] == 's'))
        {
            char full_device_path[ 260 ];
            snprintf( full_device_path, sizeof( full_device_path ), "%s/%s", "/dev/input", entry.d_name );
            int fd = open( full_device_path, O_RDONLY );

            if (fd < 0)
            {
                // Permissions could cause this code path.
                //std::cerr << "Unable to open joystick file" << std::endl;
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
            //std::cout << "d_name: " << std::string( entry.d_name ) << ", name " << std::string( name ) << ", version " << version << ", axes " << axes << ", buttons " << buttons << std::endl; 
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

static int CreateWindowAndContext( Display* display, xcb_connection_t* connection, int default_screen, xcb_screen_t* screen, int width, int height, ae3d::WindowCreateFlags flags )
{
    GLXFBConfig* fb_configs = nullptr;
    int num_fb_configs = 0;
    fb_configs = glXGetFBConfigs( display, default_screen, &num_fb_configs );
    
    if (!fb_configs || num_fb_configs == 0)
    {
        std::cerr << "glXGetFBConfigs failed." << std::endl;
        return -1;
    }
    
    /* Select first framebuffer config and query visualID */
    int visualID = 0;
    GLXFBConfig fb_config = fb_configs[ 0 ];
    glXGetFBConfigAttrib( display, fb_config, GLX_VISUAL_ID, &visualID );
    
    GLXContext context = glXCreateNewContext( display, fb_config, GLX_RGBA_TYPE, nullptr, True );
    
    if (!context)
    {
        std::cerr << "glXCreateNewContext failed." << std::endl;
        return -1;
    }
    
    /* Create XID's for colormap and window */
    xcb_colormap_t colormap = xcb_generate_id( connection );
    WindowGlobal::window = xcb_generate_id( connection );

    WindowGlobal::windowWidth = width == 0 ? screen->width_in_pixels : width;
    WindowGlobal::windowHeight = height == 0 ? screen->height_in_pixels : height;
    std::cout << "width " << WindowGlobal::windowWidth << ", height " << WindowGlobal::windowHeight << std::endl;
    
    xcb_create_colormap(
                        connection,
                        XCB_COLORMAP_ALLOC_NONE,
                        colormap,
                        screen->root,
                        visualID
                        );
    
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

    if ((flags & ae3d::WindowCreateFlags::Fullscreen) != 0)
    {
        WindowGlobal::EWMHCookie = xcb_ewmh_init_atoms( WindowGlobal::connection, &WindowGlobal::EWMH );

        if (!xcb_ewmh_init_atoms_replies( &WindowGlobal::EWMH, WindowGlobal::EWMHCookie, nullptr ))
        {
            std::cout << "Fullscreen not supported." << std::endl;
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
            std::cerr << "Full screen reply failed" << std::endl;
        }
    }
    //xcb_change_property( WindowGlobal::connection, XCB_PROP_MODE_REPLACE, WindowGlobal::window, WindowGlobal::EWMH._NET_WM_STATE, XCB_ATOM, 32, 1, &(WindowGlobal::EWMH._NET_WM_STATE_FULLSCREEN)); 
    // End test
    
    GLXWindow glxwindow = glXCreateWindow( display, fb_config, WindowGlobal::window, nullptr );

    if (!glxwindow)
    {
        xcb_destroy_window( connection, WindowGlobal::window );
        glXDestroyContext( display, context );
        
        std::cerr << "glXDestroyContext failed" << std::endl;
        return -1;
    }
    
    WindowGlobal::drawable = glxwindow;
    
    if (!glXMakeContextCurrent( display, WindowGlobal::drawable, WindowGlobal::drawable, context ))
    {
        xcb_destroy_window( connection, WindowGlobal::window );
        glXDestroyContext( display, context );
        
        std::cerr << "glXMakeContextCurrent failed" << std::endl;
        return -1;
    }

    return 0;
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
        std::cerr << "Can't open display" << std::endl;
        return;
    }
    
    int default_screen = DefaultScreen( WindowGlobal::display );

    WindowGlobal::connection = XGetXCBConnection( WindowGlobal::display );
    LoadAtoms();

    if (!WindowGlobal::connection)
    {
        XCloseDisplay( WindowGlobal::display );
        std::cerr << "Can't get xcb connection from display" << std::endl;
        return;
    }
    
    XSetEventQueueOwner( WindowGlobal::display, XCBOwnsEventQueue );

    xcb_screen_t* screen = nullptr;
    xcb_screen_iterator_t screen_iter = xcb_setup_roots_iterator( xcb_get_setup( WindowGlobal::connection ) );
    for (int screen_num = default_screen; screen_iter.rem && screen_num > 0; --screen_num, xcb_screen_next( &screen_iter ));
    screen = screen_iter.data;
    
    WindowGlobal::key_symbols = xcb_key_symbols_alloc( WindowGlobal::connection );
    
    if (CreateWindowAndContext( WindowGlobal::display, WindowGlobal::connection, default_screen, screen, width, height, flags ) == -1)
    {
        return;
    }
    
    GfxDevice::Init( WindowGlobal::windowWidth, WindowGlobal::windowHeight );
    
    const char *title = "Aether3D Game Engine";
    xcb_change_property(WindowGlobal::connection,
                        XCB_PROP_MODE_REPLACE,
                        WindowGlobal::window,
                        XCB_ATOM_WM_NAME,
                        XCB_ATOM_STRING,
                        8,
                        strlen (title),
                        title );
    WindowGlobal::isOpen = true;
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
        //const bool synthetic_event = (event->response_type & 0x80) != 0;
        const uint8_t response_type = event->response_type & ~0x80;

        switch (response_type)
        {
            case XCB_EVENT_MASK_BUTTON_PRESS:
            case XCB_EVENT_MASK_BUTTON_RELEASE:
            {
                const xcb_query_pointer_reply_t* pointer = xcb_query_pointer_reply( WindowGlobal::connection, xcb_query_pointer(WindowGlobal::connection, XDefaultRootWindow(WindowGlobal::display)), nullptr );
                const bool newb1 = (pointer->mask & XCB_BUTTON_MASK_1) != 0;
                const bool newb2 = (pointer->mask & XCB_BUTTON_MASK_2) != 0;
                const bool newb3 = (pointer->mask & XCB_BUTTON_MASK_3) != 0;
                const xcb_button_press_event_t* be = (xcb_button_press_event_t*)event;

                const auto b1Type = response_type == XCB_EVENT_MASK_BUTTON_PRESS ? ae3d::WindowEventType::Mouse1Down : ae3d::WindowEventType::Mouse1Up;
                const auto b2Type = response_type == XCB_EVENT_MASK_BUTTON_PRESS ? ae3d::WindowEventType::Mouse2Down : ae3d::WindowEventType::Mouse2Up;
                const auto b3Type = response_type == XCB_EVENT_MASK_BUTTON_PRESS ? ae3d::WindowEventType::MouseMiddleDown : ae3d::WindowEventType::MouseMiddleUp;

                WindowGlobal::IncEventIndex();
                
                if (newb1)
                {
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = b1Type;
                }
                else if (newb3)
                {
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = b2Type;
                }
                else if (newb2)
                {
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = b3Type;
                }
                else
                {
                    std::cerr << "Unhandled mouse button." << std::endl;
                    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = b1Type;
                }
                
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = be->event_x;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = be->event_y;
            }
            break;
            case XCB_KEY_PRESS:
            case XCB_KEY_RELEASE:
            {
                xcb_key_press_event_t *e = (xcb_key_press_event_t *)event;
                const bool isDown = (response_type == XCB_KEY_PRESS);
                const auto type = isDown ? ae3d::WindowEventType::KeyDown : ae3d::WindowEventType::KeyUp;

                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = type;
                const xcb_keysym_t keysym = xcb_key_symbols_get_keysym(WindowGlobal::key_symbols, e->detail, 0);
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].keyCode = WindowGlobal::keyMap[ keysym ];
            }
            case XCB_MOTION_NOTIFY:
            {
                xcb_motion_notify_event_t* e = (xcb_motion_notify_event_t*)event;
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::MouseMove;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = e->event_x;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = e->event_y;
                break;
            }
            case XCB_CLIENT_MESSAGE:
            {
                xcb_client_message_event_t* client_message_event = (xcb_client_message_event_t*)event;
                if (client_message_event->type == WindowGlobal::wm_protocols)
                {
                    if (client_message_event->data.data32[0] == WindowGlobal::wm_delete_window)
                    {
                        exit(1);
                        break;
                    }
                 }
                break;
            }
            case XCB_EXPOSE:
            {
            }
            break;
        }
    }

    if (!WindowGlobal::gamePad.isActive)
    {
        return;
    }

    js_event j;

    while (read( WindowGlobal::gamePad.fd, &j, sizeof(js_event) ) == sizeof(js_event))
        //read(WindowGlobal::gamePad.fd, &j, sizeof(js_event));
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
                //std::cout << "pressed button " << (int)j.number << std::endl;
            }
        }
        else if (j.type == JS_EVENT_AXIS)
        {
            if (j.number == WindowGlobal::gamePad.leftThumbX)
            {
                float x = ProcessGamePadStickValue( j.value, WindowGlobal::gamePad.deadZone );
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = WindowEventType::GamePadLeftThumbState;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].gamePadThumbX = x;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].gamePadThumbY = WindowGlobal::lastLeftThumbY;
                WindowGlobal::lastLeftThumbX = x;
            }
            else if (j.number == WindowGlobal::gamePad.leftThumbY)
            {
                float y = ProcessGamePadStickValue( j.value, WindowGlobal::gamePad.deadZone );
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = WindowEventType::GamePadLeftThumbState;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].gamePadThumbX = WindowGlobal::lastLeftThumbX;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].gamePadThumbY = y;
                WindowGlobal::lastLeftThumbY = y;
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

void ae3d::Window::SwapBuffers()
{
    glXSwapBuffers( WindowGlobal::display, WindowGlobal::drawable );
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

