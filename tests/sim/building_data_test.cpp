// US-250 Building pieces and building kinds as data: assets/data/buildings/pieces.json, kinds.json and prefabs.
#include "sim/building_data.h"
#include "sim/building_store.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
namespace buildings = odysseus::sim::buildings;
namespace rules = odysseus::sim::rules;

namespace {

buildings::BuildingData shipped(rules::LoadReport& report) {
    return buildings::BuildingData::load(fs::path(ODYSSEUS_DATA_DIR) / "buildings", report);
}

const char* kPieces = R"({ "materials": { "wood": { "flammability": 60, "burnSeconds": 30 } }, "pieces": [
  { "id": "wall", "type": "wall", "material": "wood", "hp": 100, "buildSeconds": 3, "cost": { "wood": 2 } },
  { "id": "roof", "type": "roof", "material": "wood", "hp": 50, "buildSeconds": 2, "cost": { "wood": 1 } } ] })";

} // namespace

TEST_CASE("US-250 Load: the shipped pieces and kinds load without a mistake") {
    rules::LoadReport report;
    const buildings::BuildingData data = shipped(report);
    for (const auto& error : report.errors) FAIL(error.text());
    for (const char* id : {"hut", "windbreak", "storage-pit", "drying-rack", "palisade"}) CHECK_MESSAGE(data.kind(id) != nullptr, id);
    CHECK(data.pieces().size() >= 10);
    for (const char* id : {"wall-wood", "floor-straw", "roof-thatch", "door-hide", "post-wood", "fence-wood"}) CHECK_MESSAGE(data.piece(id) != nullptr, id);
    const buildings::KindDef* hut = data.kind("hut");
    REQUIRE(hut != nullptr);
    CHECK(hut->width == 3);
    CHECK(hut->height == 3);
    CHECK(hut->known);
    CHECK(data.kind("windbreak")->known);
    CHECK_FALSE(data.kind("palisade")->known);
    CHECK(hut->cost.at("wood") == 14);   // seven walls of 2
    CHECK(hut->cost.at("herbs") == 10);  // nine roofs and a floor
    CHECK(hut->cost.at("fur") == 1);     // the hide flap
    CHECK(hut->buildMilli > 0);
    CHECK(data.kind("drying-rack")->cost.at("wood") == 3); // written in the file, not the sum
}

TEST_CASE("US-250 Load: a mistake names the file, the line and the field") {
    rules::LoadReport report;
    buildings::BuildingData::parse(kPieces, R"({ "kinds": [
  { "id": "shed", "footprint": [2, 1], "layout": [ { "piece": "wall", "x": 0, "y": 0 }, { "piece": "nonexistent", "x": 1, "y": 0 } ] } ] })", report);
    REQUIRE_FALSE(report.errors.empty());
    CHECK(report.errors.front().file == "buildings/kinds.json");
    CHECK(report.errors.front().line == 2);
    CHECK(report.errors.front().text().find("nonexistent") != std::string::npos);

    rules::LoadReport second;
    buildings::BuildingData::parse(R"({ "materials": { "wood": {} }, "pieces": [ { "id": "w", "type": "wall", "material": "stone", "hp": 0 } ] })", R"({ "kinds": [] })", second);
    REQUIRE(second.errors.size() >= 2);
    bool sawMaterial = false;
    bool sawHp = false;
    for (const auto& error : second.errors) {
        sawMaterial = sawMaterial || error.message.find("material") != std::string::npos;
        sawHp = sawHp || error.message.find("hp") != std::string::npos;
        CHECK(error.file == "buildings/pieces.json");
        CHECK(error.line == 1);
    }
    CHECK(sawMaterial);
    CHECK(sawHp);

    rules::LoadReport third;
    buildings::BuildingData::parse(kPieces, R"({ "kinds": [ { "id": "x", "footprint": [1, 1], "layout": [ { "piece": "wall", "x": 0, "y": 0 } ], "interior": "map" } ] })", third);
    REQUIRE_FALSE(third.errors.empty());
    CHECK(third.errors.front().message.find("interiorLevel") != std::string::npos);

    rules::LoadReport fourth;
    buildings::BuildingData::parse(kPieces, R"({ "kinds": [ { "id": "x", "footprint": [1, 1], "layout": [ { "piece": "wall", "x": 3, "y": 0 } ] } ] })", fourth);
    REQUIRE_FALSE(fourth.errors.empty());
    CHECK(fourth.errors.front().message.find("outside") != std::string::npos);

    rules::LoadReport fifth;
    buildings::BuildingData::parse(kPieces, R"({ "kinds": [ { "id": "x", "footprint": [1, 1], "layout": [ { "piece": "wall", "x": 0, "y": 0 }, { "piece": "wall", "x": 0, "y": 0 } ] } ] })", fifth);
    REQUIRE_FALSE(fifth.errors.empty());
    CHECK(fifth.errors.front().message.find("same layer") != std::string::npos);
}

