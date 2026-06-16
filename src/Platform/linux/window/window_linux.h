#pragma once
#if ILEN_LINUX
#include <ilen/window/window.h>
#include <ilen/headers/enumerations.h>

//X11 Window (USING XCB)
#include <xcb/xcb.h>

namespace ilen::window {
    struct XCBSurfaceCreationInfo {
        xcb_window_t _window;
        xcb_connection_t* _connection;
    };

    //X11 implementation using modern XCB
    class WindowsNativeLinuxXCB final : public WindowNative {
        public:
            WindowsNativeLinuxXCB(const ilen::application::ApplicationMetadata& metadata);
            ~WindowsNativeLinuxXCB() override;
            void pollEvents() override;
            bool shouldClose() override {return m_shouldClose; }
            void updateGamepads() override;
            bool shouldPause() override;
            const void* getNativeView() const override;
            void* getNativeWindow() const override {return m_connection; }
            bool checkWindowResize(uint32_t &outWidth, uint32_t &outHeight) override;
    private:
        void applyWindowProperties(uint32_t properties);
        void setNetWMState(bool add, xcb_atom_t state);
        //void updateSizeHints(uint32_t properties);

        xcb_connection_t* m_connection = nullptr;
        xcb_window_t m_window = XCB_NONE;
        xcb_atom_t m_NetWMState = XCB_NONE;
        xcb_atom_t m_NetWMStateFullScreen = XCB_NONE;
        xcb_atom_t m_WMProtocols = XCB_NONE;
        xcb_atom_t m_WMDeleteWindow = XCB_NONE;
        xcb_atom_t m_MotifWMHints = XCB_NONE;

        bool m_shouldClose = false;
        std::atomic<bool> m_resized = false;
        uint32_t m_width = 0;
        uint32_t m_height = 0;

        XCBSurfaceCreationInfo surfaceInfo; //Necesario para pasar la estructura a Vulkan.

        static inline const char* DEBUG_PREFIX = "LINUX_XCB_WINDOW";
    };
}
#endif
