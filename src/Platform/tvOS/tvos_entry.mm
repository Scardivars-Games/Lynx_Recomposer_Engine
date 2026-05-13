// ENTRY POINT FOR tvOS

#if ILEN_TVOS
#import <UIKit/UIKit.h>
#include <ilen/core/lifetime.h>
#include <ilen/headers/ilen-logs.h>
#include <ilen/core/exceptions.h>
#include <ilen/input/input-buffer.h>
#include <window/window-tvos.h>

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
@property (nonatomic, assign) ilen::AppRuntime* app;
@property (nonatomic, strong) CADisplayLink *displayLink;
@property (nonatomic, assign) BOOL engineFailed;

@end

@implementation SceneDelegate

- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions {
    if (![scene isKindOfClass:[UIWindowScene class]]) {
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
    
    // IMPORTANTE: Cambiamos el nombre de la propiedad si tu motor distingue entre plataformas, 
    // o puedes seguir usando el puntero genérico de la ventana.
    metadata.mobileNativeWindow = (__bridge void*)self.window;
    
    try {
        self.app->init(metadata);
    } catch (const ilen::core::BasicException& e) {
        _engineFailed = YES;
        ILEN_FATAL_ERROR_P(e.prefix(), e.what());
        [self stopApp];
    }
    
    if (_engineFailed) {
        return;
    }
    
    self.displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(renderLoop)];
    
    // AJUSTE PARA tvOS: 
    // Aunque algunas TVs soportan 120Hz, la API preferredFrameRateRange está disponible 
    // en tvOS 15.0 igual que en iOS.
    if (@available(tvOS 15.0, *)) {
        // En TV es más común fijar a 60 o usar el rango nativo.
        CAFrameRateRange countPerSecond = CAFrameRateRangeMake(30, 60, 60); 
        self.displayLink.preferredFrameRateRange = countPerSecond;
    } else {
        self.displayLink.preferredFramesPerSecond = 60;
    }
    
    [self.displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
    
    // En tvOS es vital hacer visible la ventana para que el sistema tome el foco
    [self.window makeKeyAndVisible];
}

- (void)renderLoop {
    if (_engineFailed || !self.app) {
        return;
    }
    
    try {
        if (self.app->isRunning()) {
            self.app->update();
        } else {
            [self stopApp];
        }
    } catch (const ilen::core::BasicException& e) {
        _engineFailed = YES;
        ILEN_FATAL_ERROR_P(e.prefix(), e.what());
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
    ilen::input::InputBuffer::Get().ForceResetData();
    auto win = static_cast<ilen::window::WindowNativeTVOS*>(self.app->getEngineContext()->p_gameWindow->getNativeWindow());
    win->isOutOfFocus = true;
}

-(void)sceneDidBecomeActive:(UIScene *)scene {
    //Obtuvo el foco la app
    auto win = static_cast<ilen::window::WindowNativeTVOS*>(self.app->getEngineContext()->p_gameWindow->getNativeWindow());
    win->isOutOfFocus = false;
}
@end

// El main para tvOS
int main(int argc, char* argv[]) {
    @autoreleasepool {
        // UIApplicationMain funciona igual en tvOS
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}
#endif //ILEN_TVOS
