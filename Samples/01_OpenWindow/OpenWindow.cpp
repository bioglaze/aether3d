#include "CameraComponent.hpp"
#include "SpriteRendererComponent.hpp"
#include "GameObject.hpp"
#include "Scene.hpp"
#include "System.hpp"
#include "Vec3.hpp"
#include "Window.hpp"
#include "Texture2D.hpp"

using namespace ae3d;

int main()
{
    System::EnableWindowsMemleakDetection();
    Window::Instance().Create( 640, 480, WindowCreateFlags::Empty );

    GameObject camera;
    camera.AddComponent<CameraComponent>();
    //camera.GetComponent<TransformComponent>().SetPosition(Vec3(20, 0, 0)); */
    camera.GetComponent<CameraComponent>()->SetProjection( 0, 640, 0, 480, 0, 1 );
    camera.GetComponent<CameraComponent>()->SetClearColor( Vec3( 0, 1, 0 ) );

    Texture2D spriteTex;
    spriteTex.Load(System::FileContents("glider.png"));

    GameObject sprite;
    sprite.AddComponent<SpriteRendererComponent>();
    sprite.GetComponent<SpriteRendererComponent>()->SetTexture( &spriteTex );

    Scene scene;
    scene.Add( &camera );
    scene.Add( &sprite );

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

        scene.Update();
        scene.Render();

        Window::Instance().SwapBuffers();
    }

    System::Deinit();
}
