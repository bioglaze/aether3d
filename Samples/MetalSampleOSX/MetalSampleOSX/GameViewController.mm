// This sample's assets are referenced from aether3d_build/Samples. Make sure that they exist.
// Assets can be downloaded from http://twiren.kapsi.fi/files/aether3d_sample_v0.8.5.zip
// If you didn't download a release of Aether3D, some referenced assets could be missing,
// just remove the references to build.
#import "GameViewController.h"
#import <MetalKit/MetalKit.h>
#include <vector>
#include <map>
#include <math.h>
#include <stdint.h>

#import "Array.hpp"
#import "AudioSourceComponent.hpp"
#import "AudioClip.hpp"
#import "CameraComponent.hpp"
#import "ComputeShader.hpp"
#import "DirectionalLightComponent.hpp"
#import "Font.hpp"
#import "FileSystem.hpp"
#import "GameObject.hpp"
#import "MeshRendererComponent.hpp"
#import "Material.hpp"
#import "Mesh.hpp"
#import "PointLightComponent.hpp"
#import "Shader.hpp"
#import "System.hpp"
#import "Scene.hpp"
#import "SpotLightComponent.hpp"
#import "SpriteRendererComponent.hpp"
#import "Texture2D.hpp"
#import "TextureCube.hpp"
#import "TextRendererComponent.hpp"
#import "TransformComponent.hpp"
#import "Window.hpp"

//#define TEST_FORWARD_PLUS
//#define TEST_BLOOM
//#define TEST_SHADOWS_DIR
//#define TEST_SHADOWS_SPOT
//#define TEST_SHADOWS_POINT
//#define TEST_NUKLEAR_UI
//#define TEST_RENDER_TEXTURE_2D
//#define TEST_RENDER_TEXTURE_CUBE
//#define TEST_SSAO

const int POINT_LIGHT_COUNT = 50 * 40;
const int MULTISAMPLE_COUNT = 1;
const int MAX_VERTEX_MEMORY = 512 * 1024;
const int MAX_ELEMENT_MEMORY = 128 * 1024;

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
    uint32_t xorshifted = (uint32_t)( ((oldstate >> 18u) ^ oldstate) >> 27u );
    int32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

pcg32_random_t rng;

int Random100()
{
    return pcg32_random_r( &rng );
}

NSViewController* myViewController;

struct InputEvent
{
    bool isActive;
    int x, y;
    int button;
};
InputEvent inputEvent;

#ifdef TEST_NUKLEAR_UI
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#include "nuklear.h"

struct VertexPTC
{
    float position[ 3 ];
    float uv[ 2 ];
    float col[ 4 ];
};

nk_draw_null_texture nullTexture;
nk_font* nkFont = nullptr;
ae3d::Texture2D uiTextures[ 1 ];

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
    config.vertex_size = sizeof( VertexPTC );
    config.vertex_alignment = NK_ALIGNOF( VertexPTC );
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

    const nk_draw_command* cmd = nullptr;
    int offset = 0;

    nk_draw_foreach( cmd, ctx, uiCommands )
    {
        if (cmd->elem_count == 0)
        {
            continue;
        }

        ae3d::System::DrawUI( (int)(cmd->clip_rect.x),
                       (int)((height - (int)(cmd->clip_rect.y + cmd->clip_rect.h))),
                       (int)(cmd->clip_rect.w),
                       (int)(cmd->clip_rect.h),
                       cmd->elem_count, &uiTextures[ 0/*cmd->texture.id*/ ], offset, width, height );
        offset += cmd->elem_count / 3;
    }

    nk_clear( ctx );
}
#endif

int CreateConeLines()
{
    std::vector< ae3d::Vec3 > lines;

    const int angleStep = 10;

    for (int angleDeg = 0; angleDeg < 360; angleDeg += angleStep)
    {
        const float x = cosf( angleDeg * 3.14159f / 180.0f );
        const float y = sinf( angleDeg * 3.14159f / 180.0f );

        const float x2 = cosf( (angleDeg + angleStep) * 3.14159f / 180.0f );
        const float y2 = sinf( (angleDeg + angleStep) * 3.14159f / 180.0f );

        lines.push_back( ae3d::Vec3( x, y, 0 ) );
        lines.push_back( ae3d::Vec3( x2, y2, 0 ) );
    }

    for (int angleDeg = 0; angleDeg < 360; angleDeg += angleStep)
    {
        const float x = cosf( angleDeg * 3.14159f / 180.0f ) * 2;
        const float y = sinf( angleDeg * 3.14159f / 180.0f ) * 2;

        const float x2 = cosf( (angleDeg + angleStep) * 3.14159f / 180.0f ) * 2;
        const float y2 = sinf( (angleDeg + angleStep) * 3.14159f / 180.0f ) * 2;

        lines.push_back( ae3d::Vec3( x, y, 1 ) );
        lines.push_back( ae3d::Vec3( x2, y2, 1 ) );
    }

    for (int angleDeg = 0; angleDeg < 360; angleDeg += angleStep)
    {
        const float x = cosf( angleDeg * 3.14159f / 180.0f ) * 2;
        const float y = sinf( angleDeg * 3.14159f / 180.0f ) * 2;

        const float x2 = cosf( (angleDeg) * 3.14159f / 180.0f );
        const float y2 = sinf( (angleDeg) * 3.14159f / 180.0f );

        lines.push_back( ae3d::Vec3( x, y, 1 ) );
        lines.push_back( ae3d::Vec3( x2, y2, 0 ) );
    }

    return ae3d::System::CreateLineBuffer( lines.data(), (int)lines.size(), ae3d::Vec3( 1, 1, 1 ) );
}

