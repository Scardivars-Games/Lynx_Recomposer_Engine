//ENTRY POINT FOR iOS
#if ILEN_IOS
#import <UIKit/UIKit.h>
#include <ilen/core/lifetime.h>
#include <ilen/headers/ilen_logs.h>
#include <ilen/core/exceptions.h>
#include <ilen/input/input_buffer.h>
#include <ilen/core/runtime_options.h>
#include <window/window_ios.h>

@class SceneDelegate;

@interface IlenWindow : UIWindow
@end

@implementation IlenWindow

-(void)setRootViewController:(UIViewController *)rootViewController{
    [super setRootViewController:rootViewController];
    
    if(rootViewController){
        [rootViewController setNeedsUpdateOfHomeIndicatorAutoHidden];
        [rootViewController setNeedsUpdateOfScreenEdgesDeferringSystemGestures];
    }
}

-(void)layoutSubviews {
    [super layoutSubviews];
    
    UIViewController* vc = self.rootViewController;
    if(vc) {
        [vc setNeedsUpdateOfHomeIndicatorAutoHidden];
        [vc setNeedsUpdateOfScreenEdgesDeferringSystemGestures];
    }
}
@end

@interface AppDelegate : UIResponder <UIApplicationDelegate>

@end

//AppDelegate -- Necesario para la versión de iOS
@implementation AppDelegate
-(UISceneConfiguration *)application:(UIApplication *)application configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession options:(UISceneConnectionOptions *)options;{
    return [[UISceneConfiguration alloc] initWithName:@"Default Configuration" sessionRole: connectingSceneSession.role];
}
@end

@interface SceneDelegate : UIResponder<UIWindowSceneDelegate>

@property (strong, nonatomic) IlenWindow *window;
@property (nonatomic, assign) ilen::AppRuntime* app;
@property (nonatomic, strong) CADisplayLink *displayLink;
@property (nonatomic, assign) BOOL engineFailed;

@end

@implementation SceneDelegate

//Se llama cada vez que una escena quiera comunicarse con la app
- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions{
    //Confirmar que la escena sea del tipo esperado
    if(![scene isKindOfClass:[UIWindowScene class]]){
        return;
    }
    
    _engineFailed = NO;
    self.app = &ilen::AppRuntime::getInstance();
    
    UIWindowScene *windowScene = (UIWindowScene *)scene;
    self.window = [[IlenWindow alloc] initWithWindowScene:windowScene];
    
    ilen::application::ApplicationMetadata metadata{};
    metadata.applicationName = "ILEN Test";
    metadata.applicationDeveloper = "Scardivar's Games";
    metadata.applicationVersion = "0.0.1";
    metadata.applicationFramerate = ilen::core::options::GraphicsOptions::actualFPS;
    metadata.mobileNativeWindow = (__bridge void*)self.window;
    
    try {
        self.app -> init(metadata);
    } catch (const ilen::core::BasicException& e) {
        _engineFailed = YES;
        ILEN_FATAL_ERROR_P(e.prefix(), e.what());
        [self stopApp];
    }
    
    if(_engineFailed){
        return;
    }
    
    self.displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(renderLoop)];
    
    //Para dar soporte a 120 HZ en ProDisplay
    if(@available(iOS 15.0, *)){
        CAFrameRateRange countPerSecond = CAFrameRateRangeMake(10, 120, metadata.applicationFramerate);
        self.displayLink.preferredFrameRateRange = countPerSecond;
    }else{
        self.displayLink.preferredFramesPerSecond = metadata.applicationFramerate;
    }
    
    [self.displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
}

- (void)renderLoop {
    if(_engineFailed || !self.app){
        return;
    }
    
    try{
        if(self.app->isRunning()) {
            self.app -> update();
        }else{
            [self stopApp];
        }
    }catch (const ilen::core::BasicException& e) {
        _engineFailed = YES;
        ILEN_FATAL_ERROR_P(e.prefix(), e.what());
        [self stopApp];
    }
}

- (void)sceneDidDisconnect:(UIScene *)scene{
    [self stopApp];
}

- (void)stopRenderLoop {
    if(self.displayLink){
        [self.displayLink invalidate];
        self.displayLink = nullptr;
    }
}

- (void)engineCleanup {
    if(self.app){
        self.app->shutdown();
        self.app = nullptr;
    }
}

- (void)stopApp{
    [self stopRenderLoop];
    [self engineCleanup];
}

-(void)sceneWillResignActive:(UIScene *)scene {
    //Acaba de entrar a foreground
    ilen::input::InputBuffer::Get().ForceResetData();
    auto win = static_cast<ilen::window::WindowNativeIOS*>(self.app->getEngineContext()->p_gameWindow->getNativeWindow());
    win->isOutOfFocus = true;
}

-(void)sceneDidBecomeActive:(UIScene *)scene {
    //Obtuvo el foco la app
    auto win = static_cast<ilen::window::WindowNativeIOS*>(self.app->getEngineContext()->p_gameWindow->getNativeWindow());
    win->isOutOfFocus = false;
}
@end

//El main de la app de iOS!!!
int main(int argc, char* argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}

#endif //ILEN_IOS