TEST_CASE("US-250 Uses: the shipped kinds say what they are for") {
    rules::LoadReport report;
    const buildings::BuildingData data = shipped(report);
    const auto has = [&](const char* kind, const char* use) {
        const buildings::KindDef* def = data.kind(kind);
        return def != nullptr && std::ranges::find(def->uses, use) != def->uses.end();
    };
    CHECK(has("hut", "shelter"));
    CHECK(has("hut", "sleep"));
    CHECK(has("storage-pit", "store"));
    CHECK(has("drying-rack", "work"));
    // The store hands its uses on as tags, so the interactions match them (buildings.h tags()).
    buildings::BuildingStore store(&data);
    store.reset(20, 20, 1);
    const auto pit = store.place("storage-pit", 2, 2, 0, 0, true, {});
    REQUIRE(pit.problem.empty());
    const auto tags = store.tags(*store.find(pit.id));
    CHECK(std::ranges::find(tags, "store") != tags.end());
    CHECK(std::ranges::find(tags, "building") != tags.end());
}

TEST_CASE("US-250 Prefabs: a prefab file is a kind; it round-trips and may not reuse a kind's id") {
    rules::LoadReport report;
    buildings::BuildingData data = shipped(report);
    for (const auto& error : report.errors) FAIL(error.text());
    const fs::path folder = fs::temp_directory_path() / "odysseus-us250" / "buildings";
    fs::remove_all(folder.parent_path());
    fs::create_directories(folder / "prefabs");
    fs::copy_file(fs::path(ODYSSEUS_DATA_DIR) / "buildings" / "pieces.json", folder / "pieces.json");
    fs::copy_file(fs::path(ODYSSEUS_DATA_DIR) / "buildings" / "kinds.json", folder / "kinds.json");
    std::ofstream(folder / "prefabs" / "lodge.json") << R"({ "id": "lodge", "label": "Lodge", "footprint": [2, 2],
  "layout": [ { "piece": "wall-wood", "x": 0, "y": 0 }, { "piece": "door-wood", "x": 1, "y": 0 }, { "piece": "roof-thatch", "x": 0, "y": 0 } ],
  "interior": "map", "interiorLevel": "lodge-inside", "uses": ["shelter"], "buildable": true })";
    std::ofstream(folder / "prefabs" / "hut.json") << R"({ "id": "hut", "footprint": [1, 1], "layout": [ { "piece": "wall-wood", "x": 0, "y": 0 } ] })";
    rules::LoadReport prefabReport;
    const buildings::BuildingData withPrefabs = buildings::BuildingData::load(folder, prefabReport);
    REQUIRE(prefabReport.errors.size() == 1);
    CHECK(prefabReport.errors.front().file == "buildings/prefabs/hut.json");
    CHECK(prefabReport.errors.front().message.find("exists already") != std::string::npos);
    const buildings::KindDef* lodge = withPrefabs.kind("lodge");
    REQUIRE(lodge != nullptr);
    CHECK(lodge->prefab);
    CHECK(lodge->interior == buildings::InteriorMode::Map);
    CHECK(lodge->interiorLevel == "lodge-inside");
    // The Editor saves toJson(); loading that text gives the same kind back.
    rules::LoadReport again;
    const auto reread = withPrefabs.parseKind(buildings::toJson(*lodge), "buildings/prefabs/lodge.json", again, true, "lodge");
    REQUIRE(reread);
    CHECK(again.errors.empty());
    CHECK(*reread == *lodge);
}

TEST_CASE("US-250 Turns: a layout turns in quarter steps and comes back after four") {
    using buildings::turnedOffset;
    CHECK(turnedOffset(0, 0, 3, 2, 1) == buildings::Cell{1, 0});
    CHECK(turnedOffset(2, 1, 3, 2, 1) == buildings::Cell{0, 2});
    CHECK(turnedOffset(0, 0, 3, 2, 2) == buildings::Cell{2, 1});
    for (int x = 0; x < 3; ++x) {
        for (int y = 0; y < 2; ++y) {
            buildings::Cell cell{x, y};
            int w = 3;
            int h = 2;
            for (int turn = 0; turn < 4; ++turn) {
                cell = turnedOffset(cell.x, cell.y, w, h, 1);
                std::swap(w, h);
            }
            CHECK(cell == buildings::Cell{x, y});
        }
    }
}
