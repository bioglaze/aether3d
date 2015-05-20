#include "Window.hpp"
#include <map>
#import <Cocoa/Cocoa.h>
#include <GL/glxw.h>

// Based on https://github.com/vbo/handmadehero_osx_platform_layer/blob/day_29/code/osx_handmade.m

void nsLog(const char* msg)
{
    NSLog(@"%s", msg);
}

void PlatformInitGamePad()
{

}

namespace WindowGlobal
{
    bool isOpen = false;
    const int eventStackSize = 10;
    ae3d::WindowEvent eventStack[eventStackSize];
    int eventIndex = -1;
    NSOpenGLContext* glContext = nullptr;
    NSApplication *application = nullptr;

    std::map< unsigned, ae3d::KeyCode > keyMap = {
        { 0, ae3d::KeyCode::A },
        { 11, ae3d::KeyCode::B },
        { 8, ae3d::KeyCode::C },
        { 2, ae3d::KeyCode::D },
        { 14, ae3d::KeyCode::E },
        { 3, ae3d::KeyCode::F },
        { 5, ae3d::KeyCode::G },
        { 4, ae3d::KeyCode::H },
        { 34, ae3d::KeyCode::I },
        { 38, ae3d::KeyCode::J },
        { 40, ae3d::KeyCode::K },
        { 37, ae3d::KeyCode::L },
        { 46, ae3d::KeyCode::M },
        { 45, ae3d::KeyCode::N },
        { 31, ae3d::KeyCode::O },
        { 35, ae3d::KeyCode::P },
        { 12, ae3d::KeyCode::Q },
        { 15, ae3d::KeyCode::R },
        { 1, ae3d::KeyCode::S },
        { 17, ae3d::KeyCode::T },
        { 32, ae3d::KeyCode::U },
        { 9, ae3d::KeyCode::V },
        { 13, ae3d::KeyCode::W },
        { 7, ae3d::KeyCode::X },
        { 16, ae3d::KeyCode::Y },
        { 6, ae3d::KeyCode::Z },

        { 126, ae3d::KeyCode::Up },
        { 125, ae3d::KeyCode::Down },
        { 123, ae3d::KeyCode::Left },
        { 124, ae3d::KeyCode::Right },

        { 53, ae3d::KeyCode::Escape },
        { 36, ae3d::KeyCode::Enter },
        { 49, ae3d::KeyCode::Space }
    };
}

bool ae3d::Window::IsOpen()
{
    return WindowGlobal::isOpen;
}

@interface ApplicationDelegate
: NSObject <NSApplicationDelegate, NSWindowDelegate> @end

@implementation ApplicationDelegate : NSObject
- (void)applicationDidFinishLaunching: (NSNotification *)notification
{
    (void)notification;
    
    // Stop the event loop after app initialization because we have our own loop.
    [NSApp stop: nil];
    // Post empty event to get the app visible.
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    NSEvent* event =
    [NSEvent otherEventWithType: NSApplicationDefined
                       location: NSMakePoint(0, 0)
                  modifierFlags: 0
                      timestamp: 0
                   windowNumber: 0
                        context: nil
                        subtype: 0
                          data1: 0
                          data2: 0];
    [NSApp postEvent: event atStart: YES];
    [pool drain];
}

- (NSApplicationTerminateReply)
applicationShouldTerminate: (NSApplication *)sender
{
    (void)sender;
    return NSApplicationTerminateReply::NSTerminateNow;
}

- (void)dealloc
{
    [super dealloc];
}
@end

@interface WindowDelegate:
NSObject <NSWindowDelegate> @end

@implementation WindowDelegate : NSObject
- (BOOL)windowShouldClose: (id)sender
{
    (void)sender;
    //globalApplicationState.isRunning = false;
    return NO;
}

- (void)windowDidBecomeKey: (NSNotification *)notification
{
    (void)notification;
}

- (void)windowDidResignKey: (NSNotification *)notification
{
    (void)notification;
    //NSWindow *window = [notification object];
}
@end

