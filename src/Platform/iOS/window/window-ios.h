#pragma once
#if defined (__APPLE__) && ILEN_IOS
#include <ilen/window/window.h>

@class UIView;
@class UIWindow;
@class UIViewController;

namespace ilen::window
{
    class WindowNativeIOS : public WindowNative
    {
        public:
            WindowNativeIOS(const ilen::application::ApplicationMetadata& metadata);
            
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
            UIViewController* rootViewController = nullptr;
            bool isOutOfFocus = false;
            
            std::atomic<bool> _windowChanged;
            uint32_t actualWidth;
            uint32_t actualHeight;
    };
}

#endif
