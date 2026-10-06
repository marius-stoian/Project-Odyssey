#include "core/text.h"
#include "sim/hero_life.h"
#include "sim/play_rules.h"
#include "sim/schema.h"
#include "sim/schema_index.h"
#include "sim/world.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <string>

namespace sim = odysseus::sim;
namespace fs = std::filesystem;

namespace {

// A copy of the data folder the test may add rules files to.
struct Folder {
    fs::path path = fs::temp_directory_path() / ("odysseus-us195-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Folder() { fs::copy(ODYSSEUS_DATA_DIR, path, fs::copy_options::recursive); }
    ~Folder() {
        std::error_code error;
        fs::remove_all(path, error);
    }
    void rules(const std::string& name, const std::string& text) const { REQUIRE_FALSE(odysseus::core::writeTextFileSafely(path / "rules" / (name + ".json"), text).has_value()); }
};

// The day-by-day run of US-073, under the hero data of a set of rules.
struct Run {
    sim::HeroData data;
    sim::World world;
    sim::HeroLife life;
    Run(const fs::path& folder, const std::string& rules)
        : data(sim::loadHeroData(folder, rules)),
          world(1, sim::configForComfort(data, sim::loadSimConfig(folder), 1)),
          life(data, world, sim::NewGame{1, 2, 1, rules}) {}
    void days(int count) {
        for (int i = 0; i < count; ++i) {
            world.runTicks(static_cast<std::uint64_t>(world.calendar().ticksPerDay()));
            life.update();
        }
    }
};

} // namespace

TEST_CASE("US-195 The shipped rules") {
    const sim::PlayRules standard = sim::loadPlayRules(ODYSSEUS_DATA_DIR, "standard");
    CHECK(standard.presets.size() == 3);
    CHECK(standard.comforts.size() == 3);
    CHECK(standard.hasVictory);
    CHECK(standard.winPercent == 60);
    CHECK(standard.combinedWinPercent == 50);
    CHECK(standard.loseBelowPeople == 3);
    const sim::PlaySystems& on = standard.systems;
    CHECK((on.weather && on.combat && on.rivals && on.tutorial && on.markers && on.chronicle && on.politics));
    CHECK(sim::playRuleNames(ODYSSEUS_DATA_DIR) == std::vector<std::string>{"standard", "peaceful"});
    const sim::PlayRules peaceful = sim::loadPlayRules(ODYSSEUS_DATA_DIR, "peaceful");
    CHECK_FALSE(peaceful.systems.combat);
    CHECK_FALSE(peaceful.systems.rivals);
    CHECK(peaceful.systems.weather); // a switch the file leaves out is on
    CHECK(peaceful.winPercent == 40);
    CHECK(sim::describeRules(peaceful).find("combat off") != std::string::npos);
    // hero.json no longer carries what moved to the rules.
    const std::string hero = *odysseus::core::readTextFile(fs::path(ODYSSEUS_DATA_DIR) / "hero" / "hero.json");
    CHECK(hero.find("\"presets\"") == std::string::npos);
    CHECK(hero.find("\"winPercent\"") == std::string::npos);
}

TEST_CASE("US-195 Rules sets: the values of the picked file are used") {
    Folder folder;
    folder.rules("short", R"({ "title": "Short", "newGame": { "presets": [ { "name": "Quick", "startAge": 18, "mantleAge": 24 } ] },
                               "victory": { "winPercent": 5, "combinedWinPercent": 5, "loseBelowPeople": 1, "rivalFollowerPercent": 10 }, "systems": { "weather": false } })");
    const sim::HeroData standard = sim::loadHeroData(folder.path);
    CHECK(standard.config.presets.size() == 3);
    CHECK(standard.config.dominion.winPercent == 60);
    const sim::HeroData picked = sim::loadHeroData(folder.path, "short");
    REQUIRE(picked.config.presets.size() == 1);
    CHECK(picked.config.presets[0].name == "Quick");
    CHECK(picked.config.comforts.size() == 3); // what the file leaves out is the standard's
    CHECK(picked.config.dominion.winPercent == 5);
    CHECK(picked.config.dominion.rivalFollowerPercent == 10);
    CHECK_FALSE(sim::loadPlayRules(folder.path, "short").systems.weather);
    // The old place still works for a data folder that was not moved yet: hero.json carries the numbers, the rules file leaves them out.
    folder.rules("standard", R"({ "title": "Standard" })");
    std::string hero = *odysseus::core::readTextFile(folder.path / "hero" / "hero.json");
    hero.insert(1, R"("presets": [ { "name": "Old", "startAge": 12, "mantleAge": 26 } ], "comforts": [ { "name": "Plain", "needsPercent": 100, "foodPercent": 100 } ],)");
    REQUIRE_FALSE(odysseus::core::writeTextFileSafely(folder.path / "hero" / "hero.json", hero).has_value());
    const sim::HeroData old = sim::loadHeroData(folder.path);
    REQUIRE(old.config.presets.size() == 1);
    CHECK(old.config.presets[0].name == "Old");
}

