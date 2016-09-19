#include <string>
#include "Font.hpp"
#include "CameraComponent.hpp"
#include "SpriteRendererComponent.hpp"
#include "TextRendererComponent.hpp"
#include "TransformComponent.hpp"
#include "GameObject.hpp"
#include "Scene.hpp"
#include "System.hpp"
#include "FileSystem.hpp"
#include "Vec3.hpp"
#include "Window.hpp"
#include "Texture2D.hpp"
#include "Shader.hpp"

using namespace ae3d;

// Sample assets can be downloaded from here: http://twiren.kapsi.fi/files/aether3d_sample_v0.5.5.zip
// Extract them into aether3d_build that is generated next to aether3d folder.

int main()
{
    int width = 640;
    int height = 480;
    
    System::EnableWindowsMemleakDetection();
    Window::Create( width, height, WindowCreateFlags::Empty );
    Window::GetSize( width, height );
    System::LoadBuiltinAssets();

    GameObject camera;
    camera.AddComponent<CameraComponent>();
    camera.GetComponent<CameraComponent>()->SetProjection( 0, (float)width, (float)height, 0, 0, 1 );
    camera.GetComponent<CameraComponent>()->SetClearColor( Vec3( 1.0f, 0.5f, 0.5f ) );
    camera.AddComponent<TransformComponent>();
   
    Texture2D spriteTex;
    spriteTex.Load( FileSystem::FileContents( "glider.png" ), TextureWrap::Repeat, TextureFilter::Nearest, Mipmaps::None, ColorSpace::RGB, 1 );

    GameObject spriteContainer;
    spriteContainer.AddComponent<SpriteRendererComponent>();
    spriteContainer.AddComponent<TransformComponent>();
    auto sprite = spriteContainer.GetComponent<SpriteRendererComponent>();
    sprite->SetTexture( &spriteTex, Vec3( 320, 0, -0.6f ), Vec3( (float)spriteTex.GetWidth(), (float)spriteTex.GetHeight(), 1 ), Vec4( 1, 0.5f, 0.5f, 1 ) );
    
    Texture2D fontTex;
    fontTex.Load( FileSystem::FileContents( "font.png" ), TextureWrap::Clamp, TextureFilter::Linear, Mipmaps::None, ColorSpace::RGB, 1 );
    
    Font font;
    font.LoadBMFont( &fontTex, FileSystem::FileContents( "font_txt.fnt" ) );

    GameObject textContainer;
    textContainer.AddComponent<TextRendererComponent>();
    textContainer.GetComponent<TextRendererComponent>()->SetText( "Aether3D \nGame Engine" );
    textContainer.GetComponent<TextRendererComponent>()->SetFont( &font );
                                                                       
    Scene scene;
    
    bool quit = false;
	int frame = 0;

    while (Window::IsOpen() && !quit)
    {
        Window::PumpEvents();
        WindowEvent event;

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
                if (keyCode == KeyCode::A)
                {
                    System::ReloadChangedAssets();
                }
            }
        }

        // Tests renderer by rendering an empty frame or adding stuff while running
        if (frame == 1)
        {
            scene.Add( &camera );
        }
        else if (frame == 2)
        {
            scene.Add( &spriteContainer );
            scene.Add( &textContainer );
        }
        else if (frame == 3)
        {
            sprite->SetTexture( &spriteTex, Vec3( 420, 0, -0.6f ), Vec3( (float)spriteTex.GetWidth(), (float)spriteTex.GetHeight(), 1 ), Vec4( 1, 0.5f, 0.5f, 1 ) );
        }

        scene.Render();

        Window::SwapBuffers();

		++frame;
        //textContainer.GetComponent<TextRendererComponent>()->SetText( (frame % 5 == 0) ? "Aether3D \nGame Engine" : "Aether3D" );
        textContainer.GetComponent<TextRendererComponent>()->SetText( System::Statistics::GetStatistics().c_str() );
    }

    System::Deinit();
}
