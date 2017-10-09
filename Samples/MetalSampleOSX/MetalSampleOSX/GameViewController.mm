// This sample's assets are referenced from aether3d_build/Samples. Make sure that they exist.
// Assets can be downloaded from http://twiren.kapsi.fi/files/aether3d_sample_v0.7.zip
// If you didn't download a release of Aether3D, some referenced assets could be missing,
// just remove the references to build.
#import "GameViewController.h"
#import <Metal/Metal.h>
#import <simd/simd.h>
#import <MetalKit/MetalKit.h>
#include <cmath>
#include <vector>
#include <map>

#import "CameraComponent.hpp"
#import "SpriteRendererComponent.hpp"
#import "TextRendererComponent.hpp"
#import "DirectionalLightComponent.hpp"
#import "PointLightComponent.hpp"
#import "SpotLightComponent.hpp"
#import "SpriteRendererComponent.hpp"
#import "MeshRendererComponent.hpp"
#import "TransformComponent.hpp"
#import "System.hpp"
#import "Font.hpp"
#import "FileSystem.hpp"
#import "GameObject.hpp"
#import "Material.hpp"
#import "Mesh.hpp"
#import "Texture2D.hpp"
#import "TextureCube.hpp"
#import "Shader.hpp"
#import "Scene.hpp"
#import "Window.hpp"

//#define TEST_FORWARD_PLUS
//#define TEST_SHADOWS_DIR
//#define TEST_SHADOWS_SPOT
//#define TEST_SHADOWS_POINT
//#define TEST_NUKLEAR_UI

#define POINT_LIGHT_COUNT 100
#define MULTISAMPLE_COUNT 1

#ifdef TEST_NUKLEAR_UI
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#include "nuklear.h"

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

struct VertexPTC
{
    float position[ 3 ];
    float uv[ 2 ];
    float col[ 4 ];
};

nk_draw_null_texture nullTexture;
nk_font* nkFont = nullptr;
std::map< int, ae3d::Texture2D* > uiTextures;

void DrawNuklear( nk_context* ctx, nk_buffer* uiCommands, int width, int height )
{
    struct nk_convert_config config;
    static const struct nk_draw_vertex_layout_element vertex_layout[] = {
        {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct VertexPTC, position)},
        {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct VertexPTC, uv)},
        {NK_VERTEX_COLOR, NK_FORMAT_R32G32B32A32_FLOAT, NK_OFFSETOF(struct VertexPTC, col)},
        {NK_VERTEX_LAYOUT_END}
    };
    
    NK_MEMSET( &config, 0, sizeof( config ) );
    config.vertex_layout = vertex_layout;
    config.vertex_size = sizeof( struct VertexPTC );
    config.vertex_alignment = NK_ALIGNOF( struct VertexPTC );
    config.null = nullTexture;
    config.circle_segment_count = 22;
    config.curve_segment_count = 22;
    config.arc_segment_count = 22;
    config.global_alpha = 1.0f;
    config.shape_AA = NK_ANTI_ALIASING_OFF;
    config.line_AA = NK_ANTI_ALIASING_OFF;
    
    void* vertices = nullptr;
    void* elements = nullptr;
    ae3d::System::MapUIVertexBuffer( MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY, &vertices, &elements );
    
    nk_buffer vbuf, ebuf;
    nk_buffer_init_fixed( &vbuf, vertices, MAX_VERTEX_MEMORY );
    nk_buffer_init_fixed( &ebuf, elements, MAX_ELEMENT_MEMORY );
    nk_convert( ctx, uiCommands, &vbuf, &ebuf, &config );
    
    ae3d::System::UnmapUIVertexBuffer();
    
    const struct nk_draw_command* cmd = nullptr;
    nk_draw_index* offset = nullptr;
    const float scaleX = 6;
    const float scaleY = 3;
    
    nk_draw_foreach( cmd, ctx, uiCommands )
    {
        if (cmd->elem_count == 0)
        {
            continue;
        }
        
        ae3d::System::DrawUI( (int)(cmd->clip_rect.x * scaleX),
                       (int)((height - (int)(cmd->clip_rect.y + cmd->clip_rect.h)) * scaleY),
                       (int)(cmd->clip_rect.w * scaleX),
                       (int)(cmd->clip_rect.h * scaleY),
                       cmd->elem_count, uiTextures[ cmd->texture.id ], offset, width, height );
        offset += cmd->elem_count;
    }
    
    nk_clear( ctx );
}
#endif

static const NSUInteger kMaxBuffersInFlight = 3;

using namespace ae3d;

