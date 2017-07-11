#include "Window.hpp"
#import <Cocoa/Cocoa.h>
#include <IOKit/hid/IOHidLib.h>
#include <Kernel/IOKit/hidsystem/IOHIDUsageTables.h>
#include "System.hpp"
#include "GfxDevice.hpp"

// Based on https://github.com/vbo/handmadehero_osx_platform_layer/blob/day_29/code/osx_handmade.m
// XBox One controller driver: https://github.com/FranticRain/Xone-OSX

void UpdateFrameTiming();

namespace GfxDeviceGlobal
{
    extern unsigned frameIndex;
}

struct GamePad
{
    bool isConnected = false;
    float lastX = 0;
    float lastY = 0;
};

struct game_input
{
    GamePad Controllers[5];
};

namespace WindowGlobal
{
    int windowWidth = 640;
    int windowHeight = 480;

    bool isOpen = false;
    const int eventStackSize = 15;
    ae3d::WindowEvent eventStack[ eventStackSize ];
    int eventIndex = -1;
    NSOpenGLContext* glContext = nullptr;
    NSApplication *application = nullptr;
    game_input gInput;
    ae3d::KeyCode keyMap[ 256 ];

    void IncEventIndex()
    {
        ++eventIndex;

        if (eventIndex >= eventStackSize)
        {
            eventIndex = eventStackSize;
            ae3d::System::Print( "Too many window/input events!\n" );
        }
    }

    void InitKeyMap()
    {
        for (int keyIndex = 0; keyIndex < 256; ++keyIndex)
        {
            keyMap[ keyIndex ] = ae3d::KeyCode::None;
        }

        keyMap[ 0 ] = ae3d::KeyCode::A;
        keyMap[ 11 ] = ae3d::KeyCode::B;
        keyMap[ 8 ] = ae3d::KeyCode::C;
        keyMap[ 2 ] = ae3d::KeyCode::D;
        keyMap[ 14 ] = ae3d::KeyCode::E;
        keyMap[ 3 ] = ae3d::KeyCode::F;
        keyMap[ 5 ] = ae3d::KeyCode::G;
        keyMap[ 4 ] = ae3d::KeyCode::H;
        keyMap[ 34 ] = ae3d::KeyCode::I;
        keyMap[ 38 ] = ae3d::KeyCode::J;
        keyMap[ 40 ] = ae3d::KeyCode::K;
        keyMap[ 37 ] = ae3d::KeyCode::L;
        keyMap[ 46 ] = ae3d::KeyCode::M;
        keyMap[ 45 ] = ae3d::KeyCode::N;
        keyMap[ 31 ] = ae3d::KeyCode::O;
        keyMap[ 35 ] = ae3d::KeyCode::P;
        keyMap[ 12 ] = ae3d::KeyCode::Q;
        keyMap[ 15 ] = ae3d::KeyCode::R;
        keyMap[ 1 ] = ae3d::KeyCode::S;
        keyMap[ 17 ] = ae3d::KeyCode::T;
        keyMap[ 32 ] = ae3d::KeyCode::U;
        keyMap[ 9 ] = ae3d::KeyCode::V;
        keyMap[ 13 ] = ae3d::KeyCode::W;
        keyMap[ 7 ] = ae3d::KeyCode::X;
        keyMap[ 16 ] = ae3d::KeyCode::Y;
        keyMap[ 6 ] = ae3d::KeyCode::Z;

        keyMap[ 126 ] = ae3d::KeyCode::Up;
        keyMap[ 125 ] = ae3d::KeyCode::Down;
        keyMap[ 123 ] = ae3d::KeyCode::Left;
        keyMap[ 124 ] = ae3d::KeyCode::Right;

        keyMap[ 53 ] = ae3d::KeyCode::Escape;
        keyMap[ 36 ] = ae3d::KeyCode::Enter;
        keyMap[ 49 ] = ae3d::KeyCode::Space;
    }
}

