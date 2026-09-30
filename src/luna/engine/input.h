#pragma once

#include "boundary.h"

#include "luna/platform/events.h"

#include "core/geometry.h"

#include <array>
#include <cstddef>
#include <string>

namespace luna::engine {

// What the player wants to do, independent of the device (ARC-03). Games read only these,
// so keyboard, gamepad and, later, touch all control the same actions. New intents only join
// at the end (before Count).
enum class Intent {
    MoveUp, MoveDown, MoveLeft, MoveRight, Interact, OpenMenu, SwitchWeapon,
    // Tools (M2c, US-121): F1, F2, Ctrl+Z, Ctrl+Y, Ctrl+S, Delete, G, R, Backspace, Enter.
    ModeGame, ModeEditor, Undo, Redo, Save, Delete, ToggleGrid, Rotate, Erase, Confirm,
    // Hotbar (US-134): keys 1 to 9 choose a slot. Slot2 is Slot1 + 1, and so on.
    Slot1, Slot2, Slot3, Slot4, Slot5, Slot6, Slot7, Slot8, Slot9,
    // Attack (US-139): the left mouse button, or scripted; the weapon goes toward the pointer.
    Attack,
    Count
};

inline constexpr std::size_t kIntentCount = static_cast<std::size_t>(Intent::Count);

enum class PointerButton { Left, Right, Middle, Count };

inline constexpr std::size_t kPointerButtonCount = static_cast<std::size_t>(PointerButton::Count);

// The mouse (or, later, a finger), in the game's virtual pixels (480x270): not a device, but
// where the player points and what they press there (US-121).
struct Pointer {
    int x = -1; // -1: outside the picture (on the black bars, or not over the window)
    int y = -1;
    std::array<bool, kPointerButtonCount> held{};
    std::array<bool, kPointerButtonCount> pressed{};  // went down since the previous tick
    std::array<bool, kPointerButtonCount> released{}; // went up since the previous tick
    int wheel = 0;                                    // + = scrolled up

    bool inside() const { return x >= 0 && y >= 0; }
    bool isHeld(PointerButton button) const { return held[static_cast<std::size_t>(button)]; }
    bool wasPressed(PointerButton button) const { return pressed[static_cast<std::size_t>(button)]; }
    bool wasReleased(PointerButton button) const { return released[static_cast<std::size_t>(button)]; }
};

// The intents for one simulation tick.
class Intents {
public:
    bool held(Intent intent) const;    // active right now
    bool pressed(Intent intent) const; // became active since the previous tick

    // Movement direction from the held Move intents: -1, 0 or +1 on each axis
    // (x: left/right, y: up/down; screen y grows downwards).
    int moveX() const;
    int moveY() const;

    const Pointer& pointer() const { return pointer_; }
    // Printable characters typed since the previous tick (for text fields).
    const std::string& text() const { return text_; }

    void set(Intent intent, bool held, bool pressed);
    void setPointer(const Pointer& pointer) { pointer_ = pointer; }
    void setText(std::string text) { text_ = std::move(text); }

private:
    std::array<bool, kIntentCount> held_{};
    std::array<bool, kIntentCount> pressed_{};
    Pointer pointer_;
    std::string text_;
};

// Turns Luna platform events into intents using bindings. Default bindings:
// W/Up, A/Left, S/Down, D/Right, E/Space/Enter = Interact, Escape = OpenMenu, Shift/Tab =
// SwitchWeapon; F1 = ModeGame, F2 = ModeEditor, Ctrl+Z = Undo, Ctrl+Y = Redo, Ctrl+S = Save,
// Delete, G = ToggleGrid, R = Rotate, Backspace = Erase, 1-9 = Slot1-Slot9, left mouse button = Attack; gamepad left stick and D-pad move,
// South button = Interact, Start = OpenMenu. The mouse becomes the Pointer.
class InputMap {
public:
    // Stick positions closer to the centre than this are ignored: worn sticks drift.
    static constexpr float kStickDeadZone = 0.3F;

    void handle(const platform::Event& event);

    // Where the game's picture is in the window: `area` in window pixels, drawn `scale` times
    // larger than the virtual screen. Mouse positions are turned into virtual pixels with it.
    void setPointerArea(const odysseus::core::Rect& area, int scale);

    // Holds or releases an intent as if a device did it. For automated tests and demos.
    void setScripted(Intent intent, bool held);
    // Moves the pointer to (x, y) in virtual pixels and holds (or not) a button, as if the mouse
    // did it. For automated tests and demos.
    void setScriptedPointer(int x, int y, PointerButton button, bool held);
    // Types text as if on the keyboard.
    void typeScripted(const std::string& text);

    // The intents for the next tick. "Pressed" is reported once, then cleared.
    Intents nextTick();

private:
    void setDigital(Intent intent, std::size_t source, bool down);
    void setButton(std::size_t button, bool down);
    void movePointer(float windowX, float windowY);

    // Each intent can be held by several sources at once (W and Up, stick and D-pad);
    // it is active while any of them is held.
    static constexpr std::size_t kSources = 5; // keyboard A, keyboard B, gamepad button, gamepad stick, script
    std::array<std::array<bool, kSources>, kIntentCount> sources_{};
    std::array<bool, kIntentCount> pressedSinceTick_{};
    bool ctrl_[2] = {false, false};
    odysseus::core::Rect area_{0, 0, 0, 0};
    int scale_ = 1;
    Pointer pointer_;           // held and position now; pressed/released since the last tick
    std::string text_;
};

} // namespace luna::engine
