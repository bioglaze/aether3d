#import "AppDelegate.h"

@interface AppDelegate ()

@end

extern NSViewController* myViewController;

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    [_window setTitle: @"Aether3D Metal Sample"];
    [_window center];
    [_window makeFirstResponder:myViewController];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
    // Insert code here to tear down your application
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    return YES;
}

@end