static void iokitControllerValueChangeCallbackImpl( void*/*context*/, IOReturn /*result*/, void*/*sender*/, IOHIDValueRef valueRef )
{
    //game_controller_input *controller = (game_controller_input *)context;
    
    if (IOHIDValueGetLength(valueRef) > 2)
    {
        return;
    }
    
    IOHIDElementRef element = IOHIDValueGetElement( valueRef );
    
    if (CFGetTypeID( element ) != IOHIDElementGetTypeID())
    {
        return;
    }

    const int usagePage = IOHIDElementGetUsagePage( element );
    const int usage = IOHIDElementGetUsage( element );
    const CFIndex value = IOHIDValueGetIntegerValue( valueRef );
    
    //ae3d::System::Print("usage: %d\n", usage);
    
    if (usagePage == kHIDPage_Button)
    {
        //const int isDown = value != 0;
        //ae3d::System::Print("button: %d\n", value);
        
        // Usage values obtained from XBox One controller.
        // TODO: table-driven logic.
        if (usage == 2)
        {
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadButtonB;
        }
        else if (usage == 1)
        {
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadButtonA;
        }
        else if (usage == 3)
        {
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadButtonX;
        }
        else if (usage == 4)
        {
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadButtonY;
        }
        else if (usage == 12)
        {
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadButtonDPadUp;
        }
        else if (usage == 13)
        {
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadButtonDPadDown;
        }
        else if (usage == 14)
        {
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadButtonDPadLeft;
        }
        else if (usage == 15)
        {
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadButtonDPadRight;
        }
        else if (usage == 9)
        {
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadButtonStart;
        }
        else if (usage == 10)
        {
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadButtonBack;
        }
        else if (usage == 5)
        {
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadButtonLeftShoulder;
        }
        else if (usage == 6)
        {
            WindowGlobal::IncEventIndex();
            WindowGlobal::eventStack[WindowGlobal::eventIndex].type = ae3d::WindowEventType::GamePadButtonRightShoulder;
        }
    }
    else if (usagePage == kHIDPage_GenericDesktop) // Stick
    {
        const int deadZoneMin = 120;
        const int deadZoneMax = 133;
        const int center = 32768;
        float valueNormalized;
        
        if (value > deadZoneMin && value < deadZoneMax)
        {
            valueNormalized = 0;
        }
        else
        {
            valueNormalized = value / (float)center;
        }
        
        switch (usage)
        {
            case kHIDUsage_GD_X:
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::GamePadLeftThumbState;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].gamePadThumbX = valueNormalized;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].gamePadThumbY = WindowGlobal::gInput.Controllers[0].lastY;
                WindowGlobal::gInput.Controllers[0].lastX = valueNormalized;
            } break;
            case kHIDUsage_GD_Y:
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::GamePadLeftThumbState;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].gamePadThumbX = WindowGlobal::gInput.Controllers[0].lastX;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].gamePadThumbY = valueNormalized;
                WindowGlobal::gInput.Controllers[0].lastY = valueNormalized;
            } break;
            case kHIDUsage_GD_Rx:
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::GamePadRightThumbState;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].gamePadThumbX = valueNormalized;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].gamePadThumbY = WindowGlobal::gInput.Controllers[0].lastY;
                WindowGlobal::gInput.Controllers[0].lastX = valueNormalized;
            } break;
            case kHIDUsage_GD_Ry:
            {
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::GamePadRightThumbState;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].gamePadThumbX = WindowGlobal::gInput.Controllers[0].lastX;
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].gamePadThumbY = valueNormalized;
                WindowGlobal::gInput.Controllers[0].lastY = valueNormalized;
            } break;
        }
    }

    // Crashes in cocoaProcessEvents() if uncommented.
    //CFRelease(valueRef);
}

static void iokitControllerUnplugCallbackImpl(void* context, IOReturn /*result*/, void* sender)
{
    GamePad *controller = (GamePad*)context;
    IOHIDDeviceRef device = (IOHIDDeviceRef)sender;
    controller->isConnected = false;
    IOHIDDeviceRegisterInputValueCallback(device, 0, 0);
    IOHIDDeviceRegisterRemovalCallback(device, 0, 0);
    IOHIDDeviceUnscheduleFromRunLoop(device, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOHIDDeviceClose(device, kIOHIDOptionsTypeNone);
}

static void iokitControllerPluginCallbackImpl( void* context, IOReturn /*result*/, void* /*sender*/, IOHIDDeviceRef device )
{
    // Finds the first free controller slot.
    game_input *input = (game_input *)context;
    GamePad* controller = nullptr;
    GamePad* controllers = input->Controllers;

    for (int i = 0; i < 5; ++i)
    {
        if (!controllers[ i ].isConnected)
        {
            controller = &controllers[ i ];
            break;
        }
    }

    if (controller == nullptr)
    {
        return;
    }
    
    controller->isConnected = true;
    IOHIDDeviceOpen(device, kIOHIDOptionsTypeNone);
    IOHIDDeviceScheduleWithRunLoop(device, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOHIDDeviceRegisterInputValueCallback(device, iokitControllerValueChangeCallbackImpl, (void *)controller);
    IOHIDDeviceRegisterRemovalCallback(device, iokitControllerUnplugCallbackImpl, (void *)controller);
}

static CFMutableDictionaryRef iokitCreateDeviceMatchingDict( uint32_t usagePage, uint32_t usageValue )
{
    CFMutableDictionaryRef result = CFDictionaryCreateMutable(
                                                              kCFAllocatorDefault, 0,
                                                              &kCFTypeDictionaryKeyCallBacks,
                                                              &kCFTypeDictionaryValueCallBacks);
    CFNumberRef pageNumber = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usagePage);
    CFDictionarySetValue(result, CFSTR(kIOHIDDeviceUsagePageKey), pageNumber);
    CFRelease(pageNumber);
    CFNumberRef usageNumber = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usageValue);
    CFDictionarySetValue(result, CFSTR(kIOHIDDeviceUsageKey), usageNumber);
    CFRelease(usageNumber);
    return result;
}

