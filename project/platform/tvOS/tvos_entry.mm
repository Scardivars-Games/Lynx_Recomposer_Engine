// ENTRY POINT FOR tvOS

#if LYNX_TVOS
#import <UIKit/UIKit.h>

#include <game/runtime_options.h>
#include <lynx/core/lifetime.h>
#include <lynx/headers/lynx_logs.h>
#include <lynx/core/exceptions.h>
#include <lynx/input/input_buffer.h>

@class SceneDelegate;

@interface AppDelegate : UIResponder <UIApplicationDelegate>
@end

@implementation AppDelegate
- (UISceneConfiguration *)application:(UIApplication *)application configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession options:(UISceneConnectionOptions *)options {
    return [[UISceneConfiguration alloc] initWithName:@"Default Configuration" sessionRole:connectingSceneSession.role];
}
@end

@interface SceneDelegate : UIResponder <UIWindowSceneDelegate>

@property (strong, nonatomic) UIWindow *window;
@property (nonatomic, assign) lynx::AppRuntime* app;
@property (nonatomic, strong) CADisplayLink *displayLink;
@property (nonatomic, assign) BOOL engineFailed;

@end

@implementation SceneDelegate

- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions {
    if (![scene isKindOfClass:[UIWindowScene class]]) {
        return;
    }
    
    _engineFailed = NO;
    self.app = &lynx::AppRuntime::getInstance();
    
    UIWindowScene *windowScene = (UIWindowScene *)scene;
    self.window = [[UIWindow alloc] initWithWindowScene:windowScene];
    
    auto bootstrap = game::generated::MakeProjectBootstrap();
    bootstrap.metadata.mobileNativeWindow = (__bridge void*)self.window;
    
    const NSInteger desiredFPS = bootstrap.metadata.applicationFramerate;
    
    NSInteger maximumFPS = UIScreen.mainScreen.maximumFramesPerSecond;
    NSInteger requestedFPS = MIN(
         desiredFPS,
        maximumFPS
    );
    
    
    try {
        self.app->init(std::move(bootstrap));
    } catch (const lynx::core::BasicException& e) {
        _engineFailed = YES;
        LYNX_FATAL_ERROR_P(e.prefix(), e.what());
        [self stopApp];
    }catch(const std::exception &exception) {
        self.engineFailed = YES;
        LYNX_FATAL_ERROR_P("GAME", exception.what());
        [self stopApp];
    }
    
    if (_engineFailed) {
        return;
    }

    self.displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(renderLoop:)];
    
    // AJUSTE PARA tvOS: 
    // Aunque algunas TVs soportan 120Hz, la API preferredFrameRateRange está disponible 
    // en tvOS 15.0 igual que en iOS.
    if (@available(tvOS 15.0, *)) {
        // En TV es más común fijar a 60 o usar el rango nativo.
        CAFrameRateRange countPerSecond = CAFrameRateRangeMake(requestedFPS, requestedFPS, requestedFPS); 
        self.displayLink.preferredFrameRateRange = countPerSecond;
    } else {
        self.displayLink.preferredFramesPerSecond = requestedFPS;
    }
    
    [self.displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
    
    // En tvOS es vital hacer visible la ventana para que el sistema tome el foco
    [self.window makeKeyAndVisible];
}

- (void)renderLoop:(CADisplayLink *)displayLink {
    if (_engineFailed || !self.app) {
        return;
    }
    
    try {
        if (self.app->isRunning()) {
            self.app->update();
        } else {
            [self stopApp];
        }
    } catch (const lynx::core::BasicException& e) {
        _engineFailed = YES;
        LYNX_FATAL_ERROR_P(e.prefix(), e.what());
        [self stopApp];
    }catch(const std::exception &exception) {
        self.engineFailed = YES;
        LYNX_FATAL_ERROR_P("GAME", exception.what());
        [self stopApp];
    }
}

- (void)sceneDidDisconnect:(UIScene *)scene {
    [self stopApp];
}

- (void)stopRenderLoop {
    if (self.displayLink) {
        [self.displayLink invalidate];
        self.displayLink = nil;
    }
}

- (void)engineCleanup {
    if (self.app) {
        self.app->shutdown();
        self.app = nullptr;
    }
}

- (void)stopApp {
    [self stopRenderLoop];
    [self engineCleanup];
}

-(void)sceneWillResignActive:(UIScene *)scene {
    //Acaba de entrar a foreground
    lynx::input::InputBuffer::Get().ForceResetData();
    self.displayLink.paused = YES;
}

-(void)sceneDidBecomeActive:(UIScene *)scene {
    //Obtuvo el foco la app
    self.displayLink.paused = NO;
}
@end

// El main para tvOS
int main(int argc, char* argv[]) {
    @autoreleasepool {
        // UIApplicationMain funciona igual en tvOS
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}
#endif //LYNX_TVOS
