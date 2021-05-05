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

// This sample demonstrates a big number of draw calls, good for profiling CPU load. GPU usage should also be quite big.
// The scene in its default camera position has 2000 point lights, 0.6 M triangles, ~3000 draw calls.
// Assets for this sample will be added before 0.8.7 release.

using namespace ae3d;

Scene scene;
GameObject camera;

// *Really* minimal PCG32 code / (c) 2014 M.E. O'Neill / pcg-random.org
// Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)
struct pcg32_random_t
{
    uint64_t state;
    uint64_t inc;
};

uint32_t pcg32_random_r( pcg32_random_t* rng )
{
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + (rng->inc|1);
    uint32_t xorshifted = uint32_t( ((oldstate >> 18u) ^ oldstate) >> 27u );
    int32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

pcg32_random_t rng;

int Random100()
{
    return pcg32_random_r( &rng ) % 100;
}

void sceneRenderFunc( int eye )
{
    VR::CalcCameraForEye( camera, 0, eye );
    scene.Render();
}

int main()
{
    constexpr bool testBloom = false;

    bool fullScreen = false;

    int originalWidth = 2560 / 1;
    int originalHeight = 1440 / 1;
    int width = originalWidth;
    int height = originalHeight;

    if (fullScreen)
    {
        width = 0;
        height = 0;
    }
    
    System::EnableWindowsMemleakDetection();
    Window::Create( width, height, fullScreen ? WindowCreateFlags::Fullscreen : WindowCreateFlags::Empty );
    int realHeight = 0;
    Window::GetSize( width, realHeight );
    
#if RENDERER_VULKAN
    int heightDelta = 0;
#else
    int heightDelta = (height - realHeight) / 2;
#endif
    height = realHeight;

    if (fullScreen)
    {
        originalWidth = width;
        originalHeight = height;
    }

    Window::SetTitle( "City 2021" );
    System::LoadBuiltinAssets();
    System::InitAudio();
    System::InitGamePad();

    RenderTexture cameraTex;
    cameraTex.Create2D( width, height, DataType::Float, TextureWrap::Clamp, TextureFilter::Linear, "cameraTex", true );
        
    RenderTexture camera2dTex;
    camera2dTex.Create2D( width, height, DataType::Float, TextureWrap::Clamp, TextureFilter::Linear, "camera2dTex", false );

    RenderTexture resolvedTex;
    resolvedTex.Create2D( width, height, DataType::Float, TextureWrap::Clamp, TextureFilter::Linear, "resolve", false );

    Texture2D bloomTex;
    bloomTex.CreateUAV( width / 2, height / 2, "bloomTex", DataType::UByte, nullptr );

    camera.AddComponent<CameraComponent>();
    camera.GetComponent<CameraComponent>()->SetClearColor( Vec3( 0, 0, 0 ) );
    camera.GetComponent<CameraComponent>()->SetProjectionType( CameraComponent::ProjectionType::Perspective );
    camera.GetComponent<CameraComponent>()->SetProjection( 45, (float)originalWidth / (float)originalHeight, 0.1f, 700 );
    camera.GetComponent<CameraComponent>()->GetDepthNormalsTexture().Create2D( originalWidth, originalHeight, ae3d::DataType::Float, ae3d::TextureWrap::Clamp, ae3d::TextureFilter::Nearest, "depthnormals", false );
    camera.GetComponent<CameraComponent>()->SetClearFlag( CameraComponent::ClearFlag::DepthAndColor );
    camera.GetComponent<CameraComponent>()->SetRenderOrder( 1 );
    camera.GetComponent<CameraComponent>()->SetTargetTexture( &cameraTex );
    //camera.GetComponent<CameraComponent>()->SetViewport( 0, 0, originalWidth / 2, originalHeight );
    camera.AddComponent<TransformComponent>();
    camera.GetComponent<TransformComponent>()->LookAt( { 0, 0, -20 }, { 0, 0, 100 }, { 0, 1, 0 } );
    camera.SetName( "camera" );

    camera.GetComponent<TransformComponent>()->SetLocalPosition( { -136.520813, 86.304482, -4.762257 } );
    Quaternion cameraRot;
    cameraRot.x = -0.171829f;
    cameraRot.y = -0.695113f;
    cameraRot.z = -0.176876f;
    cameraRot.w = 0.675285f;
    camera.GetComponent<TransformComponent>()->SetLocalRotation( cameraRot );

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
    fontTex.Load( FileSystem::FileContents( "textures/font.png" ), TextureWrap::Clamp, TextureFilter::Linear, Mipmaps::None, ColorSpace::SRGB, Anisotropy::k1 );

    Texture2D whiteTex;
    whiteTex.Load( FileSystem::FileContents( "textures/default_white.png" ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::SRGB, Anisotropy::k1 );
    
    Texture2D blurTex;
    blurTex.CreateUAV( width / 2, height / 2, "blurTex", DataType::UByte, nullptr );

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
    
    Mesh cityMesh;
    cityMesh.Load( FileSystem::FileContents( "city2021.ae3d" ) );

    Shader shader;
    shader.Load( "unlitVert", "unlitFrag",
                 FileSystem::FileContents( "shaders/unlit_vert.obj" ), FileSystem::FileContents( "shaders/unlit_frag.obj" ),
                 FileSystem::FileContents( "shaders/unlit_vert.spv" ), FileSystem::FileContents( "shaders/unlit_frag.spv" ) );

    Shader shaderSkin;
    shaderSkin.Load( "unlitVert", "unlitFrag",
                FileSystem::FileContents( "shaders/unlit_skin_vert.obj" ), FileSystem::FileContents( "shaders/unlit_frag.obj" ),
                FileSystem::FileContents( "shaders/unlit_skin_vert.spv" ), FileSystem::FileContents( "shaders/unlit_frag.spv" ) );
    
    ComputeShader downsampleAndThresholdShader;
    downsampleAndThresholdShader.Load( "downsampleAndThreshold", FileSystem::FileContents( "shaders/Bloom.obj" ), FileSystem::FileContents( "shaders/Bloom.spv" ) );

    ComputeShader blurShader;
    blurShader.Load( "blur", FileSystem::FileContents( "shaders/Blur.obj" ), FileSystem::FileContents( "shaders/Blur.spv" ) );

    Texture2D albedoTex2;
    albedoTex2.Load( FileSystem::FileContents( "textures/asphalt.jpg" ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::SRGB, Anisotropy::k1 );

    GameObject pointLight;
    pointLight.AddComponent<PointLightComponent>();
    pointLight.GetComponent<PointLightComponent>()->SetRadius( 1 );
    pointLight.AddComponent<TransformComponent>();
    pointLight.GetComponent<TransformComponent>()->SetLocalPosition( { 2, 0, -98 } );
    
    GameObject dirLight;
    dirLight.AddComponent<DirectionalLightComponent>();
    dirLight.GetComponent<DirectionalLightComponent>()->SetCastShadow( false, 2048 );
    dirLight.GetComponent<DirectionalLightComponent>()->SetColor( Vec3( 1, 1, 1 ) );
    dirLight.AddComponent<TransformComponent>();
    dirLight.GetComponent<TransformComponent>()->LookAt( { 0, 0, 0 }, Vec3( 0.2f, -1, 0.05f ).Normalized(), { 0, 1, 0 } );

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
        ae3d::FileSystem::FileContents( "shaders/Standard_vert.obj" ), ae3d::FileSystem::FileContents( "shaders/Standard_frag.obj" ),
        ae3d::FileSystem::FileContents( "shaders/Standard_vert.spv" ), ae3d::FileSystem::FileContents( "shaders/Standard_frag.spv" ) );

    std::vector< GameObject > cityGameObjects;
    std::map< std::string, Material* > cityMaterialNameToMaterial;
    std::map< std::string, Texture2D* > cityTextureNameToTexture;
    Array< Mesh* > cityMeshes;

    auto res = scene.Deserialize( FileSystem::FileContents( "city2021.scene" ), cityGameObjects, cityTextureNameToTexture,
                                  cityMaterialNameToMaterial, cityMeshes );
    if (res != Scene::DeserializeResult::Success)
    {
        System::Print( "Could not parse city2021.scene!\n" );
    }

    for (auto& mat : cityMaterialNameToMaterial)
    {
        mat.second->SetShader( &standardShader );
        mat.second->SetTexture( &skybox );
    }
    
    for (std::size_t i = 0; i < cityGameObjects.size(); ++i)
    {
        scene.Add( &cityGameObjects[ i ] );
    }

    std::vector< GameObject > cityGameObjects2;
    std::map< std::string, Material* > cityMaterialNameToMaterial2;
    std::map< std::string, Texture2D* > cityTextureNameToTexture2;
    Array< Mesh* > cityMeshes2;

    res = scene.Deserialize( FileSystem::FileContents( "city2021.scene" ), cityGameObjects2, cityTextureNameToTexture2,
                                  cityMaterialNameToMaterial2, cityMeshes2 );
    if (res != Scene::DeserializeResult::Success)
    {
        System::Print( "Could not parse city2021.scene!\n" );
    }

    for (auto& mat : cityMaterialNameToMaterial2)
    {
        mat.second->SetShader( &standardShader );
        mat.second->SetTexture( &skybox );
    }
    
    for (std::size_t i = 0; i < cityGameObjects2.size(); ++i)
    {
        cityGameObjects2[ i ].GetComponent< TransformComponent >()->SetLocalPosition( { 280, 0, 0 } );
        scene.Add( &cityGameObjects2[ i ] );
    }

    scene.SetSkybox( &skybox );
    scene.Add( &camera );
    scene.Add( &camera2d );
    scene.Add( &statsContainer );
    scene.Add( &dirLight );

    constexpr int PointLightCount = 50 * 40;
    GameObject pointLights[ PointLightCount ];

    // Inits point lights for Forward+
    int pointLightIndex = 0;        

    for (int row = 0; row < 50; ++row)
    {
        for (int col = 0; col < 40; ++col)
        {
            pointLights[ pointLightIndex ].AddComponent<ae3d::PointLightComponent>();
            pointLights[ pointLightIndex ].GetComponent<ae3d::PointLightComponent>()->SetRadius( 10 );
            pointLights[ pointLightIndex ].GetComponent<ae3d::PointLightComponent>()->SetColor( { (Random100() % 100 ) / 100.0f * 4, (Random100() % 100) / 100.0f * 4, (Random100() % 100) / 100.0f * 4 } );
            pointLights[ pointLightIndex ].AddComponent<ae3d::TransformComponent>();
            pointLights[ pointLightIndex ].GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( -150 + (float)row * 8, 2, -150 + (float)col * 6 ) );

            scene.Add( &pointLights[ pointLightIndex ] );
            ++pointLightIndex;
        }
    }

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
    bool isMouseDown = false;
    int animationFrame = 0;
    
    while (Window::IsOpen() && !quit)
    {
        ++animationFrame;
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

                if (isMouseDown)
                {
                    camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 0, 1, 0 ), -float( mouseDeltaX ) / 20 );
                    camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 1, 0, 0 ), float( mouseDeltaY ) / 20 );
                }

                int x = event.mouseX;
                int y = height - event.mouseY + heightDelta;
            }
            else if (event.type == WindowEventType::Mouse1Down)
            {
                int x = event.mouseX;
                int y = height - event.mouseY;
            }
            else if (event.type == WindowEventType::Mouse1Up)
            {
                int x = event.mouseX;
                int y = height - event.mouseY;
            }
            else if (event.type == WindowEventType::Mouse2Down)
            {
                isMouseDown = true;
            }
            else if (event.type == WindowEventType::Mouse2Up)
            {
                isMouseDown = false;
            }
        }

        camera.GetComponent<TransformComponent>()->MoveUp( moveDir.y );
        camera.GetComponent<TransformComponent>()->MoveForward( moveDir.z );
        camera.GetComponent<TransformComponent>()->MoveRight( moveDir.x );

        camera.GetComponent<TransformComponent>()->MoveForward( -gamePadLeftThumbY );
        camera.GetComponent<TransformComponent>()->MoveRight( gamePadLeftThumbX );
        camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 0, 1, 0 ), -float( gamePadRightThumbX ) / 1 );
        camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 1, 0, 0 ), float( gamePadRightThumbY ) / 1 );

        if (animationFrame % 60 == 0)
        {
            static char statStr[ 512 ] = {};
            System::Statistics::GetStatistics( statStr );
            statsContainer.GetComponent<TextRendererComponent>()->SetText( statStr );
        }

        if (reload)
        {
            System::Print("reloading\n");
            System::ReloadChangedAssets();
            reload = false;
        }
        
        scene.Render();
        //cameraTex.ResolveTo( &resolvedTex );
        if (!testBloom)
        {
            System::Draw( &cameraTex, 0, 0, width, height, width, height, Vec4( 1, 1, 1, 1 ), System::BlendMode::Off );
            System::Draw( &camera2dTex, 0, 0, width, height, width, height, Vec4( 1, 1, 1, 1 ), System::BlendMode::Alpha );
        }

        if (testBloom)
        {
            blurTex.SetLayout( TextureLayout::General );
            downsampleAndThresholdShader.SetRenderTexture( &resolvedTex, 0 );
            downsampleAndThresholdShader.SetTexture2D( &blurTex, 14 );
            downsampleAndThresholdShader.Begin();
            downsampleAndThresholdShader.Dispatch( width / 16, height / 16, 1, "downsample/threshold" );
            downsampleAndThresholdShader.End();

            blurTex.SetLayout( TextureLayout::ShaderRead );

            blurShader.SetTexture2D( &blurTex, 0 );
            blurShader.SetTexture2D( &bloomTex, 14 );
            blurShader.SetUniform( ComputeShader::UniformName::TilesZW, 1, 0 );
            blurShader.Begin();
            blurShader.Dispatch( width / 16, height / 16, 1, "bloom blur" );
            blurShader.End();

            blurShader.Begin();

            blurTex.SetLayout( TextureLayout::General );
            bloomTex.SetLayout( TextureLayout::ShaderRead );
            blurShader.SetTexture2D( &bloomTex, 0 );
            blurShader.SetTexture2D( &blurTex, 14 );
            blurShader.SetUniform( ComputeShader::UniformName::TilesZW, 0, 1 );
            blurShader.Dispatch( width / 16, height / 16, 1, "bloom blur" );
            blurShader.End();

            blurTex.SetLayout( TextureLayout::ShaderRead );
            System::Draw( &cameraTex, 0, 0, width, height, width, height, Vec4( 1, 1, 1, 1 ), System::BlendMode::Off );
            System::Draw( &blurTex, 0, 0, width, height, width, height, Vec4( 1, 1, 1, 0.5f ), System::BlendMode::Additive );
            bloomTex.SetLayout( TextureLayout::General );
        }

        scene.EndFrame();
        Window::SwapBuffers();
    }

    VR::Deinit();
    System::Deinit();
}
