#import <simd/simd.h>
#import "GameViewController.h"

#import "Aether3D_iOS/CameraComponent.hpp"
#import "Aether3D_iOS/DirectionalLightComponent.hpp"
#import "Aether3D_iOS/GameObject.hpp"
#import "Aether3D_iOS/FileSystem.hpp"
#import "Aether3D_iOS/Scene.hpp"
#import "Aether3D_iOS/Font.hpp"
#import "Aether3D_iOS/Mesh.hpp"
#import "Aether3D_iOS/MeshRendererComponent.hpp"
#import "Aether3D_iOS/PointLightComponent.hpp"
#import "Aether3D_iOS/Texture2D.hpp"
#import "Aether3D_iOS/TransformComponent.hpp"
#import "Aether3D_iOS/TextRendererComponent.hpp"
#import "Aether3D_iOS/SpotLightComponent.hpp"
#import "Aether3D_iOS/Shader.hpp"
#import "Aether3D_iOS/System.hpp"
#import "Aether3D_iOS/Material.hpp"

#define MULTISAMPLE_COUNT 1

struct MyTouch
{
    UITouch* uiTouch;
    int x, y;
};

struct MyTouch gTouches[ 8 ];
int gTouchCount;

@implementation GameViewController
{
    MTKView* _view;
    id <MTLDevice> device;
    
    ae3d::GameObject camera2d;
    ae3d::GameObject camera3d;
    ae3d::GameObject rtCamera;
    ae3d::GameObject cube;
    ae3d::GameObject cubeFloor;
    ae3d::GameObject text;
    ae3d::GameObject dirLight;
    ae3d::GameObject pointLight;
    ae3d::GameObject spotLight;
    ae3d::Scene scene;
    ae3d::Font font;
    ae3d::Mesh cubeMesh;
    ae3d::Material cubeMaterial;
    ae3d::Shader shader;
    ae3d::Texture2D fontTex;
    ae3d::Texture2D gliderTex;
    int touchBeginX;
    int touchBeginY;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    touchBeginX = 0;
    touchBeginY = 0;

    device = MTLCreateSystemDefaultDevice();
    assert( device );
    
    [self _setupView];
    [self _reshape];

    ae3d::System::InitMetal( device, _view, MULTISAMPLE_COUNT );
    ae3d::System::LoadBuiltinAssets();
    //ae3d::System::InitAudio();
    
    camera2d.AddComponent<ae3d::CameraComponent>();
    camera2d.GetComponent<ae3d::CameraComponent>()->SetProjection( 0, self.view.bounds.size.width, self.view.bounds.size.height, 0, 0, 1 );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetProjectionType( ae3d::CameraComponent::ProjectionType::Orthographic );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetClearFlag( ae3d::CameraComponent::ClearFlag::Depth );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetClearColor( ae3d::Vec3( 0.5f, 0.0f, 0.0f ) );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetLayerMask( 0x2 );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetRenderOrder( 2 );
    camera2d.AddComponent<ae3d::TransformComponent>();
    camera2d.SetName( "camera2d" );
    scene.Add( &camera2d );

    const float aspect = _view.bounds.size.width / (float)_view.bounds.size.height;

    camera3d.AddComponent<ae3d::CameraComponent>();
    camera3d.GetComponent<ae3d::CameraComponent>()->SetProjection( 45, aspect, 1, 200 );
    camera3d.GetComponent<ae3d::CameraComponent>()->SetClearColor( ae3d::Vec3( 1.0f, 0.5f, 0.5f ) );
    camera3d.GetComponent<ae3d::CameraComponent>()->SetClearFlag( ae3d::CameraComponent::ClearFlag::DepthAndColor );
    camera3d.GetComponent<ae3d::CameraComponent>()->SetProjectionType( ae3d::CameraComponent::ProjectionType::Perspective );
    camera3d.GetComponent<ae3d::CameraComponent>()->SetRenderOrder( 1 );
    camera3d.GetComponent<ae3d::CameraComponent>()->GetDepthNormalsTexture().Create2D( self.view.bounds.size.width, self.view.bounds.size.height, ae3d::RenderTexture::DataType::Float, ae3d::TextureWrap::Clamp, ae3d::TextureFilter::Nearest );
    camera3d.AddComponent<ae3d::TransformComponent>();
    camera3d.SetName( "camera3d" );
    scene.Add( &camera3d );
    
    fontTex.Load( ae3d::FileSystem::FileContents( "/font.png" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Nearest, ae3d::Mipmaps::None, ae3d::ColorSpace::RGB, ae3d::Anisotropy::k1 );
    gliderTex.Load( ae3d::FileSystem::FileContents( "/glider.png" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Nearest, ae3d::Mipmaps::None, ae3d::ColorSpace::RGB, ae3d::Anisotropy::k1 );
    
    font.LoadBMFont( &fontTex, ae3d::FileSystem::FileContents( "/font_txt.fnt" ) );
    text.AddComponent<ae3d::TextRendererComponent>();
    text.GetComponent<ae3d::TextRendererComponent>()->SetText( "Aether3D Game Engine" );
    text.GetComponent<ae3d::TextRendererComponent>()->SetFont( &font );
    text.GetComponent<ae3d::TextRendererComponent>()->SetColor( ae3d::Vec4( 1, 0, 0, 1 ) );
    text.AddComponent<ae3d::TransformComponent>();
    text.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( 5, 5, 0 ) );
    text.SetLayer( 2 );
    text.SetName( "text" );
    scene.Add( &text );
    
    shader.Load( ae3d::FileSystem::FileContents( "" ), ae3d::FileSystem::FileContents( "" ),
                "unlit_vertex", "unlit_fragment",
                ae3d::FileSystem::FileContents(""), ae3d::FileSystem::FileContents( "" ),
                ae3d::FileSystem::FileContents(""), ae3d::FileSystem::FileContents( "" ));
    
    cubeMaterial.SetShader( &shader );
    cubeMaterial.SetTexture( "textureMap", &gliderTex );
    cubeMaterial.SetVector( "tint", { 1, 0, 0, 1 } );
    
    cubeMesh.Load( ae3d::FileSystem::FileContents( "/textured_cube.ae3d" ) );
    
    cube.AddComponent<ae3d::MeshRendererComponent>();
    cube.GetComponent<ae3d::MeshRendererComponent>()->SetMesh( &cubeMesh );
    cube.GetComponent<ae3d::MeshRendererComponent>()->SetMaterial( &cubeMaterial, 0 );
    cube.AddComponent<ae3d::TransformComponent>();
    cube.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( 0, 0, -10 ) );
    cube.SetName( "cube" );
    scene.Add( &cube );

    cubeFloor.AddComponent<ae3d::MeshRendererComponent>();
    cubeFloor.GetComponent<ae3d::MeshRendererComponent>()->SetMesh( &cubeMesh );
    cubeFloor.GetComponent<ae3d::MeshRendererComponent>()->SetMaterial( &cubeMaterial, 0 );
    cubeFloor.AddComponent<ae3d::TransformComponent>();
    cubeFloor.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( 0, -7, -10 ) );
    cubeFloor.GetComponent<ae3d::TransformComponent>()->SetLocalScale( 4 );
    cubeFloor.SetName( "cubeFloor" );
    //scene.Add( &cubeFloor );

    dirLight.AddComponent<ae3d::DirectionalLightComponent>();
    dirLight.GetComponent<ae3d::DirectionalLightComponent>()->SetCastShadow( false, 1024 );
    dirLight.AddComponent<ae3d::TransformComponent>();
    dirLight.GetComponent<ae3d::TransformComponent>()->LookAt( { 0, 0, 0 }, ae3d::Vec3( 0, -1, 0 ).Normalized(), { 0, 1, 0 } );
    dirLight.SetName( "dirLight" );
    //scene.Add( &dirLight );

    pointLight.AddComponent<ae3d::PointLightComponent>();
    pointLight.GetComponent<ae3d::PointLightComponent>()->SetCastShadow( false, 1024 );
    pointLight.AddComponent<ae3d::TransformComponent>();
    pointLight.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( 0, -3, -10 ) );
    pointLight.SetName( "pointLight" );
    //scene.Add( &pointLight );

    spotLight.AddComponent<ae3d::SpotLightComponent>();
    spotLight.GetComponent<ae3d::SpotLightComponent>()->SetCastShadow( true, 1024 );
    spotLight.GetComponent<ae3d::SpotLightComponent>()->SetConeAngle( 45 );
    spotLight.AddComponent<ae3d::TransformComponent>();
    spotLight.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( 0, -3, -10 ) );
    spotLight.SetName( "spotLight" );
    //scene.Add( &pointLight );
}