@implementation GameViewController
{
    MTKView* _view;
    id <MTLDevice> device;
    id <MTLCommandQueue> commandQueue;
    dispatch_semaphore_t inFlightSemaphore;
    
    GameObject camera2d;
    GameObject camera3d;
    GameObject rotatingCube;
    GameObject bigCube;
    GameObject bigCube2;
    GameObject bigCube3;
    GameObject text;
    GameObject textSDF;
    GameObject dirLight;
    GameObject spotLight;
    GameObject pointLight;
    GameObject rtCamera;
    GameObject rtCube;
    GameObject renderTextureContainer;
    GameObject cubePTN; // vertex format: position, texcoord, normal
    GameObject cubePTN2;
    GameObject standardCubeBL; // bottom left
    GameObject standardCubeTL;
    GameObject standardCubeTL2;
    GameObject standardCubeTopCenter;
    GameObject standardCubeSpotReceiver;
    GameObject standardCubeBR;
    GameObject standardCubeTR;
    GameObject spriteContainer;
    GameObject cameraCubeRT;
    GameObject animatedGo;
    Scene scene;
    Font font;
    Font fontSDF;
    Mesh cubeMesh;
    Mesh cubeMeshPTN;
    Mesh animatedMesh;
    Material cubeMaterial;
    Material skinMaterial;
    Material rtCubeMaterial;
    Material transMaterial;
    Material standardMaterial;
    Shader shader;
    Shader skinShader;
    Shader skyboxShader;
    Shader standardShader;
    Texture2D fontTex;
    Texture2D fontTexSDF;
    Texture2D gliderTex;
    Texture2D transTex;
    Texture2D bc1Tex;
    Texture2D bc2Tex;
    Texture2D bc3Tex;
    TextureCube skyTex;
    RenderTexture rtTex;
    RenderTexture cubeRT;
    std::vector< GameObject > sponzaGameObjects;
    std::map< std::string, Material* > sponzaMaterialNameToMaterial;
    std::map< std::string, Texture2D* > sponzaTextureNameToTexture;
    std::vector< Mesh* > sponzaMeshes;
    
    Matrix44 lineView;
    Matrix44 lineProjection;
    int lineHandle;
    
    Scene scene2;
    GameObject bigCubeInScene2;
    GameObject pointLights[ POINT_LIGHT_COUNT ];
    
#ifdef TEST_NUKLEAR_UI
    nk_context ctx;
    nk_font_atlas atlas;
    int atlasWidth;
    int atlasHeight;
    Texture2D nkFontTexture;
    nk_buffer cmds;
#endif
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    device = MTLCreateSystemDefaultDevice();
    assert( device );

    [self _setupView];
    [self _reshape];
    //self.nextResponder = super.nextResponder;

    ae3d::System::InitMetal( device, _view, MULTISAMPLE_COUNT );
    ae3d::System::LoadBuiltinAssets();
    //ae3d::System::InitAudio();

    // Sponza can be downloaded from http://twiren.kapsi.fi/files/aether3d_sponza.zip and extracted into aether3d_build/Samples
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
    }

    for (std::size_t i = 0; i < sponzaGameObjects.size(); ++i)
    {
        scene.Add( &sponzaGameObjects[ i ] );
    }
#endif

    camera2d.SetName( "Camera2D" );
    camera2d.AddComponent<ae3d::CameraComponent>();
    camera2d.GetComponent<ae3d::CameraComponent>()->SetProjection( 0, self.view.bounds.size.width, self.view.bounds.size.height, 0, 0, 1 );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetProjectionType( ae3d::CameraComponent::ProjectionType::Orthographic );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetClearFlag( ae3d::CameraComponent::ClearFlag::Depth );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetClearColor( ae3d::Vec3( 0.5f, 0.0f, 0.0f ) );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetLayerMask( 0x2 );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetRenderOrder( 2 );
    camera2d.AddComponent<ae3d::TransformComponent>();

    const float aspect = _view.bounds.size.width / (float)_view.bounds.size.height;

    camera3d.SetName( "Camera3D" );
    camera3d.AddComponent<ae3d::CameraComponent>();
    camera3d.GetComponent<ae3d::CameraComponent>()->SetProjection( 45, aspect, 1, 200 );
    camera3d.GetComponent<ae3d::CameraComponent>()->SetClearColor( ae3d::Vec3( 0.5f, 0.5f, 0.5f ) );
    camera3d.GetComponent<ae3d::CameraComponent>()->SetClearFlag( ae3d::CameraComponent::ClearFlag::DepthAndColor );
    camera3d.GetComponent<ae3d::CameraComponent>()->SetProjectionType( ae3d::CameraComponent::ProjectionType::Perspective );
    camera3d.GetComponent<ae3d::CameraComponent>()->SetRenderOrder( 1 );
    //camera3d.GetComponent<ae3d::CameraComponent>()->SetViewport( 0, 0, 640, 480 );
#ifdef TEST_FORWARD_PLUS
    camera3d.GetComponent<ae3d::CameraComponent>()->GetDepthNormalsTexture().Create2D( self.view.bounds.size.width * 2, self.view.bounds.size.height * 2,
                                                                                      ae3d::RenderTexture::DataType::Float, ae3d::TextureWrap::Clamp, ae3d::TextureFilter::Nearest, "depthnormals" );
