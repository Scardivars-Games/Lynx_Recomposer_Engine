#pragma once
#if defined (__APPLE__) && LYNX_IOS
#include <lynx/window/window.h>

@class UIView;
@class UIWindow;
@class UIViewController;
@class IlenViewController;

namespace lynx::window
{
    class WindowNativeIOS : public WindowNative
    {
        public:
            WindowNativeIOS(const lynx::application::ApplicationMetadata& metadata);
            
            ~WindowNativeIOS() = default;
            void pollEvents() override;
            void updateGamepads() override;
            bool shouldClose() override;
            bool shouldPause() override;
            void* getNativeWindow() const override;
            const void* getNativeView() const override;
            bool checkWindowResize(uint32_t &outWidth, uint32_t &outHeight) override;
        
            //Metodos públicos de la implementación
            UIWindow* nativeWindow = nullptr;
            UIView* nativeView = nullptr;
            IlenViewController* rootViewController = nullptr;
            bool isOutOfFocus = false;
            
            std::atomic<bool> _windowChanged;
            uint32_t actualWidth;
            uint32_t actualHeight;
    };
}

#endif
