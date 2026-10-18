#if LYNX_IOS

#import <UIKit/UIKit.h>

#include <exception>
#include <utility>

#include <game/runtime_options.h>
#include <lynx/core/exceptions.h>
#include <lynx/core/lifetime.h>
#include <lynx/headers/lynx_logs.h>
#include <lynx/input/input_buffer.h>

@class SceneDelegate;

@interface LynxWindow : UIWindow
@end


@implementation LynxWindow

- (void)setRootViewController:(UIViewController *)rootViewController {
    [super setRootViewController:rootViewController];

    if (rootViewController) {
        [rootViewController setNeedsUpdateOfHomeIndicatorAutoHidden];
        [rootViewController setNeedsUpdateOfScreenEdgesDeferringSystemGestures];
    }
}

- (void)layoutSubviews {
    [super layoutSubviews];

    UIViewController *viewController = self.rootViewController;
    if (viewController) {
        [viewController setNeedsUpdateOfHomeIndicatorAutoHidden];
        [viewController setNeedsUpdateOfScreenEdgesDeferringSystemGestures];
    }
}

@end


@interface AppDelegate : UIResponder <UIApplicationDelegate>
@end


@implementation AppDelegate

- (UISceneConfiguration *)application:(UIApplication *)application
        configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession
        options:(UISceneConnectionOptions *)options {
    return [[UISceneConfiguration alloc] initWithName:@"Default Configuration"
                                          sessionRole:connectingSceneSession.role];
}

@end


@interface SceneDelegate : UIResponder <UIWindowSceneDelegate>

@property(strong, nonatomic) LynxWindow *window;
@property(nonatomic, assign) lynx::AppRuntime *app;
@property(nonatomic, strong) CADisplayLink *displayLink;
@property(nonatomic, assign) BOOL engineFailed;

@end


@implementation SceneDelegate

- (void)scene:(UIScene *)scene
        willConnectToSession:(UISceneSession *)session
        options:(UISceneConnectionOptions *)connectionOptions {
    if (![scene isKindOfClass:[UIWindowScene class]]) {
        return;
    }

    self.engineFailed = NO;
    self.app = &lynx::AppRuntime::getInstance();

    UIWindowScene *windowScene = (UIWindowScene *)scene;
    self.window = [[LynxWindow alloc] initWithWindowScene:windowScene];

    auto bootstrap = game::generated::MakeProjectBootstrap();
    bootstrap.metadata.mobileNativeWindow = (__bridge void *)self.window;
    const NSInteger requestedFPS = bootstrap.metadata.applicationFramerate;

    try {
        self.app->init(std::move(bootstrap));
    } catch (const lynx::core::BasicException &exception) {
        self.engineFailed = YES;
        LYNX_FATAL_ERROR_P(exception.prefix(), exception.what());
        [self stopApp];
    } catch (const std::exception &exception) {
        self.engineFailed = YES;
        LYNX_FATAL_ERROR_P("GAME", exception.what());
        [self stopApp];
    }

    if (self.engineFailed) {
        return;
    }

    self.displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(renderLoop:)];
    const NSInteger maximumFPS = UIScreen.mainScreen.maximumFramesPerSecond;
    const NSInteger preferredFPS = MIN(requestedFPS, maximumFPS);

    if (@available(iOS 15.0, *)) {
        self.displayLink.preferredFrameRateRange = CAFrameRateRangeMake(10, maximumFPS, preferredFPS);
    } else {
        self.displayLink.preferredFramesPerSecond = preferredFPS;
    }

    [self.displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
    [self.window makeKeyAndVisible];
}

- (void)renderLoop:(CADisplayLink *)displayLink {
    if (self.engineFailed || !self.app) {
        return;
    }

    try {
        if (self.app->isRunning()) {
            self.app->update();
        } else {
            [self stopApp];
        }
    } catch (const lynx::core::BasicException &exception) {
        self.engineFailed = YES;
        LYNX_FATAL_ERROR_P(exception.prefix(), exception.what());
        [self stopApp];
    } catch (const std::exception &exception) {
        self.engineFailed = YES;
        LYNX_FATAL_ERROR_P("GAME", exception.what());
        [self stopApp];
    }
}

- (void)sceneDidDisconnect:(UIScene *)scene {
    [self stopApp];
}

- (void)sceneWillResignActive:(UIScene *)scene {
    lynx::input::InputBuffer::Get().ForceResetData();
    self.displayLink.paused = YES;
}

- (void)sceneDidBecomeActive:(UIScene *)scene {
    self.displayLink.paused = NO;
}

- (void)stopRenderLoop {
    [self.displayLink invalidate];
    self.displayLink = nil;
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

@end


int main(int argc, char *argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}

#endif
