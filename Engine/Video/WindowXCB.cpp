#include "Window.hpp"
#include <iostream>
#include <map>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <GL/glxw.h>
#include <GL/glx.h>

// Based on https://github.com/nxsy/xcb_handmade/blob/master/src/xcb_handmade.cpp
// Reference to setting up OpenGL in XCB: http://xcb.freedesktop.org/opengl/
// Event tutorial: http://xcb.freedesktop.org/tutorial/events/

void PlatformInitGamePad()
{

}

namespace WindowGlobal
{
    bool isOpen = false;
    const int eventStackSize = 10;
    ae3d::WindowEvent eventStack[eventStackSize];
    int eventIndex = -1;
    Display* display;
    xcb_connection_t *connection = nullptr;
    xcb_key_symbols_t *key_symbols;
    GLXDrawable drawable = 0;
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

static int setup_and_run( Display* display, xcb_connection_t* connection, int default_screen, xcb_screen_t* screen, int width, int height )
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
    
    GLXContext context = glXCreateNewContext( display, fb_config, GLX_RGBA_TYPE, 0, True );
    
    if (!context)
    {
        std::cerr << "glXCreateNewContext failed." << std::endl;
        return -1;
    }
    
    /* Create XID's for colormap and window */
    xcb_colormap_t colormap = xcb_generate_id( connection );
    xcb_window_t window = xcb_generate_id( connection );
    
    xcb_create_colormap(
                        connection,
                        XCB_COLORMAP_ALLOC_NONE,
                        colormap,
                        screen->root,
                        visualID
                        );
    
    const uint32_t eventmask = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
                               XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE;
    const uint32_t valuelist[] = { eventmask, colormap, 0 };
    const uint32_t valuemask = XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
    
    xcb_create_window(
                      connection,
                      XCB_COPY_FROM_PARENT,
                      window,
                      screen->root,
                      0, 0,
                      width, height,
                      0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      visualID,
                      valuemask,
                      valuelist
                      );
    
    xcb_map_window( connection, window );
    
    GLXWindow glxwindow = glXCreateWindow( display, fb_config, window, 0 );

    if (!window)
    {
        xcb_destroy_window( connection, window );
        glXDestroyContext( display, context );
        
        std::cerr << "glXDestroyContext failed" << std::endl;
        return -1;
    }
    
    WindowGlobal::drawable = glxwindow;
    
    if (!glXMakeContextCurrent( display, WindowGlobal::drawable, WindowGlobal::drawable, context ))
    {
        xcb_destroy_window( connection, window );
        glXDestroyContext( display, context );
        
        std::cerr << "glXMakeContextCurrent failed" << std::endl;
        return -1;
    }

    if (glxwInit() != 0)
    {
        std::cerr << "Could not init GLXW!" << std::endl;
    }
    
    return 0;
}

bool ae3d::Window::IsOpen()
{
    return WindowGlobal::isOpen;
}

void ae3d::Window::Create( int width, int height, WindowCreateFlags flags )
{
    WindowGlobal::display = XOpenDisplay( 0 );

    if (!WindowGlobal::display)
    {
        std::cerr << "Can't open display" << std::endl;
        return;
    }
    
    int default_screen = DefaultScreen( WindowGlobal::display );
    
    WindowGlobal::connection = XGetXCBConnection( WindowGlobal::display );

    if (!WindowGlobal::connection)
    {
        XCloseDisplay( WindowGlobal::display );
        std::cerr << "Can't get xcb connection from display" << std::endl;
        return;
    }
    
    XSetEventQueueOwner( WindowGlobal::display, XCBOwnsEventQueue );

    /* Find XCB screen */
    xcb_screen_t *screen = 0;
    xcb_screen_iterator_t screen_iter = xcb_setup_roots_iterator( xcb_get_setup( WindowGlobal::connection ) );
    for (int screen_num = default_screen; screen_iter.rem && screen_num > 0; --screen_num, xcb_screen_next( &screen_iter ));
    screen = screen_iter.data;
    
    WindowGlobal::key_symbols = xcb_key_symbols_alloc( WindowGlobal::connection );
    
    /*int retval =*/ setup_and_run( WindowGlobal::display, WindowGlobal::connection, default_screen, screen, width, height );
    
    /* Cleanup */
    //glXDestroyWindow(display, glxwindow);
    //xcb_destroy_window(connection, window);
    //glXDestroyContext(display, context);    
    //XCloseDisplay(display);
    WindowGlobal::isOpen = true;
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
                const xcb_query_pointer_reply_t* pointer = xcb_query_pointer_reply(WindowGlobal::connection, xcb_query_pointer(WindowGlobal::connection, XDefaultRootWindow(WindowGlobal::display)), NULL);
                const bool newb1 = (pointer->mask & XCB_BUTTON_MASK_1) != 0;
                const bool newb2 = (pointer->mask & XCB_BUTTON_MASK_2) != 0;
                const bool newb3 = (pointer->mask & XCB_BUTTON_MASK_3) != 0;
                const xcb_button_press_event_t* be = (xcb_button_press_event_t*)event;

                const auto b1Type = response_type == XCB_EVENT_MASK_BUTTON_PRESS ? ae3d::WindowEventType::Mouse1Down : ae3d::WindowEventType::Mouse1Up;
                const auto b2Type = response_type == XCB_EVENT_MASK_BUTTON_PRESS ? ae3d::WindowEventType::Mouse2Down : ae3d::WindowEventType::Mouse2Up;
                const auto b3Type = response_type == XCB_EVENT_MASK_BUTTON_PRESS ? ae3d::WindowEventType::MouseMiddleDown : ae3d::WindowEventType::MouseMiddleUp;

                ++WindowGlobal::eventIndex;
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

                ++WindowGlobal::eventIndex;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = type;
                const xcb_keysym_t keysym = xcb_key_symbols_get_keysym(WindowGlobal::key_symbols, e->detail, 0);
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].keyCode = WindowGlobal::keyMap[ keysym ];
            }
            case XCB_MOTION_NOTIFY:
            {
                //xcb_motion_notify_event_t* e = (xcb_motion_notify_event_t*)event;
                //std::cout << "mouse x: " << e->event_x;
                //std::cout << "mouse y: " << e->event_y;
                break;
            }
            case XCB_CLIENT_MESSAGE:
            {
                /*xcb_client_message_event_t* client_message_event = (xcb_client_message_event_t*)event;
                 if (client_message_event->type == context.wm_protocols)
                 {
                 if (client_message_event->data.data32[0] == context.wm_delete_window)
                 {
                 std::cout << "Quitting" << std::endl;
                 exit(1);
                 break;
                 }
                 }*/
                break;
            }
            case XCB_EXPOSE:
            {
            }
            break;
        }
    }
}

void ae3d::Window::SwapBuffers() const
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

