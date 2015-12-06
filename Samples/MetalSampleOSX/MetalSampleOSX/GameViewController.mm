#import "GameViewController.h"
#import <Metal/Metal.h>
#import <simd/simd.h>
#import <MetalKit/MetalKit.h>

#import "CameraComponent.hpp"
#import "SpriteRendererComponent.hpp"
#import "TextRendererComponent.hpp"
#import "MeshRendererComponent.hpp"
#import "TransformComponent.hpp"
#import "System.hpp"
#import "Font.hpp"
#import "FileSystem.hpp"
#import "GameObject.hpp"
#import "Material.hpp"
#import "Mesh.hpp"
#import "Texture2D.hpp"
#import "Shader.hpp"
#import "Scene.hpp"

@implementation GameViewController
{
    MTKView* _view;
    id <MTLDevice> device;

    ae3d::GameObject camera2d;
    ae3d::GameObject camera3d;
    ae3d::GameObject cube;
    ae3d::GameObject text;
    ae3d::Scene scene;
    ae3d::Font font;
    ae3d::Mesh cubeMesh;
    ae3d::Material cubeMaterial;
    ae3d::Shader shader;
    ae3d::Texture2D fontTex;
    ae3d::Texture2D gliderTex;
}

- (void)viewDidLoad
{
    // This sample's assets are referenced from aether3d_build/Samples. Make sure that they exist.
    // Assets can be downloaded from http://twiren.kapsi.fi/files/aether3d_sample_v0.4.zip
    
    [super viewDidLoad];

    device = MTLCreateSystemDefaultDevice();
    assert( device );
    
    ae3d::System::InitMetalOSX( device, _view );
    ae3d::System::LoadBuiltinAssets();
    //ae3d::System::InitAudio();

    [self _setupView];
    [self _reshape];

    camera2d.AddComponent<ae3d::CameraComponent>();
    camera2d.GetComponent<ae3d::CameraComponent>()->SetProjection( 0, self.view.bounds.size.width, self.view.bounds.size.height, 0, 0, 1 );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetProjectionType( ae3d::CameraComponent::ProjectionType::Orthographic );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetClearFlag( ae3d::CameraComponent::ClearFlag::Depth );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetClearColor( ae3d::Vec3( 0.5f, 0.0f, 0.0f ) );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetLayerMask( 0x2 );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetRenderOrder( 2 );
    camera2d.AddComponent<ae3d::TransformComponent>();
    scene.Add( &camera2d );

    camera3d.AddComponent<ae3d::CameraComponent>();
    camera3d.GetComponent<ae3d::CameraComponent>()->SetProjection( 45, 4.0f / 3.0f, 1, 200 );
    camera3d.GetComponent<ae3d::CameraComponent>()->SetClearColor( ae3d::Vec3( 0.5f, 0.5f, 0.5f ) );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetClearFlag( ae3d::CameraComponent::ClearFlag::DepthAndColor );
    camera3d.GetComponent<ae3d::CameraComponent>()->SetProjectionType( ae3d::CameraComponent::ProjectionType::Perspective );
    camera3d.GetComponent<ae3d::CameraComponent>()->SetRenderOrder( 1 );
    camera3d.AddComponent<ae3d::TransformComponent>();
    scene.Add( &camera3d );
    
    fontTex.Load( ae3d::FileSystem::FileContents( "/font.png" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Nearest, ae3d::Mipmaps::None, ae3d::ColorSpace::RGB, 1 );
    gliderTex.Load( ae3d::FileSystem::FileContents( "/glider.png" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Nearest, ae3d::Mipmaps::None, ae3d::ColorSpace::RGB, 1 );
    
    font.LoadBMFont( &fontTex, ae3d::FileSystem::FileContents( "/font_txt.fnt" ) );
    text.AddComponent<ae3d::TextRendererComponent>();
    text.GetComponent<ae3d::TextRendererComponent>()->SetText( "Aether3D Game Engine" );
    text.GetComponent<ae3d::TextRendererComponent>()->SetFont( &font );
    text.GetComponent<ae3d::TextRendererComponent>()->SetColor( ae3d::Vec4( 1, 0, 0, 1 ) );
    text.AddComponent<ae3d::TransformComponent>();
    text.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( 40, 40, 0 ) );
    text.SetLayer( 2 );
    scene.Add( &text );
    
    shader.Load( ae3d::FileSystem::FileContents( "" ), ae3d::FileSystem::FileContents( "" ),
                "unlit_vertex", "unlit_fragment",
                ae3d::FileSystem::FileContents(""), ae3d::FileSystem::FileContents( "" ) );
    
    cubeMaterial.SetShader( &shader );
    cubeMaterial.SetTexture( "textureMap", &gliderTex );
    cubeMaterial.SetVector( "tintColor", { 1, 0, 0, 1 } );
    
    cubeMesh.Load( ae3d::FileSystem::FileContents( "/textured_cube.ae3d" ) );
    cube.AddComponent<ae3d::MeshRendererComponent>();
    cube.GetComponent<ae3d::MeshRendererComponent>()->SetMesh( &cubeMesh );
    cube.GetComponent<ae3d::MeshRendererComponent>()->SetMaterial( &cubeMaterial, 0 );
    cube.AddComponent<ae3d::TransformComponent>();
    cube.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( 0, 0, -10 ) );
    scene.Add( &cube );
}

- (void)_setupView
{
    _view = (MTKView *)self.view;
    _view.delegate = self;
    _view.device = device;
    _view.sampleCount = 1;
    _view.depthStencilPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
}

- (void)_render
{
    [self _update];
    ae3d::System::SetCurrentDrawableMetalOSX( _view.currentDrawable, _view.currentRenderPassDescriptor );
    scene.Render();
    ae3d::System::EndFrame();
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
    cube.GetComponent< ae3d::TransformComponent >()->SetLocalRotation( rotation );
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
