// US-251, US-252, US-255, US-257: the building store (placing, building, rooms, wear, damage, fire, saving), headless.
#include "sim/building_store.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <string>

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

buildings::BuildingStore makeStore(std::uint64_t seed = 1) {
    buildings::BuildingStore store(&data(), seed);
    store.reset(24, 24, seed);
    return store;
}

const buildings::PlacedBuilding& get(const buildings::BuildingStore& store, int id) {
    const buildings::PlacedBuilding* building = store.find(id);
    REQUIRE(building != nullptr);
    return *building;
}

// Brings everything a site needs, then works until it is done.
void buildNow(buildings::BuildingStore& store, int id) {
    store.waiveCost(id);
    store.work(id, 3600000);
}

} // namespace

TEST_CASE("US-251 Place: a blueprint stands where it is valid and is refused where it is not") {
    buildings::BuildingStore store = makeStore();
    const auto blocked = [](int x, int y) { return x == 10 && y == 10; };
    CHECK(store.whyNot("hut", 5, 5, 0, blocked).empty());
    CHECK_FALSE(store.whyNot("hut", 9, 9, 0, blocked).empty()); // a tree inside the footprint
    CHECK_FALSE(store.whyNot("hut", 22, 22, 0, blocked).empty()); // runs off the map
    CHECK_FALSE(store.whyNot("castle", 5, 5, 0, blocked).empty());
    const auto placed = store.place("hut", 5, 5, 0, 0, false, blocked);
    REQUIRE(placed.problem.empty());
    const buildings::PlacedBuilding& hut = get(store, placed.id);
    CHECK(hut.state == buildings::State::Blueprint);
    CHECK(hut.width == 3);
    CHECK(store.ruleState(hut) == "waiting");
    CHECK(store.footprintAt(6, 6) == placed.id);
    // Another whole building may not overlap it.
    CHECK_FALSE(store.whyNot("windbreak", 5, 5, 0, blocked).empty());
    // A blueprint is not solid yet: nothing stands in the way of walking.
    CHECK(store.obstacles().empty());
}

TEST_CASE("US-251 Turn: a turned blueprint swaps its footprint and keeps its door on the right side") {
    buildings::BuildingStore store = makeStore();
    const auto placed = store.place("windbreak", 4, 4, 1, 0, true, {});
    REQUIRE(placed.problem.empty());
    const buildings::PlacedBuilding& screen = get(store, placed.id);
    CHECK(screen.width == 1);
    CHECK(screen.height == 3);
    for (const auto& piece : screen.pieces) CHECK(piece.x == 4);
    CHECK(store.hasFinishedPiece(4, 4, buildings::PieceType::Wall));
    CHECK(store.hasFinishedPiece(4, 6, buildings::PieceType::Wall));
}

TEST_CASE("US-251 Build: materials, then work, and the building rises in stages") {
    buildings::BuildingStore store = makeStore();
    const int id = store.place("hut", 5, 5, 0, 0, false, {}).id;
    CHECK(store.work(id, 1000) == false);
    CHECK(get(store, id).workMilli == 0); // no materials, no work
    odysseus::sim::ItemCounts bag = {{"wood", 7}, {"herbs", 10}, {"fur", 1}};
    const auto moved = store.deliver(id, bag);
    CHECK(moved.at("wood") == 7);
    CHECK(bag.count("wood") == 0);
    CHECK(store.ruleState(get(store, id)) == "waiting");
    // Half the wood: work is limited to the share that was brought.
    const int available = store.workAvailable(get(store, id));
    CHECK(available > 0);
    CHECK(available < store.buildMilli(get(store, id)));
    store.work(id, available);
    CHECK(get(store, id).state == buildings::State::Blueprint);
    CHECK(store.work(id, 1000) == false);
    odysseus::sim::ItemCounts more = {{"wood", 7}};
    store.deliver(id, more);
    CHECK(store.ruleState(get(store, id)) == "ready");
    int stages = 0;
    std::size_t lastBuilt = 0;
    bool done = false;
    while (!done && stages < 1000) {
        done = store.work(id, 500);
        const auto& pieces = get(store, id).pieces;
        const std::size_t built = static_cast<std::size_t>(std::ranges::count_if(pieces, [](const auto& p) { return p.built; }));
        CHECK(built >= lastBuilt);
        lastBuilt = built;
        ++stages;
    }
    CHECK(done);
    CHECK(get(store, id).state == buildings::State::Finished);
    CHECK(store.condition(get(store, id)) == 100);
    // After the first stage only the first piece stood: it rose in steps, not at once.
    CHECK(stages > 3);
}

