// The story engine (M2b): reasons, quarrels, care, courtship, teaching, hunting parties and the
// story told in episodes. One test file for the whole milestone; test names start with the story.
#include "sim/chronicle.h"
#include "sim/data.h"
#include "sim/save.h"
#include "sim/world.h"

#include "story_helpers.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using odysseus::sim::EventKind;
using odysseus::sim::World;
namespace sim = odysseus::sim;

using namespace story_test;

TEST_CASE("US-110 Death with a cause") {
    World world(42, barrenConfig());
    // Person 3 robs the store before it runs dry (the thief's name must come back in the story).
    world.recordTheft(3, -1);
    const std::string thief = world.people()[3].name;
    runDays(world, 14);
    const auto deaths = entriesOf(world, EventKind::Death);
    REQUIRE_FALSE(deaths.empty());
    bool found = false;
    for (const auto* death : deaths) {
        const sim::Person& person = world.people()[static_cast<std::size_t>(death->who)];
        if (person.causeOfDeath != sim::CauseOfDeath::Starvation) {
            continue;
        }
        found = true;
        MESSAGE(sim::formatEntry(*death));
        CHECK(death->text == person.name + " died of hunger in the dry summer, after " + thief + " stole from the store.");
        CHECK_FALSE(death->causes.empty());
        CHECK(death->other == 3); // the person behind the death
    }
    CHECK(found);
}

TEST_CASE("US-110 A death after an empty store names the store even without a thief") {
    World world(42, barrenConfig());
    runDays(world, 14);
    bool found = false;
    for (const auto* death : entriesOf(world, EventKind::Death)) {
        const sim::Person& person = world.people()[static_cast<std::size_t>(death->who)];
        if (person.causeOfDeath == sim::CauseOfDeath::Starvation) {
            found = true;
            CHECK(death->text == person.name + " died of hunger in the dry summer, when the store ran empty.");
        }
    }
    CHECK(found);
}

TEST_CASE("US-110 Feud with a reason") {
    World world(42, realConfig());
    for (int i = 0; i < 3; ++i) {
        world.recordTheft(4, 5);
        world.recordTheft(5, 4);
    }
    runDays(world, 1);
    const auto feuds = entriesOf(world, EventKind::Feud);
    REQUIRE(feuds.size() == 1);
    MESSAGE(sim::formatEntry(*feuds.front()));
    CHECK(feuds.front()->text == "A feud broke out between " + world.people()[4].name + " and " + world.people()[5].name +
                                     " over stolen meat.");
    REQUIRE_FALSE(feuds.front()->causes.empty());
    for (const int cause : feuds.front()->causes) {
        CHECK(world.chronicle().find(cause)->kind == EventKind::Theft);
    }
}

TEST_CASE("US-110 Traceable") {
    World world(42, realConfig());
    for (int i = 0; i < 3; ++i) {
        world.recordTheft(4, 5);
        world.recordTheft(5, 4);
    }
    runDays(world, 1);
    const auto feuds = entriesOf(world, EventKind::Feud);
    REQUIRE(feuds.size() == 1);
    const std::vector<std::string> lines = sim::explainEvent(world.chronicle(), feuds.front()->id);
    REQUIRE(lines.size() >= 3);
    CHECK(lines.front().find("A feud broke out") != std::string::npos);
    for (std::size_t i = 1; i < lines.size(); ++i) {
        CHECK(lines[i].find("because [#") != std::string::npos);
        CHECK(lines[i].find("stole from the food store") != std::string::npos);
    }
    // Every cause happened before its effect: the log can never loop.
    for (const auto& entry : world.chronicle().entries()) {
        for (const int cause : entry.causes) {
            CHECK(cause < entry.id);
        }
    }
    // A store that runs empty after thefts lists them as its causes, and a death lists the store.
    World barren(42, barrenConfig());
    barren.recordTheft(3, -1);
    runDays(barren, 14);
    const auto deaths = entriesOf(barren, EventKind::Death);
    REQUIRE_FALSE(deaths.empty());
    const auto why = sim::explainEvent(barren.chronicle(), deaths.front()->id);
    REQUIRE(why.size() >= 3);
    CHECK(why[1].find("food store ran empty") != std::string::npos);
    CHECK(why[2].find("stole from the food store") != std::string::npos);
}

TEST_CASE("US-110 A failed harvest is where a hard winter starts") {
    sim::SimConfig config = realConfig();
    config.story.season.leanAutumnPercent = 100; // every autumn fails, so the test is exact
    World world(42, config);
    runDays(world, 28);
    const auto lean = entriesOf(world, EventKind::Lean);
    REQUIRE(lean.size() == 1);
    CHECK(lean.front()->date.season == sim::Season::Autumn);
    CHECK(lean.front()->text.find("The autumn harvest failed") != std::string::npos);
    // Nothing is lost by the failure being recorded: the same world without one runs on.
    CHECK(world.population() > 0);
}

TEST_CASE("US-110 Story data is validated") {
    // A wrong number names the file and the field.
    const auto folder = std::filesystem::temp_directory_path() / "odysseus-us110";
    std::filesystem::create_directories(folder);
    const auto file = folder / "story.json";
    std::ofstream(file) << R"({"season": {"leanAutumnPercent": 150, "leanForagePercent": 45}, "causes": {}})";
    try {
        (void)sim::loadStoryConfig(file);
        FAIL("a percentage of 150 was accepted");
    } catch (const sim::DataError& error) {
        MESSAGE(std::string(error.what()));
        CHECK(std::string(error.what()).find("season.leanAutumnPercent") != std::string::npos);
        CHECK(std::string(error.what()).find("story.json") != std::string::npos);
    }
}
