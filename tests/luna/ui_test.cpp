// US-121 Point, click and read on screen.
#include "luna/engine/input.h"
#include "luna/engine/image_io.h"
#include "luna/engine/ui.h"

#include <filesystem>

#include <doctest/doctest.h>

#include <string>
#include <vector>

using luna::engine::Intent;
using luna::engine::PointerButton;
using luna::platform::Event;
using luna::platform::EventType;
using luna::platform::Key;
namespace engine = luna::engine;

namespace {

Event mouse(EventType type, float x, float y, luna::platform::MouseButton button = luna::platform::MouseButton::Left) {
    Event e;
    e.type = type;
    e.x = x;
    e.y = y;
    e.mouseButton = button;
    return e;
}

Event key(Key k, bool down) {
    Event e;
    e.type = down ? EventType::KeyDown : EventType::KeyUp;
    e.key = k;
    return e;
}

engine::UiInput at(int x, int y, bool press = false, bool release = false, int wheel = 0) {
    engine::UiInput input;
    input.pointer.x = x;
    input.pointer.y = y;
    input.pointer.pressed[0] = press;
    input.pointer.released[0] = release;
    input.pointer.wheel = wheel;
    return input;
}

engine::UiInput typed(const std::string& text, bool erase = false, bool confirm = false) {
    engine::UiInput input = at(-1, -1);
    input.text = text;
    input.erase = erase;
    input.confirm = confirm;
    return input;
}

} // namespace

TEST_CASE("US-121 Pointer") {
    engine::InputMap input;
    input.setPointerArea({160, 90, 960, 540}, 2); // a 1280x720 window, the picture scaled x2
    SUBCASE("the mouse becomes virtual pixels") {
        input.handle(mouse(EventType::MouseMoved, 170.0F, 101.0F));
        auto tick = input.nextTick();
        CHECK(tick.pointer().x == 5);
        CHECK(tick.pointer().y == 5);
        input.handle(mouse(EventType::MouseMoved, 100.0F, 100.0F)); // on the black bar
        CHECK_FALSE(input.nextTick().pointer().inside());
    }
    SUBCASE("a click between two ticks is still seen") {
        input.handle(mouse(EventType::MouseButtonDown, 400.0F, 300.0F));
        input.handle(mouse(EventType::MouseButtonUp, 400.0F, 300.0F));
        const auto tick = input.nextTick();
        CHECK(tick.pointer().wasPressed(PointerButton::Left));
        CHECK(tick.pointer().wasReleased(PointerButton::Left));
        CHECK_FALSE(tick.pointer().isHeld(PointerButton::Left));
        CHECK_FALSE(input.nextTick().pointer().wasPressed(PointerButton::Left)); // reported once
    }
    SUBCASE("a held right button and the wheel") {
        input.handle(mouse(EventType::MouseButtonDown, 400.0F, 300.0F, luna::platform::MouseButton::Right));
        Event wheel;
        wheel.type = EventType::MouseWheel;
        wheel.wheel = -1.0F;
        input.handle(wheel);
        const auto tick = input.nextTick();
        CHECK(tick.pointer().isHeld(PointerButton::Right));
        CHECK(tick.pointer().wheel == -1);
        CHECK(input.nextTick().pointer().wheel == 0);
    }
    SUBCASE("Ctrl chords and tool keys are intents, never keys") {
        input.handle(key(Key::LCtrl, true));
        input.handle(key(Key::S, true));
        auto tick = input.nextTick();
        CHECK(tick.pressed(Intent::Save));
        CHECK_FALSE(tick.held(Intent::MoveDown)); // Ctrl+S saves; it does not also walk down
        input.handle(key(Key::LCtrl, false));
        input.handle(key(Key::S, false));
        CHECK_FALSE(input.nextTick().held(Intent::Save)); // let go, even after Ctrl
        input.handle(key(Key::RCtrl, true));
        input.handle(key(Key::Z, true));
        CHECK(input.nextTick().pressed(Intent::Undo));
        input.handle(key(Key::Z, false));
        input.handle(key(Key::Y, true));
        CHECK(input.nextTick().pressed(Intent::Redo));
        for (const auto& [k, intent] : {std::pair{Key::F1, Intent::ModeGame}, std::pair{Key::F2, Intent::ModeEditor}, std::pair{Key::Delete, Intent::Delete},
                                        std::pair{Key::G, Intent::ToggleGrid}, std::pair{Key::R, Intent::Rotate}, std::pair{Key::Backspace, Intent::Erase},
                                        std::pair{Key::Enter, Intent::Confirm}}) {
            input.handle(key(k, true));
            CHECK(input.nextTick().pressed(intent));
            input.handle(key(k, false));
        }
    }
    SUBCASE("typed text and scripted input") {
        Event text;
        text.type = EventType::TextInput;
        text.text = "Ur";
        input.handle(text);
        input.typeScripted("a");
        CHECK(input.nextTick().text() == "Ura");
        CHECK(input.nextTick().text().empty());
        input.setScriptedPointer(12, 34, PointerButton::Left, true);
        const auto tick = input.nextTick();
        CHECK(tick.pointer().x == 12);
        CHECK(tick.pointer().wasPressed(PointerButton::Left));
    }
}