TEST_CASE("US-251 Cancel: delivered materials are dropped on the site and can be picked up") {
    buildings::BuildingStore store = makeStore();
    const int id = store.place("hut", 5, 5, 0, 0, false, {}).id;
    odysseus::sim::ItemCounts bag = {{"wood", 4}};
    store.deliver(id, bag);
    CHECK(store.cancel(id));
    CHECK(store.find(id) == nullptr);
    REQUIRE(store.drops().size() == 1);
    CHECK(store.drops().front().items.at("wood") == 4);
    const auto taken = store.takeDropsNear(6, 6, 1);
    CHECK(taken.at("wood") == 4);
    CHECK(store.drops().empty());
    // A finished building cannot be cancelled.
    const int done = store.place("windbreak", 10, 10, 0, 0, true, {}).id;
    CHECK_FALSE(store.cancel(done));
}

TEST_CASE("US-252 Pieces: a ring of walls, a door and a roof, each built like a small blueprint") {
    buildings::BuildingStore store = makeStore();
    std::vector<int> ids;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (i == 1 && j == 1) continue;
            const char* piece = (i == 2 && j == 1) ? "piece:door-wood" : "piece:wall-wood";
            const auto placed = store.place(piece, 5 + i, 5 + j, 0, 0, false, {});
            REQUIRE_MESSAGE(placed.problem.empty(), placed.problem);
            ids.push_back(placed.id);
        }
    }
    // A wall on the same layer in the same cell is refused; a roof over it is allowed.
    CHECK_FALSE(store.whyNot("piece:wall-wood", 5, 5, 0, {}).empty());
    CHECK(store.whyNot("piece:roof-thatch", 5, 5, 0, {}).empty());
    for (const int id : ids) {
        CHECK(get(store, id).pieces.size() == 1);
        buildNow(store, id);
    }
    const int roof = store.place("piece:roof-thatch", 6, 6, 0, 0, true, {}).id;
    REQUIRE(roof != 0);
    CHECK(store.rooms().size() == 1); // the roof over the middle cell closes the room
    CHECK(store.roomAt(6, 6) >= 0);
    CHECK(store.sheltered(6, 6));
    CHECK(store.rooms().front().doors == 1);
}

TEST_CASE("US-252 Room: enclosed, roofed and with a door; a missing roof or door or a gap is no room") {
    buildings::BuildingStore store = makeStore();
    const int hut = store.place("hut", 4, 4, 0, 0, true, {}).id;
    REQUIRE(hut != 0);
    REQUIRE(store.rooms().size() == 1);
    CHECK(store.roomAt(5, 5) == 0);
    CHECK(store.roomAt(5, 7) == -1); // the doorstep is outside
    CHECK(store.roomAt(4, 4) == -1);
    // The wall layer blocks walking, the door does not: people walk in through the door only.
    const auto obstacles = store.obstacles();
    const auto blocks = [&](int x, int y) { return std::ranges::any_of(obstacles, [&](const auto& o) { return o.x == x && o.y == y; }); };
    CHECK(blocks(4, 4));
    CHECK(blocks(6, 5));
    CHECK_FALSE(blocks(5, 6)); // the door
    // Knock a wall down: the room opens.
    bool fell = false;
    REQUIRE(store.damageAt(4, 5, 10000, &fell));
    CHECK(fell);
    CHECK(store.rooms().empty());
    CHECK(store.roomAt(5, 5) == -1);

    // No door, no room: a ring of walls with a window only.
    buildings::BuildingStore other = makeStore();
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (i == 1 && j == 1) continue;
            other.place("piece:wall-wood", 5 + i, 5 + j, 0, 0, true, {});
        }
    }
    other.place("piece:roof-thatch", 6, 6, 0, 0, true, {});
    CHECK(other.rooms().empty());
    // No roof, no room: a ring of walls with a door.
    buildings::BuildingStore roofless = makeStore();
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (i == 1 && j == 1) continue;
            roofless.place(i == 2 && j == 1 ? "piece:door-wood" : "piece:wall-wood", 5 + i, 5 + j, 0, 0, true, {});
        }
    }
    CHECK(roofless.rooms().empty());
    // A palisade ring (fences, no roof) is no room either, and a windbreak shelters only beside it.
    buildings::BuildingStore pal = makeStore();
    pal.place("windbreak", 5, 5, 0, 0, true, {});
    CHECK(pal.sheltered(6, 6));
    CHECK_FALSE(pal.sheltered(12, 12));
}

