#include <ilen/window/window.h>

namespace ilen::window
{
    uint32_t IWindow::s_width = 0;
    uint32_t IWindow::s_height = 0;

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
            //checkWindowResize(s_width, s_height);
            p_windowNative->checkWindowResize(s_width, s_height);
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

    const void* IWindow::getSurface()
    {
        if (p_windowNative)
        {
            return p_windowNative->getNativeView();
        }
        return nullptr;
    }

   /* bool IWindow::checkWindowResize(uint32_t &outWidth, uint32_t &outHeight) {
        if (!p_windowNative) return false;

    }*/

}
