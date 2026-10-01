#include <ilen_public/API/input_api.h>

using namespace ilen::engine::api;
using namespace ilen::input;

ilen::input::InputBuffer* InputAPI::inputBuffer = nullptr;
ilen::input::Pointer* InputAPI::mouse = nullptr;

void InputAPI::Init(const ilen::EngineContext* context) {
    inputBuffer = &ilen::input::InputBuffer::Get();
    mouse = &inputBuffer->GetPointer(0);
}

bool InputAPI::IsKeyDown(ilen::input::Key key){
    return inputBuffer->CheckKeyState<ilen::input::KeyState::Down>(key);
}

bool InputAPI::IsKeyPressed(ilen::input::Key key){
    return inputBuffer->CheckKeyState<ilen::input::KeyState::Pressed>(key);
}

bool InputAPI::IsKeyReleased(ilen::input::Key key){
    return inputBuffer->CheckKeyState<ilen::input::KeyState::Released>(key);
}

bool InputAPI::IsPrimaryMouseButtonPressed() {
    if(!inputBuffer->CheckPointerType<PointerType::Mouse>()) return false;

    return mouse->isDown;
}

bool InputAPI::IsSecundaryMouseButtonPressed() {
    if(!inputBuffer->CheckPointerType<PointerType::Mouse>()) return false;
    return inputBuffer->GetPointerState().rightClick;
}

vec2 InputAPI::GetMousePosition() {
    if(!inputBuffer->CheckPointerType<PointerType::Mouse>()) return vec2(0.0f);
    return vec2(mouse->x, mouse->y);
}

int InputAPI::GetTotalTouches(){
    return static_cast<int>(inputBuffer->GetAmountOfActivePointers());
}

bool InputAPI::IsTouchDown(int index){
    if(!inputBuffer->CheckPointerType<PointerType::Touch>(index)) return false;
    return inputBuffer->CheckPointerStatus<KeyState::Down>(index);
}

bool InputAPI::IsTouchNew(int index){
    if(!inputBuffer->CheckPointerType<PointerType::Touch>(index)) return false;
    return inputBuffer->CheckPointerStatus<KeyState::Pressed>(index);
}

bool InputAPI::IsTouchLeave(int index){
    if(!inputBuffer->CheckPointerType<PointerType::Touch>(index)) return false;
    return inputBuffer->CheckPointerStatus<KeyState::Released>(index);
}

vec2 InputAPI::GetTouchPosition(int index){
    if(!inputBuffer->CheckPointerType<PointerType::Touch>(index)) return vec2(0.0f);
    return inputBuffer->GetPointerPos(index);
}

//Gamepad
bool InputAPI::IsButtonDown(GamepadButton button, int index) {
    return inputBuffer->CheckGamepadButton<KeyState::Down>(index, button);
}

bool InputAPI::IsButtonPressed(GamepadButton button, int index) {
    return inputBuffer->CheckGamepadButton<KeyState::Pressed>(index, button);
}

bool InputAPI::IsButtonReleased(GamepadButton button, int index) {
    return inputBuffer->CheckGamepadButton<KeyState::Released>(index, button);
}

vec2 InputAPI::GetGamepadAxis(ilen::input::GamepadAxis axis, int index) {
    return inputBuffer->GetGamepadAxis(axis, index);
}

float InputAPI::GetGamepadValue(ilen::input::GamepadSpecialValues value, int index) {
    return inputBuffer->GetGamepadValue(value, index);
}