#pragma once

#include <lynx/input/input_keys.h>
#include <lynx/input/input_pointer.h>
#include <lynx/input/input_gamepad.h>
#include <lynx/maths/vector.h>
#include <bitset>
#include <unordered_map>

using namespace lynx::maths;

namespace lynx::input {
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

            //Retorna un indice de slot para el puntero. Util para multitouch en dispositivos móviles.
            int GetFreePointerSlot() {
                for(int i = 0; i < PointerState::MAX_POINTERS; i++){
                    if(!GetPointer(i).isDown) return i;
                }
                return -1; //No hay un espacio
            }

            template<KeyState state>
            bool CheckPointerStatus(int pointerIndex) const {
                if constexpr(state == KeyState::Down) {
                    return m_pointerState.pointers[pointerIndex].isDown;
                } else if constexpr (state == KeyState::Pressed) {
                    Pointer p = m_pointerState.pointers[pointerIndex];
                    return p.isDown && !p.previousIsDown;
                } else if constexpr (state == KeyState::Released) {
                    Pointer p = m_pointerState.pointers[pointerIndex];
                    return !p.isDown && p.previousIsDown;
                }
            }

            size_t GetAmountOfActivePointers() {
                return m_touchMap.size();
            }

            vector::vec2 GetPointerPos(int index){
                Pointer p = m_pointerState.pointers[index];
                return vector::vec2(p.x, p.y);
            }

            int GetPointerSlot(uintptr_t pointer){
                return m_touchMap[pointer];
            }

            bool PointerSlotIsActive(uintptr_t pointer) {
                return m_touchMap.count(pointer);
            }

            void ModifyPointerSlot(uintptr_t pointer, int index, bool newState){
                if (newState) {
                    m_touchMap[pointer] = index;
                }else {
                    
                    m_touchMap.erase(pointer);
                }
            }

            //Gamepads
            GamepadState& GetGamepad(uint32_t index) {
                return m_gamepads[index];
            }

            bool IsGamepadConnected(uint32_t index) const {
                return m_gamepads.count(index) && m_gamepads.at(index).connected;
            }

            template<KeyState state>
            bool CheckGamepadButton(uint32_t gamepadIndex, GamepadButton button) const {
                if(!m_gamepads.count(gamepadIndex)) return false;

                const auto& gp = m_gamepads.at(gamepadIndex);
                size_t b = static_cast<size_t>(button);

                if constexpr (state == KeyState::Down) {
                    return gp.buttons[b];
                } else if constexpr (state == KeyState::Pressed) {
                    return gp.buttons[b] && !gp.previousButtons[b];
                } else if constexpr (state == KeyState::Released) {
                    return !gp.buttons[b] && gp.previousButtons[b];
                }

                return false;
            }

            vector::vec2 GetGamepadAxis(GamepadAxis axis, uint32_t gamepadIndex) {
                auto& gp = m_gamepads.at(gamepadIndex);
                return gp.GetGamepadAxis(axis);
            }   

            float GetGamepadValue(GamepadSpecialValues value, uint32_t gamepadIndex) {
                auto& gp = m_gamepads.at(gamepadIndex);
                return gp.GetGamepadValue(value);
            }

            //Limpieza de datos al iniciar un frame
            void NewFrame() {
                m_pointerState.scrollWheel = 0.0f;
                m_previousKeyStates = m_keyStates;

                UpatedPreviousPointerStatues();
                UpdatePreviousGamepadStatus();
            }

            void ForceResetData() {
                m_keyStates.reset();
                m_previousKeyStates.reset();
                m_touchMap.clear();

                for(Pointer& p: m_pointerState.pointers){
                    p.isDown = false;
                    p.pressure = 0.0f;
                }

                for(auto& [id, gp]: m_gamepads){
                    gp.buttons.reset();
                    gp.previousButtons.reset();
                    gp.connected = false;
                }
                
                m_pointerState.middleClick = false;
                m_pointerState.rightClick = false;
            }

            inline void UpatedPreviousPointerStatues(){
                for(Pointer& pointer: m_pointerState.pointers) {
                    pointer.previousIsDown = pointer.isDown;
                }
            }

            inline void UpdatePreviousGamepadStatus() {
                for(auto& [id, gp]: m_gamepads) {
                    gp.previousButtons = gp.buttons;
                }
            }

            template<PointerType type>
            inline bool CheckPointerType(int index = 0) {
                if constexpr (type == PointerType::Mouse) {
                    return m_pointerState.pointers[0].type == PointerType::Mouse;
                } else if constexpr (type == PointerType::Touch) {
                    return m_pointerState.pointers[index].type == PointerType::Touch;
                }
            } 

        private:
            std::bitset<static_cast<size_t>(Key::MAX_KEYS)> m_keyStates;
            std::bitset<static_cast<size_t>(Key::MAX_KEYS)> m_previousKeyStates;
            std::unordered_map<uintptr_t, uint32_t> m_touchMap;
            std::unordered_map<uint32_t, GamepadState> m_gamepads;

            PointerState m_pointerState;
            InputBuffer() = default;
    };
}
