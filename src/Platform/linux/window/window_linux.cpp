#if ILEN_LINUX
#include <window/window_linux.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <ilen/headers/ilen_logs.h>

static xcb_ewmh_connection_t* ewmh = nullptr;

using namespace ilen::window;
//Creación de la ventana de la función fábrica
std::unique_ptr<IWindow> IWindow::CreateWindow(const ilen::application::ApplicationMetadata& metadata) {
    s_width = metadata.startWidth;
    s_height = metadata.startHeight;

    //Aquí debería ir lógica para preferir Wayland antes de XCB
    auto native = std::make_unique<WindowsNativeLinuxXCB>(metadata);
    return std::make_unique<IWindow>(std::move(native));
}

WindowsNativeLinuxXCB::WindowsNativeLinuxXCB(const ilen::application::ApplicationMetadata &metadata) {
    //Conectarse al servidor X.
    m_connection = xcb_connect(nullptr, nullptr);
    if (!m_connection || xcb_connection_has_error(m_connection)) {
        ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "Imposible connectarse al servidor de X11 (XCB).");
    }

    //Init de los helper extendidos para Windows Manager
    if (!ewmh) {
        ewmh = new xcb_ewmh_connection_t;
        xcb_intern_atom_cookie_t* cookies = xcb_ewmh_init_atoms(m_connection, ewmh);
        xcb_ewmh_init_atoms_replies(ewmh, cookies, nullptr);
    }

    //Obtener la pantalla en la que dibujar
    const xcb_setup_t* setup = xcb_get_setup(m_connection);
    xcb_screen_t* screen = xcb_setup_roots_iterator(setup).data;

    //Generar una ventana
    m_window = xcb_generate_id(m_connection);

    //Eventos por defecto del motor.
    uint32_t defaultEvents =
        XCB_EVENT_MASK_STRUCTURE_NOTIFY |
        XCB_EVENT_MASK_KEY_PRESS|
        XCB_EVENT_MASK_KEY_RELEASE |
        XCB_EVENT_MASK_BUTTON_PRESS |
        XCB_EVENT_MASK_BUTTON_RELEASE |
        XCB_EVENT_MASK_POINTER_MOTION |
        XCB_EVENT_MASK_ENTER_WINDOW |
        XCB_EVENT_MASK_LEAVE_WINDOW |
        XCB_EVENT_MASK_FOCUS_CHANGE;

    uint32_t value_maks = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK; //Valores que cambiaremos de la ventana

    uint32_t value_list[2] = {
        screen->black_pixel,
        defaultEvents
    };

    //Crear finalmente la ventata
    xcb_create_window(m_connection, XCB_COPY_FROM_PARENT, m_window, screen->root, 0, 0,
        metadata.startWidth, metadata.startHeight, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
        value_maks, value_list);

    //Nombre de la ventana
    xcb_change_property(m_connection, XCB_PROP_MODE_REPLACE, m_window, ewmh->_NET_WM_NAME,
        ewmh->UTF8_STRING, 8, metadata.applicationName.size(), metadata.applicationName.c_str());

    //TODO:Implementar WM_Class
    std::string wm_class = metadata.applicationName + '\0' + metadata.applicationName; //Toma el nombre del juego como nombre
    xcb_change_property(m_connection, XCB_PROP_MODE_REPLACE, m_window, XCB_ATOM_WM_CLASS, XCB_ATOM_STRING,
        8, wm_class.size(), wm_class.c_str());
/*
    //Capturar ratón y teclado
    xcb_grab_pointer(m_connection, 0, m_window, XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
        XCB_EVENT_MASK_POINTER_MOTION, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, m_window, XCB_NONE,
        XCB_CURRENT_TIME);

    xcb_grab_keyboard(m_connection, 0, m_window, XCB_CURRENT_TIME, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
*///Esto es mejor hacerlo en el Input System, no aqui.

    m_NetWMState = ewmh->_NET_WM_STATE;
    m_NetWMStateFullScreen = ewmh->_NET_WM_STATE_FULLSCREEN;
    m_WMProtocols = ewmh->WM_PROTOCOLS;
    m_MotifWMHints = ewmh->_NET_WM_WINDOW_TYPE;

    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(m_connection, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(m_connection, cookie, nullptr);

    if (reply) {
        m_WMDeleteWindow = reply->atom;
        free(reply);
    }else {
        ILEN_ERROR_P(DEBUG_PREFIX, "Imposible obtener el atom WM_DELETE_WINDOW");
    }

    if (metadata.windowPropertiesFlags & enumerations::ILEN_WINDOW_PROPERTY_CLOSABLE) {
        xcb_change_property(m_connection, XCB_PROP_MODE_REPLACE, m_window, m_WMProtocols, XCB_ATOM_ATOM,
            32, 1, &m_WMDeleteWindow);
    }

    applyWindowProperties(metadata.windowPropertiesFlags);

    //Mostrar la ventana
    xcb_map_window(m_connection, m_window);

    //Enviar los commandos al servidor
    xcb_flush(m_connection);

    m_width = metadata.startWidth;
    m_height = metadata.startHeight;

    surfaceInfo._connection = m_connection;
    surfaceInfo._window = m_window;
}

