// US-305 Level edits win over the run save: a run save holds the level baseline (an id and a hash for every placed thing); when the run is loaded, or the Editor saves the
// level while it is loaded, the things the level changed come fresh from it, the ones it removed leave the run and everything else keeps its saved state.
#include "camp.h"

#include "game/game_rules.h"
#include "game/level_baseline.h"
#include "sim/building_store.h"
#include "sim/npc_population.h"

#include <nlohmann/json.hpp>

using namespace camp_support;
namespace sim = odysseus::sim;

namespace {

// The plants of a saved run set to `state` (the file is edited the way a run that had picked them would have written it).
void setPlantStates(const fs::path& things, const std::vector<int>& ids, const std::string& state) {
    nlohmann::json data = nlohmann::json::parse(readText(things));
    for (const int id : ids) data["plants"][std::to_string(id)] = state;
    writeText(things, data.dump(1));
}

const game::WorldPlant* plantById(const game::OdysseyGame& odyssey, int id) {
    for (const game::WorldPlant& plant : odyssey.plants()) {
        if (plant.id == id) return &plant;
    }
    return nullptr;
}

// A camp with three wheat plants (the last three of the level, in order), saved as a run whose plants are all picked; with `people`, two wanderers who have lived two days.
struct Meadow {
    fs::path data;
    fs::path levelFile;
    fs::path saves;
    std::vector<int> plantIds;
    std::vector<int> personIds;
    std::vector<int> savedAge; // the age in days of each person in the save

    explicit Meadow(const std::string& name, bool people = false) : data(dataCopy(name)) {
        Spec spec;
        spec.plants = {{"wheat", 4, 0}, {"wheat", 4, 3}};
        if (people) spec.characters = {{"wanderer", 8, 0}, {"wanderer", 8, 4}};
        Camp camp(name, data, true, {}, spec);
        levelFile = camp.odyssey.levelFile();
        const auto& plants = camp.odyssey.level().plants;
        for (std::size_t i = plants.size() - 3; i < plants.size(); ++i) plantIds.push_back(plants[i].id);
        for (const game::PlacedCharacter& character : camp.odyssey.level().characters) personIds.push_back(character.id);
        camp.play(40);
        if (people) camp.odyssey.npcPopulationMutable().runTicks(2400 * 2);
        for (const int id : personIds) savedAge.push_back(camp.odyssey.npcPopulation().ageDays(camp.odyssey.npcPopulation().indexOf(id)));
        REQUIRE(camp.odyssey.autosave());
        saves = camp.odyssey.saveDirectory();
        setPlantStates(saves / "things.json", plantIds, "picked");
    }

    game::Level level() const { return game::loadLevel(levelFile, game::loadDefinitions(data)).level; }
    void write(const game::Level& edited) const { game::saveLevel(edited, game::loadDefinitions(data), levelFile); }
};

} // namespace

TEST_CASE("US-305 Baseline: every placed thing has an id and a hash, and the text reads back the same") {
    const game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
    game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
    const game::LevelBaseline baseline = game::LevelBaseline::of(level);
    CHECK_FALSE(baseline.empty());
    CHECK(baseline.hashes(game::LevelBaseline::Plant).size() == level.plants.size());
    CHECK(baseline.hashes(game::LevelBaseline::Character).size() == level.characters.size());
    CHECK(game::LevelBaseline::fromText(baseline.toText()) == baseline);
    CHECK_FALSE(game::compareBaseline(baseline, level).any()); // the same level: nothing changed

    game::LevelBaseline damaged = game::LevelBaseline::fromText("{ not json");
    CHECK(damaged.empty());
    CHECK(game::LevelBaseline::fromText("{\"plant\": 3}").empty());
    CHECK_FALSE(game::compareBaseline(damaged, level).any()); // no baseline: nothing to compare with
}