TEST_CASE("US-121 Text") {
    // The letter A, pixel by pixel: a pointed top, a crossbar on the fourth row.
    const char* kA[7] = {".###.", "#...#", "#...#", "#...#", "#####", "#...#", "#...#"};
    for (int row = 0; row < 7; ++row) {
        for (int column = 0; column < 5; ++column) {
            CHECK(engine::glyphPixel('A', column, row) == (kA[row][column] == '#'));
        }
    }
    // Every printable character has a glyph; only the space is empty.
    for (char c = 33; c <= 126; ++c) {
        bool lit = false;
        for (int row = 0; row < 7; ++row)
            for (int column = 0; column < 5; ++column) lit = lit || engine::glyphPixel(c, column, row);
        CHECK_MESSAGE(lit, c);
    }
    CHECK(engine::UiPainter::textWidth("Hello") == 29);
    // Drawn pixel-exact: the glyph's lit pixels, and nothing else, in the text colour.
    engine::ImageRenderer screen(40, 12);
    screen.clear({0, 0, 0});
    const auto sheet = screen.createTexture(engine::makeUiSheet());
    engine::UiPainter painter(screen, sheet);
    painter.text(3, 2, "A", engine::UiColor::Gold);
    for (int y = 0; y < 12; ++y) {
        for (int x = 0; x < 40; ++x) {
            const bool lit = engine::glyphPixel('A', x - 3, y - 2);
            CHECK((screen.image().get(x, y) == luna::engine::Color{232, 196, 96}) == lit);
        }
    }
}

TEST_CASE("US-121 Widgets") {
    engine::Panel panel({10, 10, 200, 150});
    int clicks = 0;
    int chosen = -1;
    int number = 0;
    std::string name;
    auto& button = panel.add<engine::Button>(engine::Rect{20, 20, 40, 14}, "Save", [&] { ++clicks; });
    button.hint = "Save the level";
    auto& list = panel.add<engine::ListBox>(engine::Rect{20, 40, 80, 27}, std::vector<std::string>{"grass", "dirt", "path", "stone", "water"},
                                            [&](int row) { chosen = row; });
    auto& hp = panel.add<engine::NumberField>(engine::Rect{20, 80, 100, 11}, "HP", 100, 1, 999, [&](int v) { number = v; });
    auto& text = panel.add<engine::TextField>(engine::Rect{20, 100, 150, 11}, "Name", "Goblin", 16, [&](const std::string& v) { name = v; });

    SUBCASE("a button runs its action when clicked, and shows its hint") {
        CHECK(panel.handle(at(30, 25, true)));
        CHECK(clicks == 0); // pressed, not yet let go
        CHECK(panel.handle(at(30, 25, false, true)));
        CHECK(clicks == 1);
        engine::ImageRenderer screen(480, 270);
        const auto sheet = screen.createTexture(engine::makeUiSheet());
        engine::UiPainter painter(screen, sheet);
        panel.draw(painter);
        panel.drawOverlay(painter);
        CHECK(screen.image().get(30 + 8, 25 + 10) == luna::engine::Color{118, 104, 72}); // the hint box's border, at its corner
    }
    SUBCASE("a list selects and scrolls") {
        CHECK(list.rows() == 3);
        panel.handle(at(30, 40 + 9 + 2, true)); // second row
        CHECK(chosen == 1);
        panel.handle(at(30, 45, false, false, -1)); // scroll down one
        CHECK(list.first == 1);
        panel.handle(at(30, 45, false, false, -5)); // never past the end
        CHECK(list.first == 2);
        panel.handle(at(30, 40 + 2, true));
        CHECK(chosen == 2);
    }
    SUBCASE("a number field takes digits, keeps its limits, and the wheel steps it") {
        panel.handle(at(60, 85, true));
        CHECK(hp.focused());
        CHECK(panel.typing());
        panel.handle(typed("25x0"));  // letters are ignored
        panel.handle(typed("", true)); // Backspace
        panel.handle(typed("", false, true));
        CHECK_FALSE(hp.focused());
        CHECK(number == 25);
        panel.handle(at(60, 85, true));
        panel.handle(typed("5000"));
        panel.handle(at(190, 150, true)); // clicking elsewhere keeps it too, within the limits
        CHECK(hp.value == 999);
        panel.handle(at(60, 85, false, false, -1));
        CHECK(hp.value == 998);
    }
    SUBCASE("a text field edits and keeps its length") {
        panel.handle(at(100, 105, true));
        CHECK(text.focused());
        panel.handle(typed("", true));
        panel.handle(typed(" King of the North"));
        panel.handle(typed("", false, true));
        CHECK(name == "Gobli King of th"); // at most 16 characters
    }
    SUBCASE("the panel keeps clicks from the world behind it, and ignores clicks outside") {
        CHECK(panel.handle(at(190, 150, true)));
        CHECK_FALSE(panel.handle(at(300, 200, true)));
    }
}

