//ENTRY POINT FOR iOS

#import <UIKit/UIKit.h>
#include <ilen/core/lifetime.h>
#include <ilen/headers/ilen-logs.h>
#include <ilen/core/exceptions.h>

@class SceneDelegate;

@interface AppDelegate : UIResponder <UIApplicationDelegate>

@end

//AppDelegate -- Necesario para la versión de iOS
@implementation AppDelegate
-(UISceneConfiguration *)application:(UIApplication *)application configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession options:(UISceneConnectionOptions *)options;{
    return [[UISceneConfiguration alloc] initWithName:@"Default Configuration" sessionRole: connectingSceneSession.role];
}
@end

@interface SceneDelegate : UIResponder<UIWindowSceneDelegate>

@property (strong, nonatomic) UIWindow *window;
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
    self.window = [[UIWindow alloc] initWithWindowScene:windowScene];
    
    ilen::application::ApplicationMetadata metadata{};
    metadata.applicationName = "ILEN Test";
    metadata.applicationDeveloper = "Scardivar's Games";
    metadata.applicationVersion = "0.0.1";
    metadata.iOSUIWindow = (__bridge void*)self.window;
    
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
        CAFrameRateRange countPerSecond = CAFrameRateRangeMake(10, 120, 120);
        self.displayLink.preferredFrameRateRange = countPerSecond;
    }else{
        self.displayLink.preferredFramesPerSecond = 120;
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

@end


//El main de la app de iOS!!!
int main(int argc, char* argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}

