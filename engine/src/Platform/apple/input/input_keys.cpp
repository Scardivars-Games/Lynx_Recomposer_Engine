#include <lynx/input/input_keys.h>

lynx::input::Key lynx::input::TranslateNativeKey(uint16_t nativeKeyCode){
    using namespace lynx::input;
    switch (nativeKeyCode)
    {
        // Letras (macOS codes)
        case 0:  return Key::A;  case 11: return Key::B;  case 8:  return Key::C;
        case 2:  return Key::D;  case 14: return Key::E;  case 3:  return Key::F;
        case 5:  return Key::G;  case 4:  return Key::H;  case 34: return Key::I;
        case 38: return Key::J;  case 40: return Key::K;  case 37: return Key::L;
        case 46: return Key::M;  case 45: return Key::N;  case 31: return Key::O;
        case 35: return Key::P;  case 12: return Key::Q;  case 15: return Key::R;
        case 1:  return Key::S;  case 17: return Key::T;  case 32: return Key::U;
        case 9:  return Key::V;  case 13: return Key::W;  case 7:  return Key::X;
        case 16: return Key::Y;  case 6:  return Key::Z;

        // Números
        case 29: return Key::Num0; case 18: return Key::Num1; case 19: return Key::Num2;
        case 20: return Key::Num3; case 21: return Key::Num4; case 23: return Key::Num5;
        case 22: return Key::Num6; case 26: return Key::Num7; case 28: return Key::Num8;
        case 25: return Key::Num9;

        // Especiales
        case 49: return Key::Space;  case 36: return Key::Enter;
        case 48: return Key::Tab;    case 53: return Key::Escape;
        case 51: return Key::Delete;

        // Flechas
        case 126: return Key::Up;    case 125: return Key::Down;
        case 123: return Key::Left;  case 124: return Key::Right;

        // Modificadores
        case 56: return Key::LeftShift;   case 60: return Key::RightShift;
        case 55: return Key::LeftCommand; case 59: return Key::LeftControl;
        case 58: return Key::LeftOption;

        default: return Key::Unknown;
    }
}