#include "luna/engine/input.h"

#include "core/presentation.h"

#include <doctest/doctest.h>

using luna::engine::InputMap;
using luna::engine::Intent;
using luna::engine::Intents;
using luna::platform::Event;
using luna::platform::EventType;
using luna::platform::GamepadAxis;
using luna::platform::GamepadButton;
using luna::platform::Key;
using odysseus::core::ScalingMode;
using odysseus::core::presentationArea;

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

TEST_CASE("US-231 Odd sizes pointer mapping") {
    InputMap input;
    Event moved;
    moved.type = EventType::MouseMoved;

    SUBCASE("Whole rejects the black bars and maps the displayed edge") {
        const auto area = presentationArea(2560, 1440, 960, 540, ScalingMode::Whole);
        input.setPointerArea(area, 960, 540);
        moved.x = 319.0F;
        moved.y = 180.0F;
        input.handle(moved);
        CHECK_FALSE(input.nextTick().pointer().inside());
        moved.x = 320.0F;
        moved.y = 180.0F;
        input.handle(moved);
        CHECK(input.nextTick().pointer().x == 0);
        moved.x = 2239.0F;
        moved.y = 1259.0F;
        input.handle(moved);
        const auto edge = input.nextTick().pointer();
        CHECK(edge.x == 959);
        CHECK(edge.y == 539);
        moved.x = 2240.0F;
        input.handle(moved);
        CHECK_FALSE(input.nextTick().pointer().inside());
    }

    SUBCASE("fractional Fill maps through the destination ratio") {
        const auto area = presentationArea(2560, 1440, 960, 540, ScalingMode::Fill);
        input.setPointerArea(area, 960, 540);
        moved.x = 1280.0F;
        moved.y = 720.0F;
        input.handle(moved);
        const auto centre = input.nextTick().pointer();
        CHECK(centre.x == 480);
        CHECK(centre.y == 270);
        moved.x = 2559.0F;
        moved.y = 1439.0F;
        input.handle(moved);
        const auto edge = input.nextTick().pointer();
        CHECK(edge.x == 959);
        CHECK(edge.y == 539);
    }
}

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
