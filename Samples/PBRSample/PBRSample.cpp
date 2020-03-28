#include <string>
#include <stdint.h>
#include "Array.hpp"
#include "AudioClip.hpp"
#include "AudioSourceComponent.hpp"
#include "CameraComponent.hpp"
#include "ComputeShader.hpp"
#include "DirectionalLightComponent.hpp"
#include "FileSystem.hpp"
#include "Font.hpp"
#include "GameObject.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "MeshRendererComponent.hpp"
#include "PointLightComponent.hpp"
#include "RenderTexture.hpp"
#include "SpriteRendererComponent.hpp"
#include "SpotLightComponent.hpp"
#include "Scene.hpp"
#include "Shader.hpp"
#include "System.hpp"
#include "TextRendererComponent.hpp"
#include "TransformComponent.hpp"
#include "Texture2D.hpp"
#include "TextureCube.hpp"
#include "Vec3.hpp"
#include "VR.hpp"
#include "Window.hpp"

// Assets for this sample (extract into aether3d_build/Samples): http://twiren.kapsi.fi/files/aether3d_sample_v0.8.1.zip

using namespace ae3d;

Scene scene;
GameObject camera;

void sceneRenderFunc( int eye )
{
    VR::CalcCameraForEye( camera, 0, eye );
    scene.Render();
}