- (void)_setupView
{
    _view = (MTKView *)self.view;
    _view.device = device;
    _view.delegate = self;
    
    // Setup the render target, choose values based on your app
    //_view.depthStencilPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    _view.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    _view.sampleCount = MULTISAMPLE_COUNT;
}

- (void)_render
{
    [self _update];

    ae3d::System::SetCurrentDrawableMetal( _view.currentDrawable, _view.currentRenderPassDescriptor );
    ae3d::System::BeginFrame();
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
    
    std::string stats = std::string( "draw calls:" ) + std::to_string( ae3d::System::Statistics::GetDrawCallCount() );
    text.GetComponent<ae3d::TextRendererComponent>()->SetText( stats.c_str() );
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size
{
    [self _reshape];
}

// Called whenever the view needs to render
- (void)drawInMTKView:(nonnull MTKView *)view
{
    @autoreleasepool {
        [self _render];
    }
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    for (UITouch* touch in touches)
    {
        const CGPoint touchLocation = [touch locationInView:touch.view];
        
        gTouches[ gTouchCount ].uiTouch = touch;
        gTouches[ gTouchCount ].x = touchLocation.x;
        gTouches[ gTouchCount ].y = touchLocation.y;
        
        touchBeginX = touchLocation.x;
        touchBeginY = touchLocation.y;
        
        ++gTouchCount;
    }
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    // Updates input object's touch locations.
    for (UITouch* touch in touches)
    {
        // Just testing camera movement
        {
            const CGPoint touchLocation = [touch locationInView:touch.view];

            float deltaX = touchLocation.x - touchBeginX;
            float deltaY = touchLocation.y - touchBeginY;
            
            camera3d.GetComponent<ae3d::TransformComponent>()->OffsetRotate( ae3d::Vec3( 0, 1, 0 ), deltaX / 40 );
            camera3d.GetComponent<ae3d::TransformComponent>()->OffsetRotate( ae3d::Vec3( 1, 0, 0 ), deltaY / 40 );
        }
        
        // Finds the correct touch and updates its location.
        for (int t = 0; t < gTouchCount; ++t)
        {
            if (gTouches[ t ].uiTouch == touch)
            {
                const CGPoint touchLocation = [touch locationInView:touch.view];
                
                gTouches[ t ].x = touchLocation.x;
                gTouches[ t ].y = touchLocation.y;
            }
        }
    }
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    for (UITouch* touch in touches)
    {
        for (int t = 0; t < gTouchCount; ++t)
        {
            if (touch == gTouches[ t ].uiTouch)
            {
                for (int j = t; j < gTouchCount - 1; ++j)
                {
                    gTouches[ j ].x = gTouches[ j + 1 ].x;
                    gTouches[ j ].y = gTouches[ j + 1 ].y;
                    gTouches[ j ].uiTouch = gTouches [ j + 1 ].uiTouch;
                }
                
                --gTouchCount;
            }
        }
    }
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event 
{
    gTouchCount = 0;
}

@end

