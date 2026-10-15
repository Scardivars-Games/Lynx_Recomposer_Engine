#if defined (__APPLE__) && LYNX_IOS

#include <lynx/window/alert_box.h>
#include <cstdlib>
#import <UIKit/UIKit.h>

namespace lynx::window::box {
    void ShowErrorDialogBox(const std::string& title, const std::string& message){
        NSString* dialogTitle = [NSString stringWithUTF8String:title.c_str()];
        NSString* dialogMessage = [NSString stringWithUTF8String:message.c_str()];
        
        dispatch_block_t alertDialog = ^{
            UIAlertController* alert = [UIAlertController alertControllerWithTitle:dialogTitle message:dialogMessage preferredStyle:UIAlertControllerStyleAlert];
            UIAlertAction* okAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault
                                                             handler:^(UIAlertAction * _Nonnull action){
                                                                std::abort();
                                                            }];
            
            [alert addAction:okAction];
            // Prefer the active window scene, but retain a connected scene as a fallback.
            UIWindowScene* windowScene = nil;
            for (UIScene* scene in [UIApplication sharedApplication].connectedScenes) {
                if (![scene isKindOfClass:[UIWindowScene class]]) {
                    continue;
                }

                UIWindowScene* candidateScene = (UIWindowScene*)scene;
                if (!windowScene) {
                    windowScene = candidateScene;
                }
                if (scene.activationState == UISceneActivationStateForegroundActive) {
                    windowScene = candidateScene;
                    break;
                }
            }

            UIWindow* presentingWindow = nil;
            for (UIWindow* window in windowScene.windows) {
                if (window.isKeyWindow) {
                    presentingWindow = window;
                    break;
                }
            }
            if (!presentingWindow) {
                presentingWindow = windowScene.windows.firstObject;
            }

            UIViewController* presenter = presentingWindow.rootViewController;
            // Ascend to the top-most presented view controller
            while (presenter.presentedViewController) {
                presenter = presenter.presentedViewController;
            }

            if (presenter) {
                [presenter presentViewController:alert animated:YES completion:nil];
            }
        };
        dispatch_async(dispatch_get_main_queue(), alertDialog);
    }
}

#endif

