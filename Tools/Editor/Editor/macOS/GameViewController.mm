#import "GameViewController.h"
#import "Inspector.hpp"
#import "System.hpp"
#import "SceneView.hpp"

NSViewController* myViewController;

@implementation GameViewController
{
    MTKView *_view;
    SceneView sceneView;
    Inspector inspector;
    ae3d::Vec3 moveDir;
}

const int MAX_VERTEX_MEMORY = 512 * 1024;
const int MAX_ELEMENT_MEMORY = 128 * 1024;

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    _view = (MTKView *)self.view;
    _view.delegate = self;
    _view.device = MTLCreateSystemDefaultDevice();
    _view.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    _view.sampleCount = 1;
    _view.colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;

    myViewController = self;
    
    ae3d::System::InitMetal( _view.device, _view, 1, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY );
    ae3d::System::LoadBuiltinAssets();
    
    sceneView.Init( self.view.bounds.size.width * 2, self.view.bounds.size.height * 2 );
    inspector.Init();
}

- (void)keyDown:(NSEvent *)theEvent
{
    NSLog(@"keyDown");
    
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
    NSLog(@"mouseDown");
}

- (void)mouseUp:(NSEvent *)theEvent
{
    NSLog(@"mouseUp");
}

- (void)mouseDragged:(NSEvent *)theEvent
{
    sceneView.RotateCamera( -float( theEvent.deltaX ) / 20, -float( theEvent.deltaY ) / 20 );
}

- (void)_render
{
    sceneView.MoveCamera( moveDir );

    const int width = self.view.bounds.size.width * 2;
    const int height = self.view.bounds.size.height * 2;
    
    if (_view.currentRenderPassDescriptor != nil)
    {
        ae3d::System::SetCurrentDrawableMetal( _view.currentDrawable, _view.currentRenderPassDescriptor );
        ae3d::System::BeginFrame();
        sceneView.BeginRender();
        inspector.Render( width, height );
        sceneView.EndRender();
        ae3d::System::EndFrame();
    }
}

- (void)drawInMTKView:(nonnull MTKView *)view
{
    @autoreleasepool {
        [self _render];
    }
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size {

}


- (void)encodeWithCoder:(nonnull NSCoder *)aCoder {

}

@end
