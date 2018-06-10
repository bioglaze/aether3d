#include <string>
#include <cmath>
#include <cstdint>
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

// Assets for this sample (extract into aether3d_build/Samples): http://twiren.kapsi.fi/files/aether3d_sample_v0.7.8.zip

//#define TEST_RENDER_TEXTURE_2D
//#define TEST_RENDER_TEXTURE_CUBE
//#define TEST_VERTEX_LAYOUTS
//#define TEST_SHADOWS_DIR
//#define TEST_SHADOWS_SPOT
//#define TEST_SHADOWS_POINT
//#define TEST_FORWARD_PLUS

using namespace ae3d;

// *Really* minimal PCG32 code / (c) 2014 M.E. O'Neill / pcg-random.org
// Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)
struct pcg32_random_t
{
    std::uint64_t state;
    std::uint64_t inc;
};

std::uint32_t pcg32_random_r( pcg32_random_t* rng )
{
    std::uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + (rng->inc|1);
    std::uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    std::int32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

pcg32_random_t rng;

int Random100()
{
    return pcg32_random_r( &rng );
}

int main()
{
    bool fullScreen = false;

    const int originalWidth = 1920 / 1;
    const int originalHeight = 1080 / 1;
    int width = originalWidth;
    int height = originalHeight;

    if (fullScreen)
    {
        width = 0;
        height = 0;
    }
    
    System::EnableWindowsMemleakDetection();

    Window::Create( width, height, fullScreen ? WindowCreateFlags::Fullscreen : WindowCreateFlags::Empty );
    Window::GetSize( width, height );
    VR::Init();
    VR::StartTracking( width, height );
    Window::SetTitle( "Misc3D" );
    System::LoadBuiltinAssets();
    System::InitAudio();
    System::InitGamePad();

#ifdef AE3D_OPENVR
    VR::GetIdealWindowSize( width, height );
#endif

    RenderTexture cameraTex;
    cameraTex.Create2D( width, height, RenderTexture::DataType::Float, TextureWrap::Clamp, TextureFilter::Linear, "cameraTex" );

    RenderTexture camera2dTex;
    camera2dTex.Create2D( width, height, RenderTexture::DataType::Float, TextureWrap::Clamp, TextureFilter::Linear, "camera2dTex" );

    GameObject camera;
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

#ifdef TEST_RENDER_TEXTURE_CUBE
    RenderTexture cubeRT;
    cubeRT.CreateCube( 512, ae3d::RenderTexture::DataType::UByte, ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Linear, "cubeRT" );
    
    GameObject cameraCubeRT;
    cameraCubeRT.AddComponent<CameraComponent>();
    cameraCubeRT.GetComponent<CameraComponent>()->SetClearColor( Vec3( 0, 0, 1 ) );
    cameraCubeRT.GetComponent<CameraComponent>()->SetProjectionType( CameraComponent::ProjectionType::Perspective );
    cameraCubeRT.GetComponent<CameraComponent>()->SetProjection( 45, 1, 1, 400 );
    cameraCubeRT.GetComponent<CameraComponent>()->SetTargetTexture( &cubeRT );
    cameraCubeRT.GetComponent<CameraComponent>()->SetClearFlag( CameraComponent::ClearFlag::DepthAndColor );
    cameraCubeRT.AddComponent<TransformComponent>();
    cameraCubeRT.GetComponent<TransformComponent>()->LookAt( { 5, 0, -70 }, { 0, 0, -100 }, { 0, 1, 0 } );
#endif
    
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
    
    Texture2D fontTex;
    fontTex.Load( FileSystem::FileContents("font.png"), TextureWrap::Clamp, TextureFilter::Linear, Mipmaps::None, ColorSpace::SRGB, Anisotropy::k1 );
    
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

    Mesh cubeMeshScaledUV;
    //cubeMeshScaledUV.Load( FileSystem::FileContents( "cube_scaled_uv.ae3d" ) );

    GameObject cube;
    cube.AddComponent< MeshRendererComponent >();
    cube.GetComponent< MeshRendererComponent >()->SetMesh( &cubeMesh );
    cube.AddComponent< TransformComponent >();
    cube.GetComponent< TransformComponent >()->SetLocalPosition( { 0, 4, -80 } );

    GameObject rotatingCube;
    rotatingCube.AddComponent< MeshRendererComponent >();
    rotatingCube.GetComponent< MeshRendererComponent >()->SetMesh( &cubeMesh );
    rotatingCube.AddComponent< TransformComponent >();
    rotatingCube.GetComponent< TransformComponent >()->SetLocalPosition( { 0, 0, -100 } );
    rotatingCube.GetComponent< TransformComponent >()->SetLocalScale( 1 );

    GameObject childCube;
    childCube.AddComponent< MeshRendererComponent >();
    childCube.GetComponent< MeshRendererComponent >()->SetMesh( &cubeMesh );
    childCube.AddComponent< TransformComponent >();
    childCube.GetComponent< TransformComponent >()->SetLocalPosition( { 3, 0, 0 } );
    childCube.GetComponent< TransformComponent >()->SetParent( rotatingCube.GetComponent< TransformComponent >() );

    GameObject cubeScaledUV;
    cubeScaledUV.AddComponent< MeshRendererComponent >();
    cubeScaledUV.GetComponent< MeshRendererComponent >()->SetMesh( &cubeMeshScaledUV );
    cubeScaledUV.AddComponent< TransformComponent >();
    cubeScaledUV.GetComponent< TransformComponent >()->SetLocalPosition( { 0, 6, -84 } );

    Mesh cubeMesh2;
    cubeMesh2.Load( FileSystem::FileContents( "textured_cube.ae3d" ) );
    
    Mesh cubeMeshPTN; // Position-texcoord-normal
    cubeMeshPTN.Load( FileSystem::FileContents( "pnt_quads_2_meshes.ae3d" ) );

    Mesh animatedMesh;
    animatedMesh.Load( FileSystem::FileContents( "human_anim_test2.ae3d" ) );

    GameObject cubePTN;
    cubePTN.AddComponent< MeshRendererComponent >();
    cubePTN.GetComponent< MeshRendererComponent >()->SetMesh( &cubeMeshPTN );
    cubePTN.AddComponent< TransformComponent >();
    cubePTN.GetComponent< TransformComponent >()->SetLocalPosition( { 0, 4, -80 } );

#ifdef TEST_RENDER_TEXTURE_CUBE
    GameObject rtCube;
    rtCube.AddComponent< MeshRendererComponent >();
    rtCube.GetComponent< MeshRendererComponent >()->SetMesh( &cubeMesh2 );
    rtCube.AddComponent< TransformComponent >();
    rtCube.GetComponent< TransformComponent >()->SetLocalPosition( { 5, 0, -70 } );
#endif
    
    GameObject animatedGo;
    animatedGo.AddComponent< MeshRendererComponent >();
    animatedGo.GetComponent< MeshRendererComponent >()->SetMesh( &animatedMesh );
    animatedGo.AddComponent< TransformComponent >();
    animatedGo.GetComponent< TransformComponent >()->SetLocalPosition( { 13, -14, -80 } );
    animatedGo.GetComponent< TransformComponent >()->SetLocalScale( 0.0075f );
    animatedGo.GetComponent< TransformComponent >()->SetLocalRotation( Quaternion::FromEuler( { 180, 90, 0 } ) );
    
    Shader shader;
    shader.Load( FileSystem::FileContents( "unlit.vsh" ), FileSystem::FileContents( "unlit.fsh" ),
                 "unlitVert", "unlitFrag",
                 FileSystem::FileContents( "unlit_vert.obj" ), FileSystem::FileContents( "unlit_frag.obj" ),
                 FileSystem::FileContents( "unlit_vert.spv" ), FileSystem::FileContents( "unlit_frag.spv" ) );

    Shader shaderSkin;
    shaderSkin.Load( FileSystem::FileContents( "unlit_skin.vsh" ), FileSystem::FileContents( "unlit.fsh" ),
                "unlitVert", "unlitFrag",
                FileSystem::FileContents( "unlit_skin_vert.obj" ), FileSystem::FileContents( "unlit_frag.obj" ),
                FileSystem::FileContents( "unlit_skin_vert.spv" ), FileSystem::FileContents( "unlit_frag.spv" ) );

    Texture2D gliderTex;
    gliderTex.Load( FileSystem::FileContents( "glider.png" ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::SRGB, Anisotropy::k1 );

    Texture2D gliderClampTex;
    gliderClampTex.Load( FileSystem::FileContents( "font.png" ), TextureWrap::Clamp, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::SRGB, Anisotropy::k1 );

    Texture2D playerTex;
    playerTex.Load( FileSystem::FileContents( "player.png" ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::SRGB, Anisotropy::k1 );

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

    Material materialSkin;
    materialSkin.SetShader( &shaderSkin );
    materialSkin.SetTexture( "textureMap", &playerTex );
    materialSkin.SetVector( "tint", { 1, 1, 1, 1 } );

    cube.GetComponent< MeshRendererComponent >()->SetMaterial( &material, 0 );
    cubePTN.GetComponent< MeshRendererComponent >()->SetMaterial( &material, 0 );

    childCube.GetComponent< MeshRendererComponent >()->SetMaterial( &material, 0 );
    rotatingCube.GetComponent< MeshRendererComponent >()->SetMaterial( &material, 0 );
    cubeScaledUV.GetComponent< MeshRendererComponent >()->SetMaterial( &materialClamp, 0 );

    GameObject copiedCube = cube;
    copiedCube.GetComponent< TransformComponent >()->SetLocalPosition( { 0, 6, -80 } );
    copiedCube.GetComponent< MeshRendererComponent >()->SetMaterial( &material, 0 );

    animatedGo.GetComponent< MeshRendererComponent >()->SetMaterial( &materialSkin, 0 );
    
    Shader shaderCubeMap;
    shaderCubeMap.Load( FileSystem::FileContents( "unlit_cube.vsh" ), FileSystem::FileContents( "unlit_cube.fsh" ),
                        "unlitVert", "unlitFrag",
                        FileSystem::FileContents( "unlit_cube_vert.obj" ), FileSystem::FileContents( "unlit_cube_frag.obj" ),
                        FileSystem::FileContents( "unlit_vert.spv" ), FileSystem::FileContents( "unlit_frag.spv" ) );

    GameObject lightParent;
    //lightParent.AddComponent< MeshRendererComponent >();
    //lightParent.GetComponent< MeshRendererComponent >()->SetMesh( &cubeMesh );
    lightParent.AddComponent< TransformComponent >();
    lightParent.GetComponent< TransformComponent >()->SetLocalPosition( { 0, -2, -80 } );

    GameObject dirLight;
    dirLight.AddComponent<DirectionalLightComponent>();
#ifdef TEST_SHADOWS_DIR
    dirLight.GetComponent<DirectionalLightComponent>()->SetCastShadow( true, 512 );
#else
    dirLight.GetComponent<DirectionalLightComponent>()->SetCastShadow( false, 512 );
#endif
    dirLight.AddComponent<TransformComponent>();
    dirLight.GetComponent<TransformComponent>()->LookAt( { 0, 0, 0 }, Vec3( 0, -1, 0 ).Normalized(), { 0, 1, 0 } );

    GameObject spotLight;
    spotLight.AddComponent<SpotLightComponent>();
#ifdef TEST_SHADOWS_SPOT
    spotLight.GetComponent<SpotLightComponent>()->SetCastShadow( true, 1024 );
#else
    spotLight.GetComponent<SpotLightComponent>()->SetCastShadow( false, 1024 );
#endif
    spotLight.GetComponent<SpotLightComponent>()->SetConeAngle( 25 );
    spotLight.GetComponent<SpotLightComponent>()->SetColor( { 1, 0.5f, 0.5f } );
    spotLight.AddComponent<TransformComponent>();
    spotLight.GetComponent<TransformComponent>()->LookAt( { 0, -2, -80 }, { 0, -1, 0 }, { 0, 1, 0 } );
    //spotLight.GetComponent<TransformComponent>()->LookAt( { 0, 0, 0 }, { 0, -1, 0 }, { 0, 1, 0 } );
    //spotLight.GetComponent< TransformComponent >()->SetLocalPosition( { 4, 0, 0 } );
    //spotLight.GetComponent<TransformComponent>()->SetParent( lightParent.GetComponent< TransformComponent >() );

    GameObject pointLight;
    pointLight.AddComponent<PointLightComponent>();
#ifdef TEST_SHADOWS_POINT
    pointLight.GetComponent<PointLightComponent>()->SetCastShadow( true, 1024 );
#else
    pointLight.GetComponent<PointLightComponent>()->SetCastShadow( false, 1024 );
#endif
    pointLight.GetComponent<PointLightComponent>()->SetRadius( 1 );
    pointLight.AddComponent<TransformComponent>();
    pointLight.GetComponent<TransformComponent>()->SetLocalPosition( { 2, 0, -98 } );
    
#ifdef TEST_SHADOWS_POINT
    GameObject cube1;
    cube1.AddComponent< MeshRendererComponent >();
    cube1.GetComponent< MeshRendererComponent >()->SetMesh( &cubeMesh );
    cube1.AddComponent< TransformComponent >();
    cube1.GetComponent< TransformComponent >()->SetLocalPosition( { -20, 7, -80 } );
    cube1.GetComponent< TransformComponent >()->SetLocalScale( 5 );
    
    GameObject cube2;
    cube2.AddComponent< MeshRendererComponent >();
    cube2.GetComponent< MeshRendererComponent >()->SetMesh( &cubeMesh );
    cube2.AddComponent< TransformComponent >();
    cube2.GetComponent< TransformComponent >()->SetLocalPosition( { -30, 17, -80 } );
    cube2.GetComponent< TransformComponent >()->SetLocalScale( 5 );

    GameObject cube3;
    cube3.AddComponent< MeshRendererComponent >();
    cube3.GetComponent< MeshRendererComponent >()->SetMesh( &cubeMesh );
    cube3.AddComponent< TransformComponent >();
    cube3.GetComponent< TransformComponent >()->SetLocalPosition( { -30, 7, -70 } );
    cube3.GetComponent< TransformComponent >()->SetLocalScale( 5 );

    GameObject cube4;
    cube4.AddComponent< MeshRendererComponent >();
    cube4.GetComponent< MeshRendererComponent >()->SetMesh( &cubeMesh );
    cube4.AddComponent< TransformComponent >();
    cube4.GetComponent< TransformComponent >()->SetLocalPosition( { -30, 7, -70 } );
    cube4.GetComponent< TransformComponent >()->SetLocalScale( 5 );

    GameObject smallCube1;
    smallCube1.AddComponent< MeshRendererComponent >();
    smallCube1.GetComponent< MeshRendererComponent >()->SetMesh( &cubeMesh );
    smallCube1.AddComponent< TransformComponent >();
    smallCube1.GetComponent< TransformComponent >()->SetLocalPosition( { -25, 2, -80 } );
    smallCube1.GetComponent< TransformComponent >()->SetLocalScale( 1 );

#endif

    Scene scene;
    
    TextureCube skybox;
    skybox.Load( FileSystem::FileContents( "skybox/left.jpg" ), FileSystem::FileContents( "skybox/right.jpg" ),
                 FileSystem::FileContents( "skybox/bottom.jpg" ), FileSystem::FileContents( "skybox/top.jpg" ),
                 FileSystem::FileContents( "skybox/front.jpg" ), FileSystem::FileContents( "skybox/back.jpg" ),
                 TextureWrap::Clamp, TextureFilter::Linear, Mipmaps::None, ColorSpace::RGB );
    /*skybox.Load( FileSystem::FileContents( "test_dxt1.dds" ), FileSystem::FileContents( "test_dxt1.dds" ),
        FileSystem::FileContents( "test_dxt1.dds" ), FileSystem::FileContents( "test_dxt1.dds" ),
        FileSystem::FileContents( "test_dxt1.dds" ), FileSystem::FileContents( "test_dxt1.dds" ),
        TextureWrap::Clamp, TextureFilter::Linear, Mipmaps::None, ColorSpace::RGB );*/
#ifdef TEST_RENDER_TEXTURE_CUBE
    Material materialCubeRT;
    materialCubeRT.SetShader( &shaderCubeMap );
    materialCubeRT.SetRenderTexture( "skyMap", &cubeRT );
    materialCubeRT.SetVector( "tint", { 1, 1, 1, 1 } );
    materialCubeRT.SetBackFaceCulling( true );

    rtCube.GetComponent< MeshRendererComponent >()->SetMaterial( &materialCubeRT, 0 );
#endif

#ifdef TEST_FORWARD_PLUS
    Shader standardShader;
    standardShader.Load( ae3d::FileSystem::FileContents( "standard.vsh" ), ae3d::FileSystem::FileContents( "standard.fsh" ),
        "standard_vertex", "standard_fragment",
        ae3d::FileSystem::FileContents( "Standard_vert.obj" ), ae3d::FileSystem::FileContents( "Standard_frag.obj" ),
        ae3d::FileSystem::FileContents( "Standard_vert.spv" ), ae3d::FileSystem::FileContents( "Standard_frag.spv" ) );

    Material standardMaterial;
    standardMaterial.SetShader( &standardShader );
    standardMaterial.SetTexture( "textureMap", &gliderTex );

    GameObject standardCubeTopCenter;
    standardCubeTopCenter.SetName( "standardCubeTopCenter" );
    standardCubeTopCenter.AddComponent<ae3d::MeshRendererComponent>();
    standardCubeTopCenter.GetComponent<ae3d::MeshRendererComponent>()->SetMesh( &cubeMesh );
    standardCubeTopCenter.GetComponent<ae3d::MeshRendererComponent>()->SetMaterial( &standardMaterial, 0 );
    standardCubeTopCenter.AddComponent<ae3d::TransformComponent>();
    standardCubeTopCenter.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( 2, 0, -100 ) );
    standardCubeTopCenter.GetComponent<ae3d::TransformComponent>()->SetLocalScale( 2 );
    scene.Add( &standardCubeTopCenter );

    const int POINT_LIGHT_COUNT = 50 * 40;
    
    GameObject pointLights[ POINT_LIGHT_COUNT ];
    
    // Inits point lights for Forward+
    {
        int pointLightIndex = 0;
        

        for (int row = 0; row < 50; ++row)
        {
            for (int col = 0; col < 40; ++col)
            {
                pointLights[ pointLightIndex ].AddComponent<ae3d::PointLightComponent>();
                pointLights[ pointLightIndex ].GetComponent<ae3d::PointLightComponent>()->SetRadius( 3 );
                pointLights[ pointLightIndex ].GetComponent<ae3d::PointLightComponent>()->SetColor( { (Random100() % 100 ) / 100.0f, (Random100() % 100) / 100.0f, (Random100() % 100) / 100.0f } );
                pointLights[ pointLightIndex ].AddComponent<ae3d::TransformComponent>();
                pointLights[ pointLightIndex ].GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( -150 + (float)row * 5, -12, -150 + (float)col * 4 ) );

                scene.Add( &pointLights[ pointLightIndex ] );
                ++pointLightIndex;
            }
        }
    }
#endif

    // Sponza can be downloaded from http://twiren.kapsi.fi/files/aether3d_sponza.zip and extracted into aether3d_build/Samples
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
#ifdef TEST_FORWARD_PLUS
        mat.second->SetShader( &standardShader );
#else
        mat.second->SetShader( &shader );
#endif
    }
    
    for (std::size_t i = 0; i < sponzaGameObjects.size(); ++i)
    {
        scene.Add( &sponzaGameObjects[ i ] );
    }
#endif
    // Sponza ends
    
    RenderTexture rtTex;
    rtTex.Create2D( 512, 512, RenderTexture::DataType::UByte, TextureWrap::Clamp, TextureFilter::Linear, "rtTex" );
    
    GameObject renderTextureContainer;
    renderTextureContainer.SetName( "renderTextureContainer" );
    renderTextureContainer.AddComponent<SpriteRendererComponent>();
#ifdef TEST_RENDER_TEXTURE_2D
    renderTextureContainer.GetComponent<SpriteRendererComponent>()->SetTexture( &rtTex, Vec3( 150, 250, -0.6f ), Vec3( 512, 512, 1 ), Vec4( 1, 1, 1, 1 ) );
#endif
    //renderTextureContainer.GetComponent<SpriteRendererComponent>()->SetTexture( &camera.GetComponent< CameraComponent >()->GetDepthNormalsTexture(), Vec3( 150, 250, -0.6f ), Vec3( 256, 256, 1 ), Vec4( 1, 1, 1, 1 ) );
    renderTextureContainer.SetLayer( 2 );

    GameObject rtCamera;
    rtCamera.SetName( "RT camera" );
    rtCamera.AddComponent<CameraComponent>();
    //rtCamera.GetComponent<CameraComponent>()->SetProjection( 0, (float)rtTex.GetWidth(), 0,(float)rtTex.GetHeight(), 0, 1 );
    rtCamera.GetComponent<CameraComponent>()->SetProjection( 45, (float)width / (float)height, 1, 200 );
    rtCamera.GetComponent<CameraComponent>()->SetProjectionType( CameraComponent::ProjectionType::Perspective );
    rtCamera.GetComponent<CameraComponent>()->SetClearColor( Vec3( 0, 0, 0 ) );
    rtCamera.GetComponent<CameraComponent>()->SetTargetTexture( &rtTex );
    rtCamera.AddComponent<TransformComponent>();
    //rtCamera.GetComponent<TransformComponent>()->SetLocalPosition( Vec3( 0, 0, -70 ) );
    
    GameObject rtCameraParent;
    rtCameraParent.AddComponent<TransformComponent>();
    rtCameraParent.GetComponent<TransformComponent>()->SetLocalPosition( Vec3( 0, 0, -70 ) );
    rtCamera.GetComponent<TransformComponent>()->SetParent( rtCameraParent.GetComponent<TransformComponent>() );
    
    Texture2D transTex;
    transTex.Load( FileSystem::FileContents( "trans50.png" ), TextureWrap::Repeat, TextureFilter::Linear, Mipmaps::None, ColorSpace::SRGB, Anisotropy::k1 );

    Material transMaterial;
    transMaterial.SetShader( &shader );
    transMaterial.SetTexture( "textureMap", &transTex );
    transMaterial.SetVector( "tint", { 1, 1, 1, 1 } );
    transMaterial.SetBackFaceCulling( true );
    transMaterial.SetBlendingMode( Material::BlendingMode::Alpha );
    
    GameObject transCube1;
    transCube1.SetName( "transCube1" );
    transCube1.AddComponent< MeshRendererComponent >();
    transCube1.GetComponent< MeshRendererComponent >()->SetMesh( &cubeMesh );
    transCube1.GetComponent< MeshRendererComponent >()->SetMaterial( &transMaterial, 0 );
    transCube1.AddComponent< TransformComponent >();
    transCube1.GetComponent< TransformComponent >()->SetLocalPosition( { 2, 0, -70 } );
    
    scene.SetSkybox( &skybox );
    scene.Add( &camera );
#ifndef AE3D_OPENVR
    scene.Add( &camera2d );
    scene.Add( &statsContainer );
#endif
#ifdef TEST_RENDER_TEXTURE_CUBE
    scene.Add( &rtCube );
    scene.Add( &cameraCubeRT );
#endif
    //scene.Add( &cubeScaledUV );
    scene.Add( &lightParent );
#ifdef TEST_SHADOWS_POINT
    cube1.GetComponent< MeshRendererComponent >()->SetMaterial( &material, 0 );
    cube2.GetComponent< MeshRendererComponent >()->SetMaterial( &material, 0 );
    cube3.GetComponent< MeshRendererComponent >()->SetMaterial( &material, 0 );
    cube4.GetComponent< MeshRendererComponent >()->SetMaterial( &material, 0 );
    smallCube1.GetComponent< MeshRendererComponent >()->SetMaterial( &material, 0 );
    scene.Add( &cube1 );
    scene.Add( &cube2 );
    scene.Add( &cube3 );
    scene.Add( &cube4 );
    scene.Add( &smallCube1 );
#endif
#ifdef TEST_VERTEX_LAYOUTS
    scene.Add( &cubePTN );
#endif
    scene.Add( &animatedGo );
    scene.Add( &cubePTN );
    //scene.Add( &childCube );
    //scene.Add( &copiedCube );
    scene.Add( &rotatingCube );
    
#ifdef TEST_SHADOWS_POINT
    scene.Add( &pointLight );
#endif
#ifdef TEST_SHADOWS_DIR
    scene.Add( &dirLight );
#endif
    scene.Add( &spotLight );
#ifdef TEST_RENDER_TEXTURE_2D
    scene.Add( &renderTextureContainer );
    scene.Add( &rtCamera );
#endif
    //scene.Add( &transCube1 );

    const int cubeCount = 10;
    GameObject cubes[ cubeCount ];

    for (int i = 0; i < cubeCount; ++i)
    {
        cubes[ i ].AddComponent< MeshRendererComponent >();
        cubes[ i ].GetComponent< MeshRendererComponent >()->SetMesh( &cubeMesh );
        cubes[ i ].AddComponent< TransformComponent >();
        cubes[ i ].GetComponent< TransformComponent >()->SetLocalPosition( { i * 4.5f - 4, 0, -100 } );
        cubes[ i ].GetComponent< MeshRendererComponent >()->SetMaterial( &material, 0 );

        //scene.Add( &cubes[ i ] );
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
    //cubes[ 4 ].GetComponent<AudioSourceComponent>()->Play();

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
        cubes[ 2 ].GetComponent< TransformComponent >()->SetLocalRotation( rotation );

        lightParent.GetComponent< TransformComponent >()->SetLocalRotation( rotation );
        //spotLight.GetComponent< TransformComponent >()->SetLocalRotation( rotation );

        axis = Vec3( 1, 1, 1 ).Normalized();
        rotation.FromAxisAngle( axis, angle );
        rotatingCube.GetComponent< TransformComponent >()->SetLocalRotation( rotation );

        axis = Vec3( 0, 1, 0 ).Normalized();
        rotation.FromAxisAngle( axis, angle );
        rtCamera.GetComponent<TransformComponent>()->SetLocalRotation( rotation );

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
                    //System::ReloadChangedAssets();
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

        static int animationFrame = 0;
        ++animationFrame;
        animatedGo.GetComponent< MeshRendererComponent >()->SetAnimationFrame( animationFrame );

        /*std::string stats = std::string( "draw calls:" ) + std::to_string( System::Statistics::GetDrawCallCount() );
        stats += std::string( "\nVAO binds:" ) + std::to_string( System::Statistics::GetVertexBufferBindCount() );
        stats += std::string( "\nRT binds:" ) + std::to_string( System::Statistics::GetRenderTargetBindCount() );
        stats += std::string( "\nTexture binds:" ) + std::to_string( System::Statistics::GetTextureBindCount() );
        stats += std::string( "\nShader binds:" ) + std::to_string( System::Statistics::GetShaderBindCount() );
        statsContainer.GetComponent<TextRendererComponent>()->SetText( stats.c_str() );*/
        if (animationFrame % 60 == 0)
        {
            statsContainer.GetComponent<TextRendererComponent>()->SetText( System::Statistics::GetStatistics().c_str() );
        }
        
#ifdef TEST_FORWARD_PLUS
        static float y = -14;
        y += 0.1f;

        if (y > 30)
        {
            y = -14;
        }

        for (int pointLightIndex = 0; pointLightIndex < POINT_LIGHT_COUNT; ++pointLightIndex)
        {
            const Vec3 oldPos = pointLights[ pointLightIndex ].GetComponent<ae3d::TransformComponent>()->GetLocalPosition();
            const float xOffset = (Random100() % 10) / 20.0f - (Random100() % 10) / 20.0f;
            const float yOffset = (Random100() % 10) / 20.0f - (Random100() % 10) / 20.0f;
            
            pointLights[ pointLightIndex ].GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( oldPos.x + xOffset, -18, oldPos.z + yOffset ) );
        }
#endif
#if defined( AE3D_OPENVR )
        VR::CalcEyePose();
        
        cube.GetComponent< TransformComponent >()->SetLocalPosition( camera.GetComponent< TransformComponent >()->GetWorldPosition() + VR::GetLeftHandPosition() );
        Vec3 pos = VR::GetLeftHandPosition();
        //System::Print( "left hand pos: %f, %f, %f\n", pos.x, pos.y, pos.z );
        camera.GetComponent< CameraComponent >()->SetViewport( 0, 0, width, height );
        
        for (int eye = 0; eye < 2; ++eye)
        {
            VR::SetEye( eye );
            VR::CalcCameraForEye( camera, yaw, eye );
            scene.Render();
            scene.EndFrame();
            VR::UnsetEye( eye );
        }
        
        VR::SubmitFrame();
#else
        if (reload)
        {
            System::Print("reloading\n");
            System::ReloadChangedAssets();
            reload = false;
        }
        scene.Render();
#if RENDERER_D3D12
        System::Draw( &cameraTex, 0, 0, width, originalHeight, width, originalHeight, Vec4( 1, 1, 1, 1 ), false );
        System::Draw( &camera2dTex, 0, 0, width, originalHeight, width, originalHeight, Vec4( 1, 1, 1, 1 ), true );
#else
        System::Draw( &cameraTex, 0, 0, width, height, width, height, Vec4( 1, 1, 1, 1 ), false );
        System::Draw( &camera2dTex, 0, 0, width, height, width, height, Vec4( 1, 1, 1, 1 ), true );
#endif
        scene.EndFrame();
#endif
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