#endif
    camera3d.AddComponent<ae3d::TransformComponent>();
    camera3d.GetComponent<TransformComponent>()->LookAt( { 20, 0, -85 }, { 120, 0, -85 }, { 0, 1, 0 } );

    //scene.Add( &camera2d );
    scene.Add( &camera3d );
    scene2.Add( &camera3d );
    
    fontTex.Load( ae3d::FileSystem::FileContents( "/font.png" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Nearest, ae3d::Mipmaps::None, ae3d::ColorSpace::RGB, ae3d::Anisotropy::k1 );
    // TODO: SDF texture
    fontTexSDF.Load( ae3d::FileSystem::FileContents( "/font.png" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Nearest, ae3d::Mipmaps::None, ae3d::ColorSpace::RGB, ae3d::Anisotropy::k1 );
    gliderTex.Load( ae3d::FileSystem::FileContents( "/glider.png" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Linear, ae3d::Mipmaps::None, ae3d::ColorSpace::RGB, ae3d::Anisotropy::k1 );
    bc1Tex.Load( ae3d::FileSystem::FileContents( "/test_dxt1.dds" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Nearest, ae3d::Mipmaps::None, ae3d::ColorSpace::RGB, ae3d::Anisotropy::k1 );
    bc2Tex.Load( ae3d::FileSystem::FileContents( "/test_dxt3.dds" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Nearest, ae3d::Mipmaps::None, ae3d::ColorSpace::RGB, ae3d::Anisotropy::k1 );
    bc3Tex.Load( ae3d::FileSystem::FileContents( "/test_dxt5.dds" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Nearest, ae3d::Mipmaps::None, ae3d::ColorSpace::RGB, ae3d::Anisotropy::k1 );
    skyTex.Load( ae3d::FileSystem::FileContents( "/left.jpg" ), ae3d::FileSystem::FileContents( "/right.jpg" ),
                ae3d::FileSystem::FileContents( "/bottom.jpg" ), ae3d::FileSystem::FileContents( "/top.jpg" ),
                ae3d::FileSystem::FileContents( "/front.jpg" ), ae3d::FileSystem::FileContents( "/back.jpg" ),
                ae3d::TextureWrap::Clamp, ae3d::TextureFilter::Linear, ae3d::Mipmaps::Generate, ae3d::ColorSpace::RGB );
    
    /*skyTex.Load( FileSystem::FileContents( "/test_dxt1.dds" ), FileSystem::FileContents( "/test_dxt1.dds" ),
                FileSystem::FileContents( "/test_dxt1.dds" ), FileSystem::FileContents( "/test_dxt1.dds" ),
                FileSystem::FileContents( "/test_dxt1.dds" ), FileSystem::FileContents( "/test_dxt1.dds" ),
                TextureWrap::Clamp, TextureFilter::Linear, Mipmaps::Generate, ColorSpace::RGB );*/
    scene.SetSkybox( &skyTex );
    
    font.LoadBMFont( &fontTex, ae3d::FileSystem::FileContents( "/font_txt.fnt" ) );
    fontSDF.LoadBMFont( &fontTexSDF, ae3d::FileSystem::FileContents( "/font_txt.fnt" ) );

    text.AddComponent<ae3d::TextRendererComponent>();
    text.GetComponent<ae3d::TextRendererComponent>()->SetText( "Aether3D Game Engine" );
    text.GetComponent<ae3d::TextRendererComponent>()->SetFont( &font );
    text.GetComponent<ae3d::TextRendererComponent>()->SetColor( ae3d::Vec4( 0, 1, 0, 1 ) );
    text.AddComponent<ae3d::TransformComponent>();
    text.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( 5, 5, 0 ) );
    text.SetLayer( 2 );

    textSDF.AddComponent<ae3d::TextRendererComponent>();
    textSDF.GetComponent<ae3d::TextRendererComponent>()->SetText( "This is SDF text" );
    textSDF.GetComponent<ae3d::TextRendererComponent>()->SetFont( &fontSDF );
    textSDF.GetComponent<ae3d::TextRendererComponent>()->SetShader( ae3d::TextRendererComponent::ShaderType::SDF );
    textSDF.GetComponent<ae3d::TextRendererComponent>()->SetColor( ae3d::Vec4( 0, 1, 0, 1 ) );
    textSDF.AddComponent<ae3d::TransformComponent>();
    textSDF.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( 450, 5, 0 ) );
    textSDF.SetLayer( 2 );

    spriteContainer.AddComponent<ae3d::SpriteRendererComponent>();
    auto sprite = spriteContainer.GetComponent<SpriteRendererComponent>();
    sprite->SetTexture( &bc1Tex, Vec3( 120, 100, -0.6f ), Vec3( (float)bc1Tex.GetWidth(), (float)bc1Tex.GetHeight(), 1 ), Vec4( 1, 1, 1, 0.5f ) );
    sprite->SetTexture( &bc2Tex, Vec3( 120, 200, -0.5f ), Vec3( (float)bc2Tex.GetWidth(), (float)bc2Tex.GetHeight(), 1 ), Vec4( 1, 1, 1, 0.5f ) );
    sprite->SetTexture( &bc3Tex, Vec3( 120, 300, -0.5f ), Vec3( (float)bc3Tex.GetWidth(), (float)bc3Tex.GetHeight(), 1 ), Vec4( 1, 1, 1, 0.5f ) );
    sprite->SetTexture( &gliderTex, Vec3( 220, 120, -0.5f ), Vec3( (float)gliderTex.GetWidth(), (float)gliderTex.GetHeight(), 1 ), Vec4( 1, 1, 1, 1 ) );
    
    spriteContainer.AddComponent<TransformComponent>();
    //spriteContainer.GetComponent<TransformComponent>()->SetLocalPosition( Vec3( 20, 0, 0 ) );
    spriteContainer.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( 5, 5, 0 ) );
    spriteContainer.SetLayer( 2 );

    shader.Load( ae3d::FileSystem::FileContents( "" ), ae3d::FileSystem::FileContents( "" ),
                "unlit_vertex", "unlit_fragment",
                ae3d::FileSystem::FileContents(""), ae3d::FileSystem::FileContents( "" ),
                ae3d::FileSystem::FileContents(""), ae3d::FileSystem::FileContents( "" ));

    skinShader.Load( ae3d::FileSystem::FileContents( "" ), ae3d::FileSystem::FileContents( "" ),
                "unlit_skin_vertex", "unlit_skin_fragment",
                ae3d::FileSystem::FileContents(""), ae3d::FileSystem::FileContents( "" ),
                ae3d::FileSystem::FileContents(""), ae3d::FileSystem::FileContents( "" ));

    cubeMaterial.SetShader( &shader );
    cubeMaterial.SetTexture( "textureMap", &gliderTex );
    //cubeMaterial.SetRenderTexture( "textureMap", &camera3d.GetComponent<ae3d::CameraComponent>()->GetDepthNormalsTexture() );
    cubeMaterial.SetVector( "tint", { 1, 1, 1, 1 } );

    skinMaterial.SetShader( &skinShader );
    skinMaterial.SetTexture( "textureMap", &gliderTex );

    rtCubeMaterial.SetShader( &skyboxShader );
    rtCubeMaterial.SetRenderTexture( "skyMap", &cubeRT );
    rtCubeMaterial.SetBackFaceCulling( false );
    
    standardShader.Load( ae3d::FileSystem::FileContents( "" ), ae3d::FileSystem::FileContents( "" ),
                "standard_vertex", "standard_fragment",
                ae3d::FileSystem::FileContents(""), ae3d::FileSystem::FileContents( "" ),
                ae3d::FileSystem::FileContents(""), ae3d::FileSystem::FileContents( "" ));
    standardMaterial.SetShader( &standardShader );
    standardMaterial.SetTexture( "textureMap", &gliderTex );

    skyboxShader.Load( ae3d::FileSystem::FileContents( "" ), ae3d::FileSystem::FileContents( "" ),
                        "skybox_vertex", "skybox_fragment",
                        ae3d::FileSystem::FileContents(""), ae3d::FileSystem::FileContents( "" ),
                        ae3d::FileSystem::FileContents(""), ae3d::FileSystem::FileContents( "" ));

    cubeMesh.Load( ae3d::FileSystem::FileContents( "/textured_cube.ae3d" ) );
    rotatingCube.AddComponent<ae3d::MeshRendererComponent>();
    rotatingCube.GetComponent<ae3d::MeshRendererComponent>()->SetMesh( &cubeMesh );
    rotatingCube.GetComponent<ae3d::MeshRendererComponent>()->SetMaterial( &cubeMaterial, 0 );
    rotatingCube.AddComponent<ae3d::TransformComponent>();
    rotatingCube.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( 2, 0, -8 ) );
    rotatingCube.GetComponent<ae3d::TransformComponent>()->SetLocalScale( 1 );

    rtCube.AddComponent<ae3d::MeshRendererComponent>();
    rtCube.GetComponent<ae3d::MeshRendererComponent>()->SetMesh( &cubeMesh );
    rtCube.GetComponent<ae3d::MeshRendererComponent>()->SetMaterial( &rtCubeMaterial, 0 );
    rtCube.AddComponent<ae3d::TransformComponent>();
    rtCube.GetComponent< TransformComponent >()->SetLocalPosition( { -5, 0, -85 } );
    rtCube.GetComponent<ae3d::TransformComponent>()->SetLocalScale( 1 );

    standardCubeBL.AddComponent<ae3d::MeshRendererComponent>();
    standardCubeBL.GetComponent<ae3d::MeshRendererComponent>()->SetMesh( &cubeMesh );
    standardCubeBL.GetComponent<ae3d::MeshRendererComponent>()->SetMaterial( &standardMaterial, 0 );
    standardCubeBL.AddComponent<ae3d::TransformComponent>();
    standardCubeBL.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( -4, -4, -10 ) );

    standardCubeTL.AddComponent<ae3d::MeshRendererComponent>();
    standardCubeTL.GetComponent<ae3d::MeshRendererComponent>()->SetMesh( &cubeMesh );
    standardCubeTL.GetComponent<ae3d::MeshRendererComponent>()->SetMaterial( &standardMaterial, 0 );
    standardCubeTL.AddComponent<ae3d::TransformComponent>();
    standardCubeTL.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( -4, 4, -10 ) );

    standardCubeTL2.AddComponent<ae3d::MeshRendererComponent>();
    standardCubeTL2.GetComponent<ae3d::MeshRendererComponent>()->SetMesh( &cubeMesh );
    standardCubeTL2.GetComponent<ae3d::MeshRendererComponent>()->SetMaterial( &standardMaterial, 0 );
    standardCubeTL2.AddComponent<ae3d::TransformComponent>();
    standardCubeTL2.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( -2, 4, -10 ) );

    standardCubeTopCenter.AddComponent<ae3d::MeshRendererComponent>();
    standardCubeTopCenter.GetComponent<ae3d::MeshRendererComponent>()->SetMesh( &cubeMesh );
    standardCubeTopCenter.GetComponent<ae3d::MeshRendererComponent>()->SetMaterial( &standardMaterial, 0 );
    standardCubeTopCenter.AddComponent<ae3d::TransformComponent>();
    standardCubeTopCenter.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( -10, 0, -85 ) );
    //standardCubeTopCenter.GetComponent<ae3d::TransformComponent>()->SetLocalScale( 2 );

    standardCubeSpotReceiver.AddComponent<ae3d::MeshRendererComponent>();
    standardCubeSpotReceiver.GetComponent<ae3d::MeshRendererComponent>()->SetMesh( &cubeMesh );
    standardCubeSpotReceiver.GetComponent<ae3d::MeshRendererComponent>()->SetMaterial( &standardMaterial, 0 );
    standardCubeSpotReceiver.AddComponent<ae3d::TransformComponent>();
    standardCubeSpotReceiver.GetComponent<ae3d::TransformComponent>()->SetLocalScale( 2 );

    standardCubeTR.AddComponent<ae3d::MeshRendererComponent>();
    standardCubeTR.GetComponent<ae3d::MeshRendererComponent>()->SetMesh( &cubeMesh );
    standardCubeTR.GetComponent<ae3d::MeshRendererComponent>()->SetMaterial( &standardMaterial, 0 );
    standardCubeTR.AddComponent<ae3d::TransformComponent>();
    standardCubeTR.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( 4, 4, -10 ) );

    standardCubeBR.AddComponent<ae3d::MeshRendererComponent>();
    standardCubeBR.GetComponent<ae3d::MeshRendererComponent>()->SetMesh( &cubeMesh );
    standardCubeBR.GetComponent<ae3d::MeshRendererComponent>()->SetMaterial( &standardMaterial, 0 );
    standardCubeBR.AddComponent<ae3d::TransformComponent>();
    standardCubeBR.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( 6, -4, -10 ) );

    cubeMeshPTN.Load( ae3d::FileSystem::FileContents( "/textured_cube_ptn.ae3d" ) );
    cubePTN.AddComponent<ae3d::MeshRendererComponent>();
    cubePTN.GetComponent<ae3d::MeshRendererComponent>()->SetMesh( &cubeMeshPTN );
    cubePTN.GetComponent<ae3d::MeshRendererComponent>()->SetMaterial( &cubeMaterial, 0 );
    cubePTN.AddComponent<ae3d::TransformComponent>();
    cubePTN.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( 3, 0, -10 ) );

    cubePTN2.AddComponent<ae3d::MeshRendererComponent>();
    cubePTN2.GetComponent<ae3d::MeshRendererComponent>()->SetMesh( &cubeMesh/*PTN*/ );
    cubePTN2.GetComponent<ae3d::MeshRendererComponent>()->SetMaterial( &cubeMaterial, 0 );
    cubePTN2.AddComponent<ae3d::TransformComponent>();
    cubePTN2.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( 6, 5, -10 ) );

    bigCubeInScene2.AddComponent<ae3d::MeshRendererComponent>();
    bigCubeInScene2.GetComponent<ae3d::MeshRendererComponent>()->SetMesh( &cubeMesh );
    bigCubeInScene2.GetComponent<ae3d::MeshRendererComponent>()->SetMaterial( &cubeMaterial, 0 );
    bigCubeInScene2.AddComponent<ae3d::TransformComponent>();
    bigCubeInScene2.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( -1, -8, -10 ) );
    bigCubeInScene2.GetComponent<ae3d::TransformComponent>()->SetLocalScale( 5 );
    scene2.Add( &bigCubeInScene2 );

    bigCube.AddComponent<ae3d::MeshRendererComponent>();
    bigCube.GetComponent<ae3d::MeshRendererComponent>()->SetMesh( &cubeMesh );
    bigCube.GetComponent<ae3d::MeshRendererComponent>()->SetMaterial( &cubeMaterial, 0 );
    bigCube.AddComponent<ae3d::TransformComponent>();
    bigCube.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( 40, 0, -85 ) );
    bigCube.GetComponent<ae3d::TransformComponent>()->SetLocalScale( 5 );

    bigCube2.AddComponent<MeshRendererComponent>();
    bigCube2.GetComponent<MeshRendererComponent>()->SetMesh( &cubeMesh );
    bigCube2.GetComponent<MeshRendererComponent>()->SetMaterial( &cubeMaterial, 0 );
    bigCube2.AddComponent<TransformComponent>();
    bigCube2.GetComponent<TransformComponent>()->SetLocalPosition( ae3d::Vec3( -1, 4, -16 ) );
    bigCube2.GetComponent<TransformComponent>()->SetLocalScale( 5 );

    bigCube3.AddComponent<MeshRendererComponent>();
    bigCube3.GetComponent<MeshRendererComponent>()->SetMesh( &cubeMesh );
    bigCube3.GetComponent<MeshRendererComponent>()->SetMaterial( &cubeMaterial, 0 );
    bigCube3.AddComponent<TransformComponent>();
    bigCube3.GetComponent<TransformComponent>()->SetLocalPosition( ae3d::Vec3( 2, 4, -16 ) );
    bigCube3.GetComponent<TransformComponent>()->SetLocalScale( 5 );

    animatedMesh.Load( FileSystem::FileContents( "human_anim_test2.ae3d" ) );

    animatedGo.AddComponent< MeshRendererComponent >();
    animatedGo.GetComponent< MeshRendererComponent >()->SetMesh( &animatedMesh );
    animatedGo.GetComponent< MeshRendererComponent>()->SetMaterial( &skinMaterial, 0 );
    animatedGo.AddComponent< TransformComponent >();
    animatedGo.GetComponent< TransformComponent >()->SetLocalPosition( { -10, 0, -85 } );
    animatedGo.GetComponent< TransformComponent >()->SetLocalScale( 0.01f );

    dirLight.AddComponent<ae3d::DirectionalLightComponent>();
