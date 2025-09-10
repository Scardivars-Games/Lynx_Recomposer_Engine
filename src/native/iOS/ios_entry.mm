//ENTRY POINT FOR iOS

#import <UIKit/UIKit.h>
#include <ilen/core/lifetime.h>

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

@end

@implementation SceneDelegate

//Se llama cada vez que una escena quiera comunicarse con la app
- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions{
    //Confirmar que la escena sea del tipo esperado
    if(![scene isKindOfClass:[UIWindowScene class]]){
        return;
    }
    
    self.app = new ilen::AppRuntime();
    
    UIWindowScene *windowScene = (UIWindowScene *)scene;
    self.window = [[UIWindow alloc] initWithWindowScene:windowScene];
    
    ilen::application::ApplicationMetadata metadata{};
    metadata.applicationName = "ILEN Test";
    metadata.applicationDeveloper = "Scardivar's Games";
    metadata.applicationVersion = "0.0.1";
    metadata.iOSUIWindow = (__bridge void*)self.window;

    
    self.app -> init(metadata);
    
    self.displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(renderLoop)];
    [self.displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
}

- (void)renderLoop {
    if(self.app) {
        self.app -> update();
    }
}

- (void)sceneDidDisconnect:(UIScene *)scene{
    if(self.app) {
        delete self.app;
        self.app = nullptr;
    }
}

@end


//El main de la app de iOS!!!
int main(int argc, char* argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}
