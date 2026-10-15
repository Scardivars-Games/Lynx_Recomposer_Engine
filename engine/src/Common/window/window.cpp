#include <lynx/window/window.h>

namespace lynx::window
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

    void IWindow::updateGamepads() {
        if (p_windowNative)
        {
            p_windowNative->updateGamepads();
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

    bool IWindow::shouldPause(){
        if(p_windowNative){
            return p_windowNative->shouldPause();
        }
        return false; //No pausar si no hay window
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

    void* IWindow::getNativeWindow() {
        return p_windowNative.get();
    }

   /* bool IWindow::checkWindowResize(uint32_t &outWidth, uint32_t &outHeight) {
        if (!p_windowNative) return false;

    }*/

}
