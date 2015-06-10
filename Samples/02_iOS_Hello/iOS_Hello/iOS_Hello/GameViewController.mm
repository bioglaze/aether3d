#import "GameViewController.h"
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include "Aether3D_iOS.framework/Headers/GameObject.hpp"
#include "Aether3D_iOS.framework/Headers/CameraComponent.hpp"
#include "Aether3D_iOS.framework/Headers/SpriteRendererComponent.hpp"
#include "Aether3D_iOS.framework/Headers/TextRendererComponent.hpp"
#include "Aether3D_iOS.framework/Headers/TransformComponent.hpp"
#include "Aether3D_iOS.framework/Headers/System.hpp"
#include "Aether3D_iOS.framework/Headers/Scene.hpp"
#include "Aether3D_iOS.framework/Headers/Texture2D.hpp"
#include "Aether3D_iOS.framework/Headers/AudioSourceComponent.hpp"
#include "Aether3D_iOS.framework/Headers/AudioClip.hpp"
#include "Aether3D_iOS.framework/Headers/Font.hpp"
#include "Aether3D_iOS.framework/Headers/FileSystem.hpp"
#include "Aether3D_iOS.framework/Headers/RenderTexture.hpp"

@implementation GameViewController
{
    CAMetalLayer *_metalLayer;
    BOOL _layerSizeDidUpdate;
    
    CADisplayLink *_timer;
    
    ae3d::GameObject camera;
    ae3d::GameObject sprite;
    ae3d::GameObject text;
    ae3d::GameObject audioSource;
    ae3d::Texture2D spriteTex;
    ae3d::Texture2D spriteTexPVRv2;
    ae3d::Texture2D spriteTexPVRv3;
    ae3d::Texture2D fontTex;
    ae3d::Scene scene;
    ae3d::AudioClip audioClip;
    ae3d::Font font;
    ae3d::GameObject rtCamera;
    ae3d::RenderTexture2D rtTex;
    ae3d::GameObject renderTextureContainer;
}

