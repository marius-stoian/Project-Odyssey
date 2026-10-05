// US-302 Suggestions on every field: a field's list comes from the source its help entry names (numbers, files, catalogs, fixed values), read each time the
// list opens; comma lists complete one item; numbers offer the default, the limits and the last five typed.
#include "camp.h"

#include "game/editor_help.h"
#include "luna/engine/image_io.h"
#include "sim/npc_class.h"

#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

using namespace camp_support;
namespace rules = odysseus::sim::rules;

namespace {

luna::engine::Intents pressing(luna::engine::Intent intent) {
    luna::engine::Intents intents;
    intents.set(intent, true, true);
    return intents;
}

luna::engine::UiInput at(int x, int y, bool press = false) {
    luna::engine::UiInput input;
    input.pointer.x = x;
    input.pointer.y = y;
    input.pointer.pressed[0] = press;
    return input;
}

luna::engine::UiInput typed(const std::string& text) {
    luna::engine::UiInput input = at(-1, -1);
    input.text = text;
    return input;
}

luna::engine::UiInput enter() {
    luna::engine::UiInput input = at(-1, -1);
    input.confirm = true;
    return input;
}

luna::engine::UiInput tab() {
    luna::engine::UiInput input = at(-1, -1);
    input.tab = true;
    return input;
}

// A game in the Editor on a copy of the data folder, with one wolf and one goblin placed.
struct Studio {
    fs::path data;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;
    int wolf = 0;

    explicit Studio(const std::string& name) : data(dataCopy(name)) {
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        wolf = level.nextId++;
        level.characters.push_back({wolf, "wolf", {level.heroStart.x + 64, level.heroStart.y}, game::Facing::South, "Grey", 50, 8, {}});
        game::saveLevel(level, definitions, data / "suggest-level.json");
        odyssey = std::make_unique<game::OdysseyGame>(data, data / "suggest-level.json");
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
        odyssey->update(pressing(luna::engine::Intent::ModeEditor));
    }
};

// A help file of a few entries, read with the game's sources.
game::EditorHelp helpWith(Studio& studio, const std::string& body) {
    const fs::path file = studio.data.parent_path() / "suggest-help.json";
    writeText(file, "{ \"version\": 1, \"fields\": {" + body + "} }");
    game::EditorHelp help;
    help.setSources(studio.odyssey->suggestionSources());
    help.load(file);
    REQUIRE(help.problems().empty());
    return help;
}

const std::string kClassesEntry = "\"t.classes\": { \"purpose\": \"p\", \"example\": \"e\", \"suggest\": \"catalog:npc-classes\", \"list\": true }";

} // namespace

TEST_CASE("US-302 Text: the Script field lists the .dlg files of the dialogue folder that start with what is typed") {
    Studio studio("us302-text");
    game::EditorHelp help = helpWith(studio, "\"npc.script\": { \"purpose\": \"p\", \"example\": \"e\", \"suggest\": \"files:dialogue/*.dlg\" }");
    luna::engine::Panel panel({0, 0, 480, 270});
    auto& script = panel.add<luna::engine::TextField>(luna::engine::Rect{10, 10, 200, 11}, "Script", "", 40, [](const std::string&) {});
    help.apply(panel, "npc");
    panel.handle(at(60, 14, true));
    REQUIRE(script.focused());
    const std::vector<std::string> all = script.suggestions().rows();
    CHECK(std::find(all.begin(), all.end(), "elder-fire.dlg") != all.end());
    CHECK(std::find(all.begin(), all.end(), "smalltalk.json") == all.end()); // only the .dlg files
    panel.handle(typed("el"));
    const std::vector<std::string>& rows = script.suggestions().rows();
    REQUIRE_FALSE(rows.empty());
    CHECK(rows.front() == "elder-fire.dlg"); // the one that starts with "el" comes first
    for (const std::string& row : rows) {
        CHECK(row.find("el") != std::string::npos);
        CHECK(row.size() > 4);
        CHECK(row.substr(row.size() - 4) == ".dlg");
    }
}

