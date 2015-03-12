#include "Window.hpp"
#include "System.hpp"

int main()
{
    ae3d::System::EnableWindowsMemleakDetection();
    ae3d::Window::Instance().Create( 640, 480, ae3d::WindowCreateFlags::Empty );
    
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
            
            if (event.type == ae3d::WindowEventType::KeyDown)
            {
                int keyCode = event.keyCode;

                const int escape = 27;

                if (keyCode == escape)
                {
                    quit = true;
                }
            }
        }
    }
}
