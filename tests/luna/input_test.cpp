#include "luna/engine/input.h"

#include <doctest/doctest.h>

using luna::engine::InputMap;
using luna::engine::Intent;
using luna::engine::Intents;
using luna::platform::Event;
using luna::platform::EventType;
using luna::platform::GamepadAxis;
using luna::platform::GamepadButton;
using luna::platform::Key;

namespace {

Event key(EventType type, Key which, bool repeat = false) {
    Event event;
    event.type = type;
    event.key = which;
    event.repeat = repeat;
    return event;
}

Event stick(GamepadAxis axis, float value) {
    Event event;
    event.type = EventType::GamepadAxisMoved;
    event.axis = axis;
    event.axisValue = value;
    return event;
}

Event button(EventType type, GamepadButton which) {
    Event event;
    event.type = type;
    event.button = which;
    return event;
}

} // namespace

TEST_CASE("US-021 Default bindings") {
    InputMap input;
    input.handle(key(EventType::KeyDown, Key::W));
    Intents intents = input.nextTick();
    CHECK(intents.held(Intent::MoveUp));
    CHECK(intents.pressed(Intent::MoveUp));
    CHECK(intents.moveY() == -1);
    CHECK_FALSE(intents.held(Intent::MoveDown));

    SUBCASE("pressed is reported once, held stays while the key is down") {
        input.handle(key(EventType::KeyDown, Key::W, true)); // key repeat
        intents = input.nextTick();
        CHECK(intents.held(Intent::MoveUp));
        CHECK_FALSE(intents.pressed(Intent::MoveUp));
    }
    SUBCASE("W and the Up arrow drive the same intent") {
        input.handle(key(EventType::KeyDown, Key::Up));
        input.handle(key(EventType::KeyUp, Key::W));
        CHECK(input.nextTick().held(Intent::MoveUp)); // Up is still down
        input.handle(key(EventType::KeyUp, Key::Up));
        CHECK_FALSE(input.nextTick().held(Intent::MoveUp));
    }
    SUBCASE("the other default keys") {
        input.handle(key(EventType::KeyDown, Key::E));
        input.handle(key(EventType::KeyDown, Key::Escape));
        input.handle(key(EventType::KeyDown, Key::D));
        intents = input.nextTick();
        CHECK(intents.pressed(Intent::Interact));
        CHECK(intents.pressed(Intent::OpenMenu));
        CHECK(intents.moveX() == 1);
    }
}

TEST_CASE("US-021 Gamepad") {
    InputMap input;
    SUBCASE("left stick up produces the same Move Up intent as W") {
        input.handle(stick(GamepadAxis::LeftY, -1.0F));
        const Intents intents = input.nextTick();
        CHECK(intents.held(Intent::MoveUp));
        CHECK(intents.pressed(Intent::MoveUp));
        CHECK(intents.moveY() == -1);
    }
    SUBCASE("a stick resting near the centre does nothing (dead zone)") {
        input.handle(stick(GamepadAxis::LeftY, -0.2F));
        input.handle(stick(GamepadAxis::LeftX, 0.25F));
        const Intents intents = input.nextTick();
        CHECK(intents.moveX() == 0);
        CHECK(intents.moveY() == 0);
    }
    SUBCASE("D-pad and buttons") {
        input.handle(button(EventType::GamepadButtonDown, GamepadButton::DpadRight));
        input.handle(button(EventType::GamepadButtonDown, GamepadButton::South));
        input.handle(button(EventType::GamepadButtonDown, GamepadButton::Start));
        const Intents intents = input.nextTick();
        CHECK(intents.moveX() == 1);
        CHECK(intents.pressed(Intent::Interact));
        CHECK(intents.pressed(Intent::OpenMenu));
    }
    SUBCASE("unplugging the gamepad releases its intents") {
        input.handle(stick(GamepadAxis::LeftX, 1.0F));
        Event removed;
        removed.type = EventType::GamepadRemoved;
        input.handle(removed);
        CHECK(input.nextTick().moveX() == 0);
    }
}
