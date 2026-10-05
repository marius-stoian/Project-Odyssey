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
constexpr std::size_t kScript = 4;

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
    case Key::LShift:
    case Key::RShift: return KeyBinding{Intent::SwitchWeapon, kKeyboardA};
    case Key::Tab: return KeyBinding{Intent::SwitchWeapon, kKeyboardB};
    case Key::F1: return KeyBinding{Intent::ModeGame, kKeyboardA};
    case Key::F2: return KeyBinding{Intent::ModeEditor, kKeyboardA};
    case Key::F12: return KeyBinding{Intent::DevTools, kKeyboardA};
    case Key::F3: return KeyBinding{Intent::Overlay, kKeyboardA};
    case Key::F5: return KeyBinding{Intent::Reload, kKeyboardA};
    case Key::Equals: return KeyBinding{Intent::ZoomIn, kKeyboardA};
    case Key::KpPlus: return KeyBinding{Intent::ZoomIn, kKeyboardB};
    case Key::Minus: return KeyBinding{Intent::ZoomOut, kKeyboardA};
    case Key::KpMinus: return KeyBinding{Intent::ZoomOut, kKeyboardB};
    case Key::Delete: return KeyBinding{Intent::Delete, kKeyboardA};
    case Key::Backspace: return KeyBinding{Intent::Erase, kKeyboardA};
    case Key::G: return KeyBinding{Intent::ToggleGrid, kKeyboardA};
    case Key::C: return KeyBinding{Intent::Confront, kKeyboardA};
    case Key::X: return KeyBinding{Intent::Actions, kKeyboardA};
    case Key::B: return KeyBinding{Intent::Build, kKeyboardA};
    case Key::J: return KeyBinding{Intent::Journal, kKeyboardA};
    case Key::F10: return KeyBinding{Intent::QuestDebug, kKeyboardA};
    case Key::P: return KeyBinding{Intent::PlayHere, kKeyboardA};
    case Key::R: return KeyBinding{Intent::Rotate, kKeyboardA};
    case Key::Num1: return KeyBinding{Intent::Slot1, kKeyboardA};
    case Key::Num2: return KeyBinding{Intent::Slot2, kKeyboardA};
    case Key::Num3: return KeyBinding{Intent::Slot3, kKeyboardA};
    case Key::Num4: return KeyBinding{Intent::Slot4, kKeyboardA};
    case Key::Num5: return KeyBinding{Intent::Slot5, kKeyboardA};
    case Key::Num6: return KeyBinding{Intent::Slot6, kKeyboardA};
    case Key::Num7: return KeyBinding{Intent::Slot7, kKeyboardA};
    case Key::Num8: return KeyBinding{Intent::Slot8, kKeyboardA};
    case Key::Num9: return KeyBinding{Intent::Slot9, kKeyboardA};
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
    case EventType::KeyUp: {
        if (event.repeat) {
            return; // holding a key repeats KeyDown; the intent is already held
        }
        const bool down = event.type == EventType::KeyDown;
        if (event.key == Key::LCtrl || event.key == Key::RCtrl) {
            ctrl_[event.key == Key::LCtrl ? 0 : 1] = down;
            return;
        }
        // Ctrl chords: Ctrl+Z, Ctrl+Y, Ctrl+S. Ctrl+S saves and does not also walk down. A key
        // let go always ends its chord, even if Ctrl was let go first.
        const std::optional<Intent> chord = event.key == Key::Z ? std::optional(Intent::Undo)
                                            : event.key == Key::Y ? std::optional(Intent::Redo)
                                            : event.key == Key::S ? std::optional(Intent::Save)
                                                                  : std::nullopt;
        if (chord && down && (ctrl_[0] || ctrl_[1])) {
            setDigital(*chord, kKeyboardA, true);
            return;
        }
        if (chord && !down) {
            setDigital(*chord, kKeyboardA, false);
        }
        if (event.key == Key::Enter) {
            setDigital(Intent::Confirm, kKeyboardA, down); // Enter also confirms a text field (E does not)
        }
        if (const auto binding = keyBinding(event.key)) {
            setDigital(binding->intent, binding->source, down);
        }
        return;
    }
    case EventType::MouseMoved:
        movePointer(event.x, event.y);
        return;
    case EventType::MouseButtonDown:
    case EventType::MouseButtonUp:
        movePointer(event.x, event.y);
        if (event.mouseButton != platform::MouseButton::Unknown) {
            const auto button = event.mouseButton == platform::MouseButton::Left    ? PointerButton::Left
                                : event.mouseButton == platform::MouseButton::Right ? PointerButton::Right
                                                                                    : PointerButton::Middle;
            setButton(static_cast<std::size_t>(button), event.type == EventType::MouseButtonDown);
        }
        return;
    case EventType::MouseWheel:
        pointer_.wheel += event.wheel > 0.0F ? 1 : (event.wheel < 0.0F ? -1 : 0);
        return;
    case EventType::TextInput:
        text_ += event.text;
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

