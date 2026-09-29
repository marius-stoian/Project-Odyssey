#pragma once

#include "boundary.h"

#include "luna/platform/events.h"

#include <array>
#include <cstddef>

namespace luna::engine {

// What the player wants to do, independent of the device (ARC-03). Games read only these,
// so keyboard, gamepad and, later, touch all control the same actions.
enum class Intent { MoveUp, MoveDown, MoveLeft, MoveRight, Interact, OpenMenu, Count };

inline constexpr std::size_t kIntentCount = static_cast<std::size_t>(Intent::Count);

// The intents for one simulation tick.
class Intents {
public:
    bool held(Intent intent) const;    // active right now
    bool pressed(Intent intent) const; // became active since the previous tick

    // Movement direction from the held Move intents: -1, 0 or +1 on each axis
    // (x: left/right, y: up/down; screen y grows downwards).
    int moveX() const;
    int moveY() const;

    void set(Intent intent, bool held, bool pressed);

private:
    std::array<bool, kIntentCount> held_{};
    std::array<bool, kIntentCount> pressed_{};
};

// Turns Luna platform events into intents using bindings. Default bindings:
// W/Up, A/Left, S/Down, D/Right, E/Space/Enter = Interact, Escape = OpenMenu;
// gamepad left stick and D-pad move, South button = Interact, Start = OpenMenu.
class InputMap {
public:
    // Stick positions closer to the centre than this are ignored: worn sticks drift.
    static constexpr float kStickDeadZone = 0.3F;

    void handle(const platform::Event& event);

    // Holds or releases an intent as if a device did it. For automated tests and demos.
    void setScripted(Intent intent, bool held);

    // The intents for the next tick. "Pressed" is reported once, then cleared.
    Intents nextTick();

private:
    void setDigital(Intent intent, std::size_t source, bool down);

    // Each intent can be held by several sources at once (W and Up, stick and D-pad);
    // it is active while any of them is held.
    static constexpr std::size_t kSources = 5; // keyboard A, keyboard B, gamepad button, gamepad stick, script
    std::array<std::array<bool, kSources>, kIntentCount> sources_{};
    std::array<bool, kIntentCount> pressedSinceTick_{};
};

} // namespace luna::engine
