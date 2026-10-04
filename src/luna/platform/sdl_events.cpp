#include "luna/platform/sdl_events.h"

namespace luna::platform {

namespace {

Key toKey(SDL_Scancode scancode) {
    switch (scancode) {
    case SDL_SCANCODE_W: return Key::W;
    case SDL_SCANCODE_A: return Key::A;
    case SDL_SCANCODE_S: return Key::S;
    case SDL_SCANCODE_D: return Key::D;
    case SDL_SCANCODE_E: return Key::E;
    case SDL_SCANCODE_UP: return Key::Up;
    case SDL_SCANCODE_DOWN: return Key::Down;
    case SDL_SCANCODE_LEFT: return Key::Left;
    case SDL_SCANCODE_RIGHT: return Key::Right;
    case SDL_SCANCODE_ESCAPE: return Key::Escape;
    case SDL_SCANCODE_SPACE: return Key::Space;
    case SDL_SCANCODE_RETURN: return Key::Enter;
    case SDL_SCANCODE_LSHIFT: return Key::LShift;
    case SDL_SCANCODE_RSHIFT: return Key::RShift;
    case SDL_SCANCODE_TAB: return Key::Tab;
    case SDL_SCANCODE_F1: return Key::F1;
    case SDL_SCANCODE_F2: return Key::F2;
    case SDL_SCANCODE_F12: return Key::F12;
    case SDL_SCANCODE_F3: return Key::F3;
    case SDL_SCANCODE_F5: return Key::F5;
    case SDL_SCANCODE_EQUALS: return Key::Equals;
    case SDL_SCANCODE_MINUS: return Key::Minus;
    case SDL_SCANCODE_KP_PLUS: return Key::KpPlus;
    case SDL_SCANCODE_KP_MINUS: return Key::KpMinus;
    case SDL_SCANCODE_DELETE: return Key::Delete;
    case SDL_SCANCODE_BACKSPACE: return Key::Backspace;
    case SDL_SCANCODE_LCTRL: return Key::LCtrl;
    case SDL_SCANCODE_RCTRL: return Key::RCtrl;
    case SDL_SCANCODE_Z: return Key::Z;
    case SDL_SCANCODE_Y: return Key::Y;
    case SDL_SCANCODE_G: return Key::G;
    case SDL_SCANCODE_C: return Key::C;
    case SDL_SCANCODE_X: return Key::X;
    case SDL_SCANCODE_R: return Key::R;
    case SDL_SCANCODE_1: return Key::Num1;
    case SDL_SCANCODE_2: return Key::Num2;
    case SDL_SCANCODE_3: return Key::Num3;
    case SDL_SCANCODE_4: return Key::Num4;
    case SDL_SCANCODE_5: return Key::Num5;
    case SDL_SCANCODE_6: return Key::Num6;
    case SDL_SCANCODE_7: return Key::Num7;
    case SDL_SCANCODE_8: return Key::Num8;
    case SDL_SCANCODE_9: return Key::Num9;
    default: return Key::Unknown;
    }
}

GamepadButton toButton(Uint8 button) {
    switch (static_cast<SDL_GamepadButton>(button)) {
    case SDL_GAMEPAD_BUTTON_SOUTH: return GamepadButton::South;
    case SDL_GAMEPAD_BUTTON_EAST: return GamepadButton::East;
    case SDL_GAMEPAD_BUTTON_WEST: return GamepadButton::West;
    case SDL_GAMEPAD_BUTTON_NORTH: return GamepadButton::North;
    case SDL_GAMEPAD_BUTTON_BACK: return GamepadButton::Back;
    case SDL_GAMEPAD_BUTTON_START: return GamepadButton::Start;
    case SDL_GAMEPAD_BUTTON_DPAD_UP: return GamepadButton::DpadUp;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return GamepadButton::DpadDown;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return GamepadButton::DpadLeft;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return GamepadButton::DpadRight;
    default: return GamepadButton::Unknown;
    }
}

GamepadAxis toAxis(Uint8 axis) {
    switch (static_cast<SDL_GamepadAxis>(axis)) {
    case SDL_GAMEPAD_AXIS_LEFTX: return GamepadAxis::LeftX;
    case SDL_GAMEPAD_AXIS_LEFTY: return GamepadAxis::LeftY;
    case SDL_GAMEPAD_AXIS_RIGHTX: return GamepadAxis::RightX;
    case SDL_GAMEPAD_AXIS_RIGHTY: return GamepadAxis::RightY;
    default: return GamepadAxis::Unknown;
    }
}

} // namespace

std::optional<Event> translateEvent(const SDL_Event& event) {
    Event out;
    switch (event.type) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        out.type = EventType::Quit;
        return out;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        out.type = event.type == SDL_EVENT_KEY_DOWN ? EventType::KeyDown : EventType::KeyUp;
        out.key = toKey(event.key.scancode);
        out.repeat = event.key.repeat;
        return out;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
        out.type = event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN ? EventType::GamepadButtonDown : EventType::GamepadButtonUp;
        out.button = toButton(event.gbutton.button);
        out.gamepad = static_cast<int>(event.gbutton.which);
        return out;
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        out.type = EventType::GamepadAxisMoved;
        out.axis = toAxis(event.gaxis.axis);
        // SDL's range is -32768..32767; Luna uses -1..+1 (up and left are negative).
        out.axisValue = event.gaxis.value < 0 ? static_cast<float>(event.gaxis.value) / 32768.0F
                                              : static_cast<float>(event.gaxis.value) / 32767.0F;
        out.gamepad = static_cast<int>(event.gaxis.which);
        return out;
    case SDL_EVENT_GAMEPAD_ADDED:
        out.type = EventType::GamepadAdded;
        out.gamepad = static_cast<int>(event.gdevice.which);
        return out;
    case SDL_EVENT_GAMEPAD_REMOVED:
        out.type = EventType::GamepadRemoved;
        out.gamepad = static_cast<int>(event.gdevice.which);
        return out;
    case SDL_EVENT_MOUSE_MOTION:
        out.type = EventType::MouseMoved;
        out.x = event.motion.x;
        out.y = event.motion.y;
        return out;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        out.type = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? EventType::MouseButtonDown : EventType::MouseButtonUp;
        out.mouseButton = event.button.button == SDL_BUTTON_LEFT    ? MouseButton::Left
                          : event.button.button == SDL_BUTTON_RIGHT  ? MouseButton::Right
                          : event.button.button == SDL_BUTTON_MIDDLE ? MouseButton::Middle
                                                                     : MouseButton::Unknown;
        out.x = event.button.x;
        out.y = event.button.y;
        return out;
    case SDL_EVENT_MOUSE_WHEEL:
        out.type = EventType::MouseWheel;
        // "Flipped" wheels (natural scrolling) report the other way round; Luna always means
        // + = scroll up.
        out.wheel = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -event.wheel.y : event.wheel.y;
        return out;
    case SDL_EVENT_TEXT_INPUT: {
        // Only printable ASCII: the bitmap font has nothing else.
        for (const char* c = event.text.text; c != nullptr && *c != '\0'; ++c) {
            if (*c >= 32 && *c <= 126) {
                out.text += *c;
            }
        }
        if (out.text.empty()) {
            return std::nullopt;
        }
        out.type = EventType::TextInput;
        return out;
    }
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
    case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
        out.type = EventType::WindowResized;
        out.width = event.window.data1;
        out.height = event.window.data2;
        return out;
    default:
        return std::nullopt;
    }
}

} // namespace luna::platform
