// Platform-level tests: made-up SDL events go through Luna's translation, so the first
// half of "press W -> Move Up" and "stick up -> Move Up" is proven without real devices.
#include "luna/platform/sdl_events.h"

#include <doctest/doctest.h>

using luna::platform::EventType;
using luna::platform::GamepadAxis;
using luna::platform::GamepadButton;
using luna::platform::Key;

TEST_CASE("US-021 SDL events become Luna events") {
    SUBCASE("pressing W") {
        SDL_Event sdl{};
        sdl.type = SDL_EVENT_KEY_DOWN;
        sdl.key.scancode = SDL_SCANCODE_W;
        const auto event = luna::platform::translateEvent(sdl);
        REQUIRE(event.has_value());
        CHECK(event->type == EventType::KeyDown);
        CHECK(event->key == Key::W);
    }
    SUBCASE("pushing the left stick fully up") {
        SDL_Event sdl{};
        sdl.type = SDL_EVENT_GAMEPAD_AXIS_MOTION;
        sdl.gaxis.axis = SDL_GAMEPAD_AXIS_LEFTY;
        sdl.gaxis.value = -32768;
        const auto event = luna::platform::translateEvent(sdl);
        REQUIRE(event.has_value());
        CHECK(event->type == EventType::GamepadAxisMoved);
        CHECK(event->axis == GamepadAxis::LeftY);
        CHECK(event->axisValue == doctest::Approx(-1.0F));
    }
    SUBCASE("the South face button") {
        SDL_Event sdl{};
        sdl.type = SDL_EVENT_GAMEPAD_BUTTON_DOWN;
        sdl.gbutton.button = SDL_GAMEPAD_BUTTON_SOUTH;
        const auto event = luna::platform::translateEvent(sdl);
        REQUIRE(event.has_value());
        CHECK(event->button == GamepadButton::South);
    }
    SUBCASE("the close button") {
        SDL_Event sdl{};
        sdl.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
        const auto event = luna::platform::translateEvent(sdl);
        REQUIRE(event.has_value());
        CHECK(event->type == EventType::Quit);
    }
    SUBCASE("events Luna does not use are dropped") {
        SDL_Event sdl{};
        sdl.type = SDL_EVENT_MOUSE_MOTION;
        CHECK_FALSE(luna::platform::translateEvent(sdl).has_value());
    }
}
