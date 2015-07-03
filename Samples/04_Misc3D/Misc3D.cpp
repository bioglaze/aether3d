#include <string>
#include "AudioClip.hpp"
#include "AudioSourceComponent.hpp"
#include "Font.hpp"
#include "CameraComponent.hpp"
#include "MeshRendererComponent.hpp"
#include "SpriteRendererComponent.hpp"
#include "TextRendererComponent.hpp"
#include "TransformComponent.hpp"
#include "FileSystem.hpp"
#include "GameObject.hpp"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "System.hpp"
#include "RenderTexture.hpp"
#include "Texture2D.hpp"
#include "TextureCube.hpp"
#include "Vec3.hpp"
#include "Window.hpp"

using namespace ae3d;

int main()
{
    const int width = 640;
    const int height = 480;
    
    System::EnableWindowsMemleakDetection();
    Window::Create( width, height, WindowCreateFlags::Empty );
    System::LoadBuiltinAssets();
    System::InitAudio();
    //System::InitGamePad();

    GameObject camera;
    camera.AddComponent<CameraComponent>();
    camera.GetComponent<CameraComponent>()->SetClearColor( Vec3( 1, 0, 0 ) );
    camera.GetComponent<CameraComponent>()->SetProjection( 45, (float)width / (float)height, 1, 400 );
    camera.AddComponent<TransformComponent>();
    //perspCamera.GetComponent<TransformComponent>()->SetLocalPosition( Vec3( 0, 0, 0 ) );
    camera.GetComponent<TransformComponent>()->LookAt( {0, 0, 0 }, { 0, 0, -100 }, { 0, 1, 0 } );

    GameObject cube;
    Mesh cubeMesh;
    cubeMesh.Load( FileSystem::FileContents( "shuttle.ae3d" ) );
    cube.AddComponent< MeshRendererComponent >();
    cube.GetComponent< MeshRendererComponent >()->SetMesh( &cubeMesh );
    cube.AddComponent< TransformComponent >();
    cube.GetComponent< TransformComponent >()->SetLocalPosition( Vec3( 0, 0, -100 ) );

    Scene scene;
    
    TextureCube skybox;
    skybox.Load( FileSystem::FileContents( "skybox/left.jpg" ), FileSystem::FileContents( "skybox/right.jpg" ),
                 FileSystem::FileContents( "skybox/bottom.jpg" ), FileSystem::FileContents( "skybox/top.jpg" ),
                 FileSystem::FileContents( "skybox/front.jpg" ), FileSystem::FileContents( "skybox/back.jpg" ),
                 TextureWrap::Clamp, TextureFilter::Linear, Mipmaps::None );

    scene.SetSkybox( &skybox );
    scene.Add( &camera );
    scene.Add( &cube );

    bool quit = false;
    
    int lastMouseX = 0;
    int lastMouseY = 0;
    
    while (Window::IsOpen() && !quit)
    {
        Window::PumpEvents();
        WindowEvent event;
        
        std::string text( "draw calls:" );
        
        while (Window::PollEvent( event ))
        {
            if (event.type == WindowEventType::Close)
            {
                quit = true;
            }
            else if (event.type == WindowEventType::KeyDown ||
                event.type == WindowEventType::KeyUp)
            {
                KeyCode keyCode = event.keyCode;
                
                const float velocity = 1.5f;
                
                if (keyCode == KeyCode::Escape)
                {
                    quit = true;
                }
                else if (keyCode == KeyCode::R)
                {
                    System::ReloadChangedAssets();
                }
                else if (keyCode == KeyCode::W)
                {
                    camera.GetComponent<TransformComponent>()->MoveForward( -velocity );
                }
                else if (keyCode == KeyCode::S)
                {
                    camera.GetComponent<TransformComponent>()->MoveForward( velocity );
                }
                else if (keyCode == KeyCode::E)
                {
                    camera.GetComponent<TransformComponent>()->MoveUp( velocity );
                }
                else if (keyCode == KeyCode::Q)
                {
                    camera.GetComponent<TransformComponent>()->MoveUp( -velocity );
                }
                else if (keyCode == KeyCode::A)
                {
                    camera.GetComponent<TransformComponent>()->MoveRight( -velocity );
                }
                else if (keyCode == KeyCode::D)
                {
                    camera.GetComponent<TransformComponent>()->MoveRight( velocity );
                }
                else if (keyCode == KeyCode::Left)
                {
                    camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 0, 1, 0 ), 1 );
                }
                else if (keyCode == KeyCode::Right)
                {
                    camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 0, 1, 0 ), -1 );
                }
                else if (keyCode == KeyCode::Up)
                {
                    camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 1, 0, 0 ), 1 );
                }
                else if (keyCode == KeyCode::Down)
                {
                    camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 1, 0, 0 ), -1 );
                }
            }
            if (event.type == WindowEventType::GamePadButtonA)
            {
                text = "button a down";
            }
            if (event.type == WindowEventType::MouseMove)
            {
                const int mouseDeltaX = event.mouseX - lastMouseX;
                const int mouseDeltaY = event.mouseY - lastMouseY;
                lastMouseX = event.mouseX;
                lastMouseY = event.mouseY;
                
                camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 0, 1, 0 ), -float( mouseDeltaX ) / 20 );
                camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 1, 0, 0 ), float( mouseDeltaY ) / 20 );
            }
        }
        
        scene.Render();
        Window::SwapBuffers();
    }

    System::Deinit();
}
