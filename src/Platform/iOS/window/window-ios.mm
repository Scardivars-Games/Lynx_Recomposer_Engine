#if defined (__APPLE__) && ILEN_IOS
#include <window/window-ios.h>
#import <UIKit/UIKit.h>
#import <MetalKit/MetalKit.h>
#include <ilen/headers/core.h>

namespace ilen::window{
    std::unique_ptr<IWindow> IWindow::CreateWindow(const ilen::application::ApplicationMetadata& metadata){
        auto native = std::make_unique<WindowNativeIOS>(metadata);
        return std::make_unique<IWindow>(std::move(native));
    }


    WindowNativeIOS::WindowNativeIOS(const ilen::application::ApplicationMetadata& metadata){
        nativeWindow = (__bridge UIWindow*)metadata.iOSUIWindow;
        
        nativeView = [[MTKView alloc] initWithFrame:nativeWindow.bounds];
        nativeView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        
        rootViewController = [[UIViewController alloc] init];
        rootViewController.view = nativeView;
        
        nativeWindow.rootViewController = rootViewController;
        [nativeWindow makeKeyAndVisible];
    }
    
    void WindowNativeIOS::pollEvents() {}
    bool WindowNativeIOS::shouldClose() {return false; }
    void* WindowNativeIOS::getNativeWindow() const { return (__bridge void*)nativeWindow; }
    void* WindowNativeIOS::getNativeView() const { return (__bridge void*)nativeView; }
};

/*
GameWindow::GameWindow(void* metadata){
    auto* meta = (ilen::application::ApplicationMetadata*)metadata;
    
    s_pNative = new WindowNative_iOS(meta->iOSUIWindow);
}*/
#endif
