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
#import "Window.hpp"

NSViewController* myViewController;
const int InspectorWidth = 250;

static std::string GetOpenPath( const char* extension )
{
    NSMutableArray *types = [NSMutableArray new];
    [types addObject: [NSString stringWithUTF8String: extension]];
    
    NSOpenPanel *op = [NSOpenPanel openPanel];
    [op setAllowedFileTypes: types];
    
    if ([op runModal] == NSModalResponseOK)
    {
        NSURL *nsurl = [[op URLs] objectAtIndex:0];
        NSString *myString = [nsurl path];
        std::string tmp = [myString UTF8String];
        return tmp;
    }
    
    return "";
}

void GetOpenPath( char* outPath, const char* extension )
{
    std::string path = GetOpenPath( extension );
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
    bool isCmdDown;
}

struct InputEvent
{
    bool isActive;
    int x, y;
    int buttonState;
    int key;
    bool isDown;
    bool mouseMoved;
};

InputEvent inputEvent;

const int MAX_VERTEX_MEMORY = 512 * 1024;
const int MAX_ELEMENT_MEMORY = 128 * 1024;

- (void)viewDidLoad
{
    sceneView = nullptr;
    selectedGO = nullptr;
    
    inputEvent.isActive = false;
    inputEvent.mouseMoved = false;
    inputEvent.isDown = false;
    
    [super viewDidLoad];
    
    _view = (MTKView *)self.view;
    _view.delegate = self;
    _view.device = MTLCreateSystemDefaultDevice();
    _view.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    _view.sampleCount = 1;
    _view.colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;

    myViewController = self;
    isCmdDown = false;
    
    ae3d::System::InitMetal( _view.device, _view, (int)_view.sampleCount, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY );
    ae3d::System::LoadBuiltinAssets();
    ae3d::System::InitAudio();
    
    svInit( &sceneView, self.view.bounds.size.width * 2, self.view.bounds.size.height * 2 );
    inspector.Init();
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)keyDown:(NSEvent *)theEvent
{
    isCmdDown = [theEvent modifierFlags] & NSEventModifierFlagCommand;
    
    inputEvent.key = [theEvent keyCode];
    inputEvent.isDown = true;
    inputEvent.isActive = true;
    const float velocity = 0.3f;

    if (inspector.IsTextEditActive())
    {
        return;
    }

    // Keycodes from: https://forums.macrumors.com/threads/nsevent-keycode-list.780577/
    if ([theEvent keyCode] == 0x00) // A
    {
        moveDir.x = -velocity;
    }
    else if ([theEvent keyCode] == 0x02) // D
    {
        if (isCmdDown)
        {
            svDuplicateGameObject( sceneView );
        }
        else
        {
            moveDir.x = velocity;
        }
    }
    else if ([theEvent keyCode] == 0x0D) // W
    {
        moveDir.z = -velocity;
    }
    else if ([theEvent keyCode] == 3) // F
    {
        svFocusOnSelected( sceneView );
    }
    else if ([theEvent keyCode] == 0x01 && isCmdDown) // Cmd+S
    {
        isCmdDown = false;
        std::string path = GetSavePath();
        
        if (path.length() > 7)
        {
            // remove "file://"
            path = path.substr( 7 );
        }
        
        if (path != "")
        {
            svSaveScene( sceneView, (char*)path.c_str() );
        }
    }
    else if ([theEvent keyCode] == 0x01) // S
    {
        moveDir.z = velocity;
    }
    else if ([theEvent keyCode] == 31 && isCmdDown) // Cmd+O
    {
        isCmdDown = false;
        
        std::string path = GetOpenPath( "scene" );
        if (path != "")
        {
            auto contents = ae3d::FileSystem::FileContents( path.c_str() );
            svLoadScene( sceneView, contents );
        }
    }
    else if ([theEvent keyCode] == 0x0C) // Q
    {
        moveDir.y = -velocity;
    }
    else if ([theEvent keyCode] == 0x0E) // E
    {
        moveDir.y = velocity;
    }
    else if ([theEvent keyCode] == 51 && isCmdDown) // Cmd+backspace
    {
        svDeleteGameObject( sceneView );
        selectedGO = nullptr;
        isCmdDown = false;
    }
}