TEST_CASE("US-305 Baseline: moved and new things are updated, deleted things are removed, the rest is untouched") {
    const game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
    game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
    level.plants.push_back({level.nextId++, "wheat", {level.heroStart.x + 64, level.heroStart.y}, {}});
    level.plants.push_back({level.nextId++, "wheat", {level.heroStart.x + 96, level.heroStart.y}, {}});
    level.characters.push_back({level.nextId++, "wanderer", {level.heroStart.x, level.heroStart.y + 64}, game::Facing::South, "Ossa", 60, 4, {}});
    const game::LevelBaseline baseline = game::LevelBaseline::of(level);

    const int moved = level.plants[level.plants.size() - 1].id;
    const int kept = level.plants[level.plants.size() - 2].id;
    level.plants.back().feet.x += 32;                                   // moved
    const int gone = level.characters.back().id;
    level.characters.pop_back();                                         // deleted
    level.effects.push_back({level.nextId++, "flame", level.heroStart}); // new
    const int fresh = level.effects.back().id;

    const game::LevelChanges changes = game::compareBaseline(baseline, level);
    CHECK(changes.of(game::LevelBaseline::Plant).updated == std::set<int>{moved});
    CHECK_FALSE(changes.of(game::LevelBaseline::Plant).touches(kept));
    CHECK(changes.of(game::LevelBaseline::Character).removed == std::set<int>{gone});
    CHECK(changes.of(game::LevelBaseline::Effect).updated == std::set<int>{fresh});
    CHECK(changes.count() == 3);
    CHECK(game::levelChangesMessage(changes) == "The level updated 3 things");
    CHECK(game::levelChangesMessage({}).empty());
}

TEST_CASE("US-305 Edited and Untouched: a bush moved in the level is fresh in the run, the others keep their state") {
    Meadow meadow("merge-edited");
    game::Level edited = meadow.level();
    const int moved = meadow.plantIds[1]; // the first of the two extra plants (the wheat of the camp is [0])
    for (game::PlacedPlant& plant : edited.plants) {
        if (plant.id == moved) plant.feet.x += 64;
    }
    meadow.write(edited);

    luna::engine::RecordingRenderer renderer;
    game::OdysseyGame again(meadow.data, meadow.levelFile);
    again.start(renderer);
    REQUIRE(again.loadAutosave());
    const game::WorldPlant* movedPlant = plantById(again, moved);
    REQUIRE(movedPlant != nullptr);
    for (const game::PlacedPlant& plant : edited.plants) {
        if (plant.id == moved) CHECK(movedPlant->feet.x == plant.feet.x); // in its new place
    }
    CHECK(movedPlant->present());
    CHECK(movedPlant->state == movedPlant->def->states.front()); // fresh, not "picked"
    for (const int id : {meadow.plantIds[0], meadow.plantIds[2]}) {
        REQUIRE(plantById(again, id) != nullptr);
        CHECK(plantById(again, id)->state == "picked"); // untouched: still picked
    }
    CHECK(again.runChanges().count() == 1);
    CHECK(again.message().find("The level updated 1 thing") != std::string::npos);
    CHECK(again.runLoaded());
}

TEST_CASE("US-305 Untouched: with no edit the run loads as saved and says nothing about the level") {
    Meadow meadow("merge-untouched");
    luna::engine::RecordingRenderer renderer;
    game::OdysseyGame again(meadow.data, meadow.levelFile);
    again.start(renderer);
    REQUIRE(again.loadAutosave());
    for (const int id : meadow.plantIds) {
        REQUIRE(plantById(again, id) != nullptr);
        CHECK(plantById(again, id)->state == "picked");
    }
    CHECK_FALSE(again.runChanges().any());
    CHECK(again.message().find("The level updated") == std::string::npos);
}

TEST_CASE("US-305 Timers: what was waiting for a plant the level moved is dropped, the others still count down") {
    Meadow meadow("merge-timers");
    nlohmann::json things = nlohmann::json::parse(readText(meadow.saves / "things.json"));
    const int moved = meadow.plantIds[1];
    const int other = meadow.plantIds[2];
    const int plantKind = static_cast<int>(game::Subject::Kind::Plant);
    things["timers"]["pending"] = nlohmann::json::array({{{"in", 100}, {"actor", 0}, {"kind", plantKind}, {"id", moved}, {"effect", "set target.state ripe"}},
                                                         {{"in", 100}, {"actor", 0}, {"kind", plantKind}, {"id", other}, {"effect", "set target.state ripe"}}});
    writeText(meadow.saves / "things.json", things.dump(1));
    game::Level edited = meadow.level();
    for (game::PlacedPlant& plant : edited.plants) {
        if (plant.id == moved) plant.feet.y += 32;
    }
    meadow.write(edited);

    luna::engine::RecordingRenderer renderer;
    game::OdysseyGame again(meadow.data, meadow.levelFile);
    again.start(renderer);
    REQUIRE(again.loadAutosave());
    for (int i = 0; i < 100; ++i) again.update({});
    CHECK(plantById(again, other)->state == "ripe"); // its timer ran
    CHECK(plantById(again, moved)->state == plantById(again, moved)->def->states.front());
    CHECK(plantById(again, meadow.plantIds[0])->state == "picked"); // no timer: still picked
}

