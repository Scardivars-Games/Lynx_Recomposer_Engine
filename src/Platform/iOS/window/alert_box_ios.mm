#if defined (__APPLE__) && ILEN_IOS

#include <ilen/window/alert_box.h>
#include <cstdlib>
#import <UIKit/UIKit.h>

namespace ilen::window::box {
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
            // Find a suitable window scene and root view controller without using deprecated keyWindow
            UIScene* activeScene = nil;
            for (UIScene* scene in [UIApplication sharedApplication].connectedScenes) {
                if (scene.activationState == UISceneActivationStateForegroundActive && [scene isKindOfClass:[UIWindowScene class]]) {
                    activeScene = scene;
                    break;
                }
            }

            UIWindow* presentingWindow = nil;
            if (activeScene) {
                UIWindowScene* windowScene = (UIWindowScene*)activeScene;
                for (UIWindow* window in windowScene.windows) {
                    if (window.isKeyWindow) {
                        presentingWindow = window;
                        break;
                    }
                }
                if (!presentingWindow) {
                    presentingWindow = windowScene.windows.firstObject;
                }
            } else {
                // Fallback for older cases where scene may be nil
                presentingWindow = [UIApplication sharedApplication].windows.firstObject;
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

