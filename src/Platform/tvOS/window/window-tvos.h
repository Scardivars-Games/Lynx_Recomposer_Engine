#pragma once

// Cambiamos la macro para que sea específica de tvOS
#if defined (__APPLE__) && defined(ILEN_TVOS)
#include <ilen/window/window.h>
#include <atomic>
#include <cstdint>

// Forward declarations de Objective-C
@class UIView;
@class UIWindow;
@class UIViewController;

namespace ilen::window
{
    class WindowNativeTVOS : public WindowNative
    {
        public:
            WindowNativeTVOS(const ilen::application::ApplicationMetadata& metadata);
            
            ~WindowNativeTVOS() override = default;
            
            void pollEvents() override;
            void updateGamepads() override;
            bool shouldClose() override;
            bool shouldPause() override;
            
            void* getNativeWindow() const override;
            const void* getNativeView() const override;
            bool checkWindowResize(uint32_t &outWidth, uint32_t &outHeight) override;
        
            // Métodos públicos de la implementación (punteros a objetos UIKit)
            UIWindow* nativeWindow = nullptr;
            UIView* nativeView = nullptr;
            UIViewController* rootViewController = nullptr;
            bool isOutOfFocus = false;

            
            // Gestión de estado de la ventana
            std::atomic<bool> _windowChanged{false};
            uint32_t actualWidth = 0;
            uint32_t actualHeight = 0;

        private:
            // En tvOS, el cierre de la ventana no es común (la app se suspende),
            // pero mantenemos la lógica por compatibilidad con la interfaz WindowNative.
            bool _shouldClose = false;
    };
}

#endif