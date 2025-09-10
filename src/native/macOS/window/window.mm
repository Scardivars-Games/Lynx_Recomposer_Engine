#include <ilen/window/window.h>

#if defined (__APPLE__)
#include <TargetConditionals.h>

#if TARGET_OS_OSX
#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>

using namespace ilen::window;

class WindowNativeMacOS;

int GameWindow::s_width = 0;
int GameWindow::s_height = 0;
GameWindow::WindowNative* GameWindow::s_pNative = nullptr;

@interface AppDelegate: NSObject<NSApplicationDelegate, NSWindowDelegate>
@property (nonatomic, assign) WindowNativeMacOS* nativeWindow;
@end

class GameWindow::WindowNative {
public:
    virtual ~WindowNative() = default;
    virtual void pollEvents() = 0;
    virtual bool shouldClose() const = 0;
    virtual void* getNativeWindow() const = 0;
    virtual void* getNativeView() const = 0;
};

class WindowNativeMacOS : public GameWindow::WindowNative {
public:
    NSWindow* nativeWindow = nil;
    MTKView* nativeView = nil;
    bool closeRequested = false;
    AppDelegate* appDelegate = nil;
    
    WindowNativeMacOS(int width, int height, const std::string& title, IlenWindowPropertiesFlags windowProperties){
        @autoreleasepool {
            //Iniciar la app
            [NSApplication sharedApplication];
            [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
            
            //Creación y asignación del delegate
            appDelegate = [[AppDelegate alloc] init];
            appDelegate.nativeWindow = this;
            [NSApp setDelegate:appDelegate];
            
            //Creación de la ventana
            NSRect frame = NSMakeRect(0, 0, width, height);
            NSUInteger styleMask = NSWindowStyleMaskTitled;
            if(windowProperties & ILEN_WINDOW_CLOSABLE)
                styleMask |= NSWindowStyleMaskClosable;
            if(windowProperties & ILEN_WINDOW_RESIZABLE)
                styleMask |= NSWindowStyleMaskResizable;
            if(windowProperties & ILEN_WINDOW_BORDERLESS)
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
    
    ~WindowNativeMacOS() override {
        @autoreleasepool {
            [nativeWindow close];
        }
    }
    
    void pollEvents() override {
        @autoreleasepool {
            NSEvent* event;
            while((event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES])){
                [NSApp sendEvent:event];
            }
        }
    }
    
    bool shouldClose() const override {
        return closeRequested;
    }
    
    void* getNativeWindow() const override { return nativeWindow; }
    void* getNativeView() const override { return nativeView; }
};

@implementation AppDelegate
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender{
    return YES;
}

- (void)windowWillClose:(NSNotification *)notification{
    if(self.nativeWindow){
        self.nativeWindow -> closeRequested = true;
    }
}
@end

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