TEST_CASE("US-302 Lists: a field holding 'trader, ' completes the item after the comma and keeps the rest") {
    Studio studio("us302-lists");
    game::EditorHelp help = helpWith(studio, "\"npc.does\": { \"purpose\": \"p\", \"example\": \"e\", \"suggest\": \"catalog:interactions\", \"list\": true }");
    const std::vector<std::string> interactions = studio.odyssey->suggestionNames("interactions");
    REQUIRE_FALSE(interactions.empty());
    std::string kept;
    luna::engine::Panel panel({0, 0, 480, 270});
    auto& does = panel.add<luna::engine::TextField>(luna::engine::Rect{10, 10, 200, 11}, "Does", "trader, ", 90, [&](const std::string& v) { kept = v; });
    help.apply(panel, "npc");
    panel.handle(at(40, 14, true));
    REQUIRE(does.focused());
    panel.handle(typed("e"));
    const std::vector<std::string>& rows = does.suggestions().rows();
    REQUIRE_FALSE(rows.empty());
    CHECK(rows.front().rfind('e', 0) == 0); // the ones that start with e come first
    const std::string chosen = rows.front();
    panel.handle(tab());
    CHECK(kept == "trader, " + chosen); // the item before the comma is kept
}

TEST_CASE("US-302 Numbers: HP of a wolf offers the kind default, the limits and the last five typed") {
    Studio studio("us302-numbers");
    game::EditorHelp help = helpWith(studio, "\"npc.hp\": { \"purpose\": \"p\", \"example\": \"e\", \"suggest\": \"number\" }");
    studio.odyssey->editor().select(studio.wolf);
    int value = 50;
    luna::engine::Panel panel({0, 0, 480, 270});
    auto& hp = panel.add<luna::engine::NumberField>(luna::engine::Rect{10, 10, 150, 11}, "HP", value, 1, 9999, [&](int v) { value = v; });
    help.apply(panel, "npc");
    panel.handle(at(60, 14, true));
    REQUIRE(hp.focused());
    CHECK(hp.suggestions().rows() == std::vector<std::string>{"50", "1", "9999"}); // the wolf's own 50, then min and max
    // Type a number and keep it: it is offered next time.
    panel.handle(typed("77"));
    panel.handle(enter());
    CHECK(value == 77);
    panel.handle(at(60, 14, true));
    CHECK(hp.suggestions().rows() == std::vector<std::string>{"50", "1", "9999", "77"});
    // Only the last five are kept, newest first.
    for (const int next : {11, 12, 13, 14, 15, 16}) {
        panel.handle(typed(std::to_string(next)));
        panel.handle(enter());
        panel.handle(at(60, 14, true));
    }
    CHECK(hp.suggestions().rows() == std::vector<std::string>{"50", "1", "9999", "16", "15", "14", "13", "12"});
}

TEST_CASE("US-302 Numbers: a field with no kind default offers the limits only") {
    Studio studio("us302-nodefault");
    game::EditorHelp help = helpWith(studio, "\"level.width\": { \"purpose\": \"p\", \"example\": \"e\", \"suggest\": \"number\" }");
    luna::engine::Panel panel({0, 0, 480, 270});
    auto& width = panel.add<luna::engine::NumberField>(luna::engine::Rect{10, 10, 150, 11}, "Width", 64, 8, 256, [](int) {});
    help.apply(panel, "level");
    panel.handle(at(60, 14, true));
    CHECK(width.suggestions().rows() == std::vector<std::string>{"8", "256"});
}