- (void)keyUp:(NSEvent *)theEvent
{
    isCmdDown = false;
    
    inputEvent.key = [theEvent keyCode];
    inputEvent.isDown = false;
    inputEvent.isActive = true;
    printf( "key: %d\n", inputEvent.key );
    
    if (inspector.IsTextEditActive())
    {
        return;
    }
    
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
    else if ([theEvent keyCode] == 123) // left
    {
        svMoveSelection( sceneView, { -1, 0, 0 } );
    }
    else if ([theEvent keyCode] == 124) // right
    {
        svMoveSelection( sceneView, { 1, 0, 0 } );
    }
    else if ([theEvent keyCode] == 126) // up
    {
        svMoveSelection( sceneView, { 0, 1, 0 } );
    }
    else if ([theEvent keyCode] == 125) // down
    {
        svMoveSelection( sceneView, { 0, -1, 0 } );
    }
}

- (void)mouseDown:(NSEvent *)theEvent
{
    inputEvent.buttonState = 1;
    inputEvent.x = (int)theEvent.locationInWindow.x;
    inputEvent.y = self.view.bounds.size.height - (int)theEvent.locationInWindow.y;
    inputEvent.isActive = true;
    inputEvent.key = -1;
}

- (void)mouseUp:(NSEvent *)theEvent
{
    inputEvent.buttonState = 0;
    inputEvent.x = (int)theEvent.locationInWindow.x;
    inputEvent.y = self.view.bounds.size.height - (int)theEvent.locationInWindow.y;
    inputEvent.isActive = true;
    inputEvent.key = -1;
    
    const bool clickedOnInspector = inputEvent.x < InspectorWidth;

    if (!clickedOnInspector)
    {
        selectedGO = svSelectObject( sceneView, inputEvent.x, inputEvent.y, (int)self.view.bounds.size.width, (int)self.view.bounds.size.height );
        
        if (selectedGO)
        {
            inspector.SetTextEditText( selectedGO->GetName() );
        }
    }
}

- (void)mouseDragged:(NSEvent *)theEvent
{
    const float deltaX = -float( theEvent.deltaX ) / 10;
    const float deltaY = -float( theEvent.deltaY ) / 10;

    const bool clickedOnInspector = (int)theEvent.locationInWindow.x < (InspectorWidth / 2);

    if (clickedOnInspector)
    {
        return;
    }
    
    if (!svIsTransformGizmoSelected( sceneView ))
    {
        svRotateCamera( sceneView, deltaX, deltaY );
    }

    svHandleMouseMotion( sceneView, deltaX, deltaY );
}

- (void)mouseMoved:(NSEvent *)theEvent
{
    inputEvent.mouseMoved = true;
    inputEvent.x = (int)theEvent.locationInWindow.x;
    inputEvent.y = self.view.bounds.size.height - (int)theEvent.locationInWindow.y;
    inputEvent.isActive = true;
    inputEvent.key = -1;
}

- (void)scrollWheel:(NSEvent *)event
{
    ae3d::System::Print("y: %f\n", event.deltaY);
}