using namespace ae3d;

@implementation GameViewController
{
    MTKView* _view;

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
    GameObject spriteContainer;
    GameObject cameraCubeRT;
    GameObject animatedGo;
    GameObject wireframeGo;
    GameObject pbrCube;
    GameObject audioGo;
    GameObject audioContainer;
    AudioClip audioClip;

    Scene scene;

    Font font;
    Font fontSDF;

    Mesh cubeMesh;
    Mesh cubeMeshPTN;
    Mesh animatedMesh;

    Material cubeMaterial;
    Material skinMaterial;
    Material skinStandardMaterial;
    Material rtCubeMaterial;
    Material transMaterial;
    Material standardMaterial;
    Material pbrMaterial;

    Shader shader;
    Shader skinShader;
    Shader skyboxShader;
    Shader standardShader;
    Shader standardSkinShader;
    ComputeShader downSampleAndThresholdShader;
    ComputeShader blurShader;
    ComputeShader ssaoShader;
    
    Texture2D atlasTex;
    Texture2D fontTex;
    Texture2D fontTexSDF;
    Texture2D gliderTex;
    Texture2D transTex;
    Texture2D bc1Tex;
    Texture2D bc2Tex;
    Texture2D bc3Tex;
    Texture2D bc4Tex;
    Texture2D bc5Tex;
    Texture2D pbrDiffuseTex;
    Texture2D pbrNormalTex;
    Texture2D pbrRoughnessTex;
    Texture2D playerTex;
    Texture2D noiseTex;
    Texture2D ssaoTex;
    Texture2D bloomTex;
    Texture2D blurTex;
    Texture2D blurTex2;

    TextureCube skyTex;

    RenderTexture rtTex;
    RenderTexture cubeRT;
    RenderTexture cameraTex;
    RenderTexture camera2dTex;
    
    std::vector< GameObject > sponzaGameObjects;
    std::map< std::string, Material* > sponzaMaterialNameToMaterial;
    std::map< std::string, Texture2D* > sponzaTextureNameToTexture;
    Array< Mesh* > sponzaMeshes;
    Vec3 moveDir;

    Matrix44 lineView;
    Matrix44 lineProjection;
    int lineHandle;
    int coneLineHandle;

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

    _view = (MTKView *)self.view;
    _view.delegate = self;
    _view.device = MTLCreateSystemDefaultDevice();
    _view.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    _view.sampleCount = MULTISAMPLE_COUNT;
    _view.colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;

    myViewController = self;

    [self _reshape];

    ae3d::System::InitMetal( _view.device, _view, MULTISAMPLE_COUNT, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY );
    ae3d::System::LoadBuiltinAssets();
    ae3d::System::InitAudio();

    audioClip.Load( FileSystem::FileContents( "explosion.wav" ) );
    //audioClip.Load( FileSystem::FileContents( "example.ogg" ) );

    audioContainer.AddComponent<AudioSourceComponent>();
    audioContainer.GetComponent<AudioSourceComponent>()->SetClipId( audioClip.GetId() );
    //audioContainer.GetComponent<AudioSourceComponent>()->Play();

    // Sponza can be downloaded from http://twiren.kapsi.fi/files/aether3d_sponza.zip and extracted into aether3d_build/Samples
#if 1
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
        mat.second->SetTexture( &skyTex );
#else
        mat.second->SetShader( &shader );
#endif
    }

    for (std::size_t i = 0; i < sponzaGameObjects.size(); ++i)
    {
        scene.Add( &sponzaGameObjects[ i ] );
    }
