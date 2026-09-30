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
        sdl.type = SDL_EVENT_FINGER_DOWN; // touch comes with mobile (later)
        CHECK_FALSE(luna::platform::translateEvent(sdl).has_value());
    }
}

TEST_CASE("US-121 SDL mouse and text become Luna events") {
    SUBCASE("mouse motion") {
        SDL_Event sdl{};
        sdl.type = SDL_EVENT_MOUSE_MOTION;
        sdl.motion.x = 12.5F;
        sdl.motion.y = 30.0F;
        const auto event = luna::platform::translateEvent(sdl);
        REQUIRE(event.has_value());
        CHECK(event->type == EventType::MouseMoved);
        CHECK(event->x == doctest::Approx(12.5));
        CHECK(event->y == doctest::Approx(30.0));
    }
    SUBCASE("mouse buttons") {
        SDL_Event sdl{};
        sdl.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        sdl.button.button = SDL_BUTTON_RIGHT;
        const auto event = luna::platform::translateEvent(sdl);
        REQUIRE(event.has_value());
        CHECK(event->type == EventType::MouseButtonDown);
        CHECK(event->mouseButton == luna::platform::MouseButton::Right);
    }
    SUBCASE("the wheel, whichever way the system scrolls") {
        SDL_Event sdl{};
        sdl.type = SDL_EVENT_MOUSE_WHEEL;
        sdl.wheel.y = 1.0F;
        sdl.wheel.direction = SDL_MOUSEWHEEL_FLIPPED;
        const auto event = luna::platform::translateEvent(sdl);
        REQUIRE(event.has_value());
        CHECK(event->wheel == doctest::Approx(-1.0));
    }
    SUBCASE("typed text keeps printable ASCII only") {
        SDL_Event sdl{};
        sdl.type = SDL_EVENT_TEXT_INPUT;
        sdl.text.text = "Ura\xC3\xA9!";
        const auto event = luna::platform::translateEvent(sdl);
        REQUIRE(event.has_value());
        CHECK(event->text == "Ura!");
    }
    SUBCASE("tool keys") {
        SDL_Event sdl{};
        sdl.type = SDL_EVENT_KEY_DOWN;
        sdl.key.scancode = SDL_SCANCODE_F2;
        CHECK(luna::platform::translateEvent(sdl)->key == luna::platform::Key::F2);
    }
}