WindowsNativeLinuxXCB::~WindowsNativeLinuxXCB() {
    if (m_window != XCB_NONE) {
        xcb_unmap_window(m_connection, m_window);
        xcb_destroy_window(m_connection, m_window);
    }

    if (m_connection) {
        xcb_disconnect(m_connection);
    }
}

void WindowsNativeLinuxXCB::applyWindowProperties(uint32_t properties){
    //Fullscreen
    if (properties & enumerations::ILEN_WINDOW_PROPERTY_FULLSCREEN) {
        setNetWMState(true, m_NetWMStateFullScreen);
    }else {
        setNetWMState(false, m_NetWMStateFullScreen);
    }

    //Borderless
    if (properties & enumerations::ILEN_WINDOW_PROPERTY_BORDERLESS) {
        xcb_atom_t windowType[3] = {
            ewmh->_NET_WM_WINDOW_TYPE_UTILITY,
            ewmh->_NET_WM_WINDOW_TYPE_DOCK,
            XCB_ATOM_NONE
        };

        xcb_change_property(m_connection, XCB_PROP_MODE_REPLACE, m_window,
            m_MotifWMHints, XCB_ATOM_ATOM, 32, 1, windowType);

        setNetWMState(true, ewmh->_NET_WM_STATE_SKIP_TASKBAR);
        setNetWMState(true, ewmh->_NET_WM_STATE_SKIP_PAGER);
        setNetWMState(true, ewmh->_NET_WM_STATE_ABOVE);
        setNetWMState(false, ewmh->_NET_WM_STATE_MAXIMIZED_VERT);
        setNetWMState(false, ewmh->_NET_WM_STATE_MAXIMIZED_HORZ);
    }else {
        xcb_atom_t normalType = ewmh->_NET_WM_WINDOW_TYPE_NORMAL;
        xcb_change_property(m_connection, XCB_PROP_MODE_REPLACE, m_window, m_MotifWMHints, XCB_ATOM_ATOM, 32
            ,1, &normalType);

        setNetWMState(false, ewmh->_NET_WM_STATE_SKIP_TASKBAR);
        setNetWMState(false, ewmh->_NET_WM_STATE_SKIP_PAGER);
        setNetWMState(false, ewmh->_NET_WM_STATE_ABOVE);
    }

    //Resizable
    xcb_size_hints_t hints{};
    uint32_t flags = 0;

    if (!(properties & enumerations::ILEN_WINDOW_PROPERTY_RESIZABLE)) {
        hints.min_width = hints.max_width = static_cast<int32_t>(m_width);
        hints.min_height = hints.max_height = static_cast<int32_t>(m_height);
        flags |= XCB_ICCCM_SIZE_HINT_P_MIN_SIZE | XCB_ICCCM_SIZE_HINT_P_MAX_SIZE;
    }

    if (flags) {
        hints.flags = flags;
        xcb_icccm_set_wm_normal_hints(m_connection, m_window, &hints);
    }

    xcb_flush(m_connection);
}

void WindowsNativeLinuxXCB::setNetWMState(bool add, xcb_atom_t state){
    if (state == XCB_ATOM_NONE) return;

    xcb_client_message_event_t event{};
    event.response_type = XCB_CLIENT_MESSAGE;
    event.format = 32;
    event.window = m_window;
    event.type = m_NetWMState;
    event.data.data32[0] = add ? 1 : 0; //Bool to Int
    event.data.data32[1] = state;

    const xcb_setup_t* setup = xcb_get_setup(m_connection);
    xcb_screen_t* screen = xcb_setup_roots_iterator(setup).data;

    xcb_send_event(m_connection, false, screen->root,
        XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
        reinterpret_cast<const char*>(&event));
}

void WindowsNativeLinuxXCB::pollEvents(){
    xcb_generic_event_t* event;

    while ((event = xcb_poll_for_event(m_connection))) {
        uint8_t type = event->response_type & ~0x80; //Quitamos el byte del send event

        switch (type) {
            case XCB_CLIENT_MESSAGE: {
                auto* cm = reinterpret_cast<xcb_client_message_event_t*>(event);
                if (cm->type == m_WMProtocols && cm->data.data32[0] == m_WMDeleteWindow) {
                    m_shouldClose = true;
                }
                break;
            }
            case XCB_CONFIGURE_NOTIFY: {
                auto* cfg = reinterpret_cast<xcb_configure_notify_event_t*>(event);
                if (cfg->width != static_cast<int>(m_width) || cfg->height != static_cast<int>(m_height)) {
                    m_width = cfg->width;
                    m_height = cfg->height;
                    m_resized.store(true, std::memory_order_relaxed);
                }
                break;
            }
            default:
                break;
        }

        free(event);
    }
}

bool WindowsNativeLinuxXCB::checkWindowResize(uint32_t &outWidth, uint32_t &outHeight){
    if (m_resized.load(std::memory_order_relaxed)) {
        outWidth = m_width;
        outHeight = m_height;
        m_resized.store(false, std::memory_order_relaxed);
        return true;
    }
    return false;
}

const void* WindowsNativeLinuxXCB::getNativeView() const{
    return static_cast<const void*>(&surfaceInfo);
}

void WindowsNativeLinuxXCB::updateGamepads() {

}

bool WindowsNativeLinuxXCB::shouldPause() {

}

#endif
