#pragma once
#if defined (__APPLE__) && ILEN_MACOS
#include <ilen/window/window.h>
#include <ilen/headers/enumerations.h>

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

#ifdef __OBJC__
@class AppDelegate;
@class NSWindow;
@class NSView;
#endif


namespace ilen::window
{
    class WindowNativeMacOS : public WindowNative {
        public:
            WindowNativeMacOS(const ilen::application::ApplicationMetadata& metadata);

            ~WindowNativeMacOS() = default;

            //Implementación de la clase
            void pollEvents() override;
            bool shouldClose() override;
            void* getNativeWindow() const override;
            const void*  getNativeView() const override;
            bool checkWindowResize(uint32_t &outWidth, uint32_t &outHeight) override;

            //Métodos públicos (antiguo)
            NSWindow* nativeWindow = nil;
            NSView* nativeView = nil;
            bool closeRequested = false;
            AppDelegate* appDelegate = nil;
        
            std::atomic<bool> _windowSizeChanged;
            CGSize _drawableSize;
    };
}

#endif


