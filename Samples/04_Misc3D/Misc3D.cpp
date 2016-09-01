#include <string>
#include "AudioClip.hpp"
#include "AudioSourceComponent.hpp"
#include "Font.hpp"
#include "CameraComponent.hpp"
#include "DirectionalLightComponent.hpp"
#include "MeshRendererComponent.hpp"
#include "PointLightComponent.hpp"
#include "SpriteRendererComponent.hpp"
#include "SpotLightComponent.hpp"
#include "TextRendererComponent.hpp"
#include "TransformComponent.hpp"
#include "FileSystem.hpp"
#include "GameObject.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "Shader.hpp"
#include "System.hpp"
#include "RenderTexture.hpp"
#include "Texture2D.hpp"
#include "TextureCube.hpp"
#include "Vec3.hpp"
#include "Window.hpp"
#include "VR.hpp"

using namespace ae3d;
                      
// Assets for this sample (extract into aether3d_build/Samples): http://twiren.kapsi.fi/files/aether3d_sample_v0.5.5.zip

int main()
{
    bool fullScreen = false;

    int width = 640;
    int height = 480;
    
    if (fullScreen)
    {
        width = 0;
        height = 0;
    }
    
    System::EnableWindowsMemleakDetection();
    //VR::Init(); oculus
#if defined( OCULUS_RIFT ) || defined( AE3D_OPENVR )
    VR::GetIdealWindowSize( width, height );
#endif

    Window::Create( width, height, fullScreen ? WindowCreateFlags::Fullscreen : WindowCreateFlags::Empty );
    Window::GetSize( width, height );
    VR::Init(); // Vive
    VR::StartTracking( width, height );
    Window::SetTitle( "Misc3D" );
    System::LoadBuiltinAssets();
    System::InitAudio();
    System::InitGamePad();

    GameObject camera;
    camera.AddComponent<CameraComponent>();
    camera.GetComponent<CameraComponent>()->SetClearColor( Vec3( 1, 0, 0 ) );
    camera.GetComponent<CameraComponent>()->SetProjectionType( CameraComponent::ProjectionType::Perspective );
    camera.GetComponent<CameraComponent>()->SetProjection( 45, (float)width / (float)height, 1, 200 );
    camera.GetComponent<CameraComponent>()->SetClearFlag( CameraComponent::ClearFlag::DepthAndColor );
    camera.GetComponent<CameraComponent>()->SetRenderOrder( 1 );
    //camera.GetComponent<CameraComponent>()->GetDepthNormalsTexture().Create2D( width, height, ae3d::RenderTexture::DataType::Float, ae3d::TextureWrap::Clamp, ae3d::TextureFilter::Nearest );
    camera.AddComponent<TransformComponent>();
    camera.GetComponent<TransformComponent>()->LookAt( { 0, 0, -80 }, { 0, 0, 100 }, { 0, 1, 0 } );
    
    RenderTexture cubeRT;
    cubeRT.CreateCube( 512, ae3d::RenderTexture::DataType::UByte, ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Linear );
    
    GameObject cameraCubeRT;
    cameraCubeRT.AddComponent<CameraComponent>();
    cameraCubeRT.GetComponent<CameraComponent>()->SetClearColor( Vec3( 0, 0, 1 ) );
    cameraCubeRT.GetComponent<CameraComponent>()->SetProjectionType( CameraComponent::ProjectionType::Perspective );
    cameraCubeRT.GetComponent<CameraComponent>()->SetProjection( 45, 1, 1, 400 );
    cameraCubeRT.GetComponent<CameraComponent>()->SetTargetTexture( &cubeRT );
    cameraCubeRT.GetComponent<CameraComponent>()->SetClearFlag( CameraComponent::ClearFlag::DepthAndColor );
    cameraCubeRT.AddComponent<TransformComponent>();
    cameraCubeRT.GetComponent<TransformComponent>()->LookAt( { 5, 0, -70 }, { 0, 0, -100 }, { 0, 1, 0 } );

    GameObject camera2d;
    camera2d.AddComponent<CameraComponent>();
    camera2d.GetComponent<CameraComponent>()->SetClearColor( Vec3( 1, 0, 0 ) );
    camera2d.GetComponent<CameraComponent>()->SetProjectionType( CameraComponent::ProjectionType::Orthographic );
    camera2d.GetComponent<CameraComponent>()->SetProjection( 0, (float)width, (float)height, 0, 0, 1 );
    camera2d.GetComponent<CameraComponent>()->SetClearFlag( CameraComponent::ClearFlag::Depth );
    camera2d.GetComponent<CameraComponent>()->SetLayerMask( 0x2 );
    camera2d.GetComponent<CameraComponent>()->SetRenderOrder( 2 );
    camera2d.AddComponent<TransformComponent>();
    
    Texture2D fontTex;
    fontTex.Load( FileSystem::FileContents("font.png"), TextureWrap::Clamp, TextureFilter::Linear, Mipmaps::None, ColorSpace::SRGB, 1 );
    
    Font font;
    font.LoadBMFont( &fontTex, FileSystem::FileContents( "font_txt.fnt" ) );

    GameObject statsContainer;
    statsContainer.AddComponent<TextRendererComponent>();
    statsContainer.GetComponent<TextRendererComponent>()->SetText( "Aether3D \nGame Engine" );
    statsContainer.GetComponent<TextRendererComponent>()->SetFont( &font );
    statsContainer.AddComponent<TransformComponent>();
    statsContainer.GetComponent<TransformComponent>()->SetLocalPosition( { 20, 0, 0 } );
    statsContainer.SetLayer( 2 );
    
    Mesh cubeMesh;
    cubeMesh.Load( FileSystem::FileContents( "textured_cube.ae3d" ) );

    Mesh cubeMeshScaledUV;
    cubeMeshScaledUV.Load( FileSystem::FileContents( "cube_scaled_uv.ae3d" ) );

    GameObject cube;
    cube.AddComponent< MeshRendererComponent >();
    cube.GetComponent< MeshRendererComponent >()->SetMesh( &cubeMesh );
    cube.AddComponent< TransformComponent >();
    cube.GetComponent< TransformComponent >()->SetLocalPosition( { 0, 4, -80 } );

    GameObject cubeScaledUV;
    cubeScaledUV.AddComponent< MeshRendererComponent >();
    cubeScaledUV.GetComponent< MeshRendererComponent >()->SetMesh( &cubeMeshScaledUV );
    cubeScaledUV.AddComponent< TransformComponent >();
    cubeScaledUV.GetComponent< TransformComponent >()->SetLocalPosition( { 0, 6, -84 } );

    Mesh cubeMesh2;
    cubeMesh2.Load( FileSystem::FileContents( "textured_cube.ae3d" ) );

    GameObject rtCube;
    rtCube.AddComponent< MeshRendererComponent >();
    rtCube.GetComponent< MeshRendererComponent >()->SetMesh( &cubeMesh2 );
    rtCube.AddComponent< TransformComponent >();
    rtCube.GetComponent< TransformComponent >()->SetLocalPosition( { 5, 0, -70 } );

    Shader shader;
    shader.Load( FileSystem::FileContents( "unlit.vsh" ), FileSystem::FileContents( "unlit.fsh" ),
                 "unlitVert", "unlitFrag",
                 FileSystem::FileContents( "unlit.hlsl" ), FileSystem::FileContents( "unlit.hlsl" ),
                 FileSystem::FileContents( "unlit_vert.spv" ), FileSystem::FileContents( "unlit_frag.spv" ) );

    Texture2D gliderTex;
    gliderTex.Load( FileSystem::FileContents( "glider.png" ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::SRGB, 1 );

    Texture2D gliderClampTex;
    gliderClampTex.Load( FileSystem::FileContents( "glider.png" ), TextureWrap::Clamp, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::SRGB, 1 );

    Material materialClamp;
    materialClamp.SetShader( &shader );
    materialClamp.SetTexture( "textureMap", &gliderClampTex );
    materialClamp.SetVector( "tint", { 1, 0, 0, 1 } );
    materialClamp.SetBackFaceCulling( true );

    Material material;
    material.SetShader( &shader );
    material.SetTexture( "textureMap", &gliderTex );
    material.SetVector( "tint", { 1, 1, 1, 1 } );
    material.SetBackFaceCulling( true );
    
    cube.GetComponent< MeshRendererComponent >()->SetMaterial( &material, 0 );
    cubeScaledUV.GetComponent< MeshRendererComponent >()->SetMaterial( &materialClamp, 0 );

    GameObject copiedCube = cube;
    copiedCube.GetComponent< TransformComponent >()->SetLocalPosition( { 0, 6, -80 } );
    copiedCube.GetComponent< MeshRendererComponent >()->SetMaterial( &material, 0 );

    Shader shaderCubeMap;
    shaderCubeMap.Load( FileSystem::FileContents( "unlit_cube.vsh" ), FileSystem::FileContents( "unlit_cube.fsh" ),
                        "unlitVert", "unlitFrag",
                        FileSystem::FileContents( "unlit_cube.hlsl" ), FileSystem::FileContents( "unlit_cube.hlsl" ),
                        FileSystem::FileContents( "unlit_vert.spv" ), FileSystem::FileContents( "unlit_frag.spv" ) );
    
    GameObject dirLight;
    dirLight.AddComponent<DirectionalLightComponent>();
    dirLight.GetComponent<DirectionalLightComponent>()->SetCastShadow( false, 512 );
    dirLight.AddComponent<TransformComponent>();
	dirLight.GetComponent<TransformComponent>()->LookAt( { 0, 0, 0 }, Vec3( 0, -1, 0 ).Normalized(), { 0, 1, 0 } );

    GameObject spotLight;
    spotLight.AddComponent<SpotLightComponent>();
    spotLight.GetComponent<SpotLightComponent>()->SetCastShadow( true, 512 );
    spotLight.GetComponent<SpotLightComponent>()->SetConeAngle( 45 );
    spotLight.AddComponent<TransformComponent>();
    spotLight.GetComponent<TransformComponent>()->LookAt( { 0, 3, -80 }, { 0, -1, 0 }, { 0, 1, 0 } );

    GameObject pointLight;
    pointLight.AddComponent<PointLightComponent>();
    pointLight.GetComponent<PointLightComponent>()->SetCastShadow( false, 512 );
    pointLight.GetComponent<PointLightComponent>()->SetRadius( 100 );
    pointLight.AddComponent<TransformComponent>();

    Scene scene;
    
    TextureCube skybox;
    skybox.Load( FileSystem::FileContents( "skybox/left.jpg" ), FileSystem::FileContents( "skybox/right.jpg" ),
                 FileSystem::FileContents( "skybox/bottom.jpg" ), FileSystem::FileContents( "skybox/top.jpg" ),
                 FileSystem::FileContents( "skybox/front.jpg" ), FileSystem::FileContents( "skybox/back.jpg" ),
                 TextureWrap::Clamp, TextureFilter::Linear, Mipmaps::None, ColorSpace::RGB );

    Material materialCubeRT;
    materialCubeRT.SetShader( &shaderCubeMap );
    materialCubeRT.SetRenderTexture( "skyMap", &cubeRT );
    materialCubeRT.SetVector( "tint", { 1, 1, 1, 1 } );
    materialCubeRT.SetBackFaceCulling( true );

    rtCube.GetComponent< MeshRendererComponent >()->SetMaterial( &materialCubeRT, 0 );

    // Sponza begins.
    std::vector< GameObject > sponzaGameObjects;
    std::map< std::string, Material* > sponzaMaterialNameToMaterial;
    std::map< std::string, Texture2D* > sponzaTextureNameToTexture;
    std::vector< Mesh* > sponzaMeshes;
#if 0
    auto res = scene.Deserialize( FileSystem::FileContents( "sponza.scene" ), sponzaGameObjects, sponzaTextureNameToTexture,
                                 sponzaMaterialNameToMaterial, sponzaMeshes );
    
    if (res != Scene::DeserializeResult::Success)
    {
        System::Print( "Could not parse Sponza\n" );
    }

    for (auto& mat : sponzaMaterialNameToMaterial)
    {
        mat.second->SetShader( &shader );
        mat.second->SetVector( "tint", { 1, 1, 1, 1 } );
    }
    
    for (std::size_t i = 0; i < sponzaGameObjects.size(); ++i)
    {
        scene.Add( &sponzaGameObjects[ i ] );
    }
#endif
    // Sponza ends
    
    RenderTexture rtTex;
    rtTex.Create2D( 512, 512, RenderTexture::DataType::UByte, TextureWrap::Clamp, TextureFilter::Linear );
    
    GameObject renderTextureContainer;
    renderTextureContainer.AddComponent<SpriteRendererComponent>();
    //renderTextureContainer.GetComponent<SpriteRendererComponent>()->SetTexture( &rtTex, Vec3( 150, 250, -0.6f ), Vec3( 512, 512, 1 ), Vec4( 1, 1, 1, 1 ) );
    renderTextureContainer.GetComponent<SpriteRendererComponent>()->SetTexture( &camera.GetComponent< CameraComponent >()->GetDepthNormalsTexture(), Vec3( 150, 250, -0.6f ), Vec3( 256, 256, 1 ), Vec4( 1, 1, 1, 1 ) );
    renderTextureContainer.SetLayer( 2 );

    GameObject rtCamera;
    rtCamera.AddComponent<CameraComponent>();
    rtCamera.GetComponent<CameraComponent>()->SetProjection( 0, (float)rtTex.GetWidth(), 0,(float)rtTex.GetHeight(), 0, 1 );
    rtCamera.GetComponent<CameraComponent>()->SetClearColor( Vec3( 0.5f, 0.5f, 0.5f ) );
    rtCamera.GetComponent<CameraComponent>()->SetTargetTexture( &rtTex );
    rtCamera.AddComponent<TransformComponent>();

    Texture2D transTex;
    transTex.Load( FileSystem::FileContents( "trans50.png" ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::None, ColorSpace::SRGB, 1 );

    Material transMaterial;
    transMaterial.SetShader( &shader );
    transMaterial.SetTexture( "textureMap", &transTex );
    transMaterial.SetVector( "tint", { 1, 1, 1, 1 } );
    transMaterial.SetBackFaceCulling( true );
    transMaterial.SetBlendingMode( Material::BlendingMode::Alpha );
    
    GameObject transCube1;
    transCube1.AddComponent< MeshRendererComponent >();
    transCube1.GetComponent< MeshRendererComponent >()->SetMesh( &cubeMesh );
    transCube1.GetComponent< MeshRendererComponent >()->SetMaterial( &transMaterial, 0 );
    transCube1.AddComponent< TransformComponent >();
    transCube1.GetComponent< TransformComponent >()->SetLocalPosition( { 2, 0, -70 } );
    
    scene.SetSkybox( &skybox );
    scene.Add( &camera );
    scene.Add( &camera2d );
    //scene.Add( &cameraCubeRT );
    //scene.Add( &rtCube );
    scene.Add( &cubeScaledUV );
    scene.Add( &copiedCube );
    scene.Add( &statsContainer );
    scene.Add( &dirLight );
    //scene.Add( &spotLight );
    //scene.Add( &pointLight );
    scene.Add( &renderTextureContainer );
    //scene.Add( &rtCamera );
    scene.Add( &transCube1 );
    
    const int cubeCount = 10;
    GameObject cubes[ cubeCount ];

    for (int i = 0; i < cubeCount; ++i)
    {
        cubes[ i ].AddComponent< MeshRendererComponent >();
        cubes[ i ].GetComponent< MeshRendererComponent >()->SetMesh( &cubeMesh );
        cubes[ i ].AddComponent< TransformComponent >();
        cubes[ i ].GetComponent< TransformComponent >()->SetLocalPosition( { i * 4.5f - 4, 0, -100 } );
        cubes[ i ].GetComponent< MeshRendererComponent >()->SetMaterial( &material, 0 );

        scene.Add( &cubes[ i ] );
    }

    cubes[ 4 ].GetComponent< TransformComponent >()->SetLocalPosition( { 0, -10, -100 } );
    cubes[ 4 ].GetComponent< TransformComponent >()->SetLocalScale( 6 );

    cubes[ 5 ].GetComponent< TransformComponent >()->SetLocalPosition( { 0, 0, -110 } );
    cubes[ 5 ].GetComponent< TransformComponent >()->SetLocalScale( 6 );

    cubes[ 6 ].GetComponent< TransformComponent >()->SetLocalPosition( { -12, -10, -100 } );
    cubes[ 6 ].GetComponent< TransformComponent >()->SetLocalScale( 6 );

    cubes[ 7 ].GetComponent< TransformComponent >()->SetLocalPosition( { 12, -10, -100 } );
    cubes[ 7 ].GetComponent< TransformComponent >()->SetLocalScale( 6 );

    cubes[ 8 ].GetComponent< TransformComponent >()->SetLocalPosition( { -12, 0, -110 } );
    cubes[ 8 ].GetComponent< TransformComponent >()->SetLocalScale( 6 );

    cubes[ 9 ].GetComponent< TransformComponent >()->SetLocalPosition( { 12, 0, -110 } );
    cubes[ 9 ].GetComponent< TransformComponent >()->SetLocalScale( 6 );

    cubes[ 3 ].GetComponent< TransformComponent >()->SetLocalPosition( { 4, 0, 0 } );
    cubes[ 3 ].GetComponent< TransformComponent >()->SetParent( cubes[ 2 ].GetComponent< TransformComponent >() );
    
    AudioClip audioClip;
    audioClip.Load( FileSystem::FileContents( "sine340.wav" ) );
    
    cubes[ 4 ].AddComponent<AudioSourceComponent>();
    cubes[ 4 ].GetComponent<AudioSourceComponent>()->SetClipId( audioClip.GetId() );
    cubes[ 4 ].GetComponent<AudioSourceComponent>()->Set3D( true );
    cubes[ 4 ].GetComponent<AudioSourceComponent>()->Play();

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

    while (Window::IsOpen() && !quit)
    {
#if defined( OCULUS_RIFT ) || defined( AE3D_OPENVR )
        VR::CalcEyePose();
        
        for (int eye = 0; eye < 2; ++eye)
        {
            VR::SetEye( eye );
            VR::CalcCameraForEye( camera, yaw, eye );
            scene.Render();
            VR::UnsetEye( eye );
        }
        
        VR::SubmitFrame();
#else
        scene.Render();
        //System::Draw( &gliderTex, 20, 0, 256, 256, width, height );
#endif

        Window::PumpEvents();
        WindowEvent event;
        
        ++angle;
        Quaternion rotation;
        const Vec3 axis( 0, 1, 0 );
        rotation.FromAxisAngle( axis, angle );
        cubes[ 2 ].GetComponent< TransformComponent >()->SetLocalRotation( rotation );

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
                else if (keyCode == KeyCode::Space)
                {
                    VR::RecenterTracking();
                    cubes[ 4 ].GetComponent<AudioSourceComponent>()->Play();
                    cubes[ 3 ].SetEnabled( false );
                }
                else if (keyCode == KeyCode::R)
                {
                    System::ReloadChangedAssets();
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
            if (event.type == WindowEventType::GamePadLeftThumbState)
            {           
                gamePadLeftThumbX = event.gamePadThumbX;
                gamePadLeftThumbY = event.gamePadThumbY;
            }
            if (event.type == WindowEventType::GamePadRightThumbState)
            {
                gamePadRightThumbX = event.gamePadThumbX;
                gamePadRightThumbY = event.gamePadThumbY;
            }
            if (event.type == WindowEventType::GamePadButtonY)
            {
                camera.GetComponent<TransformComponent>()->MoveUp( 0.1f );
            }
            if (event.type == WindowEventType::GamePadButtonA)
            {
                camera.GetComponent<TransformComponent>()->MoveUp( -0.1f );
            }
        }

        camera.GetComponent<TransformComponent>()->MoveUp( moveDir.y );
        camera.GetComponent<TransformComponent>()->MoveForward( moveDir.z );
        camera.GetComponent<TransformComponent>()->MoveRight( moveDir.x );

        camera.GetComponent<TransformComponent>()->MoveForward( gamePadLeftThumbY );
        camera.GetComponent<TransformComponent>()->MoveRight( gamePadLeftThumbX );
        camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 0, 1, 0 ), -float( gamePadRightThumbX ) / 1 );
        camera.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 1, 0, 0 ), -float( gamePadRightThumbY ) / 1 );

        /*std::string stats = std::string( "draw calls:" ) + std::to_string( System::Statistics::GetDrawCallCount() );
        stats += std::string( "\nVAO binds:" ) + std::to_string( System::Statistics::GetVertexBufferBindCount() );
        stats += std::string( "\nRT binds:" ) + std::to_string( System::Statistics::GetRenderTargetBindCount() );
        stats += std::string( "\nTexture binds:" ) + std::to_string( System::Statistics::GetTextureBindCount() );
        stats += std::string( "\nShader binds:" ) + std::to_string( System::Statistics::GetShaderBindCount() );
		statsContainer.GetComponent<TextRendererComponent>()->SetText( stats.c_str() );*/
		statsContainer.GetComponent<TextRendererComponent>()->SetText( System::Statistics::GetStatistics().c_str() );

        Window::SwapBuffers();
    }

    for (auto& mat : sponzaMaterialNameToMaterial)
    {
        delete mat.second;
    }
    
    for (auto& mesh : sponzaMeshes)
    {
        delete mesh;
    }
    
    for (auto& t : sponzaTextureNameToTexture)
    {
        delete t.second;
    }

	VR::Deinit();
    System::Deinit();
}
