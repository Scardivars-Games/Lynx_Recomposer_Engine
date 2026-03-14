#if defined (__APPLE__) && ILEN_IOS
#include <window/window-ios.h>
#import <UIKit/UIKit.h>
#import <MetalKit/MetalKit.h>
#include <ilen/headers/core.h>

@interface IlenMTKView : MTKView
@property (nonatomic, assign) ilen::window::WindowNativeIOS* owner;
@end

@implementation IlenMTKView

-(void)layoutSubviews{
    [super layoutSubviews];
    if(self.owner){
        //CGSize newPixelSize = self.drawableSize;
        CGFloat scale = [[UIScreen mainScreen] nativeScale];
        
        //Esto es para ver si se puede usar la resolución Retina de la pantalla de iPhone, y no la basura de resolución que usa Points.
        if(self.contentScaleFactor != scale){
            self.contentScaleFactor = scale;
        }
        
        CGSize size = self.bounds.size;
        
        auto newWidth = (uint32_t)(size.width * scale);
        auto newHeight = (uint32_t)(size.height * scale);
        
        if(newWidth != ilen::window::IWindow::getWidth() || newHeight != ilen::window::IWindow::getHeight()){
            self.owner->actualWidth = newWidth;
            self.owner->actualHeight = newHeight;
            
            self.owner->_windowChanged.store(true);
        }
    }
}
@end

namespace ilen::window{
    std::unique_ptr<IWindow> IWindow::CreateWindow(const ilen::application::ApplicationMetadata& metadata){
        auto native = std::make_unique<WindowNativeIOS>(metadata);
        
        s_width = native->actualWidth;
        s_height = native->actualHeight;
        
        return std::make_unique<IWindow>(std::move(native));
    }


    WindowNativeIOS::WindowNativeIOS(const ilen::application::ApplicationMetadata& metadata){
        actualWidth = 0;
        actualHeight = 0;
        _windowChanged.store(false);
        
        nativeWindow = (__bridge UIWindow*)metadata.iOSUIWindow;
        
        /*
        CGRect frame = [[UIScreen mainScreen] bounds];
        IlenMTKView* ilenView = [[IlenMTKView alloc] initWithFrame:frame];
        ilenView.owner = this;*/
        
        UIScreen *mainScreen = [UIScreen mainScreen];
        CGFloat screenScale = mainScreen.nativeScale;
        CGRect frame = mainScreen.bounds;
        
        IlenMTKView* ilenView = [[IlenMTKView alloc] initWithFrame:frame];
        ilenView.contentScaleFactor = screenScale;
        ilenView.layer.contentsScale = screenScale;
        ilenView.owner = this;
        
        nativeView = ilenView;
        
        nativeView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        
        ilenView.drawableSize = CGSizeMake(frame.size.width * screenScale, frame.size.height * screenScale);
        
        rootViewController = [[UIViewController alloc] init];
        rootViewController.view = nativeView;
        
        nativeWindow.rootViewController = rootViewController;
        [nativeWindow makeKeyAndVisible];
        
        CGSize initialPixelSize = nativeView.drawableSize;
        actualWidth = (uint32_t)initialPixelSize.width;
        actualHeight = (uint32_t)initialPixelSize.height;
        _windowChanged.store(true); //Para que en el primer frame actualiz
    }
    
    void WindowNativeIOS::pollEvents() {}
    bool WindowNativeIOS::shouldClose() {return false; }
    void* WindowNativeIOS::getNativeWindow() const { return (__bridge void*)nativeWindow; }
    void* WindowNativeIOS::getNativeView() const { return (__bridge void*)nativeView; }

    bool WindowNativeIOS::checkWindowResize(uint32_t &outWidth, uint32_t &outHeight){
        if(!_windowChanged.load()){
            return false;
        }
        
        outWidth = actualWidth;
        outHeight = actualHeight;
        _windowChanged.store(false);
        
        return true;
    }
};

/*
GameWindow::GameWindow(void* metadata){
    auto* meta = (ilen::application::ApplicationMetadata*)metadata;
    
    s_pNative = new WindowNative_iOS(meta->iOSUIWindow);
}*/
#endif