NSWindow *window;

@interface OpenGLView: NSOpenGLView @end
@implementation OpenGLView : NSOpenGLView
- (void)reshape
{
    //glViewport(0, 0, 640, 480);
}

- (void) mouseDown: (NSEvent*) event
{
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    ++WindowGlobal::eventIndex;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::Mouse1Down;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = point.x;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = point.y;
}

- (void) rightMouseDown: (NSEvent*) event
{
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    ++WindowGlobal::eventIndex;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::Mouse2Down;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = point.x;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = point.y;
}

- (void) mouseUp: (NSEvent*) event
{
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    ++WindowGlobal::eventIndex;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::Mouse1Up;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = point.x;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = point.y;
}

- (void) rightMouseUp: (NSEvent*) event
{
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    ++WindowGlobal::eventIndex;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::Mouse2Up;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = point.x;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = point.y;
}

- (void) otherMouseDown: (NSEvent*) event
{
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    ++WindowGlobal::eventIndex;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::MouseMiddleDown;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = point.x;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = point.y;
}

- (void) otherMouseUp: (NSEvent*) event
{
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    ++WindowGlobal::eventIndex;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::MouseMiddleUp;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = point.x;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = point.y;
}

- (void) scrollWheel: (NSEvent*) event
{
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    float deltaY = [event deltaY];
    ++WindowGlobal::eventIndex;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = deltaY < 0 ? ae3d::WindowEventType::MouseMiddleDown : ae3d::WindowEventType::MouseMiddleUp;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = point.x;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = point.y;
}

@end

static void CreateWindow( int width, int height, ae3d::WindowCreateFlags flags )
{
    int windowStyleMask;
    NSRect windowRect;
    windowRect = NSMakeRect(0, 0, width, height);

    if (flags & ae3d::WindowCreateFlags::Fullscreen)
    {
        windowStyleMask = NSBorderlessWindowMask;
    }
    else
    {
        windowStyleMask = NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask;
    }
    
    window =
    [[NSWindow alloc] initWithContentRect: windowRect
                                styleMask: windowStyleMask
                                  backing: NSBackingStoreBuffered
                                    defer: YES];
    [window setOpaque: YES];
    WindowDelegate *windowDelegate = [[WindowDelegate alloc] init];
    [window setDelegate: windowDelegate];
    id appName = @"Aether3D";
    NSMenu *menubar = [NSMenu alloc];
    [window setTitle: appName];
    NSMenuItem *appMenuItem = [NSMenuItem alloc];
    [menubar addItem: appMenuItem];
    [WindowGlobal::application setMainMenu: menubar];
    NSMenu *appMenu = [NSMenu alloc];
    id quitTitle = [@"Quit " stringByAppendingString: appName];
    // Make menu respond to cmd+q
    id quitMenuItem =
    [[NSMenuItem alloc] initWithTitle: quitTitle
                               action: @selector(terminate:)
                        keyEquivalent: @"q"];
    [appMenu addItem: quitMenuItem];
    [appMenuItem setSubmenu: appMenu];
    // When running from console we need to manually steal focus
    // from the terminal window for some reason.
    [WindowGlobal::application activateIgnoringOtherApps:YES];
    
    // A cryptic way to ask window to open.
    [window makeKeyAndOrderFront: WindowGlobal::application];
    [window center];
    WindowGlobal::isOpen = true;
    
    if (flags & ae3d::WindowCreateFlags::Fullscreen)
    {
        [window setLevel: NSMainMenuWindowLevel + 1];
        [window setHidesOnDeactivate: YES];
        NSRect screenRect = [[NSScreen mainScreen] frame];
        [window setContentSize: screenRect.size];
        [window setFrameOrigin: screenRect.origin];
    }
}

