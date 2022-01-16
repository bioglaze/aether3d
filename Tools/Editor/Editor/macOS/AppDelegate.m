#import "AppDelegate.h"

@interface AppDelegate ()

@end

extern NSViewController* myViewController;
extern float backingScale;

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    [[ [myViewController view] window ] makeFirstResponder:myViewController];
    [[ [myViewController view] window ] setAcceptsMouseMovedEvents:YES];
    backingScale = [[ [myViewController view] window ] backingScaleFactor];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    return YES;
}

@end
