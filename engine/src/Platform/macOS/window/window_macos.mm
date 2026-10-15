
#if defined (__APPLE__) && LYNX_MACOS

#include <window/window_macos.h>
#include <lynx/headers/lynx_logs.h>
#include <lynx/input/input_buffer.h>
#include <input/apple_controllers.h>

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>

using namespace lynx::window;
using namespace lynx::enumerations;

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

-(void)windowDidResize:(NSNotification *)notification{
    if(self.nativeWindow && self.nativeWindow->nativeView){
        /*Logica antigua usando una MTKView
         MTKView* view = self.nativeWindow->nativeView;
         
         //Soportando pantallas con resolucion Retina en macOS
         NSSize logicalSize = view.bounds.size;
         NSSize pixelSize = [view convertSizeToBacking:logicalSize]; //Esto debe dar la resolucion real.
         
         view.drawableSize = pixelSize;
         self->_nativeWindow->_drawableSize = pixelSize;*/
        
        //Nueva logica de resizing usando una NSView con un CAMetalLayer
        NSView* view = self.nativeWindow->nativeView;
        CAMetalLayer* layer = (CAMetalLayer*)view.layer;
        
        CGFloat scale = view.window.backingScaleFactor;
        layer.contentsScale = scale;
        
        NSSize logicalSize = view.bounds.size;
        NSSize pixelSize = [view convertSizeToBacking:logicalSize];
        
        layer.drawableSize = pixelSize;
        self->_nativeWindow->_drawableSize = pixelSize;
        
        LYNX_LOG("New resolution por resize. Logical size: %.0f x %0.f. Pixel Size: %.0f x %.0f", logicalSize.width, logicalSize.height, pixelSize.width, pixelSize.height);
        self.nativeWindow->_windowSizeChanged.store(true);
    }
}

-(void)windowDidResignKey:(NSNotification *)notification{
    //Perdio el foco, actualizar
    lynx::input::InputBuffer::Get().ForceResetData();
    self.nativeWindow->isOutOfFocus = true;
}

-(void) windowDidBecomeKey:(NSNotification *)notification{
    //Acaba de ganar el foco la app.
    self.nativeWindow->isOutOfFocus = false;
}
@end

@interface IlenView : NSView
@end

@implementation IlenView

-(BOOL)acceptsFirstResponder {
    return YES;
}

- (void)keyDown:(NSEvent *)event {}
- (void)mouseDown:(NSEvent *)event {}
- (void)mouseMoved:(NSEvent *)event {}
@end

namespace lynx::window {

    std::unique_ptr<IWindow> IWindow::CreateWindow(const lynx::application::ApplicationMetadata& metadata){
        s_width = metadata.startWidth;
        s_height = metadata.startHeight;
        
        auto native = std::make_unique<WindowNativeMacOS>(metadata);
        
        return std::make_unique<IWindow>(std::move(native));
    }
    