- (void)_render
{
    inspector.BeginInput();
    
    if (inputEvent.mouseMoved)
    {
        inputEvent.mouseMoved = false;
        inspector.HandleMouseMotion( inputEvent.x * 2, inputEvent.y * 2 );
    }
    
    if (inputEvent.buttonState == 1 && inputEvent.isActive)
    {
        const bool clickedOnInspector = inputEvent.x < InspectorWidth;

        if (clickedOnInspector)
        {
            inspector.HandleLeftMouseClick( inputEvent.x * 2, inputEvent.y * 2, 0 );
            //inspector.HandleMouseMotion( inputEvent.x * 2, inputEvent.y * 2 );
        }
        else
        {
            svHandleLeftMouseDown( sceneView, inputEvent.x, inputEvent.y, (int)self.view.bounds.size.width, (int)self.view.bounds.size.height );
        }
    }

    if (inputEvent.buttonState == 0 && inputEvent.isActive)
    {
        const bool clickedOnInspector = inputEvent.x < InspectorWidth;

        if (clickedOnInspector)
        {
            inspector.HandleLeftMouseClick( inputEvent.x * 2, inputEvent.y * 2, 1 );
        }
    }
    //printf("inputEvent.key: %d\n", inputEvent.key);
    if (!inputEvent.isDown && inputEvent.isActive)
    {
        if (inputEvent.key == 51 /* backspace*/) inspector.HandleKey( 6, inputEvent.isDown ? 0 : 1 );
        if (inputEvent.key == 36 /* Enter */) inspector.HandleKey( 4, inputEvent.isDown ? 0 : 1 );
        if (inputEvent.key == 47) inspector.HandleChar( '.' ); // dot
        if (inputEvent.key == 29) inspector.HandleChar( 48 ); // 0
        if (inputEvent.key == 18) inspector.HandleChar( 49 ); // 1
        if (inputEvent.key == 19) inspector.HandleChar( 50 ); // 2
        if (inputEvent.key == 20) inspector.HandleChar( 51 ); // 3
        if (inputEvent.key == 21) inspector.HandleChar( 52 ); // 4
        if (inputEvent.key == 23) inspector.HandleChar( 53 ); // 5
        if (inputEvent.key == 22) inspector.HandleChar( 54 ); // 6
        if (inputEvent.key == 26) inspector.HandleChar( 55 ); // 7
        if (inputEvent.key == 28) inspector.HandleChar( 56 ); // 8
        if (inputEvent.key == 25) inspector.HandleChar( 57 ); // 9
        
        if (inspector.IsTextEditActive())
        {
            if (inputEvent.key == 12) inspector.HandleChar( 'q' );
            if (inputEvent.key == 13) inspector.HandleChar( 'w' );
            if (inputEvent.key == 14) inspector.HandleChar( 'e' );
            if (inputEvent.key == 15) inspector.HandleChar( 'r' );
            if (inputEvent.key == 17) inspector.HandleChar( 't' );
            if (inputEvent.key == 16) inspector.HandleChar( 'y' );
            if (inputEvent.key == 32) inspector.HandleChar( 'u' );
            if (inputEvent.key == 34) inspector.HandleChar( 'i' );
            if (inputEvent.key == 31) inspector.HandleChar( 'o' );
            if (inputEvent.key == 35) inspector.HandleChar( 'p' );
            if (inputEvent.key == 0) inspector.HandleChar( 'a' );
            if (inputEvent.key == 1) inspector.HandleChar( 's' );
            if (inputEvent.key == 2) inspector.HandleChar( 'd' );
            if (inputEvent.key == 3) inspector.HandleChar( 'f' );
            if (inputEvent.key == 5) inspector.HandleChar( 'g' );
            if (inputEvent.key == 4) inspector.HandleChar( 'h' );
            if (inputEvent.key == 38) inspector.HandleChar( 'j' );
            if (inputEvent.key == 40) inspector.HandleChar( 'k' );
            if (inputEvent.key == 37) inspector.HandleChar( 'l' );
            if (inputEvent.key == 6) inspector.HandleChar( 'z' );
            if (inputEvent.key == 7) inspector.HandleChar( 'x' );
            if (inputEvent.key == 8) inspector.HandleChar( 'c' );
            if (inputEvent.key == 9) inspector.HandleChar( 'v' );
            if (inputEvent.key == 11) inspector.HandleChar( 'b' );
            if (inputEvent.key == 45) inspector.HandleChar( 'n' );
            if (inputEvent.key == 46) inspector.HandleChar( 'm' );
            if (inputEvent.key == 49) inspector.HandleChar( ' ' );
        }
    }

    if (inputEvent.isActive)
    {
        svHighlightGizmo( sceneView, inputEvent.x, inputEvent.y, (int)self.view.bounds.size.width, (int)self.view.bounds.size.height );
    }
    
    inputEvent.isActive = false;
    inputEvent.x = 0;
    inputEvent.y = 0;
    inputEvent.buttonState = -1;
    inputEvent.key = 0;
    inspector.EndInput();
    
    svUpdate( sceneView );
    
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
        int inspectorGOIndex = 0;
        inspector.Render( width, height, selectedGO, inspectorCommand, gameObjects, goCount, svGetMaterial( sceneView ), inspectorGOIndex );
        svEndRender( sceneView );
        ae3d::System::EndFrame();
        
        if (inspectorGOIndex >= 0)
        {
            selectedGO = svSelectGameObjectIndex( sceneView, inspectorGOIndex );
        }

        switch (inspectorCommand)
        {
            case Inspector::Command::CreateGO:
                svAddGameObject( sceneView );
                break;
            case Inspector::Command::OpenScene:
            {
                std::string path = GetOpenPath( "scene" );
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
