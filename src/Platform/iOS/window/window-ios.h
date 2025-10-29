#pragma once
#if defined (__APPLE__) && ILEN_IOS
#include <ilen/window/window.h>

@class UIWindow;
@class MTKView;
@class UIViewController;

namespace ilen::window
{
    class WindowNativeIOS : public WindowNative
    {
        public:
            WindowNativeIOS(const ilen::application::ApplicationMetadata& metadata);
            
            ~WindowNativeIOS() = default;
            void pollEvents() override;
            bool shouldClose() override;
            void* getNativeWindow() const override;
            void* getNativeView() const override;
            bool checkWindowResize(uint32_t &outWidth, uint32_t &outHeight) override;
        
            //Metodos públicos de la implementación
            UIWindow* nativeWindow = nullptr;
            MTKView* nativeView = nullptr;
            UIViewController* rootViewController = nullptr;
            
            std::atomic<bool> _windowChanged;
            uint32_t actualWidth;
            uint32_t actualHeight;
    };
}

#endif
