#include "Window.hpp"
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CVDisplayLink.h>
#import <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>

// Based on https://github.com/nxsy/hajonta/blob/master/source/hajonta/platform/osx.mm
// TODO [2015-03-14]: ARC

namespace WindowGlobal
{
    bool isOpen = false;
    const int eventStackSize = 10;
    ae3d::WindowEvent eventStack[eventStackSize];
    int eventIndex = -1;
}

@class View;
static CVReturn GlobalDisplayLinkCallback(CVDisplayLinkRef, const CVTimeStamp*, const CVTimeStamp*, CVOptionFlags, CVOptionFlags*, void*);

@interface View : NSOpenGLView <NSWindowDelegate>
{
@public
    CVDisplayLinkRef displayLink;
    NSRect windowRect;
    NSRecursiveLock* appLock;
    
    bool running;
}
@end

@implementation View
// Initialize
- (id) initWithFrame: (NSRect) frame
{
    running = true;
    
    // No multisampling
    uint32_t samples = 0;
    
    // Keep multisampling attributes at the start of the attribute lists since code below assumes they are array elements 0 through 4.
    NSOpenGLPixelFormatAttribute windowedAttrs[] =
    {
        NSOpenGLPFAMultisample,
        NSOpenGLPFASampleBuffers, (uint32_t)(samples ? 1 : 0),
        NSOpenGLPFASamples, samples,
        NSOpenGLPFAAccelerated,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAColorSize, 32,
        NSOpenGLPFADepthSize, 24,
        NSOpenGLPFAAlphaSize, 8,
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
        0
    };
    
    // Try to choose a supported pixel format
    NSOpenGLPixelFormat* pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:windowedAttrs];
    
    if (!pf)
    {
        bool valid = false;
        
        while (!pf && samples > 0)
        {
            samples /= 2;
            windowedAttrs[2] = samples ? 1 : 0;
            windowedAttrs[4] = samples;
            pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:windowedAttrs];
            
            if (pf)
            {
                valid = true;
                break;
            }
        }
        
        if (!valid)
        {
            NSLog(@"OpenGL pixel format not supported.");
            return nil;
        }
    }
    
    self = [super initWithFrame:frame pixelFormat:[pf autorelease]];
    appLock = [[NSRecursiveLock alloc] init];
    
    return self;
}