static void iokitInit()
{
    IOHIDManagerRef hidManager = IOHIDManagerCreate( kCFAllocatorDefault, kIOHIDOptionsTypeNone );
    
    uint32_t matches[] =
    {
        kHIDUsage_GD_Joystick,
        kHIDUsage_GD_GamePad,
        kHIDUsage_GD_MultiAxisController
    };
    CFMutableArrayRef deviceMatching = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);

    for (int i = 0; i < 3; ++i)
    {
        CFDictionaryRef matching = iokitCreateDeviceMatchingDict(kHIDPage_GenericDesktop, matches[i]);
        CFArrayAppendValue(deviceMatching, matching);
        CFRelease(matching);
    }
    
    IOHIDManagerSetDeviceMatchingMultiple(hidManager, deviceMatching);
    CFRelease(deviceMatching);
    IOHIDManagerRegisterDeviceMatchingCallback(hidManager, iokitControllerPluginCallbackImpl, &WindowGlobal::gInput);
    IOHIDManagerScheduleWithRunLoop(hidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
}

void PlatformInitGamePad()
{
    iokitInit();
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
    [NSEvent otherEventWithType: NSEventTypeApplicationDefined
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
}

- (void) mouseDown: (NSEvent*) event
{
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    WindowGlobal::IncEventIndex();
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::Mouse1Down;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = (int)point.x;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = (int)point.y;
}

- (void) rightMouseDown: (NSEvent*) event
{
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    WindowGlobal::IncEventIndex();
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::Mouse2Down;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = (int)point.x;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = (int)point.y;
}

- (void) mouseUp: (NSEvent*) event
{
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    WindowGlobal::IncEventIndex();
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::Mouse1Up;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = (int)point.x;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = (int)point.y;
}

- (void) mouseMoved: (NSEvent*) event
{
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    WindowGlobal::IncEventIndex();
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::MouseMove;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = (int)point.x;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = (int)point.y;
}

- (void) rightMouseUp: (NSEvent*) event
{
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    WindowGlobal::IncEventIndex();
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::Mouse2Up;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = (int)point.x;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = (int)point.y;
}

- (void) otherMouseDown: (NSEvent*) event
{
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    WindowGlobal::IncEventIndex();
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::MouseMiddleDown;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = (int)point.x;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = (int)point.y;
}

- (void) otherMouseUp: (NSEvent*) event
{
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    WindowGlobal::IncEventIndex();
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = ae3d::WindowEventType::MouseMiddleUp;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = (int)point.x;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = (int)point.y;
}

- (void) scrollWheel: (NSEvent*) event
{
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    float deltaY = (float)[event deltaY];
    WindowGlobal::IncEventIndex();
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = deltaY < 0 ? ae3d::WindowEventType::MouseMiddleDown : ae3d::WindowEventType::MouseMiddleUp;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseX = (int)point.x;
    WindowGlobal::eventStack[ WindowGlobal::eventIndex ].mouseY = (int)point.y;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}
@end

static void CreateWindow( int width, int height, ae3d::WindowCreateFlags flags )
{
    WindowGlobal::InitKeyMap();
    
    int windowStyleMask;
    NSRect windowRect = NSMakeRect( 0, 0, width, height );
    
    if (flags & ae3d::WindowCreateFlags::Fullscreen)
    {
        windowStyleMask = NSWindowStyleMaskBorderless;
    }
    else
    {
        windowStyleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
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
    
    [window makeKeyAndOrderFront: WindowGlobal::application];
    [window center];
    
    [window setAcceptsMouseMovedEvents:YES];
    [window makeFirstResponder:WindowGlobal::application];
    
    const NSRect screenRect = [[NSScreen mainScreen] frame];

    if (flags & ae3d::WindowCreateFlags::Fullscreen)
    {
        [window setLevel: NSMainMenuWindowLevel + 1];
        [window setHidesOnDeactivate: YES];
        [window setContentSize: screenRect.size];
        [window setFrameOrigin: screenRect.origin];
    }

    WindowGlobal::windowWidth = width == 0 ? (int)screenRect.size.width : width;
    WindowGlobal::windowHeight = height == 0 ? (int)screenRect.size.height : height;
    WindowGlobal::isOpen = true;
}

static void CreateGLContext( ae3d::WindowCreateFlags flags )
{
    unsigned sampleBuffers = 0;
    
    if (flags & ae3d::WindowCreateFlags::MSAA4)
    {
        sampleBuffers = 4;
    }
    if (flags & ae3d::WindowCreateFlags::MSAA8)
    {
        sampleBuffers = 8;
    }
    if (flags & ae3d::WindowCreateFlags::MSAA16)
    {
        sampleBuffers = 16;
    }

    NSOpenGLPixelFormatAttribute attributes[] =
    {
        NSOpenGLPFAClosestPolicy,
        NSOpenGLPFASampleBuffers, sampleBuffers > 0 ? 1U : 0U,
        NSOpenGLPFASamples, sampleBuffers,
        NSOpenGLPFAAccelerated,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAClosestPolicy,
        NSOpenGLPFAColorSize, 32,
        NSOpenGLPFADepthSize, 24,
        NSOpenGLPFAAlphaSize, 8,
        // Actually this creates 4.1 context. NSOpenGLProfileVersion4_1Core didn't work for me on a 2013 MacBook Pro (NVIDIA GT 750M, OS X Mavericks)
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
    
    [WindowGlobal::glContext setView: view];
}

void ae3d::Window::Create( int width, int height, WindowCreateFlags flags )
{
    if (width < 0 || height < 0)
    {
        NSLog( @"Window::Create: Negative width or height! Setting 640x480." );
        width = 640;
        height = 480;
    }
    
    const NSRect screenRect = [[NSScreen mainScreen] frame];

    if (width == 0)
    {
        width = (int)screenRect.size.width;
    }
    if (height == 0)
    {
        height = (int)screenRect.size.height;
    }
    
    WindowGlobal::application = [NSApplication sharedApplication];
    [WindowGlobal::application setActivationPolicy: NSApplicationActivationPolicyRegular];
    ApplicationDelegate *delegate = [[ApplicationDelegate alloc] init];
    [WindowGlobal::application setDelegate: delegate];
    [WindowGlobal::application run];
    
    CreateWindow( width, height, flags );
    CreateGLContext( flags );
    GfxDevice::Init( width, height );
    
    if ((flags & ae3d::WindowCreateFlags::MSAA4) ||
        (flags & ae3d::WindowCreateFlags::MSAA8) ||
        (flags & ae3d::WindowCreateFlags::MSAA16))
    {
        ae3d::GfxDevice::SetMultiSampling( true );
    }
}

void cocoaProcessEvents()
{
    NSAutoreleasePool *eventsAutoreleasePool = [[NSAutoreleasePool alloc] init];
    
    while (true)
    {
        NSEvent* event =
        [WindowGlobal::application nextEventMatchingMask: NSEventMaskAny
                                 untilDate: [NSDate distantPast]
                                    inMode: NSDefaultRunLoopMode
                                   dequeue: YES];
        if (event == nullptr)
        {
            break;
        }
        
        switch ([event type])
        {
            case NSEventTypeKeyUp:
            case NSEventTypeKeyDown:
            {
                const int isDown = ([event type] == NSEventTypeKeyDown);
                const auto type = isDown ? ae3d::WindowEventType::KeyDown : ae3d::WindowEventType::KeyUp;
                
                WindowGlobal::IncEventIndex();
                WindowGlobal::eventStack[ WindowGlobal::eventIndex ].type = type;
                
                int hotkeyMask = NSEventModifierFlagCommand
                | NSEventModifierFlagOption
                | NSEventModifierFlagControl
                | NSEventModifierFlagCapsLock;
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

void ae3d::Window::GetSize( int& outWidth, int& outHeight )
{
    outWidth = WindowGlobal::windowWidth;
    outHeight = WindowGlobal::windowHeight;
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

void ae3d::Window::SetTitle( const char* title )
{
    if (title != nullptr)
    {
        NSString* tit = [NSString stringWithCString:title encoding:NSUTF8StringEncoding];
        [window setTitle: tit];
    }
}

void ae3d::Window::SwapBuffers()
{
    [WindowGlobal::glContext flushBuffer];
    UpdateFrameTiming();
    ++GfxDeviceGlobal::frameIndex;
}