TEST_CASE("US-255 Wear: a season takes the hp the data says and Repair is offered; wear never destroys") {
    buildings::BuildingStore store = makeStore();
    const int id = store.place("hut", 4, 4, 0, 0, true, {}).id;
    CHECK(store.condition(get(store, id)) == 100);
    store.seasonEnded(0); // spring: the hut's wear table has nothing for it
    CHECK(store.condition(get(store, id)) == 100);
    store.seasonEnded(3); // winter: one hp per piece
    const int worn = store.condition(get(store, id));
    CHECK(worn < 100);
    CHECK(worn >= 95);
    const auto tags = store.tags(get(store, id));
    CHECK(std::ranges::find(tags, "repairable") != tags.end());
    CHECK(store.ruleState(get(store, id)) == "damaged");
    // Repair heals the most damaged piece, then the next, until all is whole.
    int repairs = 0;
    while (store.repair(id, 5) && repairs < 100) ++repairs;
    CHECK(store.condition(get(store, id)) == 100);
    CHECK(store.ruleState(get(store, id)) == "finished");
    for (int i = 0; i < 400; ++i) store.seasonEnded(3);
    CHECK(get(store, id).state == buildings::State::Finished); // wear leaves 1 hp
}

TEST_CASE("US-255 Damage: a hit takes hp and a wall falls at zero into rubble; a building of rubble is cleared later") {
    buildings::BuildingStore store = makeStore();
    const int id = store.place("windbreak", 4, 4, 0, 0, true, {}).id;
    bool fell = false;
    CHECK(store.damageAt(4, 4, 10, &fell));
    CHECK_FALSE(fell);
    CHECK(store.condition(get(store, id)) < 100);
    CHECK(store.damageAt(4, 4, 1000, &fell));
    CHECK(fell);
    CHECK(store.buildingAt(4, 4) == 0);
    for (int x = 5; x <= 6; ++x) store.damageAt(x, 4, 1000);
    CHECK(get(store, id).state == buildings::State::Rubble);
    for (int day = 0; day < 10; ++day) store.dayEnded();
    CHECK(store.find(id) == nullptr);
}

TEST_CASE("US-255 Fire: a burning wooden wall spreads to wooden neighbours and ends in rubble; rain and water put it out; mud resists") {
    buildings::BuildingStore store = makeStore(7);
    const int hut = store.place("hut", 4, 4, 0, 0, true, {}).id;
    REQUIRE(hut != 0);
    CHECK(store.igniteAt(4, 4)); // a torch dropped by the wall
    CHECK(store.anyBurning());
    for (int tick = 0; tick < 20 * 120; ++tick) store.advance({});
    CHECK_FALSE(store.anyBurning());
    CHECK(get(store, hut).state == buildings::State::Rubble); // the thatch roof and the walls all burned
    CHECK(store.rooms().empty());

    // Mud does not burn.
    buildings::BuildingStore mud = makeStore();
    mud.place("piece:wall-mud", 3, 3, 0, 0, true, {});
    CHECK_FALSE(mud.igniteAt(3, 3));

    // Water puts it out.
    buildings::BuildingStore wet = makeStore();
    const int rack = wet.place("palisade", 2, 2, 0, 0, true, {}).id;
    REQUIRE(wet.igniteAt(2, 2));
    CHECK(wet.douse(rack, 2, 2));
    CHECK_FALSE(wet.anyBurning());
    CHECK_FALSE(wet.douse(rack, 2, 2));

    // Heavy rain stops a fire before it takes the whole palisade.
    buildings::BuildingStore rain = makeStore(3);
    rain.place("palisade", 2, 2, 0, 0, true, {});
    REQUIRE(rain.igniteAt(2, 2));
    for (int tick = 0; tick < 20 * 5; ++tick) rain.advance({100});
    // Each second a burning stake goes out with chance 25 percent; after five seconds the fire has not run away.
    int burningStakes = 0;
    for (const auto& piece : rain.all().front().pieces) burningStakes += piece.burning ? 1 : 0;
    CHECK(burningStakes <= 5);
}