- (void) prepareOpenGL
{
    [super prepareOpenGL];
    
    [[self window] setLevel: NSNormalWindowLevel];
    [[self window] makeKeyAndOrderFront: self];
    
    // Make all the OpenGL calls to setup rendering and build the necessary rendering objects
    [[self openGLContext] makeCurrentContext];
    // Synchronize buffer swaps with vertical refresh rate
    GLint swapInt = 1; // Vsynch on!
    [[self openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];
    
    // Create a display link capable of being used with all active displays
    CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
    
    // Set the renderer output callback function
    CVDisplayLinkSetOutputCallback(displayLink, &GlobalDisplayLinkCallback, self);
    
    CGLContextObj cglContext = (CGLContextObj)[[self openGLContext] CGLContextObj];
    CGLPixelFormatObj cglPixelFormat = (CGLPixelFormatObj)[[self pixelFormat] CGLPixelFormatObj];
    CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, cglContext, cglPixelFormat);
    
    GLint dim[2] = {(int32_t)windowRect.size.width, (int32_t)windowRect.size.height};
    CGLSetParameter(cglContext, kCGLCPSurfaceBackingSize, dim);
    CGLEnable(cglContext, kCGLCESurfaceBackingSize);
    
    [appLock lock];
    CGLLockContext((CGLContextObj)[[self openGLContext] CGLContextObj]);
    
    NSLog(@"GL version:   %s", glGetString(GL_VERSION));
    NSLog(@"GLSL version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
    
    glViewport(0, 0, windowRect.size.width, windowRect.size.height);
    
    CGLUnlockContext((CGLContextObj)[[self openGLContext] CGLContextObj]);
    [appLock unlock];
    
    // Activate the display link
    CVDisplayLinkStart(displayLink);
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)mouseMoved:(NSEvent*) event
{
    [appLock lock];
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    NSLog(@"Mouse pos: %lf, %lf", point.x, point.y);
    [appLock unlock];
}

- (void) mouseDragged: (NSEvent*) event
{
    [appLock lock];
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    NSLog(@"Mouse pos: %lf, %lf", point.x, point.y);
    [appLock unlock];
}

- (void)scrollWheel: (NSEvent*) event
{
    [appLock lock];
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    NSLog(@"Mouse wheel at: %lf, %lf. Delta: %lf", point.x, point.y, [event deltaY]);
    [appLock unlock];
}

- (void) mouseDown: (NSEvent*) event
{
    [appLock lock];
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    NSLog(@"Left mouse down: %lf, %lf", point.x, point.y);
    ++WindowGlobal::eventIndex;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::Mouse1Down;

    [appLock unlock];
}

- (void) mouseUp: (NSEvent*) event
{
    [appLock lock];
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    NSLog(@"Left mouse up: %lf, %lf", point.x, point.y);
    ++WindowGlobal::eventIndex;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::Mouse1Up;

    [appLock unlock];
}

- (void) rightMouseDown: (NSEvent*) event
{
    [appLock lock];
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    NSLog(@"Right mouse down: %lf, %lf", point.x, point.y);
    [appLock unlock];
}

- (void) rightMouseUp: (NSEvent*) event
{
    [appLock lock];
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    NSLog(@"Right mouse up: %lf, %lf", point.x, point.y);
    [appLock unlock];
}

- (void)otherMouseDown: (NSEvent*) event
{
    [appLock lock];
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    NSLog(@"Middle mouse down: %lf, %lf", point.x, point.y);
    [appLock unlock];
}

- (void)otherMouseUp: (NSEvent*) event
{
    [appLock lock];
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    NSLog(@"Middle mouse up: %lf, %lf", point.x, point.y);
    [appLock unlock];
}

- (void) mouseEntered: (NSEvent*)event
{
    [appLock lock];
    NSLog(@"Mouse entered");
    [appLock unlock];
}

- (void) mouseExited: (NSEvent*)event
{
    [appLock lock];
    NSLog(@"Mouse left");
    [appLock unlock];
}

- (void) keyDown: (NSEvent*) event
{
    [appLock lock];
    if ([event isARepeat] == NO)
    {
        NSLog(@"Key down: %d", [event keyCode]);
        ++WindowGlobal::eventIndex;
        WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::KeyDown;
        WindowGlobal::eventStack[ WindowGlobal::eventIndex ].keyCode = [event keyCode];
    }
    
    [appLock unlock];
}

- (void) keyUp: (NSEvent*) event
{
    [appLock lock];
    NSLog(@"Key up: %d", [event keyCode]);
    ++WindowGlobal::eventIndex;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::KeyUp;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].keyCode = [event keyCode];
    [appLock unlock];
}

// Update
- (CVReturn) getFrameForTime:(const CVTimeStamp*)outputTime
{
    [appLock lock];
    
    [[self openGLContext] makeCurrentContext];
    CGLLockContext((CGLContextObj)[[self openGLContext] CGLContextObj]);
    
    // FIXME [2015-03-19]: Find out why clearing in Camera code doesn't work.
    //glClearColor(1, 0, 0, 1);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    CGLFlushDrawable((CGLContextObj)[[self openGLContext] CGLContextObj]);
    CGLUnlockContext((CGLContextObj)[[self openGLContext] CGLContextObj]);
    
    /*if (state.stopping) {
     [NSApp terminate:self];
     }*/
    
    [appLock unlock];
    
    return kCVReturnSuccess;
}

// Resize
- (void)windowDidResize:(NSNotification*)notification
{
    NSSize size = [ [ _window contentView ] frame ].size;
    [appLock lock];
    [[self openGLContext] makeCurrentContext];
    CGLLockContext((CGLContextObj)[[self openGLContext] CGLContextObj]);
    NSLog(@"Window resize: %lf, %lf", size.width, size.height);
    // Temp
    windowRect.size.width = size.width;
    windowRect.size.height = size.height;
    glViewport(0, 0, windowRect.size.width, windowRect.size.height);
    // End temp
    CGLUnlockContext((CGLContextObj)[[self openGLContext] CGLContextObj]);
    [appLock unlock];
}

