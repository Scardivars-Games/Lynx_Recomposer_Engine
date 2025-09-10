#if defined (__APPLE__)
#include <TargetConditionals.h>

#if TARGET_OS_IOS
#import <UIKit/UIKit.h>
#include <ilen/window/window.h>
#import <MetalKit/MetalKit.h>
#include <ilen/headers/core.h>

namespace ilen::window{
GameWindow::WindowNative* GameWindow::s_pNative = nullptr;

class GameWindow::WindowNative {
public:
    virtual ~WindowNative() = default;
    virtual void pollEvents() = 0;
    virtual bool shouldClose() const = 0;
    virtual void* getNativeWindow() const = 0;
    virtual void* getNativeView() const = 0;
};

class WindowNative_iOS : public GameWindow::WindowNative {
public:
    UIWindow* nativeWindow = nullptr;
    MTKView* nativeView = nullptr;
    UIViewController* rootViewController = nullptr;
    
    WindowNative_iOS(void* parentWindow){
        nativeWindow = (__bridge UIWindow*)parentWindow;
        
        nativeView = [[MTKView alloc] initWithFrame:nativeWindow.bounds];
        nativeView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        
        rootViewController = [[UIViewController alloc] init];
        rootViewController.view = nativeView;
        
        nativeWindow.rootViewController = rootViewController;
        [nativeWindow makeKeyAndVisible];
    }
    
    ~WindowNative_iOS() override {}
    void pollEvents() override {}
    bool shouldClose() const override {return false; }
    void* getNativeWindow() const override { return (__bridge void*)nativeWindow; }
    void* getNativeView() const override { return (__bridge void*)nativeView; }
};

GameWindow::GameWindow(void* metadata){
    auto* meta = (ilen::application::ApplicationMetadata*)metadata;
    
    s_pNative = new WindowNative_iOS(meta->iOSUIWindow);
}

void GameWindow::processEvents() {if (s_pNative) s_pNative-> pollEvents(); }
bool GameWindow::shouldClose() {return s_pNative ? s_pNative->shouldClose() : true; }
void* GameWindow::getWindow() { return s_pNative ? s_pNative->getNativeWindow() : nullptr; }
void* GameWindow::getSurface() { return s_pNative ? s_pNative->getNativeView() : nullptr; }
}

#endif

#endif