#ifdef TEST_SHADOWS_DIR
    dirLight.GetComponent<ae3d::DirectionalLightComponent>()->SetCastShadow( true, 1024 );
#endif
    dirLight.AddComponent<ae3d::TransformComponent>();
    dirLight.GetComponent<ae3d::TransformComponent>()->LookAt( { 0, 0, 0 }, ae3d::Vec3( 0, -1, 0 ), { 0, 1, 0 } );

    spotLight.AddComponent<ae3d::SpotLightComponent>();
#ifdef TEST_SHADOWS_SPOT
    spotLight.GetComponent<ae3d::SpotLightComponent>()->SetCastShadow( true, 1024 );
#endif
    spotLight.AddComponent<ae3d::TransformComponent>();
    spotLight.GetComponent<ae3d::TransformComponent>()->LookAt( { 0, 8, -10 }, ae3d::Vec3( 0, -1, 0 ), { 0, 1, 0 } );

    pointLight.AddComponent<ae3d::PointLightComponent>();
#ifdef TEST_SHADOWS_POINT
    pointLight.GetComponent<ae3d::PointLightComponent>()->SetCastShadow( true, 1024 );
#endif
    pointLight.GetComponent<ae3d::PointLightComponent>()->SetRadius( 1.2f );
    pointLight.AddComponent<ae3d::TransformComponent>();
    pointLight.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( -80, 0, -85 ) );

    // Inits point lights for Forward+
    {
        int pointLightIndex = 0;
        
        for (int row = 0; row < 10; ++row)
        {
            for (int col = 0; col < 10; ++col)
            {
                pointLights[ pointLightIndex ].AddComponent<ae3d::PointLightComponent>();
                pointLights[ pointLightIndex ].GetComponent<ae3d::PointLightComponent>()->SetRadius( 3 );
                pointLights[ pointLightIndex ].AddComponent<ae3d::TransformComponent>();
                pointLights[ pointLightIndex ].GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( -9 + row * 2, -4, -85 + col * 2 ) );
                scene.Add( &pointLights[ pointLightIndex ] );
                ++pointLightIndex;
            }
        }
    }
    
    rtTex.Create2D( 512, 512, ae3d::RenderTexture::DataType::UByte, ae3d::TextureWrap::Clamp, ae3d::TextureFilter::Linear, "render texture" );
    
    renderTextureContainer.AddComponent<ae3d::SpriteRendererComponent>();
    //renderTextureContainer.GetComponent<ae3d::SpriteRendererComponent>()->SetTexture( &rtTex, ae3d::Vec3( 250, 150, -0.6f ), ae3d::Vec3( 256, 256, 1 ), ae3d::Vec4( 1, 1, 1, 1 ) );
    renderTextureContainer.GetComponent<ae3d::SpriteRendererComponent>()->SetTexture( &camera3d.GetComponent<ae3d::CameraComponent>()->GetDepthNormalsTexture(), ae3d::Vec3( 50, 100, -0.6f ), ae3d::Vec3( 768*2, 512*2, 1 ), ae3d::Vec4( 1, 1, 1, 1 ) );
    //renderTextureContainer.GetComponent<ae3d::SpriteRendererComponent>()->SetTexture( dirLight.GetComponent<ae3d::DirectionalLightComponent>()->GetShadowMap(), ae3d::Vec3( 250, 150, -0.6f ), ae3d::Vec3( 512, 512, 1 ), ae3d::Vec4( 1, 1, 1, 1 ) );
    renderTextureContainer.SetLayer( 2 );
    
    rtCamera.AddComponent<ae3d::CameraComponent>();
    rtCamera.GetComponent<ae3d::CameraComponent>()->SetProjection( 45, 4.0f / 3.0f, 1, 200 );
    rtCamera.GetComponent<ae3d::CameraComponent>()->SetProjectionType( ae3d::CameraComponent::ProjectionType::Perspective );
    rtCamera.GetComponent<ae3d::CameraComponent>()->SetClearFlag( ae3d::CameraComponent::ClearFlag::DepthAndColor );
    rtCamera.GetComponent<ae3d::CameraComponent>()->SetClearColor( ae3d::Vec3( 0.5f, 0, 0 ) );
    rtCamera.GetComponent<ae3d::CameraComponent>()->SetTargetTexture( &rtTex );
    rtCamera.AddComponent<ae3d::TransformComponent>();
    rtCamera.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( 5, 5, 20 ) );

    cubeRT.CreateCube( 512, ae3d::RenderTexture::DataType::UByte, ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Linear, "cube RT" );

    cameraCubeRT.AddComponent<CameraComponent>();
    cameraCubeRT.GetComponent<CameraComponent>()->SetClearColor( Vec3( 0, 0, 1 ) );
    cameraCubeRT.GetComponent<CameraComponent>()->SetProjectionType( CameraComponent::ProjectionType::Perspective );
    cameraCubeRT.GetComponent<CameraComponent>()->SetProjection( 45, 1, 1, 400 );
    cameraCubeRT.GetComponent<CameraComponent>()->SetTargetTexture( &cubeRT );
    cameraCubeRT.GetComponent<CameraComponent>()->SetClearFlag( CameraComponent::ClearFlag::DepthAndColor );
    cameraCubeRT.AddComponent<TransformComponent>();
    cameraCubeRT.GetComponent<TransformComponent>()->LookAt( { 5, 0, -70 }, { 0, 0, -100 }, { 0, 1, 0 } );

