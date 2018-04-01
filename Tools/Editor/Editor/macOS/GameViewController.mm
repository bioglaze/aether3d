#import "GameViewController.h"
#import "System.hpp"

@implementation GameViewController
{
    MTKView *_view;
}

const int MAX_VERTEX_MEMORY = 512 * 1024;
const int MAX_ELEMENT_MEMORY = 128 * 1024;

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    _view = (MTKView *)self.view;
    //_view.delegate = self;
    _view.device = MTLCreateSystemDefaultDevice();
    _view.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    _view.sampleCount = 1;
    _view.colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;

    ae3d::System::InitMetal( _view.device, _view, 1, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY );
    ae3d::System::LoadBuiltinAssets();
}

@end
