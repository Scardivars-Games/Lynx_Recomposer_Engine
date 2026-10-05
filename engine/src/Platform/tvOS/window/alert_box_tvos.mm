#if defined (__APPLE__) && ILEN_TVOS

#include <ilen/window/alert_box.h>
#include <cstdlib>
#import <UIKit/UIKit.h>

namespace ilen::window::box {
    void ShowErrorDialogBox(const std::string& title, const std::string& message){
        NSString* dialogTitle = [NSString stringWithUTF8String:title.c_str()];
        NSString* dialogMessage = [NSString stringWithUTF8String:message.c_str()];
        
        dispatch_block_t alertDialog = ^{
            // En tvOS, UIAlertControllerStyleAlert crea un cuadro central optimizado para el control remoto
            UIAlertController* alert = [UIAlertController alertControllerWithTitle:dialogTitle
                                                                           message:dialogMessage
                                                                    preferredStyle:UIAlertControllerStyleAlert];
            
            // Acción de OK que aborta la ejecución tras el error grave
            UIAlertAction* okAction = [UIAlertAction actionWithTitle:@"OK"
                                                               style:UIAlertActionStyleDefault
                                                             handler:^(UIAlertAction * _Nonnull action){
                                                                std::abort();
                                                            }];
            
            [alert addAction:okAction];

            // Buscar la escena activa (UIWindowScene) para tvOS
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
                // Si no hay keyWindow, tomamos la primera disponible
                if (!presentingWindow) {
                    presentingWindow = windowScene.windows.firstObject;
                }
            } else {
                // Fallback para versiones antiguas o estados de inicialización temprana
                presentingWindow = [UIApplication sharedApplication].windows.firstObject;
            }

            UIViewController* presenter = presentingWindow.rootViewController;
            
            // Subir hasta el controlador de vista más superficial para presentar el alert
            while (presenter.presentedViewController) {
                presenter = presenter.presentedViewController;
            }

            if (presenter) {
                // En tvOS la animación es un ligero fundido y escala
                [presenter presentViewController:alert animated:YES completion:nil];
            }
        };
        
        // Es imperativo que UIKit se ejecute en el hilo principal
        dispatch_async(dispatch_get_main_queue(), alertDialog);
    }
}

#endif