- (void)resumeDisplayRenderer
{
    [appLock lock];
    CVDisplayLinkStop(displayLink);
    [appLock unlock];
}

- (void)haltDisplayRenderer
{
    [appLock lock];
    CVDisplayLinkStop(displayLink);
    [appLock unlock];
}

// Terminate window when the red X is pressed
-(void)windowWillClose:(NSNotification *)notification
{
    if (running)
    {
        running = false;
        
        [appLock lock];
        
        CVDisplayLinkStop(displayLink);
        CVDisplayLinkRelease(displayLink);
        
        [appLock unlock];
    }
    
    [NSApp terminate:self];
}

// Cleanup
- (void) dealloc
{
    [appLock release];
    [super dealloc];
}
@end

static CVReturn GlobalDisplayLinkCallback(CVDisplayLinkRef /*displayLink*/, const CVTimeStamp* /*now*/, const CVTimeStamp* outputTime,
                                          CVOptionFlags /*flagsIn*/, CVOptionFlags* /*flagsOut*/, void* displayLinkContext)
{
    return [(View*)displayLinkContext getFrameForTime:outputTime];
}

bool ae3d::Window::IsOpen()
{
    return WindowGlobal::isOpen;
}

void ae3d::Window::Create( int width, int height, WindowCreateFlags flags )
{
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    
    // Create a shared app instance.
    // This will initialize the global variable
    // 'NSApp' with the application instance.
    [NSApplication sharedApplication];
    
    // Style flags
    NSUInteger windowStyle = NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask;
    
    // Window bounds (x, y, width, height)
    NSRect screenRect = [[NSScreen mainScreen] frame];
    NSRect viewRect = NSMakeRect(0, 0, width, height);
    NSRect windowRect = NSMakeRect(NSMidX(screenRect) - NSMidX(viewRect),
                                   NSMidY(screenRect) - NSMidY(viewRect),
                                   viewRect.size.width,
                                   viewRect.size.height);
    
    NSWindow* window = [[NSWindow alloc] initWithContentRect:windowRect
                                                    styleMask:windowStyle
                                                      backing:NSBackingStoreBuffered
                                                        defer:NO];
    [window autorelease];
    
    // Window controller
    NSWindowController * windowController = [[NSWindowController alloc] initWithWindow:window];
    [windowController autorelease];
    
    // Since Snow Leopard, programs without application bundles and Info.plist files don't get a menubar
    // and can't be brought to the front unless the presentation option is changed
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    
    id menubar = [[NSMenu new] autorelease];
    id appMenuItem = [[NSMenuItem new] autorelease];
    [menubar addItem:appMenuItem];
    [NSApp setMainMenu:menubar];
    
    // Then we add the quit item to the menu. Fortunately the action is simple since terminate: is
    // already implemented in NSApplication and the NSApplication is always in the responder chain.
    id appMenu = [[NSMenu new] autorelease];
    id appName = [[NSProcessInfo processInfo] processName];
    id quitTitle = [@"Quit " stringByAppendingString:appName];
    id quitMenuItem = [[[NSMenuItem alloc] initWithTitle:quitTitle
                                                  action:@selector(terminate:) keyEquivalent:@"q"] autorelease];
    [appMenu addItem:quitMenuItem];
    [appMenuItem setSubmenu:appMenu];
    
    // Create app delegate to handle system events
    View* view = [[[View alloc] initWithFrame:windowRect] autorelease];
    view->windowRect = windowRect;
    // [window setAcceptsMouseMovedEvents:YES];
    [window setContentView:view];
    [window setDelegate:view];
    
    [window setTitle:appName];
    [window setCollectionBehavior: NSWindowCollectionBehaviorFullScreenPrimary];
    [window orderFrontRegardless];
    
    [NSApp run];
    [pool drain];
    
    WindowGlobal::isOpen = true;
}

void ae3d::Window::PumpEvents()
{
}

bool ae3d::Window::PollEvent( WindowEvent& outEvent )
{
    if (WindowGlobal::eventIndex == -1)
    {
        return false;
    }
    
    outEvent = WindowGlobal::eventStack[ WindowGlobal::eventIndex ];
    --WindowGlobal::eventIndex;
    return true;
}

void ae3d::Window::SwapBuffers() const
{

}

