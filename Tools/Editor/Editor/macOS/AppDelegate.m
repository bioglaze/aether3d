#import "AppDelegate.h"

@interface AppDelegate ()

@end

extern NSViewController* myViewController;

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    [[ [myViewController view] window ] makeFirstResponder:myViewController];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    return YES;
}

@end
