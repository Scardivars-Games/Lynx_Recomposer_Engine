#pragma once
#if ILEN_LINUX
#include <ilen/window/window.h>
#include <ilen/headers/enumerations.h>

//X11 Window
#include <X11/Xlib.h>

namespace ilen::window {
    class WindowsNativeLinuxX11 final : public WindowNative {
        
    }
}
#endif