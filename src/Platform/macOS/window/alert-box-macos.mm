#if defined (__APPLE__) && ILEN_MACOS

#include <ilen/window/alert-box.h>

#import <Cocoa/Cocoa.h>

namespace ilen::window::box {
    void ShowErrorDialogBox(const std::string& title, const std::string& message){
        @autoreleasepool {
            NSString* dialogTitle = [NSString stringWithUTF8String:title.c_str()];
            NSString* dialogMessage = [NSString stringWithUTF8String:message.c_str()];

            dispatch_block_t alertBlock = ^{
                NSAlert* alert = [[NSAlert alloc] init];
                [alert setMessageText:dialogTitle];
                [alert setInformativeText:dialogMessage];
                [alert setAlertStyle:NSAlertStyleCritical];
                //[alert addButtonWithTitle:@"OK"];

                [alert runModal];
            };

            if([NSThread isMainThread]){
                alertBlock();
            }else{
                //Hilo secundario no puedo mostrar UI. Hay que sincronizar para forzar a que aparesca en el main thread.
                dispatch_sync(dispatch_get_main_queue(), alertBlock);
            }
        }
    }
}

#endif