    WindowNativeMacOS::WindowNativeMacOS(const lynx::application::ApplicationMetadata& metadata) : _windowSizeChanged(false) {
        @autoreleasepool {
            //Iniciar la app
            
            int width = metadata.startWidth;
            int height = metadata.startHeight;
            std::string title = metadata.applicationName;

            AddGameControllerNotifications();

            LynxWindowPropertiesFlags windowProperties = metadata.windowPropertiesFlags;
            
            [NSApplication sharedApplication];
            [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
            
            //Creación y asignación del delegate
            appDelegate = [[AppDelegate alloc] init];
            appDelegate.nativeWindow = this;
            [NSApp setDelegate:appDelegate];
            
            //Creación de la ventana
            NSRect frame = NSMakeRect(0, 0, width, height);
            NSUInteger styleMask = NSWindowStyleMaskTitled;
            if(windowProperties & LYNX_WINDOW_PROPERTY_CLOSABLE)
                styleMask |= NSWindowStyleMaskClosable;
            if(windowProperties & LYNX_WINDOW_PROPERTY_RESIZABLE)
                styleMask |= NSWindowStyleMaskResizable;
            if(windowProperties & LYNX_WINDOW_PROPERTY_BORDERLESS)
                styleMask |= NSWindowStyleMaskBorderless;
            //TODO: Añadir el resto de macros de opcion de las ventanas
            nativeWindow = [[NSWindow alloc] initWithContentRect:frame styleMask:styleMask backing:NSBackingStoreBuffered defer:NO];
            [nativeWindow setTitle:[NSString stringWithUTF8String:title.c_str()]];
            [nativeWindow center];
            [nativeWindow setDelegate:appDelegate];

            //Añadir opcion para fullscreen
            [nativeWindow setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
            
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
            /* Logica antigua generando un MTKView. Poco control sobre el proceso de rendering
             nativeView = [[MTKView alloc] initWithFrame:frame];
             [nativeWindow setContentView:nativeView];
             
             
             _drawableSize = nativeView.drawableSize;
             
             
             _windowSizeChanged.store(true);
             
             //Mostrar la ventana
             [nativeWindow makeKeyAndOrderFront:nil];*/
            
            //Nueva logica de generación de superficie (si esto funciona, borrar la anterior por legibilidad)
            //La idea aquí es un inyectar un CAMetalLayer a una NSView, y nosotros administrar toda la vida de los
            //gráficos en pantalla, algo que de por sí hace el motor y la MTKView esta arruinandolo.
            nativeView = [[IlenView alloc] initWithFrame:frame];
            [nativeView setWantsLayer:YES]; //Necesario para poder asignarle el layer de Metal.
            
            //Layer de Metal
            CAMetalLayer* metalLayer = [CAMetalLayer layer];
            metalLayer.device = MTLCreateSystemDefaultDevice();
            metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
            metalLayer.framebufferOnly = NO;
            metalLayer.opaque = YES;
            metalLayer.presentsWithTransaction = NO;
            
            //Para obtener la resolucion real
            CGFloat scale = [nativeWindow backingScaleFactor];
            metalLayer.contentsScale = scale;
            
            [nativeView setLayer:metalLayer];
            [nativeWindow setContentView:nativeView];
            
            //Tamaño inicial de la pantalla
            _drawableSize = [nativeView convertSizeToBacking:nativeView.bounds.size];
            metalLayer.drawableSize = _drawableSize;
            
            _windowSizeChanged.store(true);
            
            [nativeWindow setIsVisible:YES];
            [nativeWindow makeKeyAndOrderFront:nil];
            [nativeWindow makeFirstResponder:nativeView];
            
            [NSApp activateIgnoringOtherApps:YES];

            if(windowProperties & LYNX_WINDOW_PROPERTY_FULLSCREEN){
                dispatch_async(dispatch_get_main_queue(), ^{
                    [nativeWindow toggleFullScreen:nil];
                });            
            }
        }
    }


    void WindowNativeMacOS::pollEvents() {
        @autoreleasepool {
            NSEvent* event;
            
            auto& input = lynx::input::InputBuffer::Get();
            
            //Limpiamos los deltas anteriores
            //input.NewFrame();
            
            while((event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES])){
                NSEventType type = [event type];
                switch (type) {
                    case NSEventTypeKeyDown:
                        input.SetKeyState(lynx::input::TranslateNativeKey([event keyCode]), true);
                        break;
                    case NSEventTypeKeyUp:
                        input.SetKeyState(lynx::input::TranslateNativeKey([event keyCode]), false);
                        break;
                    case NSEventTypeFlagsChanged: {
                        uint16_t k = [event keyCode];
                        NSUInteger f = [event modifierFlags];
                        bool p = false;
                        if(k == 56 || k == 60) p = (f & NSEventModifierFlagShift);
                        else if (k == 59 || k == 62) p = (f & NSEventModifierFlagControl);
                        else if (k == 58 || k == 61) p = (f & NSEventModifierFlagOption);
                        else if (k == 55 || k == 54) p = (f & NSEventModifierFlagCommand);
                        input.SetKeyState(lynx::input::TranslateNativeKey(k), p);
                        break;
                    }
                    //Click izquierdo
                    case NSEventTypeLeftMouseUp:
                    case NSEventTypeLeftMouseDown: {
                        auto& ptr = input.GetPointer(0); //Mouse es siempre 0
                        ptr.type = lynx::input::PointerType::Mouse;
                        ptr.isDown = (type == NSEventTypeLeftMouseDown);
                        break;
                    }
                        
                    //Click derecho
                    case NSEventTypeRightMouseUp:
                    case NSEventTypeRightMouseDown:
                        input.GetPointerState().rightClick = (type == NSEventTypeRightMouseDown);
                        break;
                        
                    //Puntero del mouse
                    case NSEventTypeMouseMoved:
                    case NSEventTypeLeftMouseDragged:
                    case NSEventTypeRightMouseDragged: {
                        NSPoint loc = [event locationInWindow];
                        NSPoint local = [nativeView convertPoint:loc fromView: nil];
                        float scale = [nativeWindow backingScaleFactor];
                        
                        auto& ptr = input.GetPointer(0);
                        ptr.x = local.x * scale;
                        ptr.y = local.y * scale;
                        break;
                    }
                        
                    case NSEventTypeScrollWheel:
                        input.GetPointerState().scrollWheel = [event scrollingDeltaY];
                        break;
                        
                    default:
                        break;
                }
                
                [NSApp sendEvent:event];
            }
        }
    }

    void WindowNativeMacOS::updateGamepads() {
        UpdateGamePadsApple();
    }

    bool WindowNativeMacOS::checkWindowResize(uint32_t &outWidth, uint32_t &outHeight){
        if(!_windowSizeChanged.load()){
            return false;
        }
        
        outWidth = _drawableSize.width;
        outHeight = _drawableSize.height;
        //nativeView.drawableSize = nativeView.bounds.size;
        CAMetalLayer* layer = (CAMetalLayer*)nativeView.layer;
        [layer setDrawableSize:_drawableSize];
        
        //LYNX_LOG("Drawable Size: %i x %i. Screen bounds: %i x %i", nativeView.drawableSize.width, nativeView.drawableSize.height, nativeView.bounds.size.width, nativeView.bounds.size.height);
        
        _windowSizeChanged.store(false);
        return true;
    }

    bool WindowNativeMacOS::shouldClose() {
        return closeRequested;
    }

    bool WindowNativeMacOS::shouldPause() {
        return isOutOfFocus;
    }

    void* WindowNativeMacOS::getNativeWindow() const  { return (__bridge void*)nativeWindow; }
    const void* WindowNativeMacOS::getNativeView() const  { return (__bridge void*)nativeView; }
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
    auto* meta = (lynx::application::ApplicationMetadata*)metadata;    
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

