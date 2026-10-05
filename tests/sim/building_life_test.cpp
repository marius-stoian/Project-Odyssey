// US-253 and US-257: rival builders and what buildings do for a clan, headless.
#include "sim/building_life.h"

#include <doctest/doctest.h>

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