#ifdef TEST_FORWARD_PLUS
    //scene.Add( &standardCubeBR );
    //scene.Add( &standardCubeBL );
    //scene.Add( &standardCubeTR );
    //scene.Add( &standardCubeTL2 );
    scene.Add( &standardCubeTopCenter );
    scene.Add( &standardCubeSpotReceiver );
    //scene.Add( &standardCubeTL );
#endif
    scene.Add( &bigCube );
    //scene.Add( &cubePTN2 );
    //scene.Add( &cubePTN );
    scene.Add( &rtCube );
    scene.Add( &rotatingCube );
    scene.Add( &spriteContainer );
    scene.Add( &textSDF );
    scene.Add( &text );
    //scene.Add( &bigCube2 );
    //scene.Add( &bigCube3 );
    //scene.Add( &animatedGo );
    scene.Add( &pointLight );
    scene.Add( &spotLight );
    scene.Add( &dirLight );
    //scene.Add( &renderTextureContainer );
    //scene.Add( &rtCamera );
    //scene.Add( &cameraCubeRT );

    transTex.Load( ae3d::FileSystem::FileContents( "/font.png" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Linear, ae3d::Mipmaps::None,
                  ae3d::ColorSpace::SRGB, ae3d::Anisotropy::k1 );
    
    transMaterial.SetShader( &shader );
    transMaterial.SetTexture( "textureMap", &transTex );

    transMaterial.SetBackFaceCulling( true );
    //transMaterial.SetBlendingMode( ae3d::Material::BlendingMode::Alpha );
    //rotatingCube.GetComponent<ae3d::MeshRendererComponent>()->SetMaterial( &transMaterial, 0 );
    
    lineProjection.MakeProjection( 0, self.view.bounds.size.width, self.view.bounds.size.height, 0, 0, 1 );
    std::vector< Vec3 > lines( 4 );
    lines[ 0 ] = Vec3( 10, 10, -0.5f );
    lines[ 1 ] = Vec3( 50, 10, -0.5f );
    lines[ 2 ] = Vec3( 50, 50, -0.5f );
    lines[ 3 ] = Vec3( 10, 10, -0.5f );
    lineHandle = System::CreateLineBuffer( lines, Vec3( 1, 0, 0 ) );
    
    commandQueue = [device newCommandQueue];
    inFlightSemaphore = dispatch_semaphore_create( kMaxBuffersInFlight );
    
#ifdef TEST_NUKLEAR_UI
    nk_font_atlas_init_default( &atlas );
    nk_font_atlas_begin( &atlas );
    
    nkFont = nk_font_atlas_add_default( &atlas, 13.0f, nullptr );
    const void* image = nk_font_atlas_bake( &atlas, &atlasWidth, &atlasHeight, NK_FONT_ATLAS_RGBA32 );
    
    nkFontTexture.LoadFromData( image, atlasWidth, atlasHeight, 4, "Nuklear font" );
    nk_font_atlas_end( &atlas, nk_handle_id( nkFontTexture.GetID() ), &nullTexture );
    
    uiTextures[ nk_handle_id( nkFontTexture.GetID() ).id ] = &nkFontTexture;
    
    nk_init_default( &ctx, &nkFont->handle );
    nk_buffer_init_default( &cmds );
#endif
}

