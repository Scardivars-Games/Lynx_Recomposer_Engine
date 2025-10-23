
#if defined (__APPLE__) && ILEN_MACOS

#include "window/window-macos.h"

#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>

using namespace ilen::window;
using namespace ilen::enumerations;

//Implementación del AppDelegate
@interface AppDelegate: NSObject<NSApplicationDelegate, NSWindowDelegate>
@property (nonatomic, assign) WindowNativeMacOS* nativeWindow;
@end

@implementation AppDelegate
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender{
    return YES;
}

- (void)windowWillClose:(NSNotification *)notification{
    if(self.nativeWindow){
        self.nativeWindow -> closeRequested = true;
    }
}


- (NSApplicationTerminateReply)applicationShouldTerminate: (NSApplication *)sender {
    if(self.nativeWindow){
        self.nativeWindow -> closeRequested = true;
    }
    return NSTerminateCancel; //Le decimos a Cocoa que no mate el proceso, sino que nosotros lo terminamos con gracias sin crasheos.
}
@end

namespace ilen::window {

    std::unique_ptr<IWindow> IWindow::CreateWindow(const ilen::application::ApplicationMetadata& metadata){
        s_width = metadata.startWidth;
        s_height = metadata.startHeight;
        
        auto native = std::make_unique<WindowNativeMacOS>(metadata);
        
        return std::make_unique<IWindow>(std::move(native));
    }
    
    WindowNativeMacOS::WindowNativeMacOS(const ilen::application::ApplicationMetadata& metadata) {
        @autoreleasepool {
            //Iniciar la app
            
            int width = metadata.startWidth;
            int height = metadata.startHeight;
            std::string title = metadata.applicationName;
            IlenWindowPropertiesFlags windowProperties = metadata.windowPropertiesFlags;
            
            [NSApplication sharedApplication];
            [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
            
            //Creación y asignación del delegate
            appDelegate = [[AppDelegate alloc] init];
            appDelegate.nativeWindow = this;
            [NSApp setDelegate:appDelegate];
            
            //Creación de la ventana
            NSRect frame = NSMakeRect(0, 0, width, height);
            NSUInteger styleMask = NSWindowStyleMaskTitled;
            if(windowProperties & ILEN_WINDOW_PROPERTY_CLOSABLE)
                styleMask |= NSWindowStyleMaskClosable;
            if(windowProperties & ILEN_WINDOW_PROPERTY_RESIZABLE)
                styleMask |= NSWindowStyleMaskResizable;
            if(windowProperties & ILEN_WINDOW_PROPERTY_BORDERLESS)
                styleMask |= NSWindowStyleMaskBorderless;
            //TODO: Añadir el resto de macros de opcion de las ventanas
            nativeWindow = [[NSWindow alloc] initWithContentRect:frame styleMask:styleMask backing:NSBackingStoreBuffered defer:NO];
            [nativeWindow setTitle:[NSString stringWithUTF8String:title.c_str()]];
            [nativeWindow center];
            [nativeWindow setDelegate:appDelegate];

            //Menu principal de la app
            NSMenu* mainMenu = [[NSMenu alloc] initWithTitle:@"MainMenu"];

            NSMenuItem* appMenuItem = [[NSMenuItem alloc] initWithTitle:@"AppMenu" action:NULL keyEquivalent:@""];
            NSMenu* appMenu = [[NSMenu alloc] initWithTitle:@"App Menu"];

            NSString* appName = [[NSProcessInfo processInfo] processName];
            NSString* quitTitle = [NSString stringWithFormat:@"Quit %@", appName];
            NSMenuItem* quitMenuItem = [[NSMenuItem alloc] initWithTitle:quitTitle action:@selector(terminate:) keyEquivalent:@"q"];
            
            [appMenu addItem:quitMenuItem];
            [appMenuItem setSubmenu:appMenu];
            [mainMenu addItem:appMenuItem];

            [NSApp setMainMenu:mainMenu];

            //Creación de la vista principal
            nativeView = [[MTKView alloc] initWithFrame:frame];
            [nativeWindow setContentView:nativeView];
            
            //Mostrar la ventana
            [nativeWindow makeKeyAndOrderFront:nil];
            [NSApp activateIgnoringOtherApps:YES];
        }
    }


    void WindowNativeMacOS::pollEvents() {
        @autoreleasepool {
            NSEvent* event;
            while((event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES])){
                [NSApp sendEvent:event];
            }
        }
    }

    bool WindowNativeMacOS::shouldClose() {
        return closeRequested;
    }

    void* WindowNativeMacOS::getNativeWindow() const  { return nativeWindow; }
    void* WindowNativeMacOS::getNativeView() const  { return nativeView; }
}

#endif

/*
class WindowNativeMacOS : public GameWindow::WindowNative {
public:
    NSWindow* nativeWindow = nil;
    MTKView* nativeView = nil;
    bool closeRequested = false;
    AppDelegate* appDelegate = nil;
    
    WindowNativeMacOS(int width, int height, const std::string& title, IlenWindowPropertiesFlags windowProperties){
       
    }

};

GameWindow::GameWindow(void* metadata){
    auto* meta = (ilen::application::ApplicationMetadata*)metadata;    
    s_width = meta->startWidth;
    s_height = meta->startHeight;
    
    s_pNative = new WindowNativeMacOS(s_width, s_height, meta->applicationName, meta->windowPropertiesFlags);
}

void GameWindow::processEvents(){
    if(s_pNative){
        s_pNative -> pollEvents();
    }
}

bool GameWindow::shouldClose(){
    if(s_pNative){
        return s_pNative->shouldClose();
    }
    
    return true;
}

void* GameWindow::getWindow(){
    if(s_pNative){
        return s_pNative->getNativeWindow();
    }
    
    return nullptr;
}

void* GameWindow::getSurface(){
    if(s_pNative){
        return s_pNative->getNativeView();
    }
    
    return nullptr;
}
#endif //TARGET_OS_OSX

#endif //__APPLE__
*/