TEST_CASE("US-305 Removed: a person deleted from the level is not in the run, and the status line counts the update") {
    Meadow meadow("merge-removed", true);
    REQUIRE(meadow.personIds.size() == 2);
    const int gone = meadow.personIds[1];
    const int stays = meadow.personIds[0];
    game::Level edited = meadow.level();
    edited.characters.erase(std::remove_if(edited.characters.begin(), edited.characters.end(), [&](const game::PlacedCharacter& c) { return c.id == gone; }), edited.characters.end());
    meadow.write(edited);

    luna::engine::RecordingRenderer renderer;
    game::OdysseyGame again(meadow.data, meadow.levelFile);
    again.start(renderer);
    REQUIRE(again.loadAutosave());
    CHECK(again.npcPopulation().indexOf(gone) < 0);
    REQUIRE(again.npcPopulation().indexOf(stays) >= 0);
    CHECK(again.npcPopulation().ageDays(again.npcPopulation().indexOf(stays)) == meadow.savedAge[0]); // the one who stays keeps their saved life
    CHECK(again.runChanges().of(game::LevelBaseline::Character).removed == std::set<int>{gone});
    CHECK(again.message().find("The level updated 1 thing") != std::string::npos);
}

TEST_CASE("US-305 Changed person: a person the level changed starts fresh, with the others as saved") {
    Meadow meadow("merge-person", true);
    REQUIRE(meadow.personIds.size() == 2);
    game::Level edited = meadow.level();
    for (game::PlacedCharacter& character : edited.characters) {
        if (character.id == meadow.personIds[1]) character.name = "Renamed";
    }
    meadow.write(edited);

    luna::engine::RecordingRenderer renderer;
    game::OdysseyGame again(meadow.data, meadow.levelFile);
    again.start(renderer);
    REQUIRE(again.loadAutosave());
    const sim::NpcPopulation& people = again.npcPopulation();
    REQUIRE(people.indexOf(meadow.personIds[0]) >= 0);
    REQUIRE(people.indexOf(meadow.personIds[1]) >= 0);
    CHECK(people.ageDays(people.indexOf(meadow.personIds[0])) == meadow.savedAge[0]);     // saved: two days lived
    CHECK(people.ageDays(people.indexOf(meadow.personIds[1])) == meadow.savedAge[1] - 2); // fresh: made from the level, without the two days
    CHECK(again.runChanges().of(game::LevelBaseline::Character).updated == std::set<int>{meadow.personIds[1]});
}
TEST_CASE("US-305 Old save: a save without a baseline loads as before once and is written with one") {
    Meadow meadow("merge-old");
    // The save as M10a wrote it: version 2, no baseline.
    nlohmann::json things = nlohmann::json::parse(readText(meadow.saves / "things.json"));
    REQUIRE(things.contains("baseline"));
    CHECK(things.at("version").get<int>() == 3);
    things.erase("baseline");
    things["version"] = 2;
    writeText(meadow.saves / "things.json", things.dump(1));

    game::Level edited = meadow.level();
    for (game::PlacedPlant& plant : edited.plants) plant.feet.x += 32; // the owner moved everything since the save
    meadow.write(edited);

    luna::engine::RecordingRenderer renderer;
    game::OdysseyGame again(meadow.data, meadow.levelFile);
    again.start(renderer);
    REQUIRE(again.loadAutosave());
    for (const int id : meadow.plantIds) CHECK(plantById(again, id)->state == "picked"); // as before: the saved states stand
    CHECK_FALSE(again.runChanges().any());
    CHECK(again.message().find("The level updated") == std::string::npos);

    REQUIRE(again.autosave());
    const nlohmann::json written = nlohmann::json::parse(readText(meadow.saves / "things.json"));
    CHECK(written.at("version").get<int>() == 3);
    REQUIRE(written.contains("baseline"));
    CHECK(game::LevelBaseline::fromText(written.at("baseline").dump()) == game::LevelBaseline::of(again.level())); // the level as it is now
}

TEST_CASE("US-305 Round trip: save, load, save again writes the same baseline and the same state") {
    Meadow meadow("merge-roundtrip");
    luna::engine::RecordingRenderer renderer;
    game::OdysseyGame one(meadow.data, meadow.levelFile);
    one.start(renderer);
    REQUIRE(one.loadAutosave());
    REQUIRE(one.autosave());
    const nlohmann::json first = nlohmann::json::parse(readText(meadow.saves / "things.json"));
    game::OdysseyGame two(meadow.data, meadow.levelFile);
    two.start(renderer);
    REQUIRE(two.loadAutosave());
    REQUIRE(two.autosave());
    const nlohmann::json second = nlohmann::json::parse(readText(meadow.saves / "things.json"));
    CHECK(first.at("baseline") == second.at("baseline"));
    CHECK(first.at("plants") == second.at("plants"));
    CHECK_FALSE(two.runChanges().any());
}

TEST_CASE("US-305 Buildings: a building of the level that was moved stands in its new place; the clan's own buildings are left alone") {
    const fs::path data = dataCopy("merge-buildings");
    const game::Definitions definitions = game::loadDefinitions(data);
    const fs::path folder = data.parent_path() / "level-merge-buildings";
    fs::create_directories(folder);
    game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
    level.pickups.clear();
    level.characters.clear();
    level.plants.clear();
    level.clan = true;
    const int tileX = level.heroStart.x / 32;
    const int tileY = level.heroStart.y / 32;
    level.buildings.push_back({level.nextId++, "hut", tileX + 6, tileY, 0, true, -1, {}, {}});
    level.buildings.push_back({level.nextId++, "hut", tileX + 6, tileY + 6, 0, true, -1, {}, {}});
    const int movedSpec = level.buildings[0].id;
    const fs::path levelFile = folder / "level.json";
    game::saveLevel(level, definitions, levelFile);

    luna::engine::RecordingRenderer renderer;
    {
        game::OdysseyGame first(data, levelFile);
        first.start(renderer);
        first.startNewRun({1, 2, 1}, false, false);
        first.run().close();
        REQUIRE(first.autosave());
        REQUIRE(first.buildings().store().all().size() == 2);
    }
    level.buildings[0].x += 5; // the owner moved the first hut
    game::saveLevel(level, definitions, levelFile);

    game::OdysseyGame again(data, levelFile);
    again.start(renderer);
    REQUIRE(again.loadAutosave());
    CHECK(again.runChanges().of(game::LevelBaseline::Building).updated == std::set<int>{movedSpec});
    std::set<int> xs;
    for (const auto& building : again.buildings().store().all()) xs.insert(building.x);
    CHECK(xs == std::set<int>{tileX + 11, tileX + 6}); // the moved hut is in its new place, the other is where it was, and there are still two
    CHECK(again.buildings().store().all().size() == 2);

    REQUIRE(again.autosave()); // round trip: the links are written and read again
    game::OdysseyGame third(data, levelFile);
    third.start(renderer);
    REQUIRE(third.loadAutosave());
    CHECK_FALSE(third.runChanges().any());
    CHECK(third.buildings().store().all().size() == 2);
}

TEST_CASE("US-305 Play from the Editor: saving the level under a loaded run, then F1, loads the run again with the edits merged in") {
    Meadow meadow("merge-f1");
    luna::engine::RecordingRenderer renderer;
    game::OdysseyGame odyssey(meadow.data, meadow.levelFile);
    odyssey.start(renderer);
    REQUIRE(odyssey.loadAutosave());
    CHECK(odyssey.runLoaded());
    const int moved = meadow.plantIds[1];

    odyssey.switchMode(game::Mode::Editor);
    for (game::PlacedPlant& plant : odyssey.editor().level().plants) {
        if (plant.id == moved) plant.feet.x += 64;
    }
    REQUIRE(odyssey.editor().save());
    odyssey.switchMode(game::Mode::Game);

    REQUIRE(plantById(odyssey, moved) != nullptr);
    CHECK(plantById(odyssey, moved)->state == plantById(odyssey, moved)->def->states.front()); // fresh
    CHECK(plantById(odyssey, meadow.plantIds[0])->state == "picked");                            // the run is back, not started over
    CHECK(odyssey.runChanges().count() == 1);
    CHECK(odyssey.message().find("The level updated 1 thing") != std::string::npos);
}