- (void)_setupView
{
    _view = (MTKView *)self.view;
    _view.delegate = self;
    _view.device = device;
    _view.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    _view.sampleCount = MULTISAMPLE_COUNT;
}

- (void)keyDown:(NSEvent *)theEvent
{
    NSLog(@"onKeyDown Detected");
}

- (void)mouseDown:(NSEvent *)theEvent
{
    ae3d::System::Print( "mouseDown x: %f, y: %f\n", theEvent.locationInWindow.x, theEvent.locationInWindow.y );
    
    camera3d.GetComponent<ae3d::TransformComponent>()->MoveForward( -1 );
    
#ifdef TEST_NUKLEAR_UI
    nk_input_begin( &ctx );
    //nk_input_button( &ctx, NK_BUTTON_LEFT, (int)theEvent.locationInWindow.x, self.view.bounds.size.height - (int)theEvent.locationInWindow.y, 1 );
    nk_input_button( &ctx, NK_BUTTON_LEFT, (int)theEvent.locationInWindow.x, (int)theEvent.locationInWindow.y, 1 );
    nk_input_end( &ctx );
#endif
}

// nk_input_motion( &ctx, (int)x, (int)y );

- (void)mouseUp:(NSEvent *)theEvent
{
    ae3d::System::Print( "mouseUp x: %f, y: %f, height: %f\n", theEvent.locationInWindow.x, theEvent.locationInWindow.y, self.view.bounds.size.height );
#ifdef TEST_NUKLEAR_UI
    nk_input_begin( &ctx );
    //nk_input_button( &ctx, NK_BUTTON_LEFT, (int)theEvent.locationInWindow.x, self.view.bounds.size.height - (int)theEvent.locationInWindow.y, 0 );
    nk_input_button( &ctx, NK_BUTTON_LEFT, (int)theEvent.locationInWindow.x, (int)theEvent.locationInWindow.y, 0 );
    nk_input_end( &ctx );
#endif
}

