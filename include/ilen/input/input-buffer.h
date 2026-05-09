#pragma once

#include <ilen/input/input-keys.h>
#include <ilen/input/input-pointer.h>
#include <bitset>

namespace ilen::input {
    enum class KeyState {
        Pressed,
        Down,
        Released,
       // Free //No esta apretada ni este ni otro frame
    };

    //Clase que almacenara todas las entradas del motor en un frame, actuando como un buffer
    class InputBuffer {
        public: 
            //Este objeto seguira la estructura de un Singleton (se precisa un único buffer)
            static InputBuffer& Get() {
                static InputBuffer instance;
                return instance;
            }

            ///KEYBOARD///

            //Comandos para teclados
            void SetKeyState(Key key, bool pressed){
                if(key != Key::Unknown)
                    m_keyStates[static_cast<size_t>(key)] = pressed;
            }

            //Obtiene un bool de acuerdo al estado actual de la tecla.
            template<KeyState state>
            bool CheckKeyState(Key key) const {
                if constexpr (state == KeyState::Down) {
                    return m_keyStates[static_cast<size_t>(key)];
                } else if constexpr (state == KeyState::Pressed){
                    size_t k = static_cast<size_t>(key);
                    return m_keyStates[k] && !m_previousKeyStates[k];
                } else if constexpr (state == KeyState::Released){
                    size_t k = static_cast<size_t>(key);
                    return !m_keyStates[k] && m_previousKeyStates[k];
                }
            }

            ///POINTERS///
            Pointer& GetPointer(uint32_t index){
                return m_pointerState.pointers[index];
            }

            PointerState& GetPointerState() {
                return m_pointerState;
            }

            //Limpieza de datos al iniciar un frame
            void NewFrame() {
                m_pointerState.scrollWheel = 0.0f;
                m_previousKeyStates = m_keyStates;
            }

        private:
            std::bitset<static_cast<size_t>(Key::MAX_KEYS)> m_keyStates;
            std::bitset<static_cast<size_t>(Key::MAX_KEYS)> m_previousKeyStates;
            PointerState m_pointerState;
            InputBuffer() = default;
    };
}