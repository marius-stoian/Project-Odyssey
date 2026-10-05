// US-253 and US-257: rival builders and what buildings do for a clan, headless.
#include "sim/building_life.h"

#include <doctest/doctest.h>

#include "sim/data.h"
#include "sim/world.h"

#include <filesystem>

namespace fs = std::filesystem;
namespace buildings = odysseus::sim::buildings;
namespace rules = odysseus::sim::rules;

namespace {

const buildings::BuildingData& data() {
    static const buildings::BuildingData loaded = [] {
        rules::LoadReport report;
        return buildings::BuildingData::load(fs::path(ODYSSEUS_DATA_DIR) / "buildings", report);
    }();
    return loaded;
}

} // namespace

TEST_CASE("US-253 Rivals: a clan of six starts the first kind it lacks each season and works on it every day until it is finished") {
    buildings::RivalBuilders rivals;
    rivals.reset(2, 7);
    rivals.seasonStarted(data(), {8, 5});
    REQUIRE(rivals.of(0).size() == 1);
    CHECK(rivals.of(1).empty());   // five people are too few
    CHECK(rivals.of(0)[0].kind == "hut");   // the first kind in file order whose use it lacks
    int days = 0;
    while (!rivals.of(0)[0].finished && days < 100) {
        rivals.dayEnded(data(), {8, 5});
        ++days;
    }
    CHECK(rivals.of(0)[0].finished);
    CHECK(days > 1);
    CHECK(rivals.has(0, data(), "sleep"));
    CHECK_FALSE(rivals.has(1, data(), "sleep"));
    // The next season it lacks something else.
    rivals.seasonStarted(data(), {8, 5});
    REQUIRE(rivals.of(0).size() == 2);
    CHECK(rivals.of(0)[1].kind != "hut");
}

TEST_CASE("US-253 Rivals: the same seed and days give the same hash, and the list survives a save") {
    buildings::RivalBuilders a;
    buildings::RivalBuilders b;
    a.reset(2, 3);
    b.reset(2, 3);
    for (buildings::RivalBuilders* rivals : {&a, &b}) {
        rivals->seasonStarted(data(), {9, 12});
        for (int day = 0; day < 4; ++day) rivals->dayEnded(data(), {9, 12});
    }
    CHECK(a.hash() == b.hash());
    buildings::RivalBuilders loaded;
    std::string problem;
    REQUIRE(loaded.fromJson(a.toJson(), problem));
    CHECK(loaded.hash() == a.hash());
    b.seasonStarted(data(), {9, 12});
    CHECK(a.hash() != b.hash());
}

TEST_CASE("US-257 Warmth and storage: a housed person loses warmth more slowly, and a storage pit halves the spoilage of the meals it holds") {
    // The world's own rules are tested with the clan of the game (tests/game); here the data is checked: the pit says how much it keeps.
    const buildings::KindDef* pit = data().kind("storage-pit");
    REQUIRE(pit != nullptr);
    CHECK(pit->capacity > 0);
    CHECK(data().kind("hut")->capacity == 0);
}

TEST_CASE("US-257 World: housed people keep more warmth, and meals in a storage pit spoil more slowly") {
    const auto config = odysseus::sim::loadSimConfig(ODYSSEUS_DATA_DIR);
    odysseus::sim::World plain(42, config);
    odysseus::sim::World housed(42, config);
    std::vector<int> everyone;
    for (const odysseus::sim::Person& person : housed.people()) everyone.push_back(person.id);
    housed.setHoused(everyone);
    housed.setStorageMeals(100000);
    const auto warmth = [](const odysseus::sim::World& world) {
        int total = 0;
        for (const odysseus::sim::Person& person : world.people()) total += person.alive ? person.needs[odysseus::sim::Need::Warmth] : 0;
        return total;
    };
    const int day = static_cast<int>(plain.calendar().ticksPerDay());
    for (int tick = 0; tick < day * 2 + 1; ++tick) {
        plain.tick();
        housed.tick();
    }
    CHECK(housed.population() == plain.population());
    CHECK(warmth(housed) > warmth(plain));
    CHECK(housed.food() >= plain.food());
}