int main()
{
    bool fullScreen = false;

    int originalWidth = 1920 / 1;
    int originalHeight = 1080 / 1;
    int width = originalWidth;
    int height = originalHeight;

    if (fullScreen)
    {
        width = 0;
        height = 0;
    }
    
    System::EnableWindowsMemleakDetection();
    Window::Create( width, height, fullScreen ? WindowCreateFlags::Fullscreen : WindowCreateFlags::MSAA4 );
    Window::GetSize( width, height );
    
    if (fullScreen)
    {
        originalWidth = width;
        originalHeight = height;
    }

    Window::SetTitle( "PBRSample" );
    VR::Init();
    System::LoadBuiltinAssets();
    System::InitAudio();
    System::InitGamePad();

    RenderTexture cameraTex;
    cameraTex.Create2D( width, height, RenderTexture::DataType::Float, TextureWrap::Clamp, TextureFilter::Linear, "cameraTex" );

    Texture2D bloomTex;
    bloomTex.CreateUAV( width / 2, height / 2, "bloomTex" );

	Texture2D blurTex;
	blurTex.CreateUAV( width / 2, height / 2, "blurTex" );

    RenderTexture resolvedTex;
    resolvedTex.Create2D( width, height, RenderTexture::DataType::Float, TextureWrap::Clamp, TextureFilter::Linear, "resolve" );
        
    RenderTexture camera2dTex;
    camera2dTex.Create2D( width, height, RenderTexture::DataType::Float, TextureWrap::Clamp, TextureFilter::Linear, "camera2dTex" );

    camera.AddComponent<CameraComponent>();
    camera.GetComponent<CameraComponent>()->SetClearColor( Vec3( 0, 0, 0 ) );
    camera.GetComponent<CameraComponent>()->SetProjectionType( CameraComponent::ProjectionType::Perspective );
    camera.GetComponent<CameraComponent>()->SetProjection( 45, (float)originalWidth / (float)originalHeight, 0.1f, 200 );
#ifdef TEST_FORWARD_PLUS
    camera.GetComponent<CameraComponent>()->GetDepthNormalsTexture().Create2D( originalWidth, originalHeight, ae3d::RenderTexture::DataType::Float, ae3d::TextureWrap::Clamp, ae3d::TextureFilter::Nearest, "depthnormals" );
#endif
    camera.GetComponent<CameraComponent>()->SetClearFlag( CameraComponent::ClearFlag::DepthAndColor );
    camera.GetComponent<CameraComponent>()->SetRenderOrder( 1 );
#ifndef AE3D_OPENVR
    camera.GetComponent<CameraComponent>()->SetTargetTexture( &cameraTex );
#endif
    //camera.GetComponent<CameraComponent>()->SetViewport( 0, 0, originalWidth / 2, originalHeight );
    camera.AddComponent<TransformComponent>();
    camera.GetComponent<TransformComponent>()->LookAt( { 0, 0, -80 }, { 0, 0, 100 }, { 0, 1, 0 } );
    camera.SetName( "camera" );
    
    GameObject camera2d;
    camera2d.AddComponent<CameraComponent>();
    camera2d.GetComponent<CameraComponent>()->SetClearColor( Vec3( 0, 0, 0 ) );
    camera2d.GetComponent<CameraComponent>()->SetProjectionType( CameraComponent::ProjectionType::Orthographic );
    camera2d.GetComponent<CameraComponent>()->SetProjection( 0, (float)width, (float)height, 0, 0, 1 );
    camera2d.GetComponent<CameraComponent>()->SetClearFlag( CameraComponent::ClearFlag::DepthAndColor );
    camera2d.GetComponent<CameraComponent>()->SetLayerMask( 0x2 );
    camera2d.GetComponent<CameraComponent>()->SetTargetTexture( &camera2dTex );
    camera2d.GetComponent<CameraComponent>()->SetRenderOrder( 2 );
    camera2d.AddComponent<TransformComponent>();
    camera2d.SetName( "camera2d" );
    
    Texture2D fontTex;
    fontTex.Load( FileSystem::FileContents( "font.png" ), TextureWrap::Clamp, TextureFilter::Linear, Mipmaps::None, ColorSpace::SRGB, Anisotropy::k1 );

    Texture2D normalTex;
    normalTex.Load( FileSystem::FileContents( "textures/default_n.png" ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::Linear, Anisotropy::k1 );

    Texture2D whiteTex;
    whiteTex.Load( FileSystem::FileContents( "default_white.png" ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::SRGB, Anisotropy::k1 );
    
    Font font;
    font.LoadBMFont( &fontTex, FileSystem::FileContents( "font_txt.fnt" ) );

    GameObject statsContainer;
    statsContainer.AddComponent<TextRendererComponent>();
    statsContainer.GetComponent<TextRendererComponent>()->SetText( "Aether3D \nGame Engine" );
    statsContainer.GetComponent<TextRendererComponent>()->SetFont( &font );
    statsContainer.AddComponent<TransformComponent>();
    statsContainer.GetComponent<TransformComponent>()->SetLocalPosition( { 20, 40, 0 } );
    //statsContainer.GetComponent<TransformComponent>()->SetLocalScale( 0.5f );
    statsContainer.SetLayer( 2 );
    
    Mesh cubeMesh;
    cubeMesh.Load( FileSystem::FileContents( "textured_cube.ae3d" ) );

    GameObject cube;
    cube.AddComponent< MeshRendererComponent >();
    cube.GetComponent< MeshRendererComponent >()->SetMesh( &cubeMesh );
    cube.AddComponent< TransformComponent >();
    cube.GetComponent< TransformComponent >()->SetLocalPosition( { 0, 4, -80 } );

    Shader shader;
    shader.Load( "unlitVert", "unlitFrag",
                 FileSystem::FileContents( "unlit_vert.obj" ), FileSystem::FileContents( "unlit_frag.obj" ),
                 FileSystem::FileContents( "unlit_vert.spv" ), FileSystem::FileContents( "unlit_frag.spv" ) );

    Shader shaderSkin;
    shaderSkin.Load( "unlitVert", "unlitFrag",
                FileSystem::FileContents( "unlit_skin_vert.obj" ), FileSystem::FileContents( "unlit_frag.obj" ),
                FileSystem::FileContents( "unlit_skin_vert.spv" ), FileSystem::FileContents( "unlit_frag.spv" ) );
#ifdef TEST_BLOOM
    ComputeShader blurShader;
	blurShader.Load( "blur", FileSystem::FileContents( "Blur.obj" ), FileSystem::FileContents( "Blur.spv" ) );

	ComputeShader downsampleAndThresholdShader;
	downsampleAndThresholdShader.Load( "downsampleAndThreshold", FileSystem::FileContents( "Bloom.obj" ), FileSystem::FileContents( "Bloom.spv" ) );
#endif
    
    Texture2D gliderTex;
    gliderTex.Load( FileSystem::FileContents( "glider.png" ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::SRGB, Anisotropy::k1 );

    Material material;
    material.SetShader( &shader );
    material.SetTexture( &gliderTex, 0 );
    material.SetBackFaceCulling( true );

    cube.GetComponent< MeshRendererComponent >()->SetMaterial( &material, 0 );

    Shader shaderCubeMap;
    shaderCubeMap.Load( "unlitVert", "unlitFrag",
                        FileSystem::FileContents( "unlit_cube_vert.obj" ), FileSystem::FileContents( "unlit_cube_frag.obj" ),
                        FileSystem::FileContents( "unlit_cube_vert.spv" ), FileSystem::FileContents( "unlit_cube_frag.spv" ) );

    GameObject pointLight;
    pointLight.AddComponent<PointLightComponent>();
    pointLight.GetComponent<PointLightComponent>()->SetRadius( 1 );
    pointLight.AddComponent<TransformComponent>();
    pointLight.GetComponent<TransformComponent>()->SetLocalPosition( { 2, 0, -98 } );
    
    scene.SetAmbient( { 0.1f, 0.1f, 0.1f } );
    
    TextureCube skybox;
    skybox.Load( FileSystem::FileContents( "skybox/left.jpg" ), FileSystem::FileContents( "skybox/right.jpg" ),
                 FileSystem::FileContents( "skybox/bottom.jpg" ), FileSystem::FileContents( "skybox/top.jpg" ),
                 FileSystem::FileContents( "skybox/front.jpg" ), FileSystem::FileContents( "skybox/back.jpg" ),
                 TextureWrap::Clamp, TextureFilter::Linear, Mipmaps::None, ColorSpace::SRGB );
    /*const char* path = "test_dxt1.dds";
    skybox.Load( FileSystem::FileContents( path ), FileSystem::FileContents( path ),
        FileSystem::FileContents( path ), FileSystem::FileContents( path ),
        FileSystem::FileContents( path ), FileSystem::FileContents( path ),
        TextureWrap::Clamp, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::SRGB );*/
    Shader standardShader;
    standardShader.Load( "standard_vertex", "standard_fragment",
        ae3d::FileSystem::FileContents( "Standard_vert.obj" ), ae3d::FileSystem::FileContents( "Standard_frag.obj" ),
        ae3d::FileSystem::FileContents( "Standard_vert.spv" ), ae3d::FileSystem::FileContents( "Standard_frag.spv" ) );

    Material standardMaterial;
    standardMaterial.SetShader( &standardShader );
    standardMaterial.SetTexture( &gliderTex, 0 );
    standardMaterial.SetTexture( &skybox );
    
    scene.SetSkybox( &skybox );
    scene.Add( &camera );
    scene.Add( &camera2d );
    scene.Add( &statsContainer );
    //scene.Add( &dirLight );

    bool quit = false;
    
    int lastMouseX = 0;
    int lastMouseY = 0;
    
    float yaw = 0;
    float gamePadLeftThumbX = 0;
    float gamePadLeftThumbY = 0;
    float gamePadRightThumbX = 0;
    float gamePadRightThumbY = 0;
    
    float angle = 0;
    Vec3 moveDir;

    bool reload = false;

    while (Window::IsOpen() && !quit)
    {
        Window::PumpEvents();
        WindowEvent event;
        
        ++angle;
        Quaternion rotation;
        Vec3 axis( 0, 1, 0 );
        rotation.FromAxisAngle( axis, angle );

        while (Window::PollEvent( event ))
        {
            if (event.type == WindowEventType::Close)
            {
                quit = true;
            }
            else if (event.type == WindowEventType::KeyDown)
            {
                KeyCode keyCode = event.keyCode;
                
                const float velocity = 0.3f;
                
                if (keyCode == KeyCode::Escape)
                {
                    quit = true;
                }
                else if (keyCode == KeyCode::W)
                {
                    moveDir.z = -velocity;
                }
                else if (keyCode == KeyCode::S)
                {
                    moveDir.z = velocity;
                }
                else if (keyCode == KeyCode::E)
                {
                    moveDir.y = velocity;
                }
                else if (keyCode == KeyCode::Q)
                {
                    moveDir.y = -velocity;
                }
                else if (keyCode == KeyCode::A)
                {
                    moveDir.x = -velocity;
                }
                else if (keyCode == KeyCode::D)
                {
                    moveDir.x = velocity;
                }
                else if (keyCode == KeyCode::Left)
                {
                    camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 0, 1, 0 ), 1 );
                    yaw += 4;
                }
                else if (keyCode == KeyCode::Right)
                {
                    camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 0, 1, 0 ), -1 );
                    yaw -= 4;
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
            else if (event.type == WindowEventType::KeyUp)
            {
                KeyCode keyCode = event.keyCode;

                if (keyCode == KeyCode::W)
                {
                    moveDir.z = 0;
                }
                else if (keyCode == KeyCode::S)
                {
                    moveDir.z = 0;
                }
                else if (keyCode == KeyCode::E)
                {
                    moveDir.y = 0;
                }
                else if (keyCode == KeyCode::Q)
                {
                    moveDir.y = 0;
                }
                else if (keyCode == KeyCode::A)
                {
                    moveDir.x = 0;
                }
                else if (keyCode == KeyCode::D)
                {
                    moveDir.x = 0;
                }
                else if (keyCode == KeyCode::R)
                {
                    reload = true;
                }
            }
            else if (event.type == WindowEventType::MouseMove)
            {
                const int mouseDeltaX = event.mouseX - lastMouseX;
                const int mouseDeltaY = event.mouseY - lastMouseY;
                lastMouseX = event.mouseX;
                lastMouseY = event.mouseY;
                camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 0, 1, 0 ), -float( mouseDeltaX ) / 20 );
                camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 1, 0, 0 ), float( mouseDeltaY ) / 20 );
            }
            else if (event.type == WindowEventType::GamePadLeftThumbState)
            {           
                gamePadLeftThumbX = event.gamePadThumbX;
                gamePadLeftThumbY = event.gamePadThumbY;
            }
            else if (event.type == WindowEventType::GamePadRightThumbState)
            {
                gamePadRightThumbX = event.gamePadThumbX;
                gamePadRightThumbY = event.gamePadThumbY;
            }
            else if (event.type == WindowEventType::GamePadButtonY)
            {
                camera.GetComponent<TransformComponent>()->MoveUp( 0.1f );
            }
            else if (event.type == WindowEventType::GamePadButtonA)
            {
                camera.GetComponent<TransformComponent>()->MoveUp( -0.1f );
            }
        }

        camera.GetComponent<TransformComponent>()->MoveUp( moveDir.y );
        camera.GetComponent<TransformComponent>()->MoveForward( moveDir.z );
        camera.GetComponent<TransformComponent>()->MoveRight( moveDir.x );

        camera.GetComponent<TransformComponent>()->MoveForward( -gamePadLeftThumbY );
        camera.GetComponent<TransformComponent>()->MoveRight( gamePadLeftThumbX );
        camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 0, 1, 0 ), -float( gamePadRightThumbX ) / 1 );
        camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 1, 0, 0 ), float( gamePadRightThumbY ) / 1 );

        if (reload)
        {
            System::Print("reloading\n");
            System::ReloadChangedAssets();
            reload = false;
        }
        scene.Render();
        cameraTex.ResolveTo( &resolvedTex );
        System::Draw( &resolvedTex, 0, 0, width, height, width, height, Vec4( 1, 1, 1, 1 ), System::BlendMode::Off );
        scene.EndFrame();
        Window::SwapBuffers();
    }

    VR::Deinit();
    System::Deinit();
}
