// US-191 Schema-driven form editor: the Data tab opens any data file as forms built from its schema, edits it with undo, and writes only what changed.
#include "core/text.h"
#include "game/data_editor.h"
#include "game/editor_help.h"
#include "luna/engine/image_io.h"
#include "luna/engine/ui.h"
#include "sim/schema.h"
#include "sim/schema_index.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace game = odysseus::game;
namespace luna_ui = luna::engine;

namespace {

struct Rig {
    fs::path base;
    fs::path folder; // the data folder
    std::vector<std::string> said;
    std::vector<fs::path> saves;
    game::DataEditor editor;

    explicit Rig(bool withLevels = false)
        : base(fs::temp_directory_path() / ("odysseus-us191-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))),
          folder(base / "data"),
          editor(960, 540, [this](const std::string& message) { said.push_back(message); }) {
        fs::create_directories(folder);
        fs::copy(ODYSSEUS_DATA_DIR, folder, fs::copy_options::recursive);
        if (withLevels) fs::copy(fs::path(ODYSSEUS_DATA_DIR).parent_path() / "levels", base / "levels", fs::copy_options::recursive);
        editor.setFolder(folder, [this](const fs::path& file) { saves.push_back(file); });
        editor.show(true);
    }
    ~Rig() {
        std::error_code error;
        fs::remove_all(base, error);
    }
    std::string text(const std::string& relative) const { return *odysseus::core::readTextFile(folder / relative); }
    std::string levelText(const std::string& name) const { return *odysseus::core::readTextFile(base / "levels" / name); }
};

int differingLines(const std::string& a, const std::string& b) {
    std::vector<std::string> left;
    std::vector<std::string> right;
    for (const std::string* text : {&a, &b}) {
        std::vector<std::string>& lines = text == &a ? left : right;
        std::size_t at = 0;
        while (at <= text->size()) {
            const std::size_t end = text->find('\n', at);
            lines.push_back(text->substr(at, end == std::string::npos ? std::string::npos : end - at));
            if (end == std::string::npos) break;
            at = end + 1;
        }
    }
    if (left.size() != right.size()) return -1;
    int count = 0;
    for (std::size_t i = 0; i < left.size(); ++i) count += left[i] != right[i] ? 1 : 0;
    return count;
}

// A tick of the screen with the pointer at a place, pressed and let go (a Button reacts when the pointer is let go over it).
luna_ui::Intents click(int x, int y) {
    luna_ui::Intents intents;
    luna_ui::Pointer pointer;
    pointer.x = x;
    pointer.y = y;
    pointer.pressed[0] = true;
    pointer.held[0] = true;
    pointer.released[0] = true;
    intents.setPointer(pointer);
    return intents;
}

const luna_ui::TextField* fieldLabelled(const luna_ui::Panel& panel, const std::string& label) {
    for (const auto& child : panel.children()) {
        const auto* field = dynamic_cast<const luna_ui::TextField*>(child.get());
        if (field == nullptr) continue;
        const std::size_t end = field->label.find_last_not_of(' ');
        const std::size_t start = field->label.find_first_not_of(' ');
        if (end != std::string::npos && field->label.substr(start, end - start + 1) == label) return field;
    }
    return nullptr;
}

} // namespace

TEST_CASE("US-191 Data tab opens a file") {
    Rig rig;
    REQUIRE(rig.editor.shown());
    const std::vector<std::string> files = rig.editor.files();
    CHECK(files.size() > 150);
    CHECK(std::find(files.begin(), files.end(), "weapons.json") != files.end());
    CHECK(std::find(files.begin(), files.end(), "sim/needs.json") != files.end());
    CHECK(std::find(files.begin(), files.end(), "schemas/index.json") == files.end()); // the schemas are not data
    REQUIRE(rig.editor.open("weapons.json"));
    CHECK(rig.editor.openFile() == "weapons.json");
    CHECK(rig.editor.entries().size() > 10);
    CHECK(rig.editor.entries()[1].label == "iron sword");
    CHECK(rig.editor.entryPath() == "weapons[0]"); // a catalog opens on its first entry
    const auto damage = std::find_if(rig.editor.rows().begin(), rig.editor.rows().end(), [](const auto& row) { return row.path == "weapons[0].damage"; });
    REQUIRE(damage != rig.editor.rows().end());
    CHECK(damage->value == "5");
    CHECK_FALSE(rig.editor.dirty());
    // Another entry, another file.
    REQUIRE(rig.editor.selectEntry("weapons[2]"));
    CHECK(rig.editor.rows().front().path.starts_with("weapons[2]"));
    REQUIRE(rig.editor.open("sim/needs.json"));
    CHECK(rig.editor.entries().size() == 1);
    CHECK(rig.editor.entryPath().empty());
    CHECK_FALSE(rig.editor.open("nothing.json"));
}

TEST_CASE("US-191 Data tab edit and save") {
    Rig rig;
    REQUIRE(rig.editor.open("weapons.json"));
    const std::string before = rig.text("weapons.json");
    CHECK_FALSE(rig.editor.setField("weapons[0].damage", "5000")); // refused with the allowed range, nothing changes
    CHECK(rig.editor.problem().find("must be between 0 and 1000") != std::string::npos);
    CHECK_FALSE(rig.editor.dirty());
    REQUIRE(rig.editor.setField("weapons[0].damage", "9"));
    CHECK(rig.editor.dirty());
    CHECK(rig.text("weapons.json") == before); // nothing is written until Save
    REQUIRE(rig.editor.save());
    CHECK_FALSE(rig.editor.dirty());
    REQUIRE(rig.saves.size() == 1);
    CHECK(rig.saves.front() == rig.folder / "weapons.json"); // the game is told which file to read again
    const std::string after = rig.text("weapons.json");
    CHECK(differingLines(before, after) == 1); // only the damage changed
    CHECK(after.find("\"damage\":9,") != std::string::npos);
}

TEST_CASE("US-191 Data tab keeps notes and comments") {
    Rig rig;
    REQUIRE(rig.editor.open("interactions/gather.json"));
    const std::string before = rig.text("interactions/gather.json");
    REQUIRE(before.starts_with("//")); // a leading comment
    REQUIRE(before.find("\"note\"") != std::string::npos);
    REQUIRE(rig.editor.setField("duration", "4"));
    REQUIRE(rig.editor.save());
    const std::string after = rig.text("interactions/gather.json");
    CHECK(differingLines(before, after) == 1);
    CHECK(after.substr(0, after.find('\n')) == before.substr(0, before.find('\n')));
    CHECK(after.find("\"note\"") != std::string::npos);
}

TEST_CASE("US-191 Data tab undo") {
    Rig rig;
    REQUIRE(rig.editor.open("sim/needs.json"));
    const std::string before = rig.text("sim/needs.json");
    REQUIRE(rig.editor.setField("maximum", "90"));
    REQUIRE(rig.editor.setField("dailyDecay.hunger", "31"));
    REQUIRE(rig.editor.setField("dailyDecay.energy", "36"));
    REQUIRE(rig.editor.setField("mealValue", "41"));
    REQUIRE(rig.editor.setField("daysAtZeroBeforeDeath.warmth", "4"));
    CHECK(rig.editor.dirty());
    for (int i = 0; i < 5; ++i) CHECK(rig.editor.undo());
    CHECK_FALSE(rig.editor.dirty());
    CHECK_FALSE(rig.editor.undo());
    for (int i = 0; i < 5; ++i) CHECK(rig.editor.redo());
    CHECK(rig.editor.dirty());
    REQUIRE(rig.editor.save());
    CHECK(differingLines(before, rig.text("sim/needs.json")) == 4); // hunger and energy stand on one line of the file
}

TEST_CASE("US-191 Data tab does not drop unsaved edits") {
    Rig rig;
    REQUIRE(rig.editor.open("sim/needs.json"));
    REQUIRE(rig.editor.setField("maximum", "90"));
    CHECK_FALSE(rig.editor.open("weapons.json"));
    CHECK(rig.editor.openFile() == "sim/needs.json");
    CHECK(rig.editor.problem().find("unsaved changes") != std::string::npos);
    REQUIRE(rig.editor.undo());
    CHECK(rig.editor.open("weapons.json"));
}

TEST_CASE("US-191 Data tab fields, groups and lists") {
    Rig rig;
    REQUIRE(rig.editor.open("hero/recipes.json"));
    REQUIRE(rig.editor.selectEntry("recipes[1]"));
    // A link field offers the names of its catalog.
    const auto output = std::find_if(rig.editor.rows().begin(), rig.editor.rows().end(), [](const auto& row) { return row.path == "recipes[1].output"; });
    REQUIRE(output != rig.editor.rows().end());
    CHECK(output->catalog == "items");
    const std::vector<std::string> items = rig.editor.catalogNames("items");
    CHECK(std::find(items.begin(), items.end(), "spearhead") != items.end());
    // A group folds.
    REQUIRE(rig.editor.open("sim/needs.json"));
    const auto count = [&] { return rig.editor.rows().size(); };
    const std::size_t open = count();
    REQUIRE(rig.editor.toggleFold("dailyDecay"));
    CHECK(count() < open);
    REQUIRE(rig.editor.toggleFold("dailyDecay"));
    CHECK(count() == open);
    // A list of objects gains an entry with a name of its own, and loses it again.
    REQUIRE(rig.editor.open("buildings/kinds.json"));
    REQUIRE(rig.editor.selectEntry("kinds[0]"));
    REQUIRE(rig.editor.add("kinds[0].layout"));
    const std::size_t pieces = rig.editor.document()->root()["kinds"][0]["layout"].size();
    REQUIRE(rig.editor.remove("kinds[0].layout[" + std::to_string(pieces - 1) + "]"));
    CHECK(rig.editor.document()->root()["kinds"][0]["layout"].size() == pieces - 1);
    REQUIRE(rig.editor.move("kinds[0].layout[1]", -1));
    REQUIRE(rig.editor.undo());
    REQUIRE(rig.editor.undo());
    REQUIRE(rig.editor.undo());
    CHECK_FALSE(rig.editor.dirty());
}

TEST_CASE("US-191 Data tab new entry") {
    Rig rig;
    REQUIRE(rig.editor.open("plants.json"));
    const std::size_t before = rig.editor.entries().size();
    REQUIRE(rig.editor.newEntry());
    CHECK(rig.editor.entries().size() == before + 1);
    CHECK(rig.editor.entries().back().label == "new-entry");
    CHECK(rig.editor.entryPath() == rig.editor.entries().back().path);
    REQUIRE(rig.editor.setField(rig.editor.entryPath() + ".frame", "bush"));
    REQUIRE(rig.editor.newEntry());
    CHECK(rig.editor.entries().back().label == "new-entry-2");
    REQUIRE(rig.editor.save());
    // The file still passes its schema: a new entry is made of the required fields.
    const std::string text = rig.text("plants.json");
    CHECK(text.find("\"new-entry\"") != std::string::npos);
    CHECK(text.find("\"new-entry-2\"") != std::string::npos);
}

TEST_CASE("US-191 Data tab by hand") {
    // The same edit made the way a hand makes it: click the field, type, press Enter; then Ctrl+S.
    Rig rig;
    REQUIRE(rig.editor.open("weapons.json"));
    luna_ui::Intents idle;
    rig.editor.update(idle);
    const luna_ui::TextField* field = fieldLabelled(rig.editor.panel(), "damage");
    REQUIRE(field != nullptr);
    const int x = field->bounds.x + field->bounds.width - 20;
    const int y = field->bounds.y + 5;
    rig.editor.update(click(x, y));
    CHECK(rig.editor.typing());
    luna_ui::Intents typed;
    typed.setText("12345");
    rig.editor.update(typed);
    luna_ui::Intents enter;
    enter.set(luna_ui::Intent::Confirm, true, true);
    rig.editor.update(enter); // the number is refused: it is over the maximum
    CHECK_FALSE(rig.editor.dirty());
    CHECK(rig.editor.problem().find("must be between 0 and 1000") != std::string::npos);
    rig.editor.update(idle);
    field = fieldLabelled(rig.editor.panel(), "damage");
    REQUIRE(field != nullptr);
    rig.editor.update(click(field->bounds.x + field->bounds.width - 20, field->bounds.y + 5));
    luna_ui::Intents erase; // clear what the field held, then type
    for (int i = 0; i < 3; ++i) {
        erase.set(luna_ui::Intent::Erase, true, true);
        rig.editor.update(erase);
    }
    luna_ui::Intents digits;
    digits.setText("7");
    rig.editor.update(digits);
    rig.editor.update(enter);
    rig.editor.update(idle);
    CHECK(rig.editor.dirty());
    CHECK(rig.editor.document()->find("weapons[0].damage")->get<int>() == 7);
    luna_ui::Intents save;
    save.set(luna_ui::Intent::Save, true, true);
    rig.editor.update(save);
    CHECK_FALSE(rig.editor.dirty());
    CHECK(rig.saves.size() == 1);
    // Esc with unsaved changes asks first, and a second Esc throws them away and comes back.
    REQUIRE(rig.editor.setField("weapons[0].damage", "8"));
    luna_ui::Intents escape;
    escape.set(luna_ui::Intent::OpenMenu, true, true);
    rig.editor.update(escape);
    CHECK(rig.editor.shown());
    rig.editor.update(escape);
    CHECK_FALSE(rig.editor.shown());
}

TEST_CASE("US-191 Data tab help") {
    // Every field of every form has its help: one entry made from its schema (K-M11 step 1), found by the id the row carries.
    Rig rig;
    game::EditorHelp help;
    rig.editor.setHelp(&help);
    REQUIRE_FALSE(help.generated().empty());
    int rows = 0;
    for (const std::string& file : rig.editor.files()) {
        REQUIRE(rig.editor.open(file));
        for (const auto& entry : std::vector<odysseus::sim::form::Entry>(rig.editor.entries())) {
            REQUIRE(rig.editor.selectEntry(entry.path));
            luna_ui::Intents idle;
            rig.editor.update(idle); // builds the widgets, which ask for their help
            for (const auto& row : rig.editor.rows()) {
                INFO(file << " " << row.path << " " << row.helpId);
                CHECK(help.find(row.helpId) != nullptr);
                ++rows;
            }
        }
    }
    CHECK(rows > 3000);
    CHECK(help.missing().empty());
}

TEST_CASE("US-191 Data tab draws") {
    Rig rig;
    REQUIRE(rig.editor.open("weapons.json"));
    luna_ui::ImageRenderer renderer(960, 540);
    renderer.clear({20, 20, 28, 255});
    const luna_ui::Texture sheet = renderer.createTexture(luna_ui::makeUiSheet());
    luna_ui::UiPainter painter(renderer, sheet);
    painter.setScreen({0, 0, 960, 540});
    luna_ui::Intents idle;
    rig.editor.update(idle);
    rig.editor.draw(painter);
    rig.editor.drawOverlay(painter);
    int lit = 0;
    for (int y = 0; y < 540; y += 2) {
        for (int x = 0; x < 960; x += 2) lit += renderer.image().get(x, y).green > 100 ? 1 : 0;
    }
    CHECK(lit > 200); // text and frames are there
    // With ODYSSEUS_EVIDENCE_DIR set, the tab is also saved as a picture (a story's evidence).
    char* folder = nullptr;
    std::size_t size = 0;
    if (_dupenv_s(&folder, &size, "ODYSSEUS_EVIDENCE_DIR") == 0 && folder != nullptr && *folder != 0) {
        fs::create_directories(folder);
        luna_ui::savePng(renderer.image(), fs::path(folder) / "data-tab-weapons.png");
        REQUIRE(rig.editor.open("sim/needs.json"));
        rig.editor.setField("maximum", "5"); // refused: the red message at the bottom
        rig.editor.update(idle);
        renderer.clear({20, 20, 28, 255});
        rig.editor.draw(painter);
        rig.editor.drawOverlay(painter);
        luna_ui::savePng(renderer.image(), fs::path(folder) / "data-tab-needs-refused.png");
    }
    std::free(folder);
}

TEST_CASE("US-193 Copy an entry") {
    Rig rig;
    REQUIRE(rig.editor.open("plants.json"));
    REQUIRE(rig.editor.selectEntry("plants[2]"));
    const std::string original = rig.editor.document()->root()["plants"][2]["name"].get<std::string>();
    const std::size_t before = rig.editor.entries().size();
    REQUIRE(rig.editor.copyEntry());
    CHECK(rig.editor.entries().size() == before + 1);
    CHECK(rig.editor.entryPath() == "plants[3]"); // beside the original, and chosen
    CHECK(rig.editor.document()->root()["plants"][3]["name"].get<std::string>() == original + "-copy");
    CHECK(rig.editor.document()->root()["plants"][3]["frame"] == rig.editor.document()->root()["plants"][2]["frame"]); // everything else is the same
    // A copy of a copy gets a name of its own too, and a name with a space is copied as "<name> copy".
    REQUIRE(rig.editor.copyEntry());
    CHECK(rig.editor.document()->root()["plants"][4]["name"].get<std::string>() == original + "-copy-copy");
    REQUIRE(rig.editor.open("weapons.json") == false); // unsaved: the file stays
    REQUIRE(rig.editor.undo());
    REQUIRE(rig.editor.undo());
    REQUIRE(rig.editor.open("weapons.json"));
    REQUIRE(rig.editor.selectEntry("weapons[0]"));
    REQUIRE(rig.editor.copyEntry());
    CHECK(rig.editor.document()->root()["weapons"][1]["name"] == "iron sword copy");
}

TEST_CASE("US-193 Rename an entry everywhere it is used") {
    Rig rig(true);
    REQUIRE(rig.editor.open("hero/items.json"));
    const auto& entries = rig.editor.entries();
    const auto berries = std::find_if(entries.begin(), entries.end(), [](const auto& entry) { return entry.label == "Berries"; });
    REQUIRE(berries != entries.end());
    REQUIRE(rig.editor.selectEntry(berries->path));
    // The rename first lists what it would change; nothing is written yet.
    REQUIRE(rig.editor.beginRename("red-berries"));
    CHECK(rig.editor.question().kind == game::DataEditor::Question::Kind::Rename);
    CHECK(rig.editor.question().lines.size() > 8);
    const auto mentions = [&](const std::string& file) {
        return std::any_of(rig.editor.question().lines.begin(), rig.editor.question().lines.end(), [&](const std::string& line) { return line.starts_with(file + ":"); });
    };
    CHECK(mentions("hero/items.json"));
    CHECK(mentions("quests/first-day.json"));
    CHECK(mentions("dialogue/elder-fire.dlg"));
    CHECK(mentions("interactions/give-berries.json"));
    CHECK(mentions("levels/npc-test.json"));
    CHECK(rig.text("hero/items.json").find("\"id\": \"berries\"") != std::string::npos);
    // Esc (cancel) leaves everything as it was.
    rig.editor.cancel();
    CHECK(rig.editor.question().kind == game::DataEditor::Question::Kind::None);
    CHECK(rig.text("hero/items.json").find("\"id\": \"berries\"") != std::string::npos);
    // Confirm writes every file at once, the game is told each data file, and the form shows the new name.
    REQUIRE(rig.editor.beginRename("red-berries"));
    REQUIRE(rig.editor.confirm());
    CHECK(rig.text("hero/items.json").find("\"id\": \"red-berries\"") != std::string::npos);
    CHECK(rig.text("quests/first-day.json").find("gather red-berries 1") != std::string::npos);
    CHECK(rig.text("dialogue/elder-fire.dlg").find("has(hero, red-berries, 1)") != std::string::npos);
    CHECK(rig.levelText("npc-test.json").find("\"red-berries\"") != std::string::npos);
    CHECK(rig.saves.size() > 5);
    CHECK(std::find(rig.saves.begin(), rig.saves.end(), rig.folder / "hero" / "items.json") != rig.saves.end());
    CHECK_FALSE(rig.editor.dirty());
    const auto& items = rig.editor.document()->root()["items"];
    CHECK(std::any_of(items.begin(), items.end(), [](const auto& item) { return item["id"] == "red-berries"; }));
    CHECK_FALSE(std::any_of(items.begin(), items.end(), [](const auto& item) { return item["id"] == "berries"; }));
    // The data folder is still consistent.
    const odysseus::sim::schema::SchemaSet set = odysseus::sim::schema::SchemaSet::load(rig.folder / "schemas");
    CHECK(odysseus::sim::schema::buildIndex(rig.folder, set).clean());
}

TEST_CASE("US-193 A rename that cannot be done") {
    Rig rig;
    REQUIRE(rig.editor.open("hero/items.json"));
    const auto& all = rig.editor.entries();
    const auto berries = std::find_if(all.begin(), all.end(), [](const auto& entry) { return entry.label == "Berries"; });
    REQUIRE(berries != all.end());
    REQUIRE(rig.editor.selectEntry(berries->path));
    CHECK_FALSE(rig.editor.beginRename("flint")); // an item of that name exists
    CHECK(rig.editor.problem().find("already") != std::string::npos);
    CHECK_FALSE(rig.editor.beginRename("bad\"name"));
    // Unsaved changes in the open file: a rename writes files, so it asks for the save first.
    REQUIRE(rig.editor.setField(rig.editor.entryPath() + ".value", "7"));
    CHECK_FALSE(rig.editor.beginRename("fine-name"));
    CHECK(rig.editor.problem().find("unsaved changes") != std::string::npos);
    // The file's own entry is not an entry of a list: nothing to rename.
    REQUIRE(rig.editor.undo());
    REQUIRE(rig.editor.selectEntry(""));
    CHECK_FALSE(rig.editor.beginRename("fine-name"));
}

TEST_CASE("US-193 Delete an entry that is still used") {
    Rig rig(true);
    REQUIRE(rig.editor.open("hero/items.json"));
    const auto& entries = rig.editor.entries();
    const auto berries = std::find_if(entries.begin(), entries.end(), [](const auto& entry) { return entry.label == "Berries"; });
    REQUIRE(berries != entries.end());
    REQUIRE(rig.editor.selectEntry(berries->path));
    const std::size_t count = rig.editor.document()->root()["items"].size();
    // Used: the places are listed and the delete waits.
    REQUIRE(rig.editor.beginDelete());
    CHECK(rig.editor.question().kind == game::DataEditor::Question::Kind::Delete);
    CHECK(rig.editor.question().lines.size() > 5);
    CHECK(rig.editor.document()->root()["items"].size() == count);
    rig.editor.cancel();
    CHECK(rig.editor.document()->root()["items"].size() == count);
    // Confirmed: it is gone from the open file (an edit that waits for Save), and Ctrl+Z brings it back.
    REQUIRE(rig.editor.beginDelete());
    REQUIRE(rig.editor.confirm());
    CHECK(rig.editor.document()->root()["items"].size() == count - 1);
    CHECK(rig.editor.dirty());
    REQUIRE(rig.editor.undo());
    CHECK(rig.editor.document()->root()["items"].size() == count);
    CHECK_FALSE(rig.editor.dirty());
}

TEST_CASE("US-193 Delete an entry nothing uses") {
    Rig rig(true);
    REQUIRE(rig.editor.open("plants.json"));
    REQUIRE(rig.editor.newEntry());
    const std::size_t count = rig.editor.document()->root()["plants"].size();
    REQUIRE(rig.editor.beginDelete()); // nothing names the new entry: no question
    CHECK(rig.editor.question().kind == game::DataEditor::Question::Kind::None);
    CHECK(rig.editor.document()->root()["plants"].size() == count - 1);
}

TEST_CASE("US-193 The question has a screen") {
    // The dialog is drawn and answered with the mouse and Esc.
    Rig rig(true);
    REQUIRE(rig.editor.open("hero/items.json"));
    const auto& entries = rig.editor.entries();
    const auto berries = std::find_if(entries.begin(), entries.end(), [](const auto& entry) { return entry.label == "Berries"; });
    REQUIRE(berries != entries.end());
    REQUIRE(rig.editor.selectEntry(berries->path));
    luna_ui::Intents idle;
    rig.editor.update(idle);
    const luna_ui::Button* rename = nullptr;
    for (const auto& child : rig.editor.panel().children()) {
        if (const auto* button = dynamic_cast<const luna_ui::Button*>(child.get()); button != nullptr && button->label == "Rename") rename = button;
    }
    REQUIRE(rename != nullptr);
    rig.editor.update(click(rename->bounds.x + 3, rename->bounds.y + 3));
    REQUIRE(rig.editor.question().kind == game::DataEditor::Question::Kind::Rename);
    rig.editor.update(idle); // the dialog is built
    CHECK(fieldLabelled(rig.editor.panel(), "new name:") != nullptr);
    luna_ui::ImageRenderer renderer(960, 540);
    renderer.clear({20, 20, 28, 255});
    const luna_ui::Texture sheet = renderer.createTexture(luna_ui::makeUiSheet());
    luna_ui::UiPainter painter(renderer, sheet);
    painter.setScreen({0, 0, 960, 540});
    rig.editor.draw(painter);
    rig.editor.drawOverlay(painter);
    luna_ui::Intents escape; // Esc answers no
    escape.set(luna_ui::Intent::OpenMenu, true, true);
    rig.editor.update(escape);
    CHECK(rig.editor.question().kind == game::DataEditor::Question::Kind::None);
    CHECK(rig.editor.shown());
}
