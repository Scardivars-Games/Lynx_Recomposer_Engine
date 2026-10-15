#if defined (__APPLE__) && LYNX_TVOS
#include <window/window_tvos.h>
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#include <lynx/headers/core.h>
#include <input/apple_controllers.h>

@interface IlenViewTV : UIView
@property (nonatomic, assign) lynx::window::WindowNativeTVOS* owner;
@end

@implementation IlenViewTV

+ (Class)layerClass {
    return [CAMetalLayer class];
}

- (instancetype)initWithFrame:(CGRect)frame {
    if(self = [super initWithFrame:frame]) {
        // En tvOS, la escala suele ser 1.0 (1080p) o 2.0 (4K), UIScreen sigue siendo válido
        self.contentScaleFactor = [UIScreen mainScreen].scale;
    }
    return self;
}

- (void)layoutSubviews {
    [super layoutSubviews];
    if(self.owner) {
        CAMetalLayer *metalLayer = (CAMetalLayer *)self.layer;
        
        // nativeScale es la forma más precisa de obtener los píxeles reales en Apple TV
        CGFloat scale = [[UIScreen mainScreen] nativeScale];
        if(self.contentScaleFactor != scale) {
            self.contentScaleFactor = scale;
        }
        
        CGSize size = self.bounds.size;
        uint32_t newWidth = (uint32_t)(size.width * scale);
        uint32_t newHeight = (uint32_t)(size.height * scale);
        
        // Sincronizar el tamaño del buffer de Metal
        metalLayer.drawableSize = CGSizeMake(newWidth, newHeight);
        
        if(newWidth != lynx::window::IWindow::getWidth() || newHeight != lynx::window::IWindow::getHeight()) {
            self.owner->actualWidth = newWidth;
            self.owner->actualHeight = newHeight;
            self.owner->_windowChanged.store(true);
        }
    }
}
@end

namespace lynx::window {
    
    std::unique_ptr<IWindow> IWindow::CreateWindow(const lynx::application::ApplicationMetadata& metadata) {
        auto native = std::make_unique<WindowNativeTVOS>(metadata);
        
        s_width = native->actualWidth;
        s_height = native->actualHeight;
        
        return std::make_unique<IWindow>(std::move(native));
    }

    WindowNativeTVOS::WindowNativeTVOS(const lynx::application::ApplicationMetadata& metadata) {
        actualWidth = 0;
        actualHeight = 0;
        _windowChanged.store(false);
        
        AddGameControllerNotifications();
        // Asumimos que en el metadata pasas el puntero de la UIWindow desde el SceneDelegate
        nativeWindow = (__bridge UIWindow*)metadata.mobileNativeWindow;
        
        UIScreen *mainScreen = [UIScreen mainScreen];
        CGFloat screenScale = mainScreen.nativeScale;
        CGRect frame = mainScreen.bounds;
        
        IlenViewTV* lynxView = [[IlenViewTV alloc] initWithFrame:frame];
        lynxView.layer.contentsScale = screenScale;
        lynxView.owner = this;
        
        nativeView = lynxView;
        
        // Importante en tvOS para que la vista llene siempre la pantalla
        nativeView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        
        ((CAMetalLayer *)lynxView.layer).drawableSize = CGSizeMake(frame.size.width * screenScale, frame.size.height * screenScale);
        
        rootViewController = [[UIViewController alloc] init];
        rootViewController.view = nativeView;
        
        nativeWindow.rootViewController = rootViewController;
        // makeKeyAndVisible es vital en tvOS para el sistema de Focus
        [nativeWindow makeKeyAndVisible];
        
        CGSize initialPixelSize = ((CAMetalLayer *)lynxView.layer).drawableSize;
        actualWidth = (uint32_t)initialPixelSize.width;
        actualHeight = (uint32_t)initialPixelSize.height;
        _windowChanged.store(true);
    }
    
    void WindowNativeTVOS::pollEvents() {
        // En tvOS los eventos se gestionan vía UIKit/GameController,
        // normalmente no necesitas polling manual aquí.
    }

    bool WindowNativeTVOS::shouldClose() {
        return false;
    }

    bool WindowNativeTVOS::shouldPause() {
        return isOutOfFocus;
    }

    void WindowNativeTVOS::updateGamepads() {
        UpdateGamePadsApple();
    }


    void* WindowNativeTVOS::getNativeWindow() const {
        return (__bridge void*)nativeWindow;
    }

    const void* WindowNativeTVOS::getNativeView() const {
        return (__bridge void*)nativeView;
    }

    bool WindowNativeTVOS::checkWindowResize(uint32_t &outWidth, uint32_t &outHeight) {
        if(!_windowChanged.load()) {
            return false;
        }
        
        outWidth = actualWidth;
        outHeight = actualHeight;
        _windowChanged.store(false);
        
        return true;
    }
}
#endif
