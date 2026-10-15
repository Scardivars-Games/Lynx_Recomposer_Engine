#if defined (__APPLE__) && LYNX_IOS
#include <window/window_ios.h>
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#include <lynx/headers/core.h>
#include <lynx/input/input_buffer.h>
#include <input/apple_controllers.h>

@interface IlenView : UIView
@property (nonatomic, assign) lynx::window::WindowNativeIOS* owner;
@end

@implementation IlenView

+ (Class)layerClass {
    return [CAMetalLayer class];
}

- (instancetype)initWithFrame:(CGRect)frame{
    if(self = [super initWithFrame:frame]){
        self.contentScaleFactor = [UIScreen mainScreen].scale;
        self.multipleTouchEnabled = YES;
        self.userInteractionEnabled = YES;
    }
    return self;
}

-(void)layoutSubviews{
    [super layoutSubviews];
    if(self.owner){
        CAMetalLayer *metalLayer = (CAMetalLayer *)self.layer;
        CGFloat scale = [[UIScreen mainScreen] nativeScale];
        if(self.contentScaleFactor != scale){
            self.contentScaleFactor = scale;
        }
        CGSize size = self.bounds.size;
        uint32_t newWidth = (uint32_t)(size.width * scale);
        uint32_t newHeight = (uint32_t)(size.height * scale);
        // Keep CAMetalLayer's drawableSize in sync as well
        metalLayer.drawableSize = CGSizeMake(newWidth, newHeight);
        if(newWidth != lynx::window::IWindow::getWidth() || newHeight != lynx::window::IWindow::getHeight()){
            self.owner->actualWidth = newWidth;
            self.owner->actualHeight = newHeight;
            self.owner->_windowChanged.store(true);
        }
    }
}

//Capturar cuando un dedo toca la pantalla
-(void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event{
    auto& input =lynx::input::InputBuffer::Get();
    
    for(UITouch *touch in touches) {
        int slot = input.GetFreePointerSlot();
        if(slot != -1) {
            //Hay un slot válido para recibir el puntero.
            input.ModifyPointerSlot((uintptr_t)touch, slot, true);
            
            auto& ptr = input.GetPointer(slot);
            ptr.isDown = true;
            ptr.previousIsDown = false;
            ptr.type = lynx::input::PointerType::Touch;
            
            CGPoint loc = [touch locationInView:self];
            float scale = self.contentScaleFactor;
            
            ptr.x = loc.x * scale;
            ptr.y = self.owner->actualHeight - (loc.y * scale); //Necesitamos invertir y porque el motor requiere que el 0,0 sea la esquina inferior inzquierda.

            ptr.pressure = touch.force / touch.maximumPossibleForce;
        }
    }
}

-(void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event{
    auto& input = lynx::input::InputBuffer::Get();
    
    for(UITouch *touch in touches) {
        if(input.PointerSlotIsActive((uintptr_t)touch)) {
            uint32_t slot = input.GetPointerSlot((uintptr_t)touch);
            auto& ptr = input.GetPointer(slot);
            
            CGPoint loc = [touch locationInView:self];
            float scale = self.contentScaleFactor;
            ptr.x = loc.x * scale;
            ptr.y = self.owner->actualHeight - (loc.y * scale); //Necesitamos invertir y porque el motor requiere que el 0,0 sea la esquina inferior inzquierda.
            ptr.pressure = touch.force / touch.maximumPossibleForce;
        }
    }
}

-(void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    auto& input = lynx::input::InputBuffer::Get();
    
    for(UITouch *touch in touches) {
        if(input.PointerSlotIsActive((uintptr_t)touch)){
            uint32_t slot = input.GetPointerSlot((uintptr_t)touch);
            input.GetPointer(slot).isDown = false;
            input.ModifyPointerSlot((uintptr_t)touch, slot, false);
        }
    }
}
@end

@interface IlenViewController: UIViewController
@end

@implementation IlenViewController

-(bool)prefersStatusBarHidden {
    return YES;
}

-(bool)prefersHomeIndicatorAutoHidden {
    return YES;
}

-(UIRectEdge)preferredScreenEdgesDeferringSystemGestures {
    return UIRectEdgeAll;
}
@end

namespace lynx::window{
    std::unique_ptr<IWindow> IWindow::CreateWindow(const lynx::application::ApplicationMetadata& metadata){
        auto native = std::make_unique<WindowNativeIOS>(metadata);
        
        s_width = native->actualWidth;
        s_height = native->actualHeight;
        
        return std::make_unique<IWindow>(std::move(native));
    }


    WindowNativeIOS::WindowNativeIOS(const lynx::application::ApplicationMetadata& metadata){
        actualWidth = 0;
        actualHeight = 0;
        _windowChanged.store(false);
        
        AddGameControllerNotifications();
        
        nativeWindow = (__bridge UIWindow*)metadata.mobileNativeWindow;
        
        /*
        CGRect frame = [[UIScreen mainScreen] bounds];
        IlenMTKView* lynxView = [[IlenMTKView alloc] initWithFrame:frame];
        lynxView.owner = this;*/
        
        UIScreen *mainScreen = [UIScreen mainScreen];
        CGFloat screenScale = mainScreen.nativeScale;
        CGRect frame = mainScreen.bounds;
        
        //IlenMTKView* lynxView = [[IlenMTKView alloc] initWithFrame:frame];
        IlenView* lynxView = [[IlenView alloc] initWithFrame:frame];
        lynxView.layer.contentsScale = screenScale;
        lynxView.owner = this;
        
        nativeView = lynxView;
        
        nativeView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        
        ((CAMetalLayer *)lynxView.layer).drawableSize = CGSizeMake(frame.size.width * screenScale, frame.size.height * screenScale);
        
        //rootViewController = [[UIViewController alloc] init];
        //rootViewController.view = nativeView;
        
        IlenViewController* customVC = [[IlenViewController alloc] init];
        customVC.view = nativeView;
        
        rootViewController = customVC;
        
        nativeWindow.rootViewController = rootViewController;
        [nativeWindow makeKeyAndVisible];

        CGSize initialPixelSize = ((CAMetalLayer *)lynxView.layer).drawableSize;
        actualWidth = (uint32_t)initialPixelSize.width;
        actualHeight = (uint32_t)initialPixelSize.height;
        _windowChanged.store(true); //Para que en el primer frame actualiz
    }
    
    void WindowNativeIOS::pollEvents() {}
    void WindowNativeIOS::updateGamepads() { UpdateGamePadsApple(); }
    bool WindowNativeIOS::shouldClose() {return false; }
    bool WindowNativeIOS::shouldPause() {return isOutOfFocus; }
    void* WindowNativeIOS::getNativeWindow() const { return (__bridge void*)nativeWindow; }
    const void* WindowNativeIOS::getNativeView() const { return (__bridge void*)nativeView; }

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
    auto* meta = (lynx::application::ApplicationMetadata*)metadata;
    
    s_pNative = new WindowNative_iOS(meta->iOSUIWindow);
}*/
#endif


