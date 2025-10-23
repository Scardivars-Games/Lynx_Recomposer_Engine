#include <ilen/window/window.h>

namespace ilen::window
{
    int IWindow::s_width = 0;
    int IWindow::s_height = 0;

    //Creation is in platform specific window

    IWindow::IWindow(std::unique_ptr<WindowNative> nativeWindow)
    {
        p_windowNative = std::move(nativeWindow);
    }

    IWindow::~IWindow() {};

    void IWindow::processEvents()
    {
        if (p_windowNative)
        {
            p_windowNative->pollEvents();
        }
    }

    bool IWindow::shouldClose()
    {
        if (p_windowNative)
        {
            return p_windowNative->shouldClose();
        }
        return true;
    }

    void* IWindow::getWindow()
    {
        if (p_windowNative)
        {
            return p_windowNative->getNativeWindow();
        }
        return nullptr;
    }

    void* IWindow::getSurface()
    {
        if (p_windowNative)
        {
            return p_windowNative->getNativeView();
        }
        return nullptr;
    }
}
