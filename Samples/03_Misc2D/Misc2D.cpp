#include <string>
#include "AudioClip.hpp"
#include "AudioSourceComponent.hpp"
#include "Font.hpp"
#include "CameraComponent.hpp"
#include "SpriteRendererComponent.hpp"
#include "TextRendererComponent.hpp"
#include "TransformComponent.hpp"
#include "FileSystem.hpp"
#include "GameObject.hpp"
#include "Scene.hpp"
#include "System.hpp"
#include "RenderTexture.hpp"
#include "Texture2D.hpp"
#include "Vec3.hpp"
#include "Window.hpp"

using namespace ae3d;

// Sample assets can be downloaded from here:  http://twiren.kapsi.fi/files/aether3d_sample_v0.7.5.zip

int main()
{
    const int width = 640;
    const int height = 480;
    
    System::EnableWindowsMemleakDetection();
    Window::Create( width, height, WindowCreateFlags::Empty );
    System::LoadBuiltinAssets();
    System::InitAudio();
    System::InitGamePad();
    System::RunUnitTests();

    GameObject camera;
    camera.AddComponent<CameraComponent>();
    camera.GetComponent<CameraComponent>()->SetProjection( 0, (float)width, (float)height, 0, 0, 1 );
    camera.GetComponent<CameraComponent>()->SetClearColor( Vec3( 0.5f, 0.5f, 0.5f ) );
    camera.GetComponent<CameraComponent>()->SetClearFlag( CameraComponent::ClearFlag::DepthAndColor );
    //camera.GetComponent<CameraComponent>()->SetViewport( 0, 0, 200, 480 );
    camera.AddComponent<TransformComponent>();

    Texture2D spriteTex;
    spriteTex.Load( FileSystem::FileContents("glider.png"), TextureWrap::Repeat, TextureFilter::Nearest, Mipmaps::Generate, ColorSpace::RGB, Anisotropy::k1 );

    // This texture has 7 mipmaps.
    Texture2D bc1Tex;
    bc1Tex.Load( FileSystem::FileContents( "test_dxt1.dds" ), TextureWrap::Repeat, TextureFilter::Nearest, Mipmaps::None, ColorSpace::RGB, Anisotropy::k1 );

    // This texture has 1 mipmap.
    Texture2D bc2Tex;
    bc2Tex.Load( FileSystem::FileContents( "test_dxt3.dds" ), TextureWrap::Repeat, TextureFilter::Nearest, Mipmaps::Generate, ColorSpace::RGB, Anisotropy::k1 );

    // This texture has 1 mipmap.
    Texture2D bc3Tex;
    bc3Tex.Load( FileSystem::FileContents( "test_dxt5.dds" ), TextureWrap::Repeat, TextureFilter::Nearest, Mipmaps::None, ColorSpace::RGB, Anisotropy::k1 );

    Texture2D nonSquareTex;
    nonSquareTex.Load( FileSystem::FileContents( "textures/chain_texture.png" ), TextureWrap::Repeat, TextureFilter::Nearest, Mipmaps::Generate, ColorSpace::RGB, Anisotropy::k1 );

    Texture2D notLoadedTex;

    Texture2D spriteTexFromAtlas;
    spriteTexFromAtlas.LoadFromAtlas( FileSystem::FileContents( "atlas_cegui.png" ), FileSystem::FileContents( "atlas_cegui.xml" ), "marble", TextureWrap::Repeat, TextureFilter::Nearest, ColorSpace::RGB, Anisotropy::k1 );

    GameObject spriteContainer;
    spriteContainer.AddComponent<SpriteRendererComponent>();
    auto sprite = spriteContainer.GetComponent<SpriteRendererComponent>();
    sprite->SetTexture( &spriteTex, Vec3( 320, 0, -0.6f ), Vec3( (float)spriteTex.GetWidth(), (float)spriteTex.GetHeight(), 1 ), Vec4( 1, 0.5f, 0.5f, 1 ) );
    sprite->SetTexture( &spriteTex, Vec3( 340, 80, -0.5f ), Vec3( (float)spriteTex.GetWidth()/2, (float)spriteTex.GetHeight()/2, 1 ), Vec4( 0.5f, 1, 0.5f, 1 ) );
    sprite->SetTexture( &bc1Tex, Vec3( 120, 60, -0.5f ), Vec3( (float)spriteTex.GetWidth(), (float)spriteTex.GetHeight(), 1 ), Vec4( 1, 1, 1, 0.5f ) );
    sprite->SetTexture( &bc2Tex, Vec3( 120, 170, -0.5f ), Vec3( (float)spriteTex.GetWidth(), (float)spriteTex.GetHeight(), 1 ), Vec4( 1, 1, 1, 0.5f ) );
    sprite->SetTexture( &bc3Tex, Vec3( 120, 270, -0.5f ), Vec3( (float)spriteTex.GetWidth(), (float)spriteTex.GetHeight(), 1 ), Vec4( 1, 1, 1, 0.5f ) );
    //sprite->SetTexture( &notLoadedTex, Vec3( 120, 370, -0.5f ), Vec3( (float)spriteTex.GetWidth(), (float)spriteTex.GetHeight(), 1 ), Vec4( 1, 1, 1, 0.5f ) );
    sprite->SetTexture( &spriteTexFromAtlas, Vec3( 260, 160, -0.5f ), Vec3( (float)spriteTexFromAtlas.GetWidth(), (float)spriteTexFromAtlas.GetHeight(), 1 ), Vec4( 1, 0, 1, 1 ) );

    spriteContainer.AddComponent<TransformComponent>();
    spriteContainer.GetComponent<TransformComponent>()->SetLocalPosition( Vec3( 20, 0, 0 ) );

    AudioClip audioClip;
    audioClip.Load( FileSystem::FileContents( "sine340.wav" ) );
    
    GameObject audioContainer;
    audioContainer.AddComponent<AudioSourceComponent>();
    audioContainer.GetComponent<AudioSourceComponent>()->SetClipId( audioClip.GetId() );
    audioContainer.GetComponent<AudioSourceComponent>()->Play();
    
    Texture2D fontTex;
    fontTex.Load( FileSystem::FileContents("font.png"), TextureWrap::Clamp, TextureFilter::Linear, Mipmaps::None, ColorSpace::RGB, Anisotropy::k1 );

    Font font;
    font.LoadBMFont( &fontTex, FileSystem::FileContents("font_txt.fnt"));

    Texture2D fontTexSDF;
    //fontTexSDF.Load( FileSystem::FileContents( "font_sdf.tga" ), TextureWrap::Clamp, TextureFilter::Linear, Mipmaps::None, 1 );
    fontTexSDF.Load( FileSystem::FileContents( "font.png" ), TextureWrap::Clamp, TextureFilter::Linear, Mipmaps::None, ColorSpace::RGB, Anisotropy::k1 );

    Font sdfFont;
    sdfFont.LoadBMFont( &fontTexSDF, FileSystem::FileContents( "font_txt.fnt" ) );

    GameObject textContainer;
    textContainer.AddComponent<TextRendererComponent>();
    textContainer.GetComponent<TextRendererComponent>()->SetText( "Aether3D \nGame Engine" );
    textContainer.GetComponent<TextRendererComponent>()->SetFont( &font );
    textContainer.GetComponent<TextRendererComponent>()->SetShader( TextRendererComponent::ShaderType::SDF );
    textContainer.AddComponent<TransformComponent>();
    textContainer.GetComponent<TransformComponent>()->SetLocalPosition( Vec3( 20, 160, 0 ) );
    //textContainer.GetComponent<TransformComponent>()->SetLocalScale( 2 );
    textContainer.GetComponent<TransformComponent>()->SetLocalRotation( Quaternion::FromEuler( Vec3( 0, 0, 45 ) ) );

    GameObject textContainerSDF;
    textContainerSDF.AddComponent<TextRendererComponent>();
    textContainerSDF.GetComponent<TextRendererComponent>()->SetText( "Aether3D \nGame Engine" );
    textContainerSDF.GetComponent<TextRendererComponent>()->SetFont( &sdfFont );
    textContainerSDF.GetComponent<TextRendererComponent>()->SetShader( TextRendererComponent::ShaderType::SDF );
    textContainerSDF.AddComponent<TransformComponent>();
    textContainerSDF.GetComponent<TransformComponent>()->SetLocalPosition( Vec3( 100, 160, 0 ) );
    //textContainerSDF<TransformComponent>()->SetLocalScale( 2 );
    textContainerSDF.GetComponent<TransformComponent>()->SetLocalRotation( Quaternion::FromEuler( Vec3( 0, 0, 45 ) ) );

    GameObject statsParent;
    statsParent.AddComponent<TransformComponent>();
    statsParent.GetComponent<TransformComponent>()->SetLocalPosition( Vec3( 0, -80, 0 ) );
    
    GameObject statsContainer;
    statsContainer.AddComponent<TextRendererComponent>();
    statsContainer.GetComponent<TextRendererComponent>()->SetText( "Aether3D \nGame Engine" );
    statsContainer.GetComponent<TextRendererComponent>()->SetFont( &font );
    statsContainer.AddComponent<TransformComponent>();
    statsContainer.GetComponent<TransformComponent>()->SetLocalPosition( Vec3( 20, 80, 0 ) );
    statsContainer.GetComponent<TransformComponent>()->SetParent( statsParent.GetComponent<TransformComponent>() );

    RenderTexture rtTex;
    rtTex.Create2D( 512, 512, RenderTexture::DataType::UByte, TextureWrap::Clamp, TextureFilter::Linear, "render texture" );
    
    GameObject renderTextureContainer;
    renderTextureContainer.AddComponent<SpriteRendererComponent>();
    renderTextureContainer.GetComponent<SpriteRendererComponent>()->SetTexture( &rtTex, Vec3( 150, 250, -0.6f ), Vec3( (float)spriteTex.GetWidth() * 2, (float)spriteTex.GetHeight() * 2, 1 ), Vec4( 1, 1, 1, 1 ) );
    
    GameObject rtCamera;
    rtCamera.AddComponent<CameraComponent>();
    rtCamera.GetComponent<CameraComponent>()->SetProjection( 0, (float)rtTex.GetWidth(), 0,(float)rtTex.GetHeight(), 0, 1 );
    rtCamera.GetComponent<CameraComponent>()->SetClearColor( Vec3( 1, 0, 0 ) );
    rtCamera.GetComponent<CameraComponent>()->SetClearFlag( CameraComponent::ClearFlag::DepthAndColor );
    rtCamera.GetComponent<CameraComponent>()->SetTargetTexture( &rtTex );
    rtCamera.AddComponent<TransformComponent>();

    Scene scene;
    scene.Add( &camera );
    scene.Add( &spriteContainer );
    scene.Add( &textContainer );
    //scene.Add( &textContainerSDF );
    scene.Add( &statsContainer );
    scene.Add( &statsParent );
    //scene.Add( &renderTextureContainer );
    //scene.Add( &rtCamera );
    //System::Print( "%s\n", scene.GetSerialized().c_str() );

    Matrix44 lineView;
    Matrix44 lineProjection;
    lineProjection.MakeProjection( 0, (float)width, (float)height, 0, 0, 1 );
    std::vector< Vec3 > lines( 4 );
    lines[ 0 ] = Vec3( 10, 10, -0.5f );
    lines[ 1 ] = Vec3( 50, 10, -0.5f );
    lines[ 2 ] = Vec3( 50, 50, -0.5f );
    lines[ 3 ] = Vec3( 10, 10, -0.5f );
    const int lineHandle = System::CreateLineBuffer( lines, Vec3( 1, 0, 0 ) );
    
    bool quit = false;

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
            
            if (event.type == WindowEventType::KeyDown ||
                event.type == WindowEventType::KeyUp)
            {
                KeyCode keyCode = event.keyCode;
                
                if (keyCode == KeyCode::Escape)
                {
                    quit = true;
                }
                else if (keyCode == KeyCode::R)
                {
                    System::ReloadChangedAssets();
                }
                else if (keyCode == KeyCode::B && event.type == WindowEventType::KeyUp)
                {
                    audioContainer.GetComponent<AudioSourceComponent>()->Play();
                }
            }
            if (event.type == WindowEventType::GamePadButtonA)
            {
                text = "button a down";
            }
        }

        //std::string stats = text + std::to_string( System::Statistics::GetDrawCallCount() );
        
        //std::string stats = System::Statistics::GetStatistics();
        //statsContainer.GetComponent<TextRendererComponent>()->SetText( stats.c_str() );

        //stats += std::string( "\nbarriers: " ) + std::to_string( System::Statistics::GetBarrierCallCount() );

        /*unsigned gpuUsage, gpuBudget;
        System::Statistics::GetGpuMemoryUsage( gpuUsage, gpuBudget );
        stats += std::string( "\nused VRAM: " ) + std::to_string( gpuUsage ) + std::string( " MB, budget: " ) +
            std::to_string( gpuBudget ) + std::string( " MB" );
          */  
        //statsContainer.GetComponent<TextRendererComponent>()->SetText( stats.c_str() );

        scene.Render();
        System::Draw( &spriteTex, 40, 240, 100, 100, width, height );
        System::DrawLines( lineHandle, lineView, lineProjection );
        scene.EndFrame();
        
        Window::SwapBuffers();
    }

    System::Deinit();
}
