#import "Cocoa/Cocoa.h"
#include <string>
#include <string.h>
#import "GameViewController.h"
#include "GameObject.hpp"
#include "FileSystem.hpp"
#import "Inspector.hpp"
#import "System.hpp"
#import "SceneView.hpp"
#import "Vec3.hpp"

NSViewController* myViewController;

std::string GetOpenPath()
{
    NSOpenPanel *op = [NSOpenPanel openPanel];
    
    if ([op runModal] == NSModalResponseOK)
    {
        NSURL *nsurl = [[op URLs] objectAtIndex:0];
        std::string tmp = std::string([[nsurl path] UTF8String]);
        printf("GetOpenPath: %s\n", tmp.c_str());
        return std::string([[nsurl path] UTF8String]);
    }
    
    return "";
}

void GetOpenPath( char* outPath, const char* extension )
{
    static std::string path = GetOpenPath();
    strcpy( outPath, path.c_str() );
}

std::string GetSavePath()
{
    NSSavePanel *op = [NSSavePanel savePanel];
    [op setExtensionHidden:NO];
    [op setDirectoryURL:[NSURL URLWithString:NSHomeDirectory()]];
    
    if ([op runModal] == NSModalResponseOK)
    {
        NSMutableString* saveString = [NSMutableString stringWithString:
                                  [[op URL] absoluteString]];
        return std::string([saveString UTF8String]);
    }
    
    return "";
}

@implementation GameViewController
{
    MTKView *_view;
    SceneView* sceneView;
    Inspector inspector;
    ae3d::Vec3 moveDir;
    ae3d::GameObject* selectedGO;
}

struct InputEvent
{
    bool isActive;
    int x, y;
    int button;
};

InputEvent inputEvent;

const int MAX_VERTEX_MEMORY = 512 * 1024;
const int MAX_ELEMENT_MEMORY = 128 * 1024;

- (void)viewDidLoad
{
    sceneView = nullptr;
    selectedGO = nullptr;
    
    [super viewDidLoad];
    
    _view = (MTKView *)self.view;
    _view.delegate = self;
    _view.device = MTLCreateSystemDefaultDevice();
    _view.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    _view.sampleCount = 1;
    _view.colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;

    myViewController = self;
    
    ae3d::System::InitMetal( _view.device, _view, (int)_view.sampleCount, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY );
    ae3d::System::LoadBuiltinAssets();
    
    svInit( &sceneView, self.view.bounds.size.width * 2, self.view.bounds.size.height * 2 );
    inspector.Init();
}

- (BOOL)acceptsFirstResponder
{
    return YES;
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
    inputEvent.button = 1;
    inputEvent.x = (int)theEvent.locationInWindow.x;
    inputEvent.y = self.view.bounds.size.height - (int)theEvent.locationInWindow.y;
    inputEvent.isActive = true;
}

- (void)mouseUp:(NSEvent *)theEvent
{
    selectedGO = svSelectObject( sceneView, (int)theEvent.locationInWindow.x, (int)self.view.bounds.size.height - (int)theEvent.locationInWindow.y, (int)self.view.bounds.size.width, (int)self.view.bounds.size.height );
    NSLog(@"mouseUp");
    inputEvent.button = 0;
    inputEvent.x = (int)theEvent.locationInWindow.x;
    inputEvent.y = self.view.bounds.size.height - (int)theEvent.locationInWindow.y;
    inputEvent.isActive = true;
}

- (void)mouseDragged:(NSEvent *)theEvent
{
    const float deltaX = -float( theEvent.deltaX ) / 20;
    const float deltaY = -float( theEvent.deltaY ) / 20;
    
    if (!svIsTransformGizmoSelected( sceneView ))
    {
        svRotateCamera( sceneView, deltaX, deltaY );
    }
    
    svHandleMouseMotion( sceneView, deltaX, deltaY );
}

- (void)_render
{
    inspector.BeginInput();
    
    if (inputEvent.button == 1 && inputEvent.isActive)
    {
        svHandleLeftMouseDown( sceneView, inputEvent.x * 2, inputEvent.y * 2, (int)self.view.bounds.size.width, (int)self.view.bounds.size.height );
        inspector.HandleLeftMouseClick( inputEvent.x * 2, inputEvent.y * 2, 1 );
        inspector.HandleMouseMotion( inputEvent.x * 2, inputEvent.y * 2 );
    }

    if (inputEvent.button == 0 && inputEvent.isActive)
    {
        inspector.HandleLeftMouseClick( inputEvent.x * 2, inputEvent.y * 2, 0 );
    }
    
    inputEvent.isActive = false;
    inputEvent.x = 0;
    inputEvent.y = 0;
    inputEvent.button = -1;
    inspector.EndInput();
    
    if (_view.currentRenderPassDescriptor != nil && sceneView != nullptr)
    {
        const int width = self.view.bounds.size.width;
        const int height = self.view.bounds.size.height;

        Inspector::Command inspectorCommand;
        
        ae3d::System::SetCurrentDrawableMetal( _view );
        ae3d::System::BeginFrame();
        svMoveCamera( sceneView, moveDir );
        svBeginRender( sceneView );
        svDrawSprites( sceneView, width, height );
        int goCount = 0;
        ae3d::GameObject** gameObjects = svGetGameObjects( sceneView, goCount );
        inspector.Render( width, height, selectedGO, inspectorCommand, gameObjects, goCount );
        svEndRender( sceneView );
        ae3d::System::EndFrame();
        
        switch (inspectorCommand)
        {
            case Inspector::Command::CreateGO:
                svAddGameObject( sceneView );
                break;
            case Inspector::Command::OpenScene:
            {
                std::string path = GetOpenPath();
                NSLog(@"path: %s", path.c_str());
                if (path != "")
                {
                    auto contents = ae3d::FileSystem::FileContents( path.c_str() );
                    svLoadScene( sceneView, contents );
                }
            }
                break;
            case Inspector::Command::SaveScene:
            {
                std::string path = GetSavePath();
                
                if (path.length() > 7)
                {
                    // remove "file://"
                    path = path.substr( 7 );
                }
                
                NSLog(@"path: %s", path.c_str());
                if (path != "")
                {
                    svSaveScene( sceneView, (char*)path.c_str() );
                }
            }
                break;
            default:
                break;
        }
    }
}

- (void)drawInMTKView:(nonnull MTKView *)view
{
    @autoreleasepool { [self _render]; }
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size
{
}

- (void)encodeWithCoder:(nonnull NSCoder *)aCoder
{
}

@end