- (void)dealloc
{
    [_timer invalidate];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    _metalLayer = [CAMetalLayer layer];

    ae3d::System::InitMetal( _metalLayer );
    
    [_metalLayer setFrame:self.view.layer.frame];
    [self.view.layer addSublayer:_metalLayer];

    self.view.opaque = YES;
    self.view.backgroundColor = nil;
    self.view.contentScaleFactor = [UIScreen mainScreen].scale;

    _timer = [CADisplayLink displayLinkWithTarget:self selector:@selector(_gameloop)];
    [_timer addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];

    ae3d::System::LoadBuiltinAssets();
    ae3d::System::InitAudio();
    
    camera.AddComponent<ae3d::CameraComponent>();
    camera.GetComponent<ae3d::CameraComponent>()->SetProjection(0, self.view.bounds.size.width, self.view.bounds.size.height, 0, 0, 1);
    scene.Add( &camera );

    spriteTex.Load( ae3d::FileSystem::FileContents( "/Assets/glider120.png" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Nearest, ae3d::Mipmaps::None );
    spriteTexPVRv2.Load( ae3d::FileSystem::FileContents( "/Assets/checker.pvr" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Nearest, ae3d::Mipmaps::None );
    spriteTexPVRv3.Load( ae3d::FileSystem::FileContents( "/Assets/hotair.2bpp.pvr" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Nearest, ae3d::Mipmaps::None );

    sprite.AddComponent<ae3d::SpriteRendererComponent>();
    sprite.GetComponent<ae3d::SpriteRendererComponent>()->SetTexture(&spriteTexPVRv3, ae3d::Vec3( 60, 60, -0.6f ), ae3d::Vec3( 100, 100, 1 ), ae3d::Vec4( 1, 1, 1, 1 ) );
    sprite.GetComponent<ae3d::SpriteRendererComponent>()->SetTexture(&spriteTex, ae3d::Vec3( 180, 60, -0.6f ), ae3d::Vec3( 100, 100, 1 ), ae3d::Vec4( 1, 1, 1, 1 ) );
    //sprite.GetComponent<ae3d::SpriteRendererComponent>()->SetTexture(&spriteTexPVRv2, ae3d::Vec3( 240, 60, -0.6f ), ae3d::Vec3( 100, 100, 1 ), ae3d::Vec4( 1, 1, 1, 1 ) );
    scene.Add( &sprite );

    audioClip.Load( ae3d::FileSystem::FileContents( "/Assets/explosion.wav" ) );
    audioSource.AddComponent<ae3d::AudioSourceComponent>();
    audioSource.GetComponent<ae3d::AudioSourceComponent>()->SetClipId( audioClip.GetId() );
    audioSource.GetComponent<ae3d::AudioSourceComponent>()->Play();
    
    fontTex.Load( ae3d::FileSystem::FileContents( "/Assets/font.png" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Nearest, ae3d::Mipmaps::None );

    font.LoadBMFont( &fontTex, ae3d::FileSystem::FileContents("/Assets/font_txt.fnt"));
    text.AddComponent<ae3d::TextRendererComponent>();
    text.GetComponent<ae3d::TextRendererComponent>()->SetText("Aether3D Game Engine");
    text.GetComponent<ae3d::TextRendererComponent>()->SetFont( &font );
    text.GetComponent<ae3d::TextRendererComponent>()->SetColor( ae3d::Vec4( 1, 0, 0, 1 ) );
    //text.AddComponent<ae3d::TransformComponent>();
    //text.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( 40, 40, 0 ) );
    
    scene.Add( &text );
    
    rtTex.Create( 512, 512, ae3d::TextureWrap::Clamp, ae3d::TextureFilter::Linear );
    
    renderTextureContainer.AddComponent<ae3d::SpriteRendererComponent>();
    renderTextureContainer.GetComponent<ae3d::SpriteRendererComponent>()->SetTexture( &rtTex, ae3d::Vec3( 150, 250, -0.6f ), ae3d::Vec3( (float)spriteTex.GetWidth(), (float)spriteTex.GetHeight(), 1 ), ae3d::Vec4( 1, 1, 1, 1 ) );
    
    rtCamera.AddComponent<ae3d::CameraComponent>();
    rtCamera.GetComponent<ae3d::CameraComponent>()->SetProjection( 0, (float)rtTex.GetWidth(), 0,(float)rtTex.GetHeight(), 0, 1 );
    rtCamera.GetComponent<ae3d::CameraComponent>()->SetClearColor( ae3d::Vec3( 0.5f, 0.5f, 0.5f ) );
    rtCamera.GetComponent<ae3d::CameraComponent>()->SetTargetTexture( &rtTex );
    scene.Add( &renderTextureContainer );
    scene.Add( &rtCamera );
}

-(void) touchesBegan: (NSSet *) touches withEvent: (UIEvent *) event
{
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    UITouch *touch = [touches anyObject];
    CGPoint location = [touch locationInView: self.view];
    
    sprite.GetComponent<ae3d::SpriteRendererComponent>()->Clear();
    sprite.GetComponent<ae3d::SpriteRendererComponent>()->SetTexture(&spriteTex, ae3d::Vec3( location.x, location.y, -0.6f ), ae3d::Vec3( 100, 100, 1 ), ae3d::Vec4( 1, 1, 1, 1 ) );
    sprite.GetComponent<ae3d::SpriteRendererComponent>()->SetTexture(&spriteTexPVRv2, ae3d::Vec3( 180, 60, -0.6f ), ae3d::Vec3( 100, 100, 1 ), ae3d::Vec4( 1, 1, 1, 1 ) );
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (BOOL)prefersStatusBarHidden {
    return YES;
}

- (void)_reshape
{
}

- (void)_update
{
}

// The main game loop called by the CADisplayLine timer
- (void)_gameloop
{
    @autoreleasepool {
        if (_layerSizeDidUpdate)
        {
            CGFloat nativeScale = self.view.window.screen.nativeScale;
            CGSize drawableSize = self.view.bounds.size;
            drawableSize.width *= nativeScale;
            drawableSize.height *= nativeScale;
            _metalLayer.drawableSize = drawableSize;
            [self _reshape];
            _layerSizeDidUpdate = NO;
        }
        
        ae3d::System::BeginFrame();
        scene.Render();
        ae3d::System::EndFrame();
    }
}

// Called whenever view changes orientation or layout is changed
- (void)viewDidLayoutSubviews
{
    _layerSizeDidUpdate = YES;
    [_metalLayer setFrame:self.view.layer.frame];
}

@end