TEST_CASE("US-121 Showcase") {
    // A 480x270 screen with every glyph in every colour and each widget, saved for review.
    engine::ImageRenderer screen(480, 270);
    screen.clear({60, 110, 50});
    const auto sheet = screen.createTexture(engine::makeUiSheet());
    engine::UiPainter painter(screen, sheet);
    engine::Panel glyphs({4, 4, 472, 128});
    glyphs.draw(painter);
    std::string all;
    for (char c = 32; c <= 126; ++c) all += c;
    for (int color = 0; color < static_cast<int>(engine::UiColor::Count); ++color) {
        const auto ui = static_cast<engine::UiColor>(color);
        painter.text(8, 8 + color * engine::kLineHeight, all.substr(0, 47), ui);
        painter.text(8 + 48 * engine::kTextAdvance, 8 + color * engine::kLineHeight, all.substr(47, 30), ui);
    }
    engine::Panel tools({4, 136, 300, 130});
    auto& save = tools.add<engine::Button>(engine::Rect{10, 142, 40, 14}, "Save", [] {});
    save.hint = "Save the level (Ctrl+S)";
    tools.add<engine::Button>(engine::Rect{54, 142, 40, 14}, "Paint", [] {}).selected = true;
    auto& list = tools.add<engine::ListBox>(engine::Rect{10, 162, 90, 45}, std::vector<std::string>{"grass", "dirt", "path", "stone", "sand", "snow", "water"}, [](int) {});
    list.selected = 2;
    tools.add<engine::NumberField>(engine::Rect{110, 162, 90, 11}, "HP", 100, 1, 999, [](int) {});
    tools.add<engine::TextField>(engine::Rect{110, 178, 180, 11}, "Name", "Goblin chief", 20, [](const std::string&) {});
    tools.handle(at(30, 148)); // hovering the Save button
    tools.draw(painter);
    tools.drawOverlay(painter);
    const auto file = std::filesystem::temp_directory_path() / "odysseus-us121-showcase.png";
    REQUIRE(engine::savePng(screen.image(), file));
    MESSAGE("showcase: ", file.string());
}

TEST_CASE("US-126 Hints stay on screen") {
    engine::ImageRenderer screen(480, 270);
    const auto sheet = screen.createTexture(engine::makeUiSheet());
    engine::UiPainter painter(screen, sheet);
    painter.setScreen({0, 0, 480, 270});
    CHECK(painter.keepOnScreen({470, 260, 100, 13}) == engine::Rect{380, 257, 100, 13}); // pushed back inside
    CHECK(painter.keepOnScreen({10, 10, 100, 13}) == engine::Rect{10, 10, 100, 13});     // already inside: unchanged
}