TEST_CASE("US-195 Rules files with mistakes name the file and the field") {
    Folder folder;
    folder.rules("bad", R"({ "systems": { "weather": "no" } })");
    CHECK_THROWS_WITH(sim::loadPlayRules(folder.path, "bad"), doctest::Contains("systems.weather"));
    folder.rules("worse", R"({ "victory": { "winPercent": 500, "combinedWinPercent": 5, "loseBelowPeople": 1, "rivalFollowerPercent": 10 } })");
    CHECK_THROWS_WITH(sim::loadPlayRules(folder.path, "worse"), doctest::Contains("victory.winPercent"));
    CHECK_THROWS_WITH(sim::loadPlayRules(folder.path, "missing"), doctest::Contains("missing.json"));
}

TEST_CASE("US-195 Thresholds: victory follows the value of the rules") {
    Folder folder;
    folder.rules("easy", R"({ "victory": { "winPercent": 1, "combinedWinPercent": 1, "loseBelowPeople": 1, "rivalFollowerPercent": 60 } })");
    {
        Run run(folder.path, "standard");
        run.life.setRivals({{"the River Clan", 15}, {"the Stone Clan", 15}});
        run.life.addTradePoints(30);
        run.days(1);
        CHECK(run.life.phase() != sim::Phase::Ended); // a little trade does not lead the region at 60%
    }
    {
        Run run(folder.path, "easy");
        run.life.setRivals({{"the River Clan", 15}, {"the Stone Clan", 15}});
        run.life.addTradePoints(30);
        run.days(1);
        CHECK(run.life.phase() == sim::Phase::Ended); // and wins at 1%
        CHECK(run.life.outcome() == sim::Outcome::Victory);
    }
}

TEST_CASE("US-195 The rules a run began with are in its save") {
    Folder folder;
    folder.rules("easy", R"({ "victory": { "winPercent": 1, "combinedWinPercent": 1, "loseBelowPeople": 1, "rivalFollowerPercent": 60 } })");
    Run run(folder.path, "easy");
    const fs::path file = folder.path / "hero-save.json";
    run.life.save(file);
    CHECK(sim::HeroLife::savedRules(file) == "easy");
    sim::World world(1, sim::configForComfort(run.data, sim::loadSimConfig(folder.path), 1));
    const sim::HeroLife loaded = sim::HeroLife::load(run.data, world, file);
    CHECK(loaded.game().rules == "easy");
    // A save from before the rules (version 1) has none: standard.
    std::string text = *odysseus::core::readTextFile(file);
    const std::size_t at = text.find("\"rules\"");
    REQUIRE(at != std::string::npos);
    const std::size_t end = text.find('\n', at);
    text.erase(at, end - at + 1);
    REQUIRE_FALSE(odysseus::core::writeTextFileSafely(file, text).has_value());
    CHECK(sim::HeroLife::savedRules(file).empty());
}

TEST_CASE("US-195 The rules schema and the data folder agree") {
    Folder folder;
    const sim::schema::SchemaSet set = sim::schema::SchemaSet::load(folder.path / "schemas");
    CHECK(sim::schema::buildIndex(folder.path, set).clean());
}
