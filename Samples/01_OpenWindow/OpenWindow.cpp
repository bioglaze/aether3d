#include "Window.hpp"
#include "System.hpp"
#include "GameObject.hpp"
#include "CameraComponent.hpp"
#include "Vec3.hpp"

using namespace ae3d;

int main()
{
    System::EnableWindowsMemleakDetection();
    Window::Instance().Create( 640, 480, WindowCreateFlags::Empty );
    
    GameObject camera;
    camera.AddComponent<ae3d::CameraComponent>();
    //camera.GetComponent<TransformComponent>().SetPosition(Vec3(20, 0, 0)); */
    camera.GetComponent<CameraComponent>()->SetProjection(0, 640, 0, 480, 0, 1);
    camera.GetComponent<CameraComponent>()->SetClearColor( Vec3( 0, 1, 0 ) );

    //Scene scene;
    //scene.Add( camera );
    
    bool quit = false;
    
    while (Window::Instance().IsOpen() && !quit)
    {
        Window::Instance().PumpEvents();
        WindowEvent event;

        while (Window::Instance().PollEvent( event ))
        {
            if (event.type == WindowEventType::Close)
            {
                quit = true;
            }
            
            if (event.type == WindowEventType::KeyDown)
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