TEST_CASE("US-302 Fresh: a class saved a moment ago is offered without a restart") {
    Studio studio("us302-fresh");
    game::EditorHelp help = helpWith(studio, kClassesEntry);
    const std::vector<std::string> before = help.suggestionsFor("t.classes");
    CHECK_FALSE(before.empty()); // the shipped classes
    CHECK(std::find(before.begin(), before.end(), "healer") == before.end());

    game::Editor& editor = studio.odyssey->editor();
    editor.showClasses(true);
    editor.newClass();
    rules::NpcClass& draft = editor.classDraft();
    draft.id = "healer";
    draft.label = "Healer";
    draft.colour = 0x3A9A4C;
    draft.icon = "cross";
    draft.tags = {"healer"};
    REQUIRE(editor.saveClass());

    luna::engine::Panel panel({0, 0, 480, 270});
    auto& classes = panel.add<luna::engine::TextField>(luna::engine::Rect{10, 10, 200, 11}, "Classes", "trader, ", 90, [](const std::string&) {});
    help.apply(panel, "t");
    panel.handle(at(60, 14, true));
    panel.handle(typed("h"));
    CHECK(classes.suggestions().rows().front() == "healer");
}

TEST_CASE("US-302 Catalogs: every catalog the design names gives names from the data in use") {
    Studio studio("us302-catalogs");
    const game::OdysseyGame& odyssey = *studio.odyssey;
    const auto has = [&](const std::string& catalog, const std::string& name) {
        const std::vector<std::string> names = odyssey.suggestionNames(catalog);
        return std::find(names.begin(), names.end(), name) != names.end();
    };
    CHECK(has("npc-classes", "trader"));
    CHECK(has("npc-kinds", "goblin"));
    CHECK(has("partner-types", "player"));
    CHECK(has("interactions", "gather"));
    CHECK(has("interaction-fields", "gather.delay"));
    CHECK(has("interaction-fields", "gather.duration"));
    CHECK_FALSE(odyssey.suggestionNames("light-kinds").empty());
    CHECK(has("plants", "wheat"));
    CHECK(has("characters", "wolf"));
    CHECK(has("items", "berries"));
    CHECK_FALSE(odyssey.suggestionNames("building-kinds").empty());
    CHECK_FALSE(odyssey.suggestionNames("quests").empty());
    CHECK(has("levels", "suggest-level"));
    CHECK_FALSE(odyssey.suggestionNames("tags").empty());
    CHECK_FALSE(odyssey.suggestionNames("markers").empty());
    CHECK(has("markers", "npc:grey")); // the placed wolf, by its name as a quest says it
    CHECK(odyssey.suggestionNames("not-a-catalog").empty());
    // objects, prefabs and places exist as names too (the shipped data may hold none of them yet).
    (void)odyssey.suggestionNames("objects");
    (void)odyssey.suggestionNames("prefabs");
    (void)odyssey.suggestionNames("places");
}

TEST_CASE("US-302 Values: a fixed list, and a field whose entry says none offers nothing") {
    Studio studio("us302-values");
    game::EditorHelp help = helpWith(studio, "\"a.who\": { \"purpose\": \"p\", \"example\": \"e\", \"suggest\": \"values:elder|friend\" }, "
                                             "\"a.title\": { \"purpose\": \"p\", \"example\": \"e\", \"suggest\": \"none\" }");
    CHECK(help.suggestionsFor("a.who") == std::vector<std::string>{"elder", "friend"});
    CHECK(help.suggestionsFor("a.title").empty());
    CHECK(help.suggestionsFor("a.unknown").empty());
    luna::engine::Panel panel({0, 0, 480, 270});
    auto& who = panel.add<luna::engine::TextField>(luna::engine::Rect{10, 10, 200, 11}, "who", "", 20, [](const std::string&) {});
    auto& title = panel.add<luna::engine::TextField>(luna::engine::Rect{10, 30, 200, 11}, "title", "", 20, [](const std::string&) {});
    help.apply(panel, "a");
    panel.handle(at(60, 14, true));
    CHECK(who.suggestions().rows() == std::vector<std::string>{"elder", "friend"});
    panel.handle(at(60, 34, true));
    CHECK_FALSE(title.suggestions().isOpen()); // none: no list, the field works as before
}

