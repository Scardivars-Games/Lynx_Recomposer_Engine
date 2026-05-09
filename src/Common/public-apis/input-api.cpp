#include <ilen-public/API/input-api.h>

using namespace ilen::engine::api;

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
    return mouse->isDown;
}

bool InputAPI::IsSecundaryMouseButtonPressed() {
    return inputBuffer->GetPointerState().rightClick;
}

vec2 InputAPI::GetMousePosition() {
    return vec2(mouse->x, mouse->y);
}