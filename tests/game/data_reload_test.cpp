// US-303 Live reload of every data file: the registry, the sets of the game (lights, plants and objects, interactions, help), all or nothing, placed things
// that follow the new data, a red marker for a kind that is gone, and the time each reload takes (NFR-09).
#include "camp.h"

#include "game/data_reload.h"
#include "luna/engine/image_io.h"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <format>
#include <memory>
#include <string>
#include <vector>

using namespace camp_support;

namespace {

luna::engine::Intents pressing(luna::engine::Intent intent) {
    luna::engine::Intents intents;
    intents.set(intent, true, true);
    return intents;
}

std::string replaced(std::string text, const std::string& from, const std::string& to) {
    const std::size_t at = text.find(from);
    REQUIRE_MESSAGE(at != std::string::npos, from);
    return text.replace(at, from.size(), to);
}

// A game in the Editor on a copy of the data folder, with a placed wheat plant, a placed campfire light and a placed goblin.
struct Studio {
    fs::path data;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;
    int plant = 0;
    int light = 0;
    int goblin = 0;

    explicit Studio(const std::string& name, const std::function<void(const fs::path&)>& adjust = {}, const std::string& lightKind = "campfire") : data(dataCopy(name)) {
        if (adjust) adjust(data);
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        level.plants.clear();
        level.lights.clear();
        plant = level.nextId++;
        level.plants.push_back({plant, "wheat", {level.heroStart.x + 96, level.heroStart.y}});
        light = level.nextId++;
        level.lights.push_back({light, lightKind, {level.heroStart.x + 128, level.heroStart.y}});
        goblin = level.nextId++;
        level.characters.push_back({goblin, "goblin", {level.heroStart.x + 64, level.heroStart.y}, game::Facing::South, "Ossa", 60, 4, {}});
        game::saveLevel(level, definitions, data / "reload-level.json");
        odyssey = std::make_unique<game::OdysseyGame>(data, data / "reload-level.json");
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
        odyssey->update(pressing(luna::engine::Intent::ModeEditor));
    }
    game::OdysseyGame& game() { return *odyssey; }
    void write(const std::string& relative, const std::string& text) { writeText(data / relative, text); }
    std::string read(const std::string& relative) const { return readText(data / relative); }
    // What an Editor save or the watcher does: names the file that changed.
    std::vector<game::ReloadOutcome> changed(const std::string& relative) {
        odyssey->fileChanged(data / relative);
        std::vector<game::ReloadOutcome> outcomes;
        for (const std::string& set : odyssey->dataReload().setsFor(data / relative)) {
            if (const game::ReloadOutcome* last = odyssey->dataReload().last(set)) outcomes.push_back(*last);
        }
        return outcomes;
    }
};

// The folder named by the environment variable ODYSSEUS_EVIDENCE_DIR, or empty: a story's evidence is written there only when asked for.
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

TEST_CASE("US-303 Registry: a file finds the sets that watch it, a folder watches what is under it, and each set runs once") {
    game::DataReload reloads;
    int lights = 0;
    int quests = 0;
    int later = 0;
    const fs::path root = fs::temp_directory_path() / "odysseus-us303-registry";
    reloads.add({"lights", {root / "light" / "lights.json"}, [&] { ++lights; return game::ReloadResult{}; }});
    reloads.add({"quests", {root / "quests", root / "dialogue"}, [&] { ++quests; return game::ReloadResult{}; }});
    reloads.add({"next-start", {root / "light", root / "sim"}, [&] { ++later; game::ReloadResult result; result.atNextStart = true; return result; }, true});

    CHECK(reloads.setsFor(root / "light" / "lights.json") == std::vector<std::string>{"lights", "next-start"});
    CHECK(reloads.setsFor(root / "quests" / "a" / "b.json") == std::vector<std::string>{"quests"});
    CHECK(reloads.setsFor(root / "quests-extra" / "a.json").empty()); // a folder name that only starts the same is not under it
    CHECK(reloads.setsFor(root / "dialogue/../quests/x.json") == std::vector<std::string>{"quests"}); // paths are compared normalised

    auto outcomes = reloads.changed(root / "light" / "lights.json");
    REQUIRE(outcomes.size() == 1); // a live set takes the file; the "next start" set is not also told
    CHECK(outcomes[0].set == "lights");
    CHECK(outcomes[0].file == root / "light" / "lights.json");
    CHECK(lights == 1);
    CHECK(later == 0);
    outcomes = reloads.changed(root / "light" / "sky.json"); // only the folder of the "next start" set watches it
    REQUIRE(outcomes.size() == 1);
    CHECK(outcomes[0].result.atNextStart);
    CHECK(later == 1);
    outcomes = reloads.reloadAll(); // F5: the "next start" set is left alone
    CHECK(outcomes.size() == 2);
    CHECK(lights == 2);
    CHECK(quests == 1);
    CHECK(later == 1);
    REQUIRE(reloads.last("quests") != nullptr);
    CHECK(reloads.last("quests")->result.milliseconds >= 0.0);
    CHECK(reloads.last("nothing") == nullptr);
    CHECK(reloads.reload("nothing").empty());
}

TEST_CASE("US-303 Catalog: a light kind's colour saved in lights.json shines in the new colour, and the Editor offers the kinds") {
    Studio studio("us303-lights");
    REQUIRE(studio.game().lighting().kind("campfire") != nullptr);
    CHECK(studio.game().lighting().kind("campfire")->red == 255);
    studio.write("light/lights.json", replaced(studio.read("light/lights.json"), "\"name\": \"campfire\", \"color\": [255, 199, 115]", "\"name\": \"campfire\", \"color\": [10, 20, 30]"));
    const auto outcomes = studio.changed("light/lights.json");
    REQUIRE(outcomes.size() == 1);
    CHECK(outcomes[0].set == "lights");
    CHECK(outcomes[0].result.ok);
    const game::LightKindDef* campfire = studio.game().lighting().kind("campfire");
    REQUIRE(campfire != nullptr);
    CHECK(campfire->red == 10);
    CHECK(campfire->green == 20);
    CHECK(campfire->blue == 30);
    CHECK(studio.game().toast() == "Reloaded lights");
    CHECK(studio.game().toastTicks() > 0);
    // A new kind joins the names the Editor offers, with no restart.
    studio.write("light/lights.json", replaced(studio.read("light/lights.json"), "{ \"name\": \"torch\"", "{ \"name\": \"lantern\", \"color\": [1, 2, 3], \"radiusTiles\": 3.0, \"strength\": 0.5, \"height\": 20 },\n    { \"name\": \"torch\""));
    studio.changed("light/lights.json");
    const auto& kinds = studio.game().definitions().lightKinds;
    CHECK(std::find(kinds.begin(), kinds.end(), "lantern") != kinds.end());
}

TEST_CASE("US-303 Object: an object added to objects.json is a kind of the catalog and a name of the palette without a restart") {
    Studio studio("us303-object");
    CHECK(studio.game().catalogs().plant("cooking pot") == nullptr);
    std::string objects = studio.read("objects.json");
    objects = replaced(objects, "{ \"name\": \"knapping stone\"", "{ \"name\": \"cooking pot\", \"frame\": \"cooking-pot\", \"blocks\": false, \"inspect\": \"A clay pot.\", \"tags\": [\"object\", \"pot\"] },\n    { \"name\": \"knapping stone\"");
    studio.write("objects.json", objects);
    const auto outcomes = studio.changed("objects.json");
    REQUIRE(outcomes.size() == 1);
    CHECK(outcomes[0].set == "catalog");
    CHECK(outcomes[0].result.ok);
    const game::PlantDef* pot = studio.game().catalogs().plant("cooking pot");
    REQUIRE(pot != nullptr);
    CHECK(pot->object);
    const auto& names = studio.game().definitions().objects;
    CHECK(std::find(names.begin(), names.end(), "cooking pot") != names.end()); // the object page of the palette is built from these names
    CHECK(studio.game().suggestionNames("objects").size() == names.size());
    studio.odyssey->update({});                                                  // the Editor and the game run on with the new data
    studio.odyssey->update(pressing(luna::engine::Intent::ModeGame));
    studio.odyssey->update({});
}

TEST_CASE("US-303 Follow: a placed plant takes the new values of its kind, and keeps its id") {
    Studio studio("us303-follow");
    studio.odyssey->update(pressing(luna::engine::Intent::ModeGame)); // the plants stand in the play state
    REQUIRE(studio.game().plants().size() == 1);
    CHECK(studio.game().plants()[0].def->inspect == "Something to eat grows here.");
    studio.write("plants.json", replaced(studio.read("plants.json"), "Something to eat grows here.", "Grain, nearly ripe."));
    studio.changed("plants.json");
    REQUIRE(studio.game().plants().size() == 1);
    CHECK(studio.game().plants()[0].id == studio.plant);
    REQUIRE(studio.game().plants()[0].def != nullptr);
    CHECK(studio.game().plants()[0].def->inspect == "Grain, nearly ripe.");
    CHECK(studio.game().plants()[0].def == studio.game().catalogs().plant("wheat")); // not a pointer into the catalog that went
}

TEST_CASE("US-303 All or nothing: a broken interaction file keeps the last good data and the panel names the file and line") {
    Studio studio("us303-broken");
    const std::size_t before = studio.game().interactionReport().loaded;
    REQUIRE(before > 0);
    const std::string good = studio.read("interactions/gather.json");
    studio.write("interactions/gather.json", replaced(good, "\"range\": 2,", "\"range\": 2,,"));
    const auto outcomes = studio.changed("interactions/gather.json");
    REQUIRE_FALSE(outcomes.empty());
    CHECK(outcomes[0].set == "interactions");
    CHECK_FALSE(outcomes[0].result.ok);
    REQUIRE_FALSE(outcomes[0].result.errors.empty());
    CHECK(outcomes[0].result.errors[0].find("gather.json:") != std::string::npos); // the file and the line
    CHECK(studio.game().toast().rfind("Not reloaded", 0) == 0);
    // The last good data is still in use: the other interactions and the old gather work.
    CHECK_FALSE(studio.game().interactionReport().errors.empty());
    studio.write("interactions/gather.json", good);
    studio.changed("interactions/gather.json");
    CHECK(studio.game().interactionReport().errors.empty()); // fixed: the panel clears
    CHECK(studio.game().interactionReport().loaded == before);
}

TEST_CASE("US-303 All or nothing: a broken lights.json or objects.json changes nothing and says why") {
    Studio studio("us303-broken-data");
    const int red = studio.game().lighting().kind("campfire")->red;
    studio.write("light/lights.json", "{ \"version\": 1, \"lights\": [ { \"name\": \"campfire\", ");
    const auto lights = studio.changed("light/lights.json");
    REQUIRE(lights.size() == 1);
    CHECK_FALSE(lights[0].result.ok);
    CHECK(studio.game().lighting().kind("campfire")->red == red);
    CHECK_FALSE(studio.game().reloadErrors().empty());
    studio.write("light/lights.json", readText(fs::path(ODYSSEUS_DATA_DIR) / "light" / "lights.json"));
    studio.changed("light/lights.json");
    CHECK(studio.game().reloadErrors().empty()); // the file is whole again

    const std::size_t plants = studio.game().catalogs().plants.size();
    studio.write("objects.json", "{ \"objects\": [ { \"name\": \"fire pit\" } ] }"); // a frame is missing
    const auto objects = studio.changed("objects.json");
    REQUIRE(objects.size() == 1);
    CHECK_FALSE(objects[0].result.ok);
    CHECK(studio.game().catalogs().plants.size() == plants); // nothing was taken
    CHECK(studio.game().catalogs().plant("fire pit") != nullptr);
}

TEST_CASE("US-303 Missing kind: a placed thing whose kind is gone gets a red marker and a warning, is skipped by play, and the game runs on") {
    Studio studio("us303-missing");
    studio.odyssey->update(pressing(luna::engine::Intent::ModeGame));
    REQUIRE(studio.game().plants().size() == 1);
    CHECK(studio.game().missingKinds().empty());
    // The owner deletes wheat from plants.json.
    std::string plants = studio.read("plants.json");
    const std::size_t start = plants.find("    {\"name\":\"wheat\"");
    REQUIRE(start != std::string::npos);
    const std::size_t end = plants.find('\n', start);
    plants.erase(start, end - start + 1);
    studio.write("plants.json", plants);
    const auto outcomes = studio.changed("plants.json");
    REQUIRE(outcomes.size() == 1);
    CHECK(outcomes[0].result.ok);
    REQUIRE(studio.game().missingKinds().size() == 1);
    const game::MissingKind& missing = studio.game().missingKinds()[0];
    CHECK(missing.what.find("plant #" + std::to_string(studio.plant)) != std::string::npos); // names the level entry
    CHECK(missing.what.find("\"wheat\"") != std::string::npos);
    CHECK(missing.what.find("reload-level") == std::string::npos); // (the level name, not the file: "The Valley")
    REQUIRE_FALSE(outcomes[0].result.warnings.empty());
    CHECK(outcomes[0].result.warnings[0] == missing.what);
    REQUIRE(studio.game().plants().size() == 1);
    CHECK(studio.game().plants()[0].def == nullptr); // skipped by play
    studio.odyssey->update({});
    studio.odyssey->update({});
    // The wheat comes back with the file: the marker goes and the plant stands again.
    studio.write("plants.json", readText(fs::path(ODYSSEUS_DATA_DIR) / "plants.json"));
    studio.changed("plants.json");
    CHECK(studio.game().plants()[0].def != nullptr);
    bool stillMissing = false;
    for (const game::MissingKind& entry : studio.game().missingKinds()) stillMissing = stillMissing || entry.what.find("plant #") != std::string::npos;
    CHECK_FALSE(stillMissing);
}

TEST_CASE("US-303 Missing light: a placed light whose kind left lights.json is marked, and shines again when the kind comes back") {
    const std::string lantern = "{ \"name\": \"lantern\", \"color\": [1, 2, 3], \"radiusTiles\": 3.0, \"strength\": 0.5, \"height\": 20 },\n    ";
    Studio studio("us303-missing-light", [&](const fs::path& data) {
        writeText(data / "light" / "lights.json", replaced(readText(data / "light" / "lights.json"), "{ \"name\": \"torch\"", lantern + "{ \"name\": \"torch\""));
    }, "lantern");
    CHECK(studio.game().missingKinds().empty()); // the lantern exists
    const std::string withLantern = studio.read("light/lights.json");
    studio.write("light/lights.json", replaced(withLantern, lantern, "")); // the owner deletes the kind
    const auto gone = studio.changed("light/lights.json");
    REQUIRE(gone.size() == 1);
    CHECK(gone[0].result.ok);
    REQUIRE(studio.game().missingKinds().size() == 1);
    CHECK(studio.game().missingKinds()[0].what.find("light #" + std::to_string(studio.light) + " \"lantern\"") != std::string::npos);
    CHECK_FALSE(gone[0].result.warnings.empty());
    studio.write("light/lights.json", withLantern); // and puts it back
    studio.changed("light/lights.json");
    CHECK(studio.game().missingKinds().empty());
}

TEST_CASE("US-303 Next start: a file that cannot be swapped is reported, never half-applied; F5 leaves it alone") {
    Studio studio("us303-later");
    studio.write("tiles.json", studio.read("tiles.json")); // a tile is a number in the map: it stays at the next start (weather.json is live since US-191)
    const auto outcomes = studio.changed("tiles.json");
    REQUIRE(outcomes.size() == 1);
    CHECK(outcomes[0].set == "next-start");
    CHECK(outcomes[0].result.atNextStart);
    CHECK(studio.game().toast() == "tiles.json applies at the next start");
    studio.game().reloadEverything();
    CHECK(studio.game().toast().rfind("Reloaded", 0) == 0); // F5: the live sets
    CHECK(studio.game().toast().find("next-start") == std::string::npos);
}

TEST_CASE("US-303 Help: a clean help.json is taken, a broken one keeps the entries in use") {
    Studio studio("us303-help");
    const std::size_t entries = studio.game().editorHelp().entries().size();
    REQUIRE(entries > 0);
    studio.write("editor/help.json", "{ \"version\": 1,\n \"fields\": { \"npc.sword\": { \"purpose\": \"Damage\" } } }"); // no example, no suggest
    const auto broken = studio.changed("editor/help.json");
    REQUIRE(broken.size() == 1);
    CHECK_FALSE(broken[0].result.ok);
    CHECK(studio.game().editorHelp().entries().size() == entries);
    studio.write("editor/help.json", readText(fs::path(ODYSSEUS_DATA_DIR) / "editor" / "help.json"));
    studio.write("editor/help.json", replaced(studio.read("editor/help.json"), "Damage of one sword strike by this character", "Strike damage"));
    const auto clean = studio.changed("editor/help.json");
    CHECK(clean[0].result.ok);
    REQUIRE(studio.game().editorHelp().find("npc.sword") != nullptr);
    CHECK(studio.game().editorHelp().find("npc.sword")->purpose == "Strike damage");
}

TEST_CASE("US-303 Timing: every data set reloads in well under 100 ms (NFR-09)") {
    Studio studio("us303-timing");
    std::string report = "US-303 reload times on the shipped data, ms (budget 100 per set, NFR-09)\n";
#ifdef NDEBUG
    constexpr double kBudget = 100.0;
#else
    // A Debug build with AddressSanitizer is several times slower; the Release run checks 100. GitHub's shared runners are slower again (X-M11: 1340 ms there, well under
    // 1000 on the owner's PC), so the Debug sanity limit is five times wider there.
#pragma warning(suppress : 4996)
    const double kBudget = std::getenv("GITHUB_ACTIONS") != nullptr ? 5000.0 : 1000.0;
#endif
    for (int round = 0; round < 2; ++round) { // the second round: files in the operating system's cache, like a save during play
        const auto outcomes = studio.game().dataReload().reloadAll();
        REQUIRE(outcomes.size() == 6); // interactions, npc-classes, lights, catalog, mechanics, help
        for (const game::ReloadOutcome& outcome : outcomes) {
            CHECK_MESSAGE(outcome.result.ok, outcome.set);
            CHECK_MESSAGE(outcome.result.milliseconds < kBudget, outcome.set << " took " << outcome.result.milliseconds << " ms");
            if (round == 1) report += std::format("{:<14} {:8.1f}\n", outcome.set, outcome.result.milliseconds);
        }
    }
    MESSAGE(report);
    if (const std::string evidence = evidenceFolder(); !evidence.empty()) {
        fs::create_directories(evidence);
#ifdef NDEBUG
        writeText(fs::path(evidence) / "timings-release.txt", report);
#else
        writeText(fs::path(evidence) / "timings-debug.txt", report);
#endif
    }
}

TEST_CASE("US-303 Screen: the red marker stands where the placed plant was") {
    const fs::path data = dataCopy("us303-screen");
    const game::Definitions definitions = game::loadDefinitions(data);
    game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
    level.characters.clear();
    level.pickups.clear();
    level.plants.clear();
    const int plant = level.nextId++;
    level.plants.push_back({plant, "wheat", {level.heroStart.x + 64, level.heroStart.y}});
    game::saveLevel(level, definitions, data / "screen-level.json");
    luna::engine::ImageRenderer renderer(960, 540);
    game::OdysseyGame odyssey(data, data / "screen-level.json");
    odyssey.setViewScales(1, 1);
    odyssey.start(renderer);
    odyssey.update(pressing(luna::engine::Intent::ModeEditor));
    std::string plants = readText(data / "plants.json");
    const std::size_t start = plants.find("    {\"name\":\"wheat\"");
    REQUIRE(start != std::string::npos);
    plants.erase(start, plants.find('\n', start) - start + 1);
    writeText(data / "plants.json", plants);
    odyssey.fileChanged(data / "plants.json");
    odyssey.update({});
    renderer.clear({0, 0, 0, 255});
    odyssey.render(renderer, 0.0);
    // The red of the marker's frame is somewhere on the screen.
    bool found = false;
    for (int y = 0; y < 540 && !found; ++y) {
        for (int x = 0; x < 960 && !found; ++x) {
            const luna::engine::Color c = renderer.image().get(x, y);
            found = c.red > 200 && c.green < 90 && c.blue < 90;
        }
    }
    CHECK(found);
    if (const std::string evidence = evidenceFolder(); !evidence.empty()) {
        fs::create_directories(evidence);
        luna::engine::savePng(renderer.image(), fs::path(evidence) / "missing-marker.png");
    }
}
