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
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        out.type = EventType::WindowResized;
        out.width = event.window.data1;
        out.height = event.window.data2;
        return out;
    default:
        return std::nullopt;
    }
}

} // namespace luna::platform
