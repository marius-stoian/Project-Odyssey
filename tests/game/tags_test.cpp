// US-151: tags and states on the catalogs, and what a thing offers (smart objects).
#include "game/catalogs.h"
#include "game/level.h"
#include "game/odyssey_game.h"
#include "luna/engine/renderer.h"
#include "sim/data.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace game = odysseus::game;

namespace {

bool has(const std::vector<std::string>& list, const std::string& word) { return std::find(list.begin(), list.end(), word) != list.end(); }

std::string readText(const fs::path& file) {
    std::ifstream in(file, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

void writeText(const fs::path& file, const std::string& text) { std::ofstream(file, std::ios::binary) << text; }

// A copy of the data folder (so a test may edit it), with `extraPlant` added at the top of plants.json when given.
fs::path dataCopy(const std::string& name, const std::string& extraPlant) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us151" / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    fs::copy(ODYSSEUS_DATA_DIR, folder / "data", fs::copy_options::recursive);
    if (!extraPlant.empty()) {
        std::string text = readText(folder / "data" / "plants.json");
        const std::string marker = "\"plants\": [";
        const std::size_t at = text.find(marker);
        REQUIRE(at != std::string::npos);
        text.insert(at + marker.size(), "\n" + extraPlant + ",");
        writeText(folder / "data" / "plants.json", text);
    }
    return folder / "data";
}

// A mango: not flagged edible, but written as tagged edible and plant, with two states.
const char* const kMango = R"({"name":"mango","frame":"bush","size":"tall","blocks":false,"edible":false,"inspect":"Sweet and heavy.","tags":["edible","plant"],"states":["ripe","picked"]})";

// The demo level with these plants (kind, metres east of the hero) and nothing else to get in the way.
fs::path levelWith(const fs::path& data, const std::string& name, const std::vector<std::pair<std::string, int>>& plants) {
    const fs::path folder = data.parent_path() / ("level-" + name);
    fs::create_directories(folder);
    const game::Definitions definitions = game::loadDefinitions(data);
    game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
    level.pickups.clear();
    level.characters.clear();
    for (const auto& [kind, metres] : plants) level.plants.push_back({level.nextId++, kind, {level.heroStart.x + 32 * metres, level.heroStart.y}});
    game::saveLevel(level, definitions, folder / "level.json");
    return folder / "level.json";
}

} // namespace

TEST_CASE("US-151 Catalog entries get tags, and written tags replace them") {
    const game::Catalogs catalogs = game::loadCatalogs(ODYSSEUS_DATA_DIR);
    const game::PlantDef* wheat = catalogs.plant("wheat");
    REQUIRE(wheat != nullptr);
    CHECK(wheat->tags == std::vector<std::string>{"plant", "edible"});
    CHECK(wheat->states == std::vector<std::string>{"ripe", "picked"});
    const game::PlantDef* clover = catalogs.plant("clover");
    REQUIRE(clover != nullptr);
    CHECK(clover->tags == std::vector<std::string>{"plant"});
    CHECK(clover->states.empty()); // a plant that never changes has no state
    for (const game::PlantDef& plant : catalogs.plants) {
        CHECK_MESSAGE(has(plant.tags, "plant"), plant.name);
        if (plant.edible && !plant.blocks) CHECK_MESSAGE(has(plant.tags, "edible"), plant.name);
        if (plant.edible && plant.blocks) CHECK_MESSAGE((has(plant.tags, "fruit-bearing") && !has(plant.tags, "edible")), plant.name); // chopped, not gathered
        if (plant.blocks) CHECK_MESSAGE(has(plant.tags, "solid"), plant.name);
    }
    for (const game::AnimalDef& animal : catalogs.animals) {
        CHECK_MESSAGE(has(animal.tags, "animal"), animal.name);
        CHECK_MESSAGE(has(animal.tags, animal.enemy ? "hostile" : "prey"), animal.name);
    }
    for (const game::WeaponDef& weapon : catalogs.weapons) {
        CHECK_MESSAGE((has(weapon.tags, "weapon") && has(weapon.tags, "item") && has(weapon.tags, game::weaponClassName(weapon.weaponClass))), weapon.name);
    }
    const game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
    REQUIRE(definitions.character("hero") != nullptr);
    CHECK(definitions.character("hero")->tags == std::vector<std::string>{"hero", "person"});
    REQUIRE(definitions.character("goblin") != nullptr);
    CHECK(definitions.character("goblin")->tags == std::vector<std::string>{"hostile"});

    const std::set<std::string> known = catalogs.knownTags();
    for (const char* tag : {"plant", "edible", "solid", "tree", "animal", "hostile", "prey", "weapon", "item", "sword", "starter"}) CHECK_MESSAGE(known.count(tag) == 1, tag);

    // Written tags and states replace the derived ones.
    const game::Catalogs changed = game::loadCatalogs(dataCopy("mango", kMango));
    const game::PlantDef* mango = changed.plant("mango");
    REQUIRE(mango != nullptr);
    CHECK(mango->tags == std::vector<std::string>{"edible", "plant"});
    CHECK(mango->states == std::vector<std::string>{"ripe", "picked"});
}

TEST_CASE("US-151 Bad tags and states are named with their file and field") {
    const auto problem = [](const std::string& entry) {
        try {
            game::loadCatalogs(dataCopy("bad", entry));
        } catch (const odysseus::sim::DataError& error) {
            return std::string(error.what());
        }
        return std::string();
    };
    const std::string base = R"("name":"zz","frame":"bush","size":"tall","blocks":false,"edible":false,"inspect":"x")";
    CHECK(problem("{" + base + R"(,"tags":["not a word"]})").find("plants.json: plants[0].tags[0]: must be one word") != std::string::npos);
    CHECK(problem("{" + base + R"(,"tags":["a","a"]})").find("plants[0].tags[1]: \"a\" is listed twice") != std::string::npos);
    CHECK(problem("{" + base + R"(,"tags":"plant"})").find("plants[0].tags: must be a list of words") != std::string::npos);
    CHECK(problem("{" + base + R"(,"states":[1]})").find("plants[0].states[0]") != std::string::npos);
    CHECK(problem("{" + base + R"(,"tags":["plant"],"states":["ripe"]})").empty());
}

TEST_CASE("US-151 A new plant kind tagged edible and plant is offered Gather and Inspect with no code change") {
    const fs::path data = dataCopy("advertise", kMango);
    game::OdysseyGame odyssey(data, levelWith(data, "advertise", {{"mango", 1}, {"clover", 1}, {"mango", 10}}));
    REQUIRE(odyssey.interactionReport().errors.empty());
    REQUIRE(odyssey.plants().size() == 3);

    auto offers = odyssey.plantOffers(0); // the mango next to the hero
    REQUIRE(offers.size() == 2);
    CHECK(offers[0].interaction->id == "gather");
    CHECK(offers[0].enabled);
    CHECK(offers[1].interaction->id == "inspect"); // menu order: gather 100, inspect 900
    CHECK(offers[1].enabled);

    offers = odyssey.plantOffers(1); // a clover: nothing to gather
    REQUIRE(offers.size() == 1);
    CHECK(offers[0].interaction->id == "inspect");

    offers = odyssey.plantOffers(2); // the far mango: gather is greyed out, inspect still works
    REQUIRE(offers.size() == 2);
    CHECK_FALSE(offers[0].enabled);
    CHECK(offers[0].reason == "Too far away");
    CHECK(offers[1].enabled);
}

TEST_CASE("US-151 A plant in state picked shows Gather disabled with its reason") {
    const fs::path data = dataCopy("picked", ""); // the level is written beside a copy, never into the real assets
    game::OdysseyGame odyssey(data, levelWith(data, "picked", {{"wheat", 1}}));
    REQUIRE(odyssey.plants().size() == 1);
    CHECK(odyssey.plants()[0].state == "ripe"); // the first of its states
    CHECK(odyssey.plantOffers(0)[0].enabled);

    odyssey.setPlantState(0, "picked");
    const auto offers = odyssey.plantOffers(0);
    REQUIRE(offers.size() == 2);
    CHECK(offers[0].interaction->id == "gather");
    CHECK_FALSE(offers[0].enabled);
    CHECK(offers[0].reason == "Nothing to pick yet");
    CHECK(offers[1].enabled); // you can still look at it
    odyssey.setPlantState(0, "ripe");
    CHECK(odyssey.plantOffers(0)[0].enabled);
}

TEST_CASE("US-151 An interaction aimed at a tag no catalog uses gets a warning naming file and tag") {
    const fs::path data = dataCopy("unknown-tag", "");
    writeText(data / "interactions" / "warm.json",
              "{ \"id\": \"warm\", \"label\": \"Warm up\", \"actors\": [\"hero\"], \"target\": { \"tags\": [\"hearth\"] },\n \"effects\": [\"fx fire\"] }");
    game::OdysseyGame odyssey(data, levelWith(data, "unknown-tag", {}));
    CHECK(odyssey.interactionReport().errors.empty()); // a warning, not an error: the file still loads
    CHECK(odyssey.interactions().find("warm") != nullptr);
    REQUIRE(odyssey.interactionReport().warnings.size() == 1);
    CHECK(odyssey.interactionReport().warnings[0].text().find("interactions/warm.json:1: unknown tag \"hearth\"") == 0);
    // The shipped files are clean.
    game::OdysseyGame shipped(ODYSSEUS_DATA_DIR, ODYSSEUS_DEMO_LEVEL);
    CHECK(shipped.interactionReport().warnings.empty());
}

// ---- US-156: F5 reload and the validation panel

namespace {

// gather.json with one text swapped for another, in a copy of the data folder.
void editGather(const fs::path& data, const std::string& from, const std::string& to) {
    const fs::path file = data / "interactions" / "gather.json";
    std::string text = readText(file);
    const std::size_t at = text.find(from);
    REQUIRE_MESSAGE(at != std::string::npos, from);
    text.replace(at, from.size(), to);
    writeText(file, text);
}

luna::engine::Intents reloadPressed() {
    luna::engine::Intents intents;
    intents.set(luna::engine::Intent::Reload, true, true);
    return intents;
}

} // namespace

TEST_CASE("US-156 F5 takes an edited file within a second") {
    const fs::path data = dataCopy("reload", "");
    luna::engine::RecordingRenderer renderer;
    game::OdysseyGame odyssey(data, levelWith(data, "reload", {}));
    odyssey.start(renderer);
    REQUIRE(odyssey.interactions().find("gather")->rangeMilli == 2000);

    editGather(data, "\"range\": 2,", "\"range\": 3,");
    odyssey.update(reloadPressed()); // the F5 key, as the game receives it
    REQUIRE(odyssey.interactions().find("gather") != nullptr);
    CHECK(odyssey.interactions().find("gather")->rangeMilli == 3000);
    CHECK_FALSE(odyssey.interactionPanelOpen());
    CHECK(odyssey.lastInteractionReloadMilliseconds() < 1000.0);

    // A new file appears too.
    writeText(data / "interactions" / "wave.json", "{ \"id\": \"wave\", \"label\": \"Wave\", \"actors\": [\"hero\"], \"target\": { \"tags\": [\"plant\"] }, \"effects\": [\"fx leaves\"] }");
    CHECK(odyssey.reloadInteractions());
    CHECK(odyssey.interactions().find("wave") != nullptr);
}

TEST_CASE("US-156 A bad file lists file:line: message and the last good data stays; fixing it closes the panel") {
    const fs::path data = dataCopy("bad-file", "");
    luna::engine::RecordingRenderer renderer;
    game::OdysseyGame odyssey(data, levelWith(data, "bad-file", {}));
    odyssey.start(renderer);
    REQUIRE_FALSE(odyssey.interactionPanelOpen());
    odyssey.render(renderer, 0.0);
    const std::size_t quietDraws = renderer.draws().size();

    editGather(data, "\"do gather-berries\"", "\"giv gather-berries\"");
    editGather(data, "\"range\": 2,", "\"range\": 3,"); // a good change in the same file must not sneak in
    odyssey.update(reloadPressed());
    REQUIRE(odyssey.interactionPanelOpen());
    REQUIRE(odyssey.interactionReport().errors.size() == 1);
    CHECK(odyssey.interactionReport().errors[0].text().find("interactions/gather.json:") == 0);
    CHECK(odyssey.interactionReport().errors[0].message == "unknown effect verb \"giv\"");
    REQUIRE(odyssey.interactions().find("gather") != nullptr);
    CHECK(odyssey.interactions().find("gather")->rangeMilli == 2000); // the old data, whole

    renderer.clear();
    odyssey.render(renderer, 0.0);
    CHECK(renderer.draws().size() > quietDraws); // the panel is on screen

    editGather(data, "\"giv gather-berries\"", "\"do gather-berries\"");
    odyssey.update(reloadPressed());
    CHECK_FALSE(odyssey.interactionPanelOpen());
    CHECK(odyssey.interactions().find("gather")->rangeMilli == 3000); // now the new data
    renderer.clear();
    odyssey.render(renderer, 0.0);
    CHECK(renderer.draws().size() == quietDraws);
}

TEST_CASE("US-156 A syntax error is reported with its line and a missing folder never crashes") {
    const fs::path data = dataCopy("syntax", "");
    game::OdysseyGame odyssey(data, levelWith(data, "syntax", {}));
    editGather(data, "\"range\": 2,", "\"range\": 2\n  \"order\": 10,"); // a comma is missing
    CHECK_FALSE(odyssey.reloadInteractions());
    REQUIRE(odyssey.interactionReport().errors.size() == 1);
    CHECK(odyssey.interactionReport().errors[0].line > 0);
    CHECK(odyssey.interactionReport().errors[0].message.find("not valid JSON") == 0);

    fs::remove_all(data / "interactions");
    CHECK_FALSE(odyssey.reloadInteractions());
    CHECK(odyssey.interactionPanelOpen());
    CHECK(odyssey.interactions().find("gather") != nullptr); // still the data from before
}

TEST_CASE("US-156 F5 works in the Editor and the panel shows there too") {
    const fs::path data = dataCopy("editor-reload", "");
    luna::engine::RecordingRenderer renderer;
    game::OdysseyGame odyssey(data, levelWith(data, "editor-reload", {}));
    odyssey.start(renderer);
    luna::engine::Intents toEditor;
    toEditor.set(luna::engine::Intent::ModeEditor, true, true);
    odyssey.update(toEditor);
    REQUIRE(odyssey.mode() == game::Mode::Editor);
    odyssey.render(renderer, 0.0);
    const std::size_t quietDraws = renderer.draws().size();

    editGather(data, "\"do gather-berries\"", "\"giv gather-berries\"");
    odyssey.update(reloadPressed());
    REQUIRE(odyssey.interactionPanelOpen());
    renderer.clear();
    odyssey.render(renderer, 0.0);
    CHECK(renderer.draws().size() > quietDraws);

    editGather(data, "\"giv gather-berries\"", "\"do gather-berries\"");
    odyssey.update(reloadPressed());
    CHECK_FALSE(odyssey.interactionPanelOpen());
}