TEST_CASE("US-302 Mistakes: an entry with no suggest, or one that names an unknown source, is a mistake of the file") {
    const fs::path folder = dataCopy("us302-mistakes").parent_path();
    writeText(folder / "help.json", "{ \"version\": 1,\n \"fields\": {\n  \"a.b\": { \"purpose\": \"p\", \"example\": \"x\" },\n  \"a.c\": { \"purpose\": \"p\", \"example\": \"x\", \"suggest\": \"magic\" } } }");
    game::EditorHelp help;
    help.load(folder / "help.json");
    REQUIRE(help.problems().size() == 2);
    CHECK(help.problems()[0].find("help.json:3: \"a.b\" has no suggest") != std::string::npos);
    CHECK(help.problems()[1].find("help.json:4: \"a.c\"") != std::string::npos);
}

TEST_CASE("US-302 Wired: every field whose entry offers values got its list, in all four editors") {
    Studio studio("us302-wired");
    game::Editor& editor = studio.odyssey->editor();
    // The same tour the coverage test of US-300 takes is too long to repeat; the panels the two cases need are enough here.
    editor.select(studio.wolf);
    editor.showSettings(true);
    editor.showEconomy(true);
    editor.showClasses(true);
    editor.newClass();
    for (int i = 0; i < 3; ++i) studio.odyssey->update({});
    const game::EditorHelp& help = studio.odyssey->editorHelp();
    REQUIRE(help.problems().empty());
    for (const char* id : {"npc.hp", "npc.sword", "level.width", "level.height", "economy.money", "class.tags", "class.allow", "class.deny", "class.does"}) {
        CHECK_MESSAGE(help.wired().contains(id), id);
    }
    CHECK_FALSE(help.wired().contains("level.name")); // none
}

namespace {

// The folder named by the environment variable ODYSSEUS_EVIDENCE_DIR, or empty: a story's screenshots are written there only when asked for.
std::string evidenceFolder() {
#ifdef _MSC_VER
    char* value = nullptr;
    std::size_t size = 0;
    std::string folder;
    if (_dupenv_s(&value, &size, "ODYSSEUS_EVIDENCE_DIR") == 0 && value != nullptr) folder = value;
    std::free(value);
    return folder;
#else
    const char* value = std::getenv("ODYSSEUS_EVIDENCE_DIR");
    return value != nullptr ? value : "";
#endif
}

} // namespace

TEST_CASE("US-302 Screen: clicking the HP field of a placed wolf opens its list in the Editor") {
    const fs::path data = dataCopy("us302-screen");
    const game::Definitions definitions = game::loadDefinitions(data);
    game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
    level.characters.clear();
    level.pickups.clear();
    const int wolf = level.nextId++;
    level.characters.push_back({wolf, "wolf", {level.heroStart.x + 64, level.heroStart.y}, game::Facing::South, "Grey", 50, 8, {}});
    game::saveLevel(level, definitions, data / "screen-level.json");

    luna::engine::ImageRenderer renderer(960, 540);
    game::OdysseyGame odyssey(data, data / "screen-level.json");
    odyssey.setViewScales(1, 1);
    odyssey.start(renderer);
    odyssey.update(pressing(luna::engine::Intent::ModeEditor));
    odyssey.editor().select(wolf);
    odyssey.update({});
    odyssey.update({});

    luna::engine::Pointer click; // the HP field: the third row of the properties panel, top right
    click.x = 960 - 136 - 2 + 40;
    click.y = 18 + 4 + 32 + 4;
    click.pressed[0] = true;
    luna::engine::Intents press;
    press.setPointer(click);
    odyssey.update(press);
    click.pressed[0] = false;
    luna::engine::Intents rest;
    rest.setPointer(click);
    odyssey.update(rest);
    renderer.clear({0, 0, 0, 255});
    odyssey.render(renderer, 0.0);
    // The list's top-left corner sits just under the field (the colour of an outline).
    CHECK(renderer.image().get(click.x - 40 + 28 + 1, click.y + 7 + 1) != luna::engine::Color{0, 0, 0, 255});
    if (const std::string evidence = evidenceFolder(); !evidence.empty()) {
        fs::create_directories(evidence);
        luna::engine::savePng(renderer.image(), fs::path(evidence) / "list-npc-hp.png");
    }
}
