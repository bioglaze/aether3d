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
    System::LoadBuiltinAssets();

    bool quit = false;   
    double x = 0, y = 0;

    SceneView sceneView;
    sceneView.Init( width, height );

    Vec3 moveDir;
    constexpr float velocity = 0.2f;
    
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
				System::Deinit();
				return 0;
            }
            
            if (event.type == WindowEventType::KeyDown ||
                event.type == WindowEventType::KeyUp)
            {
                KeyCode keyCode = event.keyCode;

                if (keyCode == KeyCode::Escape)
                {
                    quit = true;
                }
                else if (keyCode == KeyCode::A)
                {
                    moveDir.x = event.type == WindowEventType::KeyDown ? -velocity : 0;
                }
                else if (keyCode == KeyCode::D)
                {
                    moveDir.x = event.type == WindowEventType::KeyDown ? velocity : 0;
                }
                else if (keyCode == KeyCode::W)
                {
                    moveDir.z = event.type == WindowEventType::KeyDown ? -velocity : 0;
                }
                else if (keyCode == KeyCode::S)
                {
                    moveDir.z = event.type == WindowEventType::KeyDown ? velocity : 0;
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

        //sceneView.RotateCamera( -float( theEvent.deltaX ) / 20, -float( theEvent.deltaY ) / 20 );
        sceneView.MoveCamera( moveDir );
        sceneView.Render();

        Window::SwapBuffers();
    }

    System::Deinit();

    return 0;
}
