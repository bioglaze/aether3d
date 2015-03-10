#include "Window.hpp"

int main()
{
    ae3d::Window::Instance().Create( 640, 400 );

    bool quit = false;
    
    while (ae3d::Window::Instance().IsOpen() && !quit)
    {
        ae3d::Window::Instance().PumpEvents();
        ae3d::WindowEvent event;

        while (ae3d::Window::Instance().PollEvent( event ))
        {
            if (event.type == ae3d::WindowEventType::Close)
            {
                quit = true;
            }
        }
    }
}
