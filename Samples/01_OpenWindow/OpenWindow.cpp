#include "Window.hpp"
#include "System.hpp"
#include "GameObject.hpp"
#include "CameraComponent.hpp"

int main()
{
    ae3d::System::EnableWindowsMemleakDetection();
    ae3d::Window::Instance().Create( 640, 480, ae3d::WindowCreateFlags::Empty );
    
    ae3d::GameObject camera;
    camera.AddComponent<ae3d::CameraComponent>();
    //camera.GetComponent<TransformComponent>().SetPosition(Vec3(20, 0, 0)); */
    camera.GetComponent<ae3d::CameraComponent>()->SetProjection(0, 640, 0, 480, 0, 1);
    
    //Scene scene;
    //scene.Add( camera );
    
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
        
        //ae3d::Window::Instance().GetRenderer().Render( scene );
    }
}