static void CreateGLContext()
{
    NSOpenGLPixelFormatAttribute attributes[] =
    {
        NSOpenGLPFAClosestPolicy,
        NSOpenGLPFASampleBuffers, 0,
        NSOpenGLPFAAccelerated,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAClosestPolicy,
        NSOpenGLPFAColorSize, 32,
        NSOpenGLPFADepthSize, 24,
        NSOpenGLPFAAlphaSize, 8,
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
        0
    };
    
    NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
    
    WindowGlobal::glContext =
    [[NSOpenGLContext alloc] initWithFormat: pixelFormat
                               shareContext: 0];
    if (WindowGlobal::glContext == nullptr)
    {
        NSLog(@"Could not create OpenGL context!");
        return;
    }
    
    [WindowGlobal::glContext makeCurrentContext];
    
    GLint vsync = 1;
    [WindowGlobal::glContext setValues: &vsync
     forParameter: NSOpenGLCPSwapInterval];
    OpenGLView *view = [[OpenGLView alloc] init];
    [window setContentView: view];
    [view setOpenGLContext: WindowGlobal::glContext];
    [view setPixelFormat: pixelFormat];
    
    if (glxwInit() != 0)
    {
        NSLog(@"Failed to load OpenGL function pointers using GLXW!");
    }
    
    NSLog(@"GL version:   %s", glGetString(GL_VERSION));
    glEnable( GL_DEPTH_TEST );
    
    [WindowGlobal::glContext setView: view];
}

void ae3d::Window::Create( int width, int height, WindowCreateFlags flags )
{
    if (width < 0 || height < 0)
    {
        nsLog("Window::Create: Negative width or height! Setting 640x480.");
        width = 640;
        height = 480;
    }
    
    WindowGlobal::application = [NSApplication sharedApplication];
    // In Snow Leopard, programs without application bundles and
    // Info.plist files don't get a menubar and can't be brought
    // to the front unless the presentation option is changed.
    [WindowGlobal::application setActivationPolicy: NSApplicationActivationPolicyRegular];
    // Specify application delegate impl.
    ApplicationDelegate *delegate = [[ApplicationDelegate alloc] init];
    [WindowGlobal::application setDelegate: delegate];
    // Normally this function would block, so if we want
    // to make our own main loop we need to stop it just
    // after initialization (see ApplicationDelegate implementation).
    [WindowGlobal::application run];
    
    CreateWindow( width, height, flags );
    CreateGLContext();
}

void cocoaProcessEvents()
{
    NSAutoreleasePool *eventsAutoreleasePool = [[NSAutoreleasePool alloc] init];
    
    while (true)
    {
        NSEvent* event =
        [WindowGlobal::application nextEventMatchingMask: NSAnyEventMask
                                 untilDate: [NSDate distantPast]
                                    inMode: NSDefaultRunLoopMode
                                   dequeue: YES];
        if (event == nullptr)
        {
            break;
        }
        
        switch ([event type])
        {
            case NSKeyUp:
            case NSKeyDown:
            {
                const int isDown = ([event type] == NSKeyDown);
                const auto type = isDown ? ae3d::WindowEventType::KeyDown : ae3d::WindowEventType::KeyUp;
                
                ++WindowGlobal::eventIndex;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = type;
                
                int hotkeyMask = NSCommandKeyMask
                | NSAlternateKeyMask
                | NSControlKeyMask
                | NSAlphaShiftKeyMask;
                if ([event modifierFlags] & hotkeyMask)
                {
                    // Handle events like cmd+q etc
                    [WindowGlobal::application sendEvent:event];
                    break;
                }

                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].keyCode = WindowGlobal::keyMap[ [event keyCode] ];
                //NSLog(@"key: %d", [event keyCode]);
            } break;
            default:
            {
                // Handle events like app focus/unfocus etc
                [WindowGlobal::application sendEvent:event];
            } break;
        }
    }
    [eventsAutoreleasePool drain];
}

void ae3d::Window::PumpEvents()
{
    cocoaProcessEvents();
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
    [WindowGlobal::glContext flushBuffer];
}

