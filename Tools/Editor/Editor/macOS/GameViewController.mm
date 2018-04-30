#import "GameViewController.h"
#import "System.hpp"
#import "SceneView.hpp"

@implementation GameViewController
{
    MTKView *_view;
    SceneView sceneView;
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

    ae3d::System::InitMetal( _view.device, _view, 1, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY );
    ae3d::System::LoadBuiltinAssets();
    
    sceneView.Init( self.view.bounds.size.width * 2, self.view.bounds.size.height * 2 );
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
    //[self _update];
    
    const int width = self.view.bounds.size.width * 2;
    const int height = self.view.bounds.size.height * 2;
    
    if (_view.currentRenderPassDescriptor != nil)
    {
        ae3d::System::SetCurrentDrawableMetal( _view.currentDrawable, _view.currentRenderPassDescriptor );
        ae3d::System::BeginFrame();
        sceneView.Render();
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