- (void)mouseDragged:(NSEvent *)theEvent
{
    camera3d.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 0, 1, 0 ), -float( theEvent.deltaX ) / 20 );
    camera3d.GetComponent<TransformComponent>()->OffsetRotate( Vec3( 1, 0, 0 ), -float( theEvent.deltaY ) / 20 );
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

#if 0
- (void)_render
{
    //dispatch_semaphore_wait( inFlightSemaphore, DISPATCH_TIME_FOREVER );

    //id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];

    //__block dispatch_semaphore_t block_sema = inFlightSemaphore;
    /*[commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer)
     {
         dispatch_semaphore_signal(block_sema);
     }];*/

    if (_view.currentRenderPassDescriptor != nil)
    {
        //id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:_view.currentRenderPassDescriptor];
        //[renderEncoder endEncoding];
        //[commandBuffer presentDrawable:_view.currentDrawable];
        //[commandBuffer commit];
    }
}
#endif

- (void)_render
{
    [self _update];
    
    if (_view.currentRenderPassDescriptor != nil)
    {
        ae3d::System::SetCurrentDrawableMetal( _view.currentDrawable, _view.currentRenderPassDescriptor );
        ae3d::System::BeginFrame();
        scene.Render();
        //scene2.Render();
        //System::DrawLines( lineHandle, lineView, lineProjection );
        
#ifdef TEST_NUKLEAR_UI
        enum {EASY, HARD};
        static int op = EASY;
        static float value = 0.6f;
        
        if (nk_begin( &ctx, "Demo", nk_rect( 0, 0, 300, 400 ), NK_WINDOW_BORDER ))
        {
            nk_layout_row_static( &ctx, 30, 80, 1 );
            
            if (nk_button_label( &ctx, "button" ))
            {
                System::Print("Pressed a button\n");
            }
            
            /* fixed widget window ratio width */
            nk_layout_row_dynamic( &ctx, 30, 2 );
            if (nk_option_label( &ctx, "easy", op == EASY)) op = EASY;
            if (nk_option_label( &ctx, "hard", op == HARD)) op = HARD;
            
            /* custom widget pixel width */
            nk_layout_row_begin( &ctx, NK_STATIC, 30, 2 );
            {
                nk_layout_row_push( &ctx, 50 );
                nk_label( &ctx, "Volume:", NK_TEXT_LEFT );
                nk_layout_row_push( &ctx, 110 );
                nk_slider_float( &ctx, 0, &value, 1.0f, 0.1f );
            }
            nk_layout_row_end( &ctx );
            nk_end( &ctx );
        }
        DrawNuklear( &ctx, &cmds, self.view.bounds.size.width, self.view.bounds.size.height );
#endif
        scene.EndRenderMetal();
        ae3d::System::EndFrame();
    }
}