#endif

    constexpr int noiseDim = 64;
    Vec4 noiseData[ noiseDim * noiseDim ];

    for (int i = 0; i < noiseDim * noiseDim; ++i)
    {
        Vec3 dir = Vec3( (Random100() / 100.0f) * 2 - 1, (Random100() / 100.0f) * 2 - 1, 0 ).Normalized();
        noiseData[ i ].x = dir.x;
        noiseData[ i ].y = dir.y;
        noiseData[ i ].z = 0;
        noiseData[ i ].w = 0;
    }

    noiseTex.LoadFromData( noiseData, noiseDim, noiseDim, 4, "noiseData" );

    cameraTex.Create2D( self.view.bounds.size.width * 2, self.view.bounds.size.height * 2, DataType::Float16, TextureWrap::Clamp, TextureFilter::Linear, "cameraTex", MULTISAMPLE_COUNT > 1 );
    ae3d::System::Print( "width: %f, height: %f\n", self.view.bounds.size.width, self.view.bounds.size.height );
    camera2dTex.Create2D( self.view.bounds.size.width * 2, self.view.bounds.size.height * 2, DataType::Float16, TextureWrap::Clamp, TextureFilter::Linear, "camera2dTex", false );

    camera2d.SetName( "Camera2D" );
    camera2d.AddComponent<ae3d::CameraComponent>();
    camera2d.GetComponent<ae3d::CameraComponent>()->SetProjection( 0, self.view.bounds.size.width, self.view.bounds.size.height, 0, 0, 1 );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetProjectionType( ae3d::CameraComponent::ProjectionType::Orthographic );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetClearFlag( ae3d::CameraComponent::ClearFlag::DepthAndColor );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetClearColor( ae3d::Vec3( 0.5f, 0.0f, 0.0f ) );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetLayerMask( 0x2 );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetRenderOrder( 2 );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetTargetTexture( &camera2dTex );
    camera2d.AddComponent<ae3d::TransformComponent>();

    const float aspect = _view.bounds.size.width / (float)_view.bounds.size.height;

    camera3d.SetName( "Camera3D" );
    camera3d.AddComponent<ae3d::CameraComponent>();
    camera3d.GetComponent<ae3d::CameraComponent>()->SetProjection( 45, aspect, 1, 200 );
    camera3d.GetComponent<ae3d::CameraComponent>()->SetClearColor( ae3d::Vec3( 0.5f, 0.5f, 0.5f ) );
    camera3d.GetComponent<ae3d::CameraComponent>()->SetClearFlag( ae3d::CameraComponent::ClearFlag::DepthAndColor );
    camera3d.GetComponent<ae3d::CameraComponent>()->SetProjectionType( ae3d::CameraComponent::ProjectionType::Perspective );
    camera3d.GetComponent<ae3d::CameraComponent>()->SetRenderOrder( 1 );
    camera3d.GetComponent<ae3d::CameraComponent>()->SetTargetTexture( &cameraTex );
    //camera3d.GetComponent<ae3d::CameraComponent>()->SetViewport( 0, 0, 640, 480 );
    camera3d.GetComponent<ae3d::CameraComponent>()->GetDepthNormalsTexture().Create2D( self.view.bounds.size.width * 2, self.view.bounds.size.height * 2,
                                                                                       DataType::Float, ae3d::TextureWrap::Clamp, ae3d::TextureFilter::Nearest,  "depthnormals", false );
    camera3d.AddComponent<ae3d::TransformComponent>();
    camera3d.GetComponent<TransformComponent>()->LookAt( { 20, 0, -85 }, { 120, 0, -85 }, { 0, 1, 0 } );

    scene.Add( &camera2d );
    scene.Add( &camera3d );
    scene2.Add( &camera3d );

    fontTex.Load( ae3d::FileSystem::FileContents( "font.png" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Linear, ae3d::Mipmaps::None, ae3d::ColorSpace::Linear, ae3d::Anisotropy::k1 );
    // TODO: SDF texture
    fontTexSDF.Load( ae3d::FileSystem::FileContents( "font.png" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Nearest, ae3d::Mipmaps::None, ae3d::ColorSpace::Linear, ae3d::Anisotropy::k1 );
    gliderTex.Load( ae3d::FileSystem::FileContents( "glider.png" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Linear, ae3d::Mipmaps::None, ae3d::ColorSpace::Linear, ae3d::Anisotropy::k1 );
    bc1Tex.Load( ae3d::FileSystem::FileContents( "test_dxt1.dds" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Nearest, ae3d::Mipmaps::Generate, ae3d::ColorSpace::Linear, ae3d::Anisotropy::k1 );
    bc2Tex.Load( ae3d::FileSystem::FileContents( "test_dxt3.dds" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Nearest, ae3d::Mipmaps::None, ae3d::ColorSpace::Linear, ae3d::Anisotropy::k1 );
    bc3Tex.Load( ae3d::FileSystem::FileContents( "test_dxt5.dds" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Nearest, ae3d::Mipmaps::None, ae3d::ColorSpace::Linear, ae3d::Anisotropy::k1 );
    bc4Tex.Load( ae3d::FileSystem::FileContents( "spnza_bricks_a_spec_bc4.dds" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Nearest, ae3d::Mipmaps::None, ae3d::ColorSpace::Linear, ae3d::Anisotropy::k1 );
    bc5Tex.Load( ae3d::FileSystem::FileContents( "grass_n_bc5.dds" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Nearest, ae3d::Mipmaps::None, ae3d::ColorSpace::Linear, ae3d::Anisotropy::k1 );

    playerTex.Load( ae3d::FileSystem::FileContents( "player.png" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Linear, ae3d::Mipmaps::Generate, ae3d::ColorSpace::SRGB, ae3d::Anisotropy::k1 );

    skyTex.Load( ae3d::FileSystem::FileContents( "left.jpg" ), ae3d::FileSystem::FileContents( "right.jpg" ),
                ae3d::FileSystem::FileContents( "bottom.jpg" ), ae3d::FileSystem::FileContents( "top.jpg" ),
                ae3d::FileSystem::FileContents( "front.jpg" ), ae3d::FileSystem::FileContents( "back.jpg" ),
                ae3d::TextureWrap::Clamp, ae3d::TextureFilter::Linear, ae3d::Mipmaps::Generate, ae3d::ColorSpace::SRGB );

    /*skyTex.Load( FileSystem::FileContents( "test_dxt1.dds" ), FileSystem::FileContents( "test_dxt1.dds" ),
                FileSystem::FileContents( "test_dxt1.dds" ), FileSystem::FileContents( "test_dxt1.dds" ),
                FileSystem::FileContents( "test_dxt1.dds" ), FileSystem::FileContents( "test_dxt1.dds" ),
                TextureWrap::Clamp, TextureFilter::Linear, Mipmaps::None, ColorSpace::SRGB );*/
    scene.SetSkybox( &skyTex );

    pbrDiffuseTex.Load( ae3d::FileSystem::FileContents( "textures/pbr_metal_texture/metal_plate_d.png" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Linear, ae3d::Mipmaps::Generate, ae3d::ColorSpace::SRGB, ae3d::Anisotropy::k1 );
    pbrNormalTex.Load( ae3d::FileSystem::FileContents( "textures/pbr_metal_texture/metal_plate_n.png" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Linear, ae3d::Mipmaps::Generate, ae3d::ColorSpace::Linear, ae3d::Anisotropy::k1 );
    pbrRoughnessTex.Load( ae3d::FileSystem::FileContents( "textures/pbr_metal_texture/metal_plate_rough.png" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Linear, ae3d::Mipmaps::Generate, ae3d::ColorSpace::Linear, ae3d::Anisotropy::k1 );

    font.LoadBMFont( &fontTex, ae3d::FileSystem::FileContents( "font_txt.fnt" ) );
    fontSDF.LoadBMFont( &fontTexSDF, ae3d::FileSystem::FileContents( "font_txt.fnt" ) );
    atlasTex.LoadFromAtlas( FileSystem::FileContents( "atlas_cegui.png" ), FileSystem::FileContents( "atlas_cegui.xml" ), "granite", TextureWrap::Repeat, TextureFilter::Nearest, ColorSpace::Linear, Anisotropy::k1 );

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
    sprite->SetTexture( &bc1Tex, Vec3( 120, 100, -0.6f ), Vec3( (float)bc1Tex.GetWidth(), (float)bc1Tex.GetHeight(), 1 ), Vec4( 1, 1, 1, 1 ) );
    sprite->SetTexture( &bc2Tex, Vec3( 120, 200, -0.5f ), Vec3( (float)bc2Tex.GetWidth(), (float)bc2Tex.GetHeight(), 1 ), Vec4( 1, 1, 1, 1 ) );
    sprite->SetTexture( &bc3Tex, Vec3( 120, 300, -0.5f ), Vec3( (float)bc3Tex.GetWidth(), (float)bc3Tex.GetHeight(), 1 ), Vec4( 1, 1, 1, 1 ) );
    sprite->SetTexture( &gliderTex, Vec3( 220, 120, -0.5f ), Vec3( (float)gliderTex.GetWidth(), (float)gliderTex.GetHeight(), 1 ), Vec4( 1, 1, 1, 1 ) );
    //sprite->SetTexture( &atlasTex, Vec3( 300, 120, -0.5f ), Vec3( (float)atlasTex.GetWidth(), (float)atlasTex.GetHeight(), 1 ), Vec4( 1, 1, 1, 1 ) );

    spriteContainer.AddComponent<TransformComponent>();
    //spriteContainer.GetComponent<TransformComponent>()->SetLocalPosition( Vec3( 20, 0, 0 ) );
    spriteContainer.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( 5, 5, 0 ) );
    spriteContainer.SetLayer( 2 );

    shader.Load( "unlit_vertex", "unlit_fragment",
                ae3d::FileSystem::FileContents(""), ae3d::FileSystem::FileContents( "" ),
                ae3d::FileSystem::FileContents(""), ae3d::FileSystem::FileContents( "" ));

    skinShader.Load( "unlit_skin_vertex", "unlit_skin_fragment",
                ae3d::FileSystem::FileContents(""), ae3d::FileSystem::FileContents( "" ),
                ae3d::FileSystem::FileContents(""), ae3d::FileSystem::FileContents( "" ));

    downSampleAndThresholdShader.Load( "downsampleAndThreshold", ae3d::FileSystem::FileContents( "" ), ae3d::FileSystem::FileContents( "" ) );
    blurShader.Load( "blur", ae3d::FileSystem::FileContents( "" ), ae3d::FileSystem::FileContents( "" ) );
    ssaoShader.Load( "ssao", ae3d::FileSystem::FileContents( "" ), ae3d::FileSystem::FileContents( "" ) );
    
    cubeMaterial.SetShader( &shader );
    cubeMaterial.SetTexture( &gliderTex, 0 );
    //cubeMaterial.SetRenderTexture( "textureMap", &camera3d.GetComponent<ae3d::CameraComponent>()->GetDepthNormalsTexture() );

    skinMaterial.SetShader( &skinShader );
    skinMaterial.SetTexture( &playerTex, 0 );

    skinStandardMaterial.SetShader( &standardSkinShader );
    skinStandardMaterial.SetTexture( &playerTex, 0 );
    skinStandardMaterial.SetTexture( &playerTex, 1 );
    
    rtCubeMaterial.SetShader( &skyboxShader );
    rtCubeMaterial.SetRenderTexture( &cubeRT, 0 );
    rtCubeMaterial.SetBackFaceCulling( false );

    standardShader.Load( "standard_vertex", "standard_fragment",
                ae3d::FileSystem::FileContents(""), ae3d::FileSystem::FileContents( "" ),
                ae3d::FileSystem::FileContents(""), ae3d::FileSystem::FileContents( "" ));

    standardSkinShader.Load( "standard_skin_vertex", "standard_fragment",
                ae3d::FileSystem::FileContents(""), ae3d::FileSystem::FileContents( "" ),
                ae3d::FileSystem::FileContents(""), ae3d::FileSystem::FileContents( "" ));

    standardMaterial.SetShader( &standardShader );
    standardMaterial.SetTexture( &gliderTex, 0 );
    standardMaterial.SetTexture( &pbrNormalTex, 1 );

    skyboxShader.Load( "skybox_vertex", "skybox_fragment",
                        ae3d::FileSystem::FileContents(""), ae3d::FileSystem::FileContents( "" ),
                        ae3d::FileSystem::FileContents(""), ae3d::FileSystem::FileContents( "" ));

    cubeMesh.Load( ae3d::FileSystem::FileContents( "textured_cube.ae3d" ) );
    rotatingCube.AddComponent<ae3d::MeshRendererComponent>();
    rotatingCube.GetComponent<ae3d::MeshRendererComponent>()->SetMesh( &cubeMesh );
    rotatingCube.GetComponent<ae3d::MeshRendererComponent>()->SetMaterial( &cubeMaterial, 0 );
    rotatingCube.AddComponent<ae3d::TransformComponent>();
    rotatingCube.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( 2, 0, -8 ) );
    rotatingCube.GetComponent<ae3d::TransformComponent>()->SetLocalScale( 1 );

    pbrMaterial.SetShader( &standardShader );
    pbrMaterial.SetTexture( &pbrNormalTex, 1 );
    pbrMaterial.SetTexture( &pbrDiffuseTex, 0 );
#ifdef TEST_RENDER_TEXTURE_CUBE
    pbrMaterial.SetRenderTexture( &cubeRT, 4 );
#else
    pbrMaterial.SetTexture( &skyTex );
#endif

    pbrCube.AddComponent<ae3d::MeshRendererComponent>();
    pbrCube.GetComponent<ae3d::MeshRendererComponent>()->SetMesh( &cubeMesh );
    pbrCube.GetComponent<ae3d::MeshRendererComponent>()->SetMaterial( &pbrMaterial, 0 );
    pbrCube.AddComponent<ae3d::TransformComponent>();
    pbrCube.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( { -10, 0, -85 } );
    pbrCube.GetComponent<ae3d::TransformComponent>()->SetLocalScale( 2 );

    wireframeGo.AddComponent<ae3d::MeshRendererComponent>();
    wireframeGo.GetComponent<ae3d::MeshRendererComponent>()->SetMesh( &cubeMesh );
    wireframeGo.GetComponent<ae3d::MeshRendererComponent>()->SetMaterial( &cubeMaterial, 0 );
    wireframeGo.GetComponent<ae3d::MeshRendererComponent>()->EnableWireframe( true );
    wireframeGo.AddComponent<ae3d::TransformComponent>();
    wireframeGo.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( -2, 0, -8 ) );
    wireframeGo.GetComponent<ae3d::TransformComponent>()->SetLocalScale( 1 );

    rtCube.AddComponent<ae3d::MeshRendererComponent>();
    rtCube.GetComponent<ae3d::MeshRendererComponent>()->SetMesh( &cubeMesh );
    rtCube.GetComponent<ae3d::MeshRendererComponent>()->SetMaterial( &rtCubeMaterial, 0 );
    rtCube.AddComponent<ae3d::TransformComponent>();
    rtCube.GetComponent< TransformComponent >()->SetLocalPosition( { -5, 2, -85 } );
    rtCube.GetComponent<ae3d::TransformComponent>()->SetLocalScale( 1 );

    cubeMeshPTN.Load( ae3d::FileSystem::FileContents( "textured_cube_ptn.ae3d" ) );
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
#ifdef TEST_FORWARD_PLUS
    animatedGo.GetComponent< MeshRendererComponent>()->SetMaterial( &skinStandardMaterial, 0 );
#else
    animatedGo.GetComponent< MeshRendererComponent>()->SetMaterial( &skinMaterial, 0 );
#endif
    animatedGo.AddComponent< TransformComponent >();
    animatedGo.GetComponent< TransformComponent >()->SetLocalPosition( { -10, -12, -85 } );
    animatedGo.GetComponent< TransformComponent >()->SetLocalRotation( Quaternion::FromEuler( { 180, 0, 0 } ) );
    animatedGo.GetComponent< TransformComponent >()->SetLocalScale( 0.01f );

    dirLight.AddComponent<ae3d::DirectionalLightComponent>();
#ifdef TEST_SHADOWS_DIR
    dirLight.GetComponent<ae3d::DirectionalLightComponent>()->SetCastShadow( true, 1024 );
#endif
    dirLight.GetComponent<ae3d::DirectionalLightComponent>()->SetColor( { 1, 1, 1 } );
    dirLight.AddComponent<ae3d::TransformComponent>();
    dirLight.GetComponent<ae3d::TransformComponent>()->LookAt( { 0, 0, 0 }, ae3d::Vec3( 0, -1, 0 ), { 0, 1, 0 } );

    spotLight.AddComponent<ae3d::SpotLightComponent>();
#ifdef TEST_SHADOWS_SPOT
    spotLight.GetComponent<ae3d::SpotLightComponent>()->SetCastShadow( true, 1024 );
#endif
    spotLight.GetComponent<ae3d::SpotLightComponent>()->SetColor( Vec3( 1, 0, 0 ) );
    spotLight.GetComponent<ae3d::SpotLightComponent>()->SetRadius( 2 );
    spotLight.GetComponent<ae3d::SpotLightComponent>()->SetConeAngle( 30 );
    spotLight.AddComponent<ae3d::TransformComponent>();
    //spotLight.GetComponent<TransformComponent>()->LookAt( { 0, -2, -80 }, { 0, -1, 0 }, { 0, 1, 0 } );
    spotLight.GetComponent<TransformComponent>()->LookAt( { 0, 0, -70 }, { 0, 0, 1 }, { 0, 1, 0 } );

    pointLight.AddComponent<ae3d::PointLightComponent>();
#ifdef TEST_SHADOWS_POINT
    pointLight.GetComponent<ae3d::PointLightComponent>()->SetCastShadow( true, 1024 );
#endif
    pointLight.GetComponent<ae3d::PointLightComponent>()->SetRadius( 10.2f );
    pointLight.AddComponent<ae3d::TransformComponent>();
    pointLight.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( -80, 0, -85 ) );

#ifdef TEST_FORWARD_PLUS
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
                pointLights[ pointLightIndex ].GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( -150 + (float)row * 5, -18, -150 + (float)col * 4 ) );

                scene.Add( &pointLights[ pointLightIndex ] );
                ++pointLightIndex;
            }
        }
    }
#endif

    rtTex.Create2D( 512, 512, ae3d::DataType::UByte, ae3d::TextureWrap::Clamp, ae3d::TextureFilter::Linear, "render texture", false );
#ifdef TEST_BLOOM
    bloomTex.CreateUAV( self.view.bounds.size.width, self.view.bounds.size.height, "bloomTex" );
    blurTex.CreateUAV( self.view.bounds.size.width, self.view.bounds.size.height, "blurTex" );
    blurTex2.CreateUAV( self.view.bounds.size.width, self.view.bounds.size.height, "blurTex" );
#endif
    ssaoTex.CreateUAV( self.view.bounds.size.width * 2, self.view.bounds.size.height * 2, "ssaoTex" );
    
    renderTextureContainer.AddComponent<ae3d::SpriteRendererComponent>();
#ifdef TEST_RENDER_TEXTURE_2D
    renderTextureContainer.GetComponent<ae3d::SpriteRendererComponent>()->SetTexture( &rtTex, ae3d::Vec3( 250, 150, -0.6f ), ae3d::Vec3( 256, 256, 1 ), ae3d::Vec4( 1, 1, 1, 1 ) );
#endif
    renderTextureContainer.SetLayer( 2 );

    rtCamera.AddComponent<ae3d::CameraComponent>();
    rtCamera.GetComponent<ae3d::CameraComponent>()->SetProjection( 45, 4.0f / 3.0f, 1, 200 );
    rtCamera.GetComponent<ae3d::CameraComponent>()->SetProjectionType( ae3d::CameraComponent::ProjectionType::Perspective );
    rtCamera.GetComponent<ae3d::CameraComponent>()->SetClearFlag( ae3d::CameraComponent::ClearFlag::DepthAndColor );
    rtCamera.GetComponent<ae3d::CameraComponent>()->SetClearColor( ae3d::Vec3( 0.5f, 0, 0 ) );
    rtCamera.GetComponent<ae3d::CameraComponent>()->SetTargetTexture( &rtTex );
    rtCamera.AddComponent<ae3d::TransformComponent>();
    rtCamera.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( 5, 5, 20 ) );

    cubeRT.CreateCube( 512, ae3d::DataType::UByte, ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Linear, "cube RT" );

    cameraCubeRT.AddComponent<CameraComponent>();
    cameraCubeRT.GetComponent<CameraComponent>()->SetClearColor( Vec3( 0, 0, 1 ) );
    cameraCubeRT.GetComponent<CameraComponent>()->SetProjectionType( CameraComponent::ProjectionType::Perspective );
    cameraCubeRT.GetComponent<CameraComponent>()->SetProjection( 45, 1, 1, 400 );
    cameraCubeRT.GetComponent<CameraComponent>()->SetTargetTexture( &cubeRT );
    cameraCubeRT.GetComponent<CameraComponent>()->SetClearFlag( CameraComponent::ClearFlag::DepthAndColor );
    cameraCubeRT.AddComponent<TransformComponent>();
    cameraCubeRT.GetComponent<TransformComponent>()->LookAt( { 5, 0, -70 }, { 0, 0, -100 }, { 0, 1, 0 } );

    scene.SetAmbient( { 0.1f, 0.1f, 0.1f } );

#ifdef TEST_FORWARD_PLUS
    scene.Add( &pbrCube );
#endif
    //scene.Add( &cubePTN2 );
    //scene.Add( &cubePTN );
    //scene.Add( &rtCube );
    scene.Add( &rotatingCube );
    //scene.Add( &wireframeGo );
    scene.Add( &spriteContainer );
    scene.Add( &textSDF );
    scene.Add( &text );
    scene.Add( &animatedGo );
#ifdef TEST_SHADOWS_POINT
    scene.Add( &pointLight );
#endif
    scene.Add( &spotLight );
//#ifdef TEST_SHADOWS_DIR
    scene.Add( &dirLight );
//#endif
#ifdef TEST_RENDER_TEXTURE_2D
    scene.Add( &renderTextureContainer );
    scene.Add( &rtCamera );
#endif
#ifdef TEST_RENDER_TEXTURE_CUBE
    scene.Add( &renderTextureContainer );
    scene.Add( &cameraCubeRT );
#endif
    transTex.Load( ae3d::FileSystem::FileContents( "font.png" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Linear, ae3d::Mipmaps::None,
                  ae3d::ColorSpace::SRGB, ae3d::Anisotropy::k1 );

    transMaterial.SetShader( &shader );
    transMaterial.SetTexture( &transTex, 0 );

    transMaterial.SetBackFaceCulling( true );
    transMaterial.SetBlendingMode( ae3d::Material::BlendingMode::Alpha );
    //rotatingCube.GetComponent<ae3d::MeshRendererComponent>()->SetMaterial( &transMaterial, 0 );

    lineProjection.MakeProjection( 0, self.view.bounds.size.width, self.view.bounds.size.height, 0, 0, 1 );
    std::vector< Vec3 > lines( 4 );
    lines[ 0 ] = Vec3( 10, 10, -0.5f );
    lines[ 1 ] = Vec3( 50, 10, -0.5f );
    lines[ 2 ] = Vec3( 50, 50, -0.5f );
    lines[ 3 ] = Vec3( 10, 10, -0.5f );
    lineHandle = System::CreateLineBuffer( lines.data(), (int)lines.size(), Vec3( 1, 0, 0 ) );

    coneLineHandle = CreateConeLines();

#ifdef TEST_NUKLEAR_UI
    nk_font_atlas_init_default( &atlas );
    nk_font_atlas_begin( &atlas );

    nkFont = nk_font_atlas_add_default( &atlas, 13.0f, nullptr );
    const void* image = nk_font_atlas_bake( &atlas, &atlasWidth, &atlasHeight, NK_FONT_ATLAS_RGBA32 );

    uiTextures[ 0 ].LoadFromData( image, atlasWidth, atlasHeight, 4, "Nuklear font" );
    nk_font_atlas_end( &atlas, nk_handle_id( nkFontTexture.GetID() ), &nullTexture );

    nk_init_default( &ctx, &nkFont->handle );
    nk_buffer_init_default( &cmds );
#endif
}

- (void)keyDown:(NSEvent *)theEvent
{
    const float velocity = 0.3f;

    // Keycodes from: https://forums.macrumors.com/threads/nsevent-keycode-list.780577/
    if ([theEvent keyCode] == 0x00) // A
    {
        moveDir.x = -velocity;
    }
    else if ([theEvent keyCode] == 0x02) // D
    {
        moveDir.x = velocity;
    }
    else if ([theEvent keyCode] == 0x0D) // W
    {
        moveDir.z = -velocity;
    }
    else if ([theEvent keyCode] == 0x01) // S
    {
        moveDir.z = velocity;
    }
    else if ([theEvent keyCode] == 0x0C) // Q
    {
        moveDir.y = -velocity;
    }
    else if ([theEvent keyCode] == 0x0E) // E
    {
        moveDir.y = velocity;
    }
}

- (void)keyUp:(NSEvent *)theEvent
{
    if ([theEvent keyCode] == 0x00) // A
    {
        moveDir.x = 0;
    }
    else if ([theEvent keyCode] == 0x02) // D
    {
        moveDir.x = 0;
    }
    else if ([theEvent keyCode] == 0x0D) // W
    {
        moveDir.z = 0;
    }
    else if ([theEvent keyCode] == 0x01) // S
    {
        moveDir.z = 0;
    }
    else if ([theEvent keyCode] == 0x0C) // Q
    {
        moveDir.y = 0;
    }
    else if ([theEvent keyCode] == 0x0E) // E
    {
        moveDir.y = 0;
    }
}

- (void)mouseDown:(NSEvent *)theEvent
{
    camera3d.GetComponent<ae3d::TransformComponent>()->MoveForward( -1 );

    inputEvent.button = 1;
    inputEvent.x = (int)theEvent.locationInWindow.x;
    inputEvent.y = /*self.view.bounds.size.height -*/ (int)theEvent.locationInWindow.y;
    inputEvent.isActive = true;
}

- (void)mouseUp:(NSEvent *)theEvent
{
    inputEvent.button = 0;
    inputEvent.x = (int)theEvent.locationInWindow.x;
    inputEvent.y = /*self.view.bounds.size.height -*/ (int)theEvent.locationInWindow.y;
    inputEvent.isActive = true;
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

- (void)_render
{
    [self _update];

    const int width = self.view.bounds.size.width * 2;
    const int height = self.view.bounds.size.height * 2;

    //if (_view.currentRenderPassDescriptor != nil)
    {
        ae3d::System::SetCurrentDrawableMetal( _view );
        ae3d::System::BeginFrame();
        scene.Render();
        System::Draw( &cameraTex, 0, 0, width, height, width, height, Vec4( 1, 1, 1, 1 ), System::BlendMode::Off );
        System::Draw( &camera2dTex, 0, 0, width, height, width, height, Vec4( 1, 1, 1, 1 ), System::BlendMode::Alpha );
#ifdef TEST_BLOOM
        downSampleAndThresholdShader.SetRenderTexture( 0, &cameraTex );
        downSampleAndThresholdShader.SetTexture2D( 1, &blurTex );
        downSampleAndThresholdShader.Dispatch( width / 16, height / 16, 1 );

        blurShader.SetTexture2D( 0, &blurTex );
        blurShader.SetTexture2D( 1, &bloomTex );
        blurShader.SetUniform( ComputeShader::UniformName::TilesZW, 1, 0 );
        blurShader.Dispatch( self.view.bounds.size.width / 16, self.view.bounds.size.height / 16, 1 );

        blurShader.SetTexture2D( 0, &bloomTex );
        blurShader.SetTexture2D( 1, &blurTex );
        blurShader.SetUniform( ComputeShader::UniformName::TilesZW, 0, 1 );
        blurShader.Dispatch( self.view.bounds.size.width / 16, self.view.bounds.size.height / 16, 1 );

        // Second blur
        blurShader.SetTexture2D( 0, &blurTex );
        blurShader.SetTexture2D( 1, &blurTex2 );
        blurShader.SetUniform( ComputeShader::UniformName::TilesZW, 1, 0 );
        blurShader.Dispatch( self.view.bounds.size.width / 16, self.view.bounds.size.height / 16, 1 );

        blurShader.SetTexture2D( 0, &blurTex2 );
        blurShader.SetTexture2D( 1, &blurTex );
        blurShader.SetUniform( ComputeShader::UniformName::TilesZW, 0, 1 );
        blurShader.Dispatch( self.view.bounds.size.width / 16, self.view.bounds.size.height / 16, 1 );

        // Third blur
        blurShader.SetTexture2D( 0, &blurTex );
        blurShader.SetTexture2D( 1, &blurTex2 );
        blurShader.SetUniform( ComputeShader::UniformName::TilesZW, 1, 0 );
        blurShader.Dispatch( self.view.bounds.size.width / 16, self.view.bounds.size.height / 16, 1 );

        blurShader.SetTexture2D( 0, &blurTex2 );
        blurShader.SetTexture2D( 1, &blurTex );
        blurShader.SetUniform( ComputeShader::UniformName::TilesZW, 0, 1 );
        blurShader.Dispatch( self.view.bounds.size.width / 16, self.view.bounds.size.height / 16, 1 );

        System::Draw( &cameraTex, 0, 0, width, height, width, height, Vec4( 1, 1, 1, 1 ), System::BlendMode::Off );
        System::Draw( &blurTex, 0, 0, width, height, width, height, Vec4( 1, 1, 1, 1 ), System::BlendMode::Additive );
#endif
#ifdef TEST_SSAO
        ssaoShader.SetRenderTexture( 0, &cameraTex );
        ssaoShader.SetRenderTexture( 1, &camera3d.GetComponent<ae3d::CameraComponent>()->GetDepthNormalsTexture() );
        ssaoShader.SetTexture2D( 2, &ssaoTex );
        ssaoShader.SetProjectionMatrix( camera3d.GetComponent<ae3d::CameraComponent>()->GetProjection() );
        ssaoShader.Dispatch( self.view.bounds.size.width / 8, self.view.bounds.size.height / 8, 1 );

        System::Draw( &ssaoTex, 0, 0, width, height, width, height, Vec4( 1, 1, 1, 1 ), System::BlendMode::Off );
#endif
        //scene2.Render();
        Matrix44 viewMat = camera3d.GetComponent< CameraComponent >()->GetView();
        Matrix44 lineTransform;
        lineTransform.MakeIdentity();

        Matrix44 spotRot;
        spotLight.GetComponent<ae3d::TransformComponent>()->GetLocalRotation().GetMatrix( spotRot );

        lineTransform.Scale( 4, 4, 18 );
        Matrix44::Multiply( lineTransform, spotRot, lineTransform );
        lineTransform.Translate( spotLight.GetComponent<ae3d::TransformComponent>()->GetLocalPosition() );
        Matrix44::Multiply( lineTransform, viewMat, viewMat );
        //System::DrawLines( coneLineHandle, viewMat,
        //                  camera3d.GetComponent< CameraComponent >()->GetProjection(), self.view.bounds.size.width * 2, self.view.bounds.size.height * 2 );
        rotatingCube.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( spotLight.GetComponent<ae3d::TransformComponent>()->GetLocalPosition() + Vec3( 0, 2, 16 ) );

#ifdef TEST_NUKLEAR_UI
        nk_input_begin( &ctx );
        if (inputEvent.button == 1 && inputEvent.isActive)
        {
            nk_input_button( &ctx, NK_BUTTON_LEFT, inputEvent.x, inputEvent.y, 1 );
            nk_input_motion( &ctx, inputEvent.x, inputEvent.y );
        }
        if (inputEvent.button == 0 && inputEvent.isActive)
        {
            nk_input_button( &ctx, NK_BUTTON_LEFT, inputEvent.x, inputEvent.y, 0 );
            nk_input_motion( &ctx, inputEvent.x, inputEvent.y );
        }
        inputEvent.isActive = false;
        inputEvent.x = 0;
        inputEvent.y = 0;
        inputEvent.button = -1;

        nk_input_end( &ctx );

        enum { EASY, HARD };
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
            if (nk_option_label( &ctx, "easy", op == EASY )) op = EASY;
            if (nk_option_label( &ctx, "hard", op == HARD )) op = HARD;

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
        scene.EndFrame();
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

    char statStr[ 512 ] = {};
    ae3d::System::Statistics::GetStatistics( statStr );
    text.GetComponent<ae3d::TextRendererComponent>()->SetText( statStr );

    static int animationFrame = 0;
    ++animationFrame;
    animatedGo.GetComponent< MeshRendererComponent >()->SetAnimationFrame( animationFrame );

    // Testing vertex buffer growing
    if (angle == 5)
    {
        text.GetComponent<ae3d::TextRendererComponent>()->SetText( "this is a long string. this is a long string" );
    }

    pointLight.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( -9.8f, 0, -85 ) );

    camera3d.GetComponent<TransformComponent>()->MoveUp( moveDir.y );
    camera3d.GetComponent<TransformComponent>()->MoveForward( moveDir.z );
    camera3d.GetComponent<TransformComponent>()->MoveRight( moveDir.x );

#ifdef TEST_FORWARD_PLUS
    static float y = -14;
    y += 0.1f;

    if (y > 30)
    {
        y = -14;
    }

    for (unsigned pointLightIndex = 0; pointLightIndex < POINT_LIGHT_COUNT; ++pointLightIndex)
    {
        const Vec3 oldPos = pointLights[ pointLightIndex ].GetComponent<ae3d::TransformComponent>()->GetLocalPosition();
        const float xOffset = (Random100() % 10) / 20.0f - (Random100() % 10) / 20.0f;
        const float zOffset = (Random100() % 10) / 20.0f - (Random100() % 10) / 20.0f;

        pointLights[ pointLightIndex ].GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( oldPos.x + xOffset, -18, oldPos.z + zOffset ) );
    }
#endif
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
