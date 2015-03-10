#include "Window.hpp"
#include <iostream>
//#define class class_name
#include <xcb/xcb_icccm.h>
//#undef class
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <cstring> // for strlen

// Based on https://github.com/nxsy/xcb_handmade/blob/master/src/xcb_handmade.cpp
// Reference to setting up OpenGL in XCB: http://xcb.freedesktop.org/opengl/

namespace WindowGlobal
{
    bool isOpen = false;
    const int eventStackSize = 10;
    ae3d::WindowEvent eventStack[eventStackSize];
    int eventIndex = -1;
    Display* display;
    xcb_connection_t *connection = nullptr;
    xcb_key_symbols_t *key_symbols;
}

int setup_and_run(Display* display, xcb_connection_t *connection, int default_screen, xcb_screen_t *screen)
{
    int visualID = 0;
    
    GLXFBConfig *fb_configs = 0;
    int num_fb_configs = 0;
    fb_configs = glXGetFBConfigs(display, default_screen, &num_fb_configs);
    if(!fb_configs || num_fb_configs == 0)
    {
        fprintf(stderr, "glXGetFBConfigs failed\n");
        return -1;
    }
    
    /* Select first framebuffer config and query visualID */
    GLXFBConfig fb_config = fb_configs[0];
    glXGetFBConfigAttrib(display, fb_config, GLX_VISUAL_ID , &visualID);
    
    GLXContext context = glXCreateNewContext(display, fb_config, GLX_RGBA_TYPE, 0, True);
    if(!context)
    {
        fprintf(stderr, "glXCreateNewContext failed\n");
        return -1;
    }
    
    /* Create XID's for colormap and window */
    xcb_colormap_t colormap = xcb_generate_id(connection);
    xcb_window_t window = xcb_generate_id(connection);
    
    /* Create colormap */
    xcb_create_colormap(
                        connection,
                        XCB_COLORMAP_ALLOC_NONE,
                        colormap,
                        screen->root,
                        visualID
                        );
    
    /* Create window */
    uint32_t eventmask = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS;
    uint32_t valuelist[] = { eventmask, colormap, 0 };
    uint32_t valuemask = XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
    
    xcb_create_window(
                      connection,
                      XCB_COPY_FROM_PARENT,
                      window,
                      screen->root,
                      0, 0,
                      150, 150,
                      0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      visualID,
                      valuemask,
                      valuelist
                      );
    
    
    // NOTE: window must be mapped before glXMakeContextCurrent
    xcb_map_window(connection, window); 
    
    /* Create GLX Window */
    GLXDrawable drawable = 0;
    
    GLXWindow glxwindow = 
            glXCreateWindow(
                display,
                fb_config,
                window,
                0
                );

    if(!window)
    {
        xcb_destroy_window(connection, window);
        glXDestroyContext(display, context);
        
        fprintf(stderr, "glXDestroyContext failed\n");
        return -1;
    }
    
    drawable = glxwindow;
    
    if(!glXMakeContextCurrent(display, drawable, drawable, context))
    {
        xcb_destroy_window(connection, window);
        glXDestroyContext(display, context);
        
        std::cerr << "glXMakeContextCurrent failed" << std::endl;
        return -1;
    }
    
    return 0;
}

namespace ae3d
{
    bool Window::IsOpen()
    {
        return WindowGlobal::isOpen;
    }

    void Window::Create(int width, int height)
    {
        int default_screen;

        /* Open Xlib Display */ 
        WindowGlobal::display = XOpenDisplay(0);
        if(!WindowGlobal::display)
        {
            std::cerr << "Can't open display" << std::endl;
            return;
        }

        default_screen = DefaultScreen(WindowGlobal::display);

        WindowGlobal::connection = XGetXCBConnection(WindowGlobal::display);
        if(!WindowGlobal::connection)
        {
            XCloseDisplay(WindowGlobal::display);
            std::cerr << "Can't get xcb connection from display" << std::endl;
            return;
        }

        /* Acquire event queue ownership */
        XSetEventQueueOwner(WindowGlobal::display, XCBOwnsEventQueue);

        /* Find XCB screen */
        xcb_screen_t *screen = 0;
        xcb_screen_iterator_t screen_iter = 
            xcb_setup_roots_iterator(xcb_get_setup(WindowGlobal::connection));
        for(int screen_num = default_screen;
            screen_iter.rem && screen_num > 0;
            --screen_num, xcb_screen_next(&screen_iter));
        screen = screen_iter.data;
        
        WindowGlobal::key_symbols = xcb_key_symbols_alloc( WindowGlobal::connection );

        /* Initialize window and OpenGL context, run main loop and deinitialize */  
        int retval = setup_and_run(WindowGlobal::display, WindowGlobal::connection, default_screen, screen);

        /* run main loop */
        //int retval = main_loop(display, connection, window, drawable);

        /* Cleanup */
        //glXDestroyWindow(display, glxwindow);

        //xcb_destroy_window(connection, window);

        //glXDestroyContext(display, context);

        //XCloseDisplay(display);
        WindowGlobal::isOpen = true;
    }

    void Window::PumpEvents()
    {
        xcb_generic_event_t *event;
        while ((event = xcb_poll_for_event(WindowGlobal::connection)))
            {
                // NOTE(nbm): The high-order bit of response_type is whether the event
                // is synthetic. I'm not sure I care, but let's grab it in case.
                bool synthetic_event = (event->response_type & 0x80) != 0;
                uint8_t response_type = event->response_type & ~0x80;
                switch(response_type)
                    {
                    case XCB_KEY_PRESS:
                    case XCB_KEY_RELEASE:
                        {
                            xcb_key_press_event_t *e = (xcb_key_press_event_t *)event;
                            bool is_down = (response_type == XCB_KEY_PRESS);
                            xcb_keysym_t keysym = xcb_key_symbols_get_keysym(WindowGlobal::key_symbols, e->detail, 0);
                            if (keysym == XK_w)
                                {
                                    std::cout << "pressed w" << std::endl;
                                }
                            if (keysym == XK_Up)
                                {
                                    std::cout << "pressed up" << std::endl;
                                }
                        }
                    case XCB_MOTION_NOTIFY:
                        {
                            xcb_motion_notify_event_t* e = (xcb_motion_notify_event_t*)event;
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
                            //glXSwapBuffers(WindowGlobal::display, drawable);
                        }
                        break;
                    }
            }
    }

    bool Window::PollEvent( WindowEvent& outEvent )
    {
        if (WindowGlobal::eventIndex == -1)
        {
            return false;
        }
        
        outEvent = WindowGlobal::eventStack[ WindowGlobal::eventIndex ];
        --WindowGlobal::eventIndex;
        return true;
    }
}