- (void)_reshape
{
}

- (void)_update
{
    static int angle = 0;
    ++angle;
    
    ae3d::Quaternion rotation;
    rotation = ae3d::Quaternion::FromEuler( ae3d::Vec3( angle, angle, angle ) );
    rotatingCube.GetComponent< ae3d::TransformComponent >()->SetLocalRotation( rotation );
    
    //std::string drawCalls = std::string( "draw calls:" ) + std::to_string( ae3d::System::Statistics::GetDrawCallCount() );
    std::string stats = ae3d::System::Statistics::GetStatistics();
    text.GetComponent<ae3d::TextRendererComponent>()->SetText( stats.c_str() );
    
    static int animationFrame = 0;
    ++animationFrame;
    animatedGo.GetComponent< MeshRendererComponent >()->SetAnimationFrame( animationFrame );

    // Testing vertex buffer growing
    if (angle == 5)
    {
        text.GetComponent<ae3d::TextRendererComponent>()->SetText( "this is a long string. this is a long string" );
    }

    //pointLight.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( -8, 0, -85 + std::sin( angle / 2 ) * 2 ) );
    //pointLight.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( 11, 0, -85 ) );
    pointLight.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( -9.8f, 0, -85 ) );
    
    standardCubeTopCenter.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( -10, 0, -85 ) );
    standardCubeSpotReceiver.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( -10, 0, -90 ) );
    
    spotLight.GetComponent<ae3d::TransformComponent>()->LookAt( { -8, 2, -90 }, ae3d::Vec3( 0, -1, 0 ), { 0, 1, 0 } );
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size
{
    [self _reshape];
}

- (void)drawInMTKView:(nonnull MTKView *)view
{
    @autoreleasepool {
        [self _render];
    }
}
@end