void InputMap::setScripted(Intent intent, bool held) {
    setDigital(intent, kScript, held);
}

void InputMap::setPointerArea(const odysseus::core::Rect& area, int scale) {
    area_ = area;
    scale_ = std::max(1, scale);
    virtualWidth_ = area.width / scale_;
    virtualHeight_ = area.height / scale_;
}

void InputMap::setPointerArea(const odysseus::core::Rect& area, int virtualWidth, int virtualHeight) {
    area_ = area;
    virtualWidth_ = virtualWidth;
    virtualHeight_ = virtualHeight;
}

void InputMap::movePointer(float windowX, float windowY) {
    const int x = static_cast<int>(windowX);
    const int y = static_cast<int>(windowY);
    const bool inside = x >= area_.x && y >= area_.y && x < area_.x + area_.width && y < area_.y + area_.height;
    pointer_.x = inside && area_.width > 0 ? static_cast<int>(static_cast<long long>(x - area_.x) * virtualWidth_ / area_.width) : -1;
    pointer_.y = inside && area_.height > 0 ? static_cast<int>(static_cast<long long>(y - area_.y) * virtualHeight_ / area_.height) : -1;
}

void InputMap::setButton(std::size_t button, bool down) {
    if (down && !pointer_.held[button]) {
        pointer_.pressed[button] = true;
    }
    if (!down && pointer_.held[button]) {
        pointer_.released[button] = true;
    }
    pointer_.held[button] = down;
}

void InputMap::setScriptedPointer(int x, int y, PointerButton button, bool held) {
    pointer_.x = x;
    pointer_.y = y;
    setButton(static_cast<std::size_t>(button), held);
}

void InputMap::typeScripted(const std::string& text) {
    text_ += text;
}

Intents InputMap::nextTick() {
    Intents intents;
    for (std::size_t i = 0; i < kIntentCount; ++i) {
        const bool held = std::ranges::any_of(sources_[i], [](bool on) { return on; });
        intents.set(static_cast<Intent>(i), held, pressedSinceTick_[i]);
    }
    pressedSinceTick_.fill(false);
    // The left mouse button is also the Attack intent (US-139); a scripted Attack counts too.
    intents.set(Intent::Attack, intents.held(Intent::Attack) || pointer_.isHeld(PointerButton::Left), intents.pressed(Intent::Attack) || pointer_.wasPressed(PointerButton::Left));
    intents.set(Intent::Inspect, intents.held(Intent::Inspect) || pointer_.isHeld(PointerButton::Right), intents.pressed(Intent::Inspect) || pointer_.wasPressed(PointerButton::Right));
    intents.setPointer(pointer_);
    intents.setText(std::move(text_));
    text_.clear();
    pointer_.pressed.fill(false);
    pointer_.released.fill(false);
    pointer_.wheel = 0;
    return intents;
}

} // namespace luna::engine
