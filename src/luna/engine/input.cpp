#include "luna/engine/input.h"

#include <algorithm>
#include <optional>
#include <utility>

namespace luna::engine {

namespace {

using platform::EventType;
using platform::GamepadAxis;
using platform::GamepadButton;
using platform::Key;

std::size_t index(Intent intent) {
    return static_cast<std::size_t>(intent);
}

// Which intent a key drives, and through which of the two keyboard "slots".
struct KeyBinding {
    Intent intent;
    std::size_t source;
};

constexpr std::size_t kKeyboardA = 0;
constexpr std::size_t kKeyboardB = 1;
constexpr std::size_t kGamepadButton = 2;
constexpr std::size_t kGamepadStick = 3;

std::optional<KeyBinding> keyBinding(Key key) {
    switch (key) {
    case Key::W: return KeyBinding{Intent::MoveUp, kKeyboardA};
    case Key::Up: return KeyBinding{Intent::MoveUp, kKeyboardB};
    case Key::S: return KeyBinding{Intent::MoveDown, kKeyboardA};
    case Key::Down: return KeyBinding{Intent::MoveDown, kKeyboardB};
    case Key::A: return KeyBinding{Intent::MoveLeft, kKeyboardA};
    case Key::Left: return KeyBinding{Intent::MoveLeft, kKeyboardB};
    case Key::D: return KeyBinding{Intent::MoveRight, kKeyboardA};
    case Key::Right: return KeyBinding{Intent::MoveRight, kKeyboardB};
    case Key::E: return KeyBinding{Intent::Interact, kKeyboardA};
    case Key::Space:
    case Key::Enter: return KeyBinding{Intent::Interact, kKeyboardB};
    case Key::Escape: return KeyBinding{Intent::OpenMenu, kKeyboardA};
    default: return std::nullopt;
    }
}

std::optional<Intent> buttonBinding(GamepadButton button) {
    switch (button) {
    case GamepadButton::DpadUp: return Intent::MoveUp;
    case GamepadButton::DpadDown: return Intent::MoveDown;
    case GamepadButton::DpadLeft: return Intent::MoveLeft;
    case GamepadButton::DpadRight: return Intent::MoveRight;
    case GamepadButton::South: return Intent::Interact;
    case GamepadButton::Start: return Intent::OpenMenu;
    default: return std::nullopt;
    }
}

} // namespace

bool Intents::held(Intent intent) const {
    return held_[index(intent)];
}

bool Intents::pressed(Intent intent) const {
    return pressed_[index(intent)];
}

int Intents::moveX() const {
    return (held(Intent::MoveRight) ? 1 : 0) - (held(Intent::MoveLeft) ? 1 : 0);
}

int Intents::moveY() const {
    return (held(Intent::MoveDown) ? 1 : 0) - (held(Intent::MoveUp) ? 1 : 0);
}

void Intents::set(Intent intent, bool held, bool pressed) {
    held_[index(intent)] = held;
    pressed_[index(intent)] = pressed;
}

void InputMap::setDigital(Intent intent, std::size_t source, bool down) {
    auto& slots = sources_[index(intent)];
    const bool wasHeld = std::ranges::any_of(slots, [](bool on) { return on; });
    slots[source] = down;
    if (down && !wasHeld) {
        pressedSinceTick_[index(intent)] = true;
    }
}

void InputMap::handle(const platform::Event& event) {
    switch (event.type) {
    case EventType::KeyDown:
    case EventType::KeyUp:
        if (event.repeat) {
            return; // holding a key repeats KeyDown; the intent is already held
        }
        if (const auto binding = keyBinding(event.key)) {
            setDigital(binding->intent, binding->source, event.type == EventType::KeyDown);
        }
        return;
    case EventType::GamepadButtonDown:
    case EventType::GamepadButtonUp:
        if (const auto intent = buttonBinding(event.button)) {
            setDigital(*intent, kGamepadButton, event.type == EventType::GamepadButtonDown);
        }
        return;
    case EventType::GamepadAxisMoved:
        if (event.axis == GamepadAxis::LeftY) {
            setDigital(Intent::MoveUp, kGamepadStick, event.axisValue < -kStickDeadZone);
            setDigital(Intent::MoveDown, kGamepadStick, event.axisValue > kStickDeadZone);
        } else if (event.axis == GamepadAxis::LeftX) {
            setDigital(Intent::MoveLeft, kGamepadStick, event.axisValue < -kStickDeadZone);
            setDigital(Intent::MoveRight, kGamepadStick, event.axisValue > kStickDeadZone);
        }
        return;
    case EventType::GamepadRemoved:
        // A pulled cable must not leave the character walking forever.
        for (auto& slots : sources_) {
            slots[kGamepadButton] = false;
            slots[kGamepadStick] = false;
        }
        return;
    default:
        return;
    }
}

Intents InputMap::nextTick() {
    Intents intents;
    for (std::size_t i = 0; i < kIntentCount; ++i) {
        const bool held = std::ranges::any_of(sources_[i], [](bool on) { return on; });
        intents.set(static_cast<Intent>(i), held, pressedSinceTick_[i]);
    }
    pressedSinceTick_.fill(false);
    return intents;
}

} // namespace luna::engine