TEST_CASE("US-253 Deterministic: the same seed and the same actions burn the same way; a different seed may not") {
    const auto run = [](std::uint64_t seed) {
        buildings::BuildingStore store = makeStore(seed);
        store.place("hut", 4, 4, 0, 0, true, {});
        store.place("palisade", 10, 4, 0, 0, true, {});
        store.igniteAt(4, 4);
        store.igniteAt(10, 4);
        for (int tick = 0; tick < 20 * 40; ++tick) store.advance({});
        return store.hash();
    };
    CHECK(run(5) == run(5));
    CHECK(run(5) != run(6));
}

TEST_CASE("US-257 Saved: buildings with owners, condition, contents and fire survive a save and a load, and the hash agrees") {
    buildings::BuildingStore store = makeStore(9);
    const int hut = store.place("hut", 4, 4, 1, 0, true, {}).id;
    const int pit = store.place("storage-pit", 10, 10, 0, 0, true, {}).id;
    const int site = store.place("windbreak", 14, 14, 0, 0, false, {}).id;
    store.findMutable(hut)->owner = 3;
    store.findMutable(hut)->interior = "map";
    store.findMutable(hut)->interiorLevel = "hut-inside";
    store.deposit(pit, "berries", 12);
    odysseus::sim::ItemCounts bag = {{"fur", 2}};
    store.deliver(site, bag);
    store.damageAt(5, 5, 30);
    store.igniteAt(8, 8);
    store.cancel(site);
    for (int tick = 0; tick < 50; ++tick) store.advance({});
    const std::string text = store.toJson();
    buildings::BuildingStore loaded(&data(), 1);
    loaded.reset(24, 24, 1);
    std::string problem;
    REQUIRE_MESSAGE(loaded.fromJson(text, problem), problem);
    CHECK(loaded.hash() == store.hash());
    CHECK(loaded.toJson() == text);
    const buildings::PlacedBuilding& again = get(loaded, hut);
    CHECK(again.owner == 3);
    CHECK(again.interior == "map");
    CHECK(loaded.interiorLevelOf(again) == "hut-inside");
    CHECK(loaded.find(pit)->contents.at("berries") == 12);
    CHECK(loaded.drops().size() == 1);
    CHECK(loaded.condition(again) == store.condition(*store.find(hut)));
    CHECK(loaded.rooms().size() == store.rooms().size());
    // The loaded store goes on exactly as the saved one would.
    for (int tick = 0; tick < 400; ++tick) {
        store.advance({});
        loaded.advance({});
    }
    CHECK(loaded.hash() == store.hash());
    // A damaged file is refused with a reason.
    std::string bad;
    CHECK_FALSE(loaded.fromJson("{ not json", bad));
    CHECK_FALSE(bad.empty());
}

TEST_CASE("US-257 Storage: a storage pit holds what is put in and gives it back") {
    buildings::BuildingStore store = makeStore();
    const int pit = store.place("storage-pit", 3, 3, 0, 0, true, {}).id;
    CHECK(store.deposit(pit, "berries", 5) == 5);
    CHECK(store.withdraw(pit, "berries", 3) == 3);
    CHECK(store.withdraw(pit, "berries", 9) == 2);
    CHECK(store.withdraw(pit, "berries", 1) == 0);
    const auto stores = store.withUse("store");
    REQUIRE(stores.size() == 1);
    CHECK(stores.front()->id == pit);
}
