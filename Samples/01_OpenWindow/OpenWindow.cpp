#include "Window.hpp"
#include "System.hpp"

int main()
{
    ae3d::System::EnableWindowsMemleakDetection();
    ae3d::Window::Instance().Create( 640, 480, ae3d::WindowCreateFlags::Empty );
    
    //GameObject camera;
    //camera.AddComponent<CameraComponent>();
    //camera.GetComponent<CameraComponent>().SetProjection( 0, 0, 640, 480, 0, 1 );
    
    /*GameObject sprite;
    sprite.AddComponent<MeshComponent>( ae3d::BuiltinMesh::Quad );
    sprite.AddComponent<MeshRendererComponent>();
    sprite.GetComponent<TransformComponent>().SetPosition( Vec3( 20, 0, 0 ) );*/
    
    //Scene scene;
    //scene.Add( camera );
    //scene.Add( sprite );
    
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
