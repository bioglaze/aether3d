#import "AppDelegate.h"

@interface AppDelegate ()

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    [_window setTitle: @"Aether3D Metal Sample"];
    [_window center];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    // Insert code here to tear down your application
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (void)keyDown:(NSEvent *)theEvent
{
    NSLog( @"Key down\n" );
}

- (void)keyUp:(NSEvent *)theEvent
{
    NSLog( @"Key up\n" );
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

@end
