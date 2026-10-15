#pragma once
#if defined (__APPLE__) && LYNX_MACOS
#include <lynx/window/window.h>
#include <lynx/headers/enumerations.h>

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

#ifdef __OBJC__
@class AppDelegate;
@class NSWindow;
@class NSView;
#endif


namespace lynx::window
{
    class WindowNativeMacOS : public WindowNative {
        public:
            WindowNativeMacOS(const lynx::application::ApplicationMetadata& metadata);

            ~WindowNativeMacOS() = default;

            //Implementación de la clase
            void pollEvents() override;
            void updateGamepads() override;
            bool shouldClose() override;
            bool shouldPause() override;
            void* getNativeWindow() const override;
            const void*  getNativeView() const override;
            bool checkWindowResize(uint32_t &outWidth, uint32_t &outHeight) override;

            //Métodos públicos (antiguo)
            NSWindow* nativeWindow = nil;
            NSView* nativeView = nil;
            bool closeRequested = false;
            bool isOutOfFocus = false;
            AppDelegate* appDelegate = nil;
        
            std::atomic<bool> _windowSizeChanged;
            CGSize _drawableSize;
    };
}

#endif


