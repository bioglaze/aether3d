#include "AudioClip.hpp"
#include "AudioSourceComponent.hpp"
#include "CameraComponent.hpp"
#include "SpriteRendererComponent.hpp"
#include "TransformComponent.hpp"
#include "GameObject.hpp"
#include "Scene.hpp"
#include "System.hpp"
#include "Vec3.hpp"
#include "Window.hpp"
#include "Texture2D.hpp"

using namespace ae3d;

int main()
{
    const int width = 640;
    const int height = 480;
    
    System::EnableWindowsMemleakDetection();
    Window::Instance().Create( width, height, WindowCreateFlags::Empty );
    System::LoadBuiltinAssets();
    System::InitAudio();
    
    GameObject camera;
    camera.AddComponent<CameraComponent>();
    camera.GetComponent<CameraComponent>()->SetProjection( 0, (float)width, (float)height, 0, 0, 1 );
    camera.GetComponent<CameraComponent>()->SetClearColor( Vec3( 0, 1, 0 ) );

    Texture2D spriteTex;
    spriteTex.Load( System::FileContents( "glider.png" ), TextureWrap::Repeat, TextureFilter::Nearest );

    Texture2D spriteTex2;
    spriteTex2.Load( System::FileContents( "test_dxt1.dds" ), TextureWrap::Repeat, TextureFilter::Nearest );

    Texture2D spriteTexFromAtlas;
    spriteTexFromAtlas.LoadFromAtlas( System::FileContents( "atlas_cegui.png" ), System::FileContents( "atlas_cegui.xml" ), "granite", TextureWrap::Repeat, TextureFilter::Nearest );

    GameObject spriteRenderer;
    spriteRenderer.AddComponent<SpriteRendererComponent>();
    auto sprite = spriteRenderer.GetComponent<SpriteRendererComponent>();
    sprite->SetTexture( &spriteTex, Vec3( 320, 0, -0.6f ), Vec3( (float)spriteTex.GetWidth(), (float)spriteTex.GetHeight(), 1 ), Vec4( 1, 0.5f, 0.5f, 1 ) );
    sprite->SetTexture( &spriteTex, Vec3( 340, 80, -0.5f ), Vec3( (float)spriteTex.GetWidth()/2, (float)spriteTex.GetHeight()/2, 1 ), Vec4( 0.5f, 1, 0.5f, 1 ) );
    sprite->SetTexture( &spriteTex2, Vec3( 280, 60, -0.4f ), Vec3( (float)spriteTex.GetWidth(), (float)spriteTex.GetHeight(), 1 ), Vec4( 1, 1, 1, 0.5f ) );
    sprite->SetTexture( &spriteTexFromAtlas, Vec3( 260, 160, -0.4f ), Vec3( (float)spriteTexFromAtlas.GetWidth(), (float)spriteTexFromAtlas.GetHeight(), 1 ), Vec4( 1, 1, 1, 1 ) );
    spriteRenderer.AddComponent<SpriteRendererComponent>();

    spriteRenderer.AddComponent<TransformComponent>();
    spriteRenderer.GetComponent<TransformComponent>()->SetLocalPosition( Vec3( 20, 0, 0 ) );

    AudioClip audioClip;
    //audioClip.Load( System::FileContents( "explosion.wav" ) );
    audioClip.Load( System::FileContents( "thingy.ogg" ) );
    
    GameObject audioContainer;
    audioContainer.AddComponent<AudioSourceComponent>();
    audioContainer.GetComponent<AudioSourceComponent>()->SetClip( &audioClip );
    audioContainer.GetComponent<AudioSourceComponent>()->Play();
    
    Scene scene;
    scene.Add( &camera );
    scene.Add( &spriteRenderer );

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
            
            if (event.type == WindowEventType::KeyDown ||
                event.type == WindowEventType::KeyUp)
            {
                KeyCode keyCode = event.keyCode;

                if (keyCode == KeyCode::Escape)
                {
                    quit = true;
                }
                if (keyCode == KeyCode::A)
                {
                    System::ReloadChangedAssets();
                }
            }
        }

        scene.Render();

        Window::Instance().SwapBuffers();
    }

    System::Deinit();
}
