//
//  GameViewController.m
//  MetalSampleOSX
//
//  Created by Timo Wiren on 04/12/15.
//  Copyright (c) 2015 Timo Wiren. All rights reserved.
//

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
    ae3d::GameObject camera2d;
    ae3d::GameObject cube;
    ae3d::GameObject text;
    ae3d::Scene scene;
    ae3d::Font font;
    ae3d::GameObject rtCamera;
    ae3d::GameObject perspCamera;
    ae3d::Mesh cubeMesh;
    ae3d::Material cubeMaterial;
    ae3d::Material whiteMaterial;
    ae3d::Shader shader;
    ae3d::Texture2D fontTex;
    id <MTLDevice> device;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    device = MTLCreateSystemDefaultDevice();
    assert( device );
    
    ae3d::System::InitMetalOSX( device, _view );
    ae3d::System::LoadBuiltinAssets();

    [self _setupView];
    [self _reshape];
    // Fallback to a blank NSView, an application could also fallback to OpenGL here.
    //{
        //NSLog(@"Metal is not supported on this device");
        //self.view = [[NSView alloc] initWithFrame:self.view.frame];
    //}
    
    //ae3d::System::InitAudio();
    
    camera2d.AddComponent<ae3d::CameraComponent>();
    camera2d.GetComponent<ae3d::CameraComponent>()->SetProjection( 0, self.view.bounds.size.width, self.view.bounds.size.height, 0, 0, 1 );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetProjectionType( ae3d::CameraComponent::ProjectionType::Orthographic );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetClearFlag( ae3d::CameraComponent::ClearFlag::Depth );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetClearColor( ae3d::Vec3( 0.5f, 0.0f, 0.0f ) );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetLayerMask( 0x2 );
    camera2d.GetComponent<ae3d::CameraComponent>()->SetRenderOrder( 2 );
    camera2d.AddComponent<ae3d::TransformComponent>();
    scene.Add( &camera2d );

    fontTex.Load( ae3d::FileSystem::FileContents( "/font.png" ), ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Nearest, ae3d::Mipmaps::None, ae3d::ColorSpace::RGB, 1 );
    
    font.LoadBMFont( &fontTex, ae3d::FileSystem::FileContents( "/font_txt.fnt" ) );
    text.AddComponent<ae3d::TextRendererComponent>();
    text.GetComponent<ae3d::TextRendererComponent>()->SetText( "Aether3D Game Engine" );
    text.GetComponent<ae3d::TextRendererComponent>()->SetFont( &font );
    text.GetComponent<ae3d::TextRendererComponent>()->SetColor( ae3d::Vec4( 1, 0, 0, 1 ) );
    text.AddComponent<ae3d::TransformComponent>();
    text.GetComponent<ae3d::TransformComponent>()->SetLocalPosition( ae3d::Vec3( 40, 40, 0 ) );
    text.SetLayer( 2 );
    
    scene.Add( &text );
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
    if (_view.currentDrawable)
    {
        ae3d::System::SetCurrentDrawableMetalOSX( _view.currentDrawable, _view.currentRenderPassDescriptor );
        scene.Render();
        ae3d::System::EndFrame();
    }
}

- (void)_reshape
{
}

- (void)_update
{
}

// Called whenever view changes orientation or layout is changed
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

@end
