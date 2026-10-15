#if LYNX_LINUX
#include <lynx/window/alert_box.h>
#include <lynx/core/lifetime.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

namespace lynx::window::box {
    bool userClickedCartel = false;

    void handle_ok_button(xcb_generic_event_t* event) {
        uint8_t response_type = event->response_type & ~0x80;
        if (response_type == XCB_CLIENT_MESSAGE) {
            auto* cm = (xcb_client_message_event_t*)event;
            if (cm->data.data32[0] == 1) { //1 es clickeado.
                userClickedCartel = true;
            }
        }
    }

    void ShowErrorDialogBox(const std::string& title, const std::string& message) {
        const std::string cmd =
                    "zenity --error --no-markup --width=500 --height=300 "
                    "--title=\"" + title + "\" "
                    "--text=\"" + message + "\" "
                    "2>/dev/null || "                     // si zenity falla → pasa al siguiente

                    "kdialog --title \"" + title + "\" --error \"" + message + "\" "
                    "2>/dev/null || "                     // si kdialog falla → pasa al siguiente

                    "notify-send -u critical \"" + title + "\" \"" + message + "\" "
                    "2>/dev/null";                        // notificación (no bloqueante, pero al menos se ve algo)

        // BLOQUEANTE: solo zenity y kdialog bloquean. notify-send no, pero es fallback
        std::system(cmd.c_str());
        //Todo el código a continuación serviría, si existiera un motor de renderizado que cree UI. Algo en desarrollo. De momento algo más básico.
        /*
        //Obtener la conexción
        auto connectionHandle = lynx::AppRuntime::getInstance().getEngineContext()->p_gameWindow->getWindow();
        auto* connection = static_cast<xcb_connection_t*>(connectionHandle);

        if (!connection) return;

        xcb_ewmh_connection_t ewmh;
        xcb_intern_atom_cookie_t* cookies = xcb_ewmh_init_atoms(connection, &ewmh);
        xcb_ewmh_init_atoms_replies(&ewmh, cookies, nullptr);

        xcb_screen_t* screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;

        //Dialog window!!
        xcb_window_t dialog = xcb_generate_id(connection);

        uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_OVERRIDE_REDIRECT;
        uint32_t values[3] = {
            screen->white_pixel,
            XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_EXPOSURE,
            1
        };

        int width = 500;
        int height = 200;
        int x = (screen->width_in_pixels - width) / 2;
        int y = (screen->height_in_pixels - height) / 2;

        xcb_create_window(connection, XCB_COPY_FROM_PARENT, dialog, screen->root, x, y,
            width, height, 4, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, values);

        xcb_atom_t type_dialog = ewmh._NET_WM_WINDOW_TYPE_DIALOG;
        xcb_change_property(connection, XCB_PROP_MODE_REPLACE, dialog, ewmh._NET_WM_WINDOW_TYPE, XCB_ATOM_ATOM,
            32, 1, &type_dialog);

        xcb_change_property(connection, XCB_PROP_MODE_REPLACE, dialog, ewmh._NET_WM_NAME, ewmh.UTF8_STRING, 8, title.size(), title.c_str());

        //Estados de la ventana: modal, always up, ignorar taskbar
        xcb_atom_t states[] = {
            ewmh._NET_WM_STATE_MODAL,
            ewmh._NET_WM_STATE_ABOVE,
            ewmh._NET_WM_STATE_SKIP_TASKBAR,
        };
        xcb_change_property(connection, XCB_PROP_MODE_REPLACE, dialog, ewmh._NET_WM_STATE,
            XCB_ATOM_ATOM, 32, 3, states);

        xcb_map_window(connection, dialog);
        xcb_flush(connection);

        userClickedCartel = false;

        while (!userClickedCartel) {
            xcb_generic_event_t* event;

            while ((event = xcb_poll_for_event(connection))) {
                handle_ok_button(event);
                free(event);
            }

            // Simulamos botón OK en la parte inferior (detectamos click aproximado)
            // En producción harías un botón real con XCB o cairo
            // Aquí solo un ejemplo simple: cualquier click izquierdo cierra
            // (para demo rápida)
            while ((event = xcb_wait_for_event(connection))) {
                uint8_t type = event->response_type & ~0x80;
                if (type == XCB_BUTTON_PRESS) {
                    userClickedCartel = true;
                }
                free(event);
            }
            usleep(10000);
        }

        xcb_destroy_window(connection, dialog);
        xcb_flush(connection);
        xcb_ewmh_connection_wipe(&ewmh);
        */
    }
}
#endif
