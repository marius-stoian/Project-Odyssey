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
        // US-134: the number keys 1-9 are Slot1-Slot9.
        for (int i = 0; i < 9; ++i) {
            const Key number = static_cast<Key>(static_cast<int>(Key::Num1) + i);
            input.handle(key(number, true));
            const auto numberTick = input.nextTick();
            CHECK(numberTick.pressed(static_cast<Intent>(static_cast<int>(Intent::Slot1) + i)));
            input.handle(key(number, false));
            input.nextTick();
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
    SUBCASE("the left button is the Attack intent (US-139)") {
        CHECK_FALSE(input.nextTick().held(Intent::Attack));
        input.setScriptedPointer(50, 60, PointerButton::Left, true);
        const auto down = input.nextTick();
        CHECK(down.held(Intent::Attack));
        CHECK(down.pressed(Intent::Attack));
        const auto still = input.nextTick();
        CHECK(still.held(Intent::Attack));
        CHECK_FALSE(still.pressed(Intent::Attack));
        input.setScriptedPointer(50, 60, PointerButton::Left, false);
        CHECK_FALSE(input.nextTick().held(Intent::Attack));
        input.setScripted(Intent::Attack, true); // a script can attack without the mouse
        CHECK(input.nextTick().held(Intent::Attack));
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

// US-300: a field's tooltip shows once the pointer has rested on it for 0.4 s, and goes when the pointer moves or the field is used.
TEST_CASE("US-300 Field tooltip") {
    engine::Panel panel({0, 0, 200, 60});
    auto& hp = panel.add<engine::NumberField>(engine::Rect{10, 10, 100, 11}, "HP", 60, 1, 999, [](int) {});
    auto& name = panel.add<engine::TextField>(engine::Rect{10, 30, 150, 11}, "Name", "Goblin", 16, [](const std::string&) {});
    hp.tip.text = "Health\nRange: 1 to 999\nExample: 60";
    const auto show = [&](engine::UiPainter& painter) { panel.drawOverlay(painter); };

    engine::ImageRenderer screen(480, 270);
    const auto sheet = screen.createTexture(engine::makeUiSheet());
    engine::UiPainter painter(screen, sheet);
    painter.setScreen({0, 0, 480, 270});

    SUBCASE("it waits, then shows purpose, range and example next to the pointer") {
        for (int tick = 0; tick < engine::FieldHint::kDelayTicks - 1; ++tick) panel.handle(at(20, 15));
        CHECK_FALSE(hp.tip.showing());
        panel.handle(at(20, 15));
        panel.handle(at(20, 15));
        CHECK(hp.tip.showing());
        screen.clear({0, 0, 0, 255});
        show(painter);
        CHECK(screen.image().get(20 + 8, 15 + 10) == luna::engine::Color{118, 104, 72}); // the box's border at its corner
    }
    SUBCASE("moving restarts the wait and leaving hides it") {
        for (int tick = 0; tick < 20; ++tick) panel.handle(at(20, 15));
        REQUIRE(hp.tip.showing());
        panel.handle(at(22, 15));
        CHECK_FALSE(hp.tip.showing());
        for (int tick = 0; tick < 20; ++tick) panel.handle(at(22, 15));
        CHECK(hp.tip.showing());
        panel.handle(at(300, 200));
        CHECK_FALSE(hp.tip.showing());
    }
    SUBCASE("a click on the field hides it; a field without text shows nothing") {
        for (int tick = 0; tick < 20; ++tick) panel.handle(at(20, 15));
        REQUIRE(hp.tip.showing());
        panel.handle(at(20, 15, true));
        CHECK_FALSE(hp.tip.showing());
        for (int tick = 0; tick < 20; ++tick) panel.handle(at(20, 35));
        CHECK_FALSE(name.tip.showing()); // no text: no tooltip
    }
    SUBCASE("a long line is broken so the box stays narrow") {
        name.tip.text = "A long purpose that goes on and on and on and on and on and on and on and on and on and on";
        for (int tick = 0; tick < 20; ++tick) panel.handle(at(20, 35));
        REQUIRE(name.tip.showing());
        screen.clear({0, 0, 0, 255});
        show(painter);
        const int widest = engine::FieldHint::kWrapColumns * engine::kTextAdvance + 6;
        CHECK(screen.image().get(20 + 8 + widest + 2, 35 + 10 + 4) == luna::engine::Color{0, 0, 0, 255}); // nothing drawn beyond the wrap width
    }
}

// ---- US-301 Suggestion list: filter, keys, rows, placement, clicks, quiet when closed.

namespace {

engine::UiInput pressKey(bool up, bool down, bool tab, bool escape, bool confirm = false) {
    engine::UiInput input = at(-1, -1);
    input.up = up;
    input.down = down;
    input.tab = tab;
    input.escape = escape;
    input.confirm = confirm;
    return input;
}
engine::UiInput arrowUp() { return pressKey(true, false, false, false); }
engine::UiInput arrowDown() { return pressKey(false, true, false, false); }
engine::UiInput tabKey() { return pressKey(false, false, true, false); }
engine::UiInput escapeKey() { return pressKey(false, false, false, true); }
engine::UiInput enterKey() { return pressKey(false, false, false, false, true); }

std::vector<std::string> classes() { return {"trader", "trapper", "elder"}; }

// A text field at (10, y) that offers `values`, focused by a click, with a record of what it was set to.
struct Rig {
    engine::Panel panel{{0, 0, 480, 270}};
    engine::TextField* field = nullptr;
    std::string kept;
    int changes = 0;
    explicit Rig(std::vector<std::string> values = classes(), int y = 10, bool list = false) {
        field = &panel.add<engine::TextField>(engine::Rect{10, y, 200, 11}, "Class", "", 40, [this](const std::string& v) {
            kept = v;
            ++changes;
        });
        field->suggest = [values](const std::string&) { return values; };
        field->listItems = list;
        panel.handle(at(60, y + 4, true)); // focus
    }
    void type(const std::string& text) { panel.handle(typed(text)); }
    const std::vector<std::string>& rows() const { return field->suggestions().rows(); }
};

const luna::engine::Color kBorder{118, 104, 72};

} // namespace

TEST_CASE("US-301 Filter: typing narrows the list, prefix matches first, any letter case") {
    Rig rig;
    CHECK(rig.field->focused());
    CHECK(rig.field->suggestions().isOpen());
    CHECK(rig.rows() == classes()); // on focus: every value
    rig.type("tr");
    CHECK(rig.rows() == std::vector<std::string>{"trader", "trapper"});
    rig.type("a"); // "tra": still both
    CHECK(rig.rows() == std::vector<std::string>{"trader", "trapper"});
    rig.type("p");
    CHECK(rig.rows() == std::vector<std::string>{"trapper"});
    // Substring matches come after prefix matches.
    Rig second;
    second.type("E");
    CHECK(second.rows() == std::vector<std::string>{"elder", "trader", "trapper"});
    // Nothing matches: no list, and the field still takes its text.
    second.type("zz");
    CHECK_FALSE(second.field->suggestions().isOpen());
    second.panel.handle(enterKey());
    CHECK(second.kept == "Ezz");
}

TEST_CASE("US-301 Keys: Down then Tab takes the second row; Escape closes the list and keeps the text") {
    {
        Rig rig;
        rig.type("tr");
        CHECK(rig.field->suggestions().highlighted() == 0);
        rig.panel.handle(arrowDown());
        CHECK(rig.field->suggestions().highlighted() == 1);
        rig.panel.handle(tabKey());
        CHECK(rig.kept == "trapper");
        CHECK(rig.changes == 1);
        CHECK_FALSE(rig.field->focused());
        CHECK_FALSE(rig.field->suggestions().isOpen());
    }
    {
        Rig rig;
        rig.type("tr");
        rig.panel.handle(escapeKey());
        CHECK_FALSE(rig.field->suggestions().isOpen());
        CHECK(rig.field->focused()); // the field is still being typed in
        CHECK(rig.changes == 0);
        rig.panel.handle(enterKey());
        CHECK(rig.kept == "tr"); // what was typed is kept
        CHECK(rig.changes == 1);
    }
    {
        Rig rig;
        rig.panel.handle(arrowDown());
        rig.panel.handle(arrowDown());
        rig.panel.handle(arrowUp());
        CHECK(rig.field->suggestions().highlighted() == 1);
        rig.panel.handle(enterKey()); // Enter takes the row once Up or Down has chosen it
        CHECK(rig.kept == "trapper");
    }
}

TEST_CASE("US-301 Enter keeps the typed text unless a row was chosen with Up or Down") {
    Rig rig;
    rig.type("tr");
    rig.panel.handle(enterKey()); // the first row is highlighted but not chosen
    CHECK(rig.kept == "tr");
}

TEST_CASE("US-301 Quiet: with the list closed or absent, Tab, Enter and the arrows are not used by it") {
    SUBCASE("a field that offers nothing") {
        engine::Panel panel({0, 0, 480, 270});
        std::string kept;
        auto& field = panel.add<engine::TextField>(engine::Rect{10, 10, 200, 11}, "Name", "Goblin", 16, [&](const std::string& v) { kept = v; });
        panel.handle(at(60, 14, true));
        REQUIRE(field.focused());
        CHECK_FALSE(field.suggestions().isOpen());
        panel.handle(arrowDown());
        panel.handle(tabKey());
        panel.handle(escapeKey());
        CHECK(field.focused()); // none of them did anything to the field
        CHECK(kept.empty());
        panel.handle(typed("s"));
        panel.handle(enterKey());
        CHECK(kept == "Goblins"); // Enter commits the typed text, as before M10b
    }
    SUBCASE("a field with a list that is not focused") {
        Rig rig;
        rig.panel.handle(at(400, 200, true)); // click elsewhere: commits and closes
        REQUIRE_FALSE(rig.field->focused());
        CHECK_FALSE(rig.field->suggestions().isOpen());
        CHECK_FALSE(rig.field->handle(arrowDown()));
        CHECK_FALSE(rig.field->handle(tabKey()));
    }
    SUBCASE("the list itself uses nothing while closed") {
        engine::SuggestList list;
        std::optional<std::string> accepted;
        CHECK_FALSE(list.handle(arrowDown(), accepted));
        CHECK_FALSE(list.handle(tabKey(), accepted));
        CHECK_FALSE(list.handle(enterKey(), accepted));
        list.open({"a", "b"});
        list.close();
        CHECK_FALSE(list.handle(escapeKey(), accepted));
        CHECK_FALSE(accepted.has_value());
    }
}

TEST_CASE("US-301 Rows: eight at a time, scrolling with the highlight") {
    std::vector<std::string> many;
    for (int i = 1; i <= 12; ++i) many.push_back("item" + std::to_string(100 + i));
    Rig rig(many);
    engine::ImageRenderer screen(480, 270);
    const auto sheet = screen.createTexture(engine::makeUiSheet());
    engine::UiPainter painter(screen, sheet);
    painter.setScreen({0, 0, 480, 270});
    screen.clear({0, 0, 0, 255});
    rig.panel.drawOverlay(painter);
    const int boxX = 10 + engine::UiPainter::textWidth("Class") + 4;
    CHECK(screen.image().get(boxX, 10 + 11) == kBorder); // the list's top-left corner, just under the field
    const int height = engine::SuggestList::kMaxRows * engine::kLineHeight + 2;
    CHECK(screen.image().get(boxX, 10 + 11 + height - 1) == kBorder); // the bottom edge: eight rows, not twelve
    CHECK(screen.image().get(boxX, 10 + 11 + height + 3) != kBorder);
    // A picture for the story's evidence: the field, its tooltip-free list with the third row highlighted and the "more rows" marks.
    {
        rig.panel.handle(arrowUp());
        for (int i = 0; i < 2; ++i) rig.panel.handle(arrowDown());
        engine::ImageRenderer picture(240, 110);
        const auto pictureSheet = picture.createTexture(engine::makeUiSheet());
        engine::UiPainter picturePainter(picture, pictureSheet);
        picturePainter.setScreen({0, 0, 240, 110});
        picture.clear({20, 20, 28, 255});
        rig.panel.draw(picturePainter);
        rig.panel.drawOverlay(picturePainter);
        const std::filesystem::path file = std::filesystem::temp_directory_path() / "odysseus-us301-list.png";
        engine::savePng(picture.image(), file);
        MESSAGE("list picture: " << file.string());
        rig.panel.handle(arrowUp());
        rig.panel.handle(arrowUp());
        rig.panel.handle(arrowUp()); // back to the first row
    }
    for (int i = 0; i < 10; ++i) rig.panel.handle(arrowDown());
    CHECK(rig.field->suggestions().highlighted() == 10);
    for (int i = 0; i < 5; ++i) rig.panel.handle(arrowDown()); // never past the last row
    CHECK(rig.field->suggestions().highlighted() == 11);
    rig.panel.handle(tabKey());
    CHECK(rig.kept == "item112");
}

TEST_CASE("US-301 Placement: under the field, and above it where it would leave the screen") {
    engine::ImageRenderer screen(480, 270);
    const auto sheet = screen.createTexture(engine::makeUiSheet());
    engine::UiPainter painter(screen, sheet);
    painter.setScreen({0, 0, 480, 270});
    const int boxX = 10 + engine::UiPainter::textWidth("Class") + 4;
    {
        Rig rig(classes(), 40);
        screen.clear({0, 0, 0, 255});
        rig.panel.drawOverlay(painter);
        CHECK(screen.image().get(boxX, 40 + 11) == kBorder); // just under the field
    }
    {
        Rig rig(classes(), 255); // 3 rows need 29 pixels; only 4 are left under the field
        screen.clear({0, 0, 0, 255});
        rig.panel.drawOverlay(painter);
        const int height = 3 * engine::kLineHeight + 2;
        CHECK(screen.image().get(boxX, 255 - height) == kBorder); // its top, above the field
        CHECK(screen.image().get(boxX, 255 + 11) != kBorder);
    }
}

TEST_CASE("US-301 Click: a click on a row takes it; a click elsewhere keeps the typed text") {
    {
        Rig rig;
        engine::ImageRenderer screen(480, 270);
        const auto sheet = screen.createTexture(engine::makeUiSheet());
        engine::UiPainter painter(screen, sheet);
        painter.setScreen({0, 0, 480, 270});
        rig.panel.drawOverlay(painter); // the list is drawn once, so it knows where its rows are
        const int boxX = 10 + engine::UiPainter::textWidth("Class") + 4;
        CHECK(rig.panel.handle(at(boxX + 20, 10 + 11 + 1 + 2 * engine::kLineHeight + 3, true)));
        CHECK(rig.kept == "elder"); // the third row
        CHECK_FALSE(rig.field->focused());
    }
    {
        Rig rig;
        rig.type("tr");
        rig.panel.handle(at(400, 200, true));
        CHECK(rig.kept == "tr");
        CHECK_FALSE(rig.field->suggestions().isOpen());
    }
}

TEST_CASE("US-301 List items: a suggestion completes the item after the last comma and keeps the others") {
    Rig rig(classes(), 10, true);
    rig.type("trader, e");
    CHECK(rig.rows() == std::vector<std::string>{"elder", "trader", "trapper"}); // "e" filters the item being typed, not the whole text
    rig.panel.handle(tabKey());
    CHECK(rig.kept == "trader, elder");
}

TEST_CASE("US-301 Numbers: the list offers numbers written as text and takes one within the limits") {
    engine::Panel panel({0, 0, 480, 270});
    int value = 0;
    auto& hp = panel.add<engine::NumberField>(engine::Rect{10, 10, 150, 11}, "HP", 5, 1, 999, [&](int v) { value = v; });
    hp.suggest = [](const std::string&) { return std::vector<std::string>{"60", "1", "999", "16"}; };
    panel.handle(at(60, 14, true));
    REQUIRE(hp.focused());
    CHECK(hp.suggestions().rows() == std::vector<std::string>{"60", "1", "999", "16"});
    panel.handle(typed("6"));
    CHECK(hp.suggestions().rows() == std::vector<std::string>{"60", "16"}); // starts with 6, then contains 6
    panel.handle(arrowDown());
    panel.handle(tabKey());
    CHECK(value == 16);
    CHECK_FALSE(hp.focused());
}

TEST_CASE("US-301 Intents: the arrows, Tab and Escape also reach the list, and still do what they did") {
    luna::engine::InputMap map;
    map.handle(key(Key::Down, true));
    map.handle(key(Key::Tab, true));
    luna::engine::Intents intents = map.nextTick();
    CHECK(intents.pressed(Intent::ListDown));
    CHECK(intents.pressed(Intent::ListTab));
    CHECK(intents.pressed(Intent::MoveDown)); // the movement and weapon intents are unchanged
    CHECK(intents.pressed(Intent::SwitchWeapon));
    CHECK(engine::UiInput::from(intents).down);
    CHECK(engine::UiInput::from(intents).tab);
    map.handle(key(Key::Down, false));
    map.handle(key(Key::Tab, false));
    map.handle(key(Key::S, true)); // a letter is not an arrow: the list ignores it
    intents = map.nextTick();
    CHECK(intents.pressed(Intent::MoveDown));
    CHECK_FALSE(intents.pressed(Intent::ListDown));
}

TEST_CASE("US-302 List items: words separated by spaces complete one word, keeping the ones before it") {
    Rig rig(classes(), 10, true);
    rig.type("elder tr");
    CHECK(rig.rows() == std::vector<std::string>{"trader", "trapper"});
    rig.panel.handle(tabKey());
    CHECK(rig.kept == "elder trader");
}
