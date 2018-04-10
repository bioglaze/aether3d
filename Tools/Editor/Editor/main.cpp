#include "SceneView.hpp"
#include "System.hpp"
#include "Window.hpp"

using namespace ae3d;

int main()
{
    int width = 640;
    int height = 480;
    
    System::EnableWindowsMemleakDetection();
    Window::Create( width, height, WindowCreateFlags::Empty );
    Window::GetSize( width, height );
    System::LoadBuiltinAssets();

    bool quit = false;   
    double x = 0, y = 0;
    
    int winWidth, winHeight;
    Window::GetSize( winWidth, winHeight );
    System::Print( "window size: %dx%d\n", winWidth, winHeight );

    SceneView sceneView;
    sceneView.Init();

    while (Window::IsOpen() && !quit)
    {
        Window::PumpEvents();
        WindowEvent event;

        //nk_input_begin( &ctx );

        while (Window::PollEvent( event ))
        {
            if (event.type == WindowEventType::Close)
            {
                quit = true;
            }
            
            if (event.type == WindowEventType::KeyDown ||
                event.type == WindowEventType::KeyUp)
            {
                KeyCode keyCode = event.keyCode;

                if (keyCode == KeyCode::Escape)
                {
                    quit = true;
                }
                /*else if (keyCode == KeyCode::A)
                {
                    nk_input_char( &ctx, 'a' );
                }
                else if (keyCode == KeyCode::K)
                {
                    nk_input_char( &ctx, 'k' );
                }
                else if (keyCode == KeyCode::Left)
                {
                    nk_input_key( &ctx, NK_KEY_LEFT, event.type == WindowEventType::KeyDown );
                }
                else if (keyCode == KeyCode::Right)
                {
                    nk_input_key( &ctx, NK_KEY_RIGHT, event.type == WindowEventType::KeyDown );
                    }*/
            }

            if (event.type == WindowEventType::MouseMove)
            {
                x = event.mouseX;
                y = height - event.mouseY;
                //std::cout << "mouse position: " << x << ", y: " << y << std::endl;
                //nk_input_motion( &ctx, (int)x, (int)y );
            }

            if (event.type == WindowEventType::Mouse1Down)
            {
                x = event.mouseX;
                y = height - event.mouseY;
                //nk_input_button( &ctx, NK_BUTTON_LEFT, (int)x, (int)y, 1 );
            }

            if (event.type == WindowEventType::Mouse1Up)
            {
                x = event.mouseX;
                y = height - event.mouseY;
                //nk_input_button( &ctx, NK_BUTTON_LEFT, (int)x, (int)y, 0 );
            }
            
            //nk_input_button( &ctx, NK_BUTTON_RIGHT, (int)x, (int)y, (event.type == WindowEventType::Mouse2Down) ? 1 : 0 );
        }

        //nk_input_end( &ctx );

        sceneView.Render();

        Window::SwapBuffers();
    }

    System::Deinit();

    return 0;
}
