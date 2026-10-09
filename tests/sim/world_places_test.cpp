// US-204 Things, people and places in the region: the entries, their stable ids, the tombstones, the properties and the world file that keeps them.
#include "core/text.h"
#include "sim/data.h"
#include "sim/region.h"
#include "sim/region_edits.h"
#include "sim/world_file.h"
#include "sim/world_places.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
namespace sim = odysseus::sim;

namespace {

sim::RegionConfig config() { return sim::loadRegionConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "region.json"); }

struct Folder {
    fs::path path;
    Folder() : path(fs::temp_directory_path() / ("odysseus-us204-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))) { fs::create_directories(path); }
    ~Folder() {
        std::error_code error;
        fs::remove_all(path, error);
    }
};

// The first tile of a biome well inside the region, scanning from `from`.
sim::Tile findTile(sim::Region& land, sim::Biome biome, int from = 20) {
    for (int y = from; y < 236; ++y) {
        for (int x = from; x < 236; ++x) {
            if (land.seedBiomeAt(x, y) == biome) return {x, y};
        }
    }
    return {-1, -1};
}

sim::PlacedEdit entryOf(sim::EditGroup group, const std::string& kind, sim::Tile at, const std::string& name = {}) {
    sim::PlacedEdit entry;
    entry.group = group;
    entry.kind = kind;
    entry.x = at.x;
    entry.y = at.y;
    entry.name = name;
    return entry;
}

} // namespace

TEST_CASE("US-204 Place: a fire pit and a person are saved as entries with ids, and the rules refuse what cannot stand") {
    sim::Region land(1, config());
    sim::RegionEdits edits;
    const sim::Tile grass = findTile(land, sim::Biome::Steppe);
    const sim::Tile water = findTile(land, sim::Biome::Water);
    const sim::Tile mountain = findTile(land, sim::Biome::Mountain, 0);
    REQUIRE(grass.x >= 0);
    REQUIRE(water.x >= 0);
    REQUIRE(mountain.x >= 0);

    std::string problem;
    const auto pit = sim::addPlaced(land, edits, entryOf(sim::EditGroup::Thing, "clan-fire", grass), problem);
    REQUIRE(pit);
    CHECK(pit->id == "t-0001");
    CHECK_FALSE(pit->before);
    CHECK(pit->after->kind == "clan-fire");
    const auto elder = sim::addPlaced(land, edits, entryOf(sim::EditGroup::Person, "elder", {grass.x + 1, grass.y}, "Old Mara"), problem);
    REQUIRE(elder);
    CHECK(elder->id == "p-0001");
    const auto second = sim::addPlaced(land, edits, entryOf(sim::EditGroup::Thing, "boulder", {grass.x, grass.y + 1}), problem);
    REQUIRE(second);
    CHECK(second->id == "t-0002"); // one more than the largest of its group
    CHECK(edits.placed.size() == 3);

    CHECK_FALSE(sim::addPlaced(land, edits, entryOf(sim::EditGroup::Thing, "boulder", water), problem));
    CHECK(problem.find("water") != std::string::npos);
    CHECK_FALSE(sim::addPlaced(land, edits, entryOf(sim::EditGroup::Thing, "boulder", mountain), problem));
    CHECK(problem.find("mountain") != std::string::npos);
    CHECK_FALSE(sim::addPlaced(land, edits, entryOf(sim::EditGroup::Thing, "boulder", {-1, 4}), problem));
    CHECK_FALSE(sim::addPlaced(land, edits, entryOf(sim::EditGroup::Thing, "", {grass.x + 5, grass.y}), problem)); // no kind
    CHECK_FALSE(sim::addPlaced(land, edits, entryOf(sim::EditGroup::Thing, "boulder", grass), problem));         // the tile holds a thing already
    CHECK(problem.find("already") != std::string::npos);
    CHECK(sim::addPlaced(land, edits, entryOf(sim::EditGroup::Person, "hunter", grass), problem)); // a person may stand where a thing is
    CHECK(edits.placed.size() == 4);
}

TEST_CASE("US-204 Places: a place needs a unique name, and the names are what a quest or a dialogue can pick") {
    sim::Region land(1, config());
    sim::RegionEdits edits;
    const sim::Tile grass = findTile(land, sim::Biome::Steppe);
    std::string problem;
    CHECK_FALSE(sim::addPlaced(land, edits, entryOf(sim::EditGroup::Place, "shrine", grass), problem)); // no name
    CHECK(problem.find("name") != std::string::npos);
    REQUIRE(sim::addPlaced(land, edits, entryOf(sim::EditGroup::Place, "shrine", grass, "Red Cliff"), problem));
    CHECK_FALSE(sim::addPlaced(land, edits, entryOf(sim::EditGroup::Place, "grave", {grass.x + 2, grass.y}, "red cliff"), problem)); // the same name in other letters
    CHECK_FALSE(sim::addPlaced(land, edits, entryOf(sim::EditGroup::Place, "grave", {grass.x + 2, grass.y}, "Red:Cliff"), problem)); // the marker syntax is not allowed
    REQUIRE(sim::addPlaced(land, edits, entryOf(sim::EditGroup::Place, "well", {grass.x + 2, grass.y}, "Old Well"), problem));
    REQUIRE(sim::addPlaced(land, edits, entryOf(sim::EditGroup::Person, "elder", {grass.x + 3, grass.y}, "Old Mara"), problem));
    REQUIRE(sim::addPlaced(land, edits, entryOf(sim::EditGroup::Person, "hunter", {grass.x + 4, grass.y}), problem));

    CHECK(sim::placedNames(edits, sim::EditGroup::Place) == std::vector<std::string>{"Red Cliff", "Old Well"});
    CHECK(sim::placedNames(edits, sim::EditGroup::Person) == std::vector<std::string>{"Old Mara", "hunter"}); // a person with no name is offered by its kind
}

TEST_CASE("US-204 Move, take away and Undo: each change can be taken back and done again") {
    sim::Region land(1, config());
    sim::RegionEdits edits;
    const sim::Tile grass = findTile(land, sim::Biome::Steppe);
    const sim::Tile water = findTile(land, sim::Biome::Water);
    std::string problem;
    const auto added = sim::addPlaced(land, edits, entryOf(sim::EditGroup::Thing, "boulder", grass), problem);
    REQUIRE(added);
    const sim::RegionEdits afterAdd = edits;

    const auto moved = sim::movePlaced(land, edits, added->id, grass.x + 3, grass.y, problem);
    REQUIRE(moved);
    CHECK(sim::findPlaced(edits, "t-0001")->x == grass.x + 3);
    CHECK(moved->id == "t-0001"); // a move keeps the id
    CHECK_FALSE(sim::movePlaced(land, edits, added->id, water.x, water.y, problem)); // not onto water
    CHECK_FALSE(sim::movePlaced(land, edits, "t-0099", grass.x, grass.y, problem));
    sim::applyPlacedChange(edits, *moved, false);
    CHECK(edits == afterAdd);
    sim::applyPlacedChange(edits, *moved, true);
    CHECK(sim::findPlaced(edits, "t-0001")->x == grass.x + 3);

    const auto removed = sim::removePlaced(edits, "t-0001", problem);
    REQUIRE(removed);
    CHECK(edits.placed.empty());
    sim::applyPlacedChange(edits, *removed, false);
    CHECK(sim::findPlaced(edits, "t-0001") != nullptr);
    sim::applyPlacedChange(edits, *moved, false);
    sim::applyPlacedChange(edits, *added, false);
    CHECK(edits.placed.empty()); // undone back to the start
}

TEST_CASE("US-204 Tombstone: a seed thing taken away stays away through saving and through a regenerated land") {
    sim::Region land(1, config());
    sim::RegionEdits edits;
    sim::Tile tree{-1, -1};
    for (int cy = 2; cy < 6 && tree.x < 0; ++cy) {
        for (int cx = 2; cx < 6 && tree.x < 0; ++cx) {
            for (const sim::Resource& resource : land.chunk(cx, cy).resources) {
                if (resource.kind == sim::ResourceKind::Wood) {
                    tree = {resource.x, resource.y};
                    break;
                }
            }
        }
    }
    REQUIRE(tree.x >= 0);
    std::string problem;
    const auto hidden = sim::hideSeedThing(land, edits, tree.x, tree.y, problem);
    REQUIRE(hidden);
    CHECK(hidden->after->removal);
    CHECK(hidden->after->kind == "wood");
    CHECK_FALSE(sim::hideSeedThing(land, edits, tree.x, tree.y, problem)); // already away
    const sim::Tile bare = findTile(land, sim::Biome::Water);
    CHECK_FALSE(sim::hideSeedThing(land, edits, bare.x, bare.y, problem)); // the seed has nothing there
    CHECK(sim::findConflicts(land, edits).empty());

    Folder folder;
    sim::WorldFile world;
    world.seed = 1;
    world.edits = edits;
    sim::saveWorld(world, folder.path / "w.json", config());
    const sim::WorldFile loaded = sim::loadWorld(folder.path / "w.json", config());
    CHECK(loaded.edits == edits);

    // The land is made again with other settings: the tombstone is kept where it is, and when the seed has nothing there any more it is listed, not dropped.
    sim::RegionConfig stony = config();
    stony.mountainLevel = 300;
    sim::Region regenerated(1, stony);
    CHECK(sim::findPlaced(loaded.edits, hidden->id) != nullptr);
    const std::vector<sim::EditConflict> conflicts = sim::findConflicts(regenerated, loaded.edits);
    for (const sim::EditConflict& conflict : conflicts) CHECK(conflict.id == hidden->id);
    CHECK(conflicts.size() <= 1);
}

TEST_CASE("US-204 Stable ids: the same ids come back from the file, and survive a land that changed under them") {
    sim::Region land(1, config());
    sim::RegionEdits edits;
    const sim::Tile grass = findTile(land, sim::Biome::Steppe);
    std::string problem;
    REQUIRE(sim::addPlaced(land, edits, entryOf(sim::EditGroup::Thing, "clan-fire", grass), problem));
    REQUIRE(sim::addPlaced(land, edits, entryOf(sim::EditGroup::Person, "elder", {grass.x + 1, grass.y}, "Old Mara"), problem));
    REQUIRE(sim::addPlaced(land, edits, entryOf(sim::EditGroup::Place, "shrine", {grass.x + 2, grass.y}, "Red Cliff"), problem));
    REQUIRE(sim::addPlaced(land, edits, entryOf(sim::EditGroup::Thing, "boulder", {grass.x + 3, grass.y}), problem));
    REQUIRE(sim::removePlaced(edits, "t-0001", problem));

    Folder folder;
    sim::WorldFile world;
    world.seed = 1;
    world.edits = edits;
    sim::saveWorld(world, folder.path / "w.json", config());
    sim::WorldFile loaded = sim::loadWorld(folder.path / "w.json", config());
    CHECK(sim::findPlaced(loaded.edits, "t-0002") != nullptr);
    CHECK(sim::findPlaced(loaded.edits, "p-0001")->name == "Old Mara");
    CHECK(sim::findPlaced(loaded.edits, "l-0001")->name == "Red Cliff");
    CHECK(sim::findPlaced(loaded.edits, "t-0001") == nullptr);
    // A new thing never takes the id of one that is still there.
    REQUIRE(sim::addPlaced(land, loaded.edits, entryOf(sim::EditGroup::Thing, "boulder", {grass.x + 4, grass.y}), problem));
    CHECK(sim::findPlaced(loaded.edits, "t-0003") != nullptr);

    // Another mountain level makes other land under the same entries: nothing moves, nothing is renamed.
    sim::RegionConfig other = config();
    other.mountainLevel = 400;
    sim::Region regenerated(1, other);
    const std::vector<sim::PlacedEdit> before = loaded.edits.placed;
    (void)sim::findConflicts(regenerated, loaded.edits);
    CHECK(loaded.edits.placed == before);
}

TEST_CASE("US-204 Conflicts: Move puts an entry on the nearest tile that can hold it, Remove takes it off") {
    sim::Region land(1, config());
    sim::RegionEdits edits;
    const sim::Tile grass = findTile(land, sim::Biome::Steppe);
    std::string problem;
    REQUIRE(sim::addPlaced(land, edits, entryOf(sim::EditGroup::Thing, "boulder", grass), problem));
    REQUIRE(sim::addPlaced(land, edits, entryOf(sim::EditGroup::Person, "elder", {grass.x + 6, grass.y}, "Old Mara"), problem));
    REQUIRE(land.setTileEdit(grass.x, grass.y, sim::Biome::Water)); // the land changes under the boulder
    const std::vector<sim::EditConflict> conflicts = sim::findConflicts(land, edits);
    REQUIRE(conflicts.size() == 1);
    CHECK(conflicts[0].id == "t-0001");

    sim::RegionEdits dropped = edits;
    CHECK(sim::resolveConflicts(land, dropped, conflicts, sim::ConflictChoice::Keep).empty());
    CHECK(dropped == edits);
    const std::vector<sim::PlacedChange> removed = sim::resolveConflicts(land, dropped, conflicts, sim::ConflictChoice::Remove);
    REQUIRE(removed.size() == 1);
    CHECK(sim::findPlaced(dropped, "t-0001") == nullptr);
    CHECK(sim::findPlaced(dropped, "p-0001") != nullptr);

    const std::vector<sim::PlacedChange> moved = sim::resolveConflicts(land, edits, conflicts, sim::ConflictChoice::Move);
    REQUIRE(moved.size() == 1);
    const sim::PlacedEdit* boulder = sim::findPlaced(edits, "t-0001");
    REQUIRE(boulder != nullptr);
    CHECK(sim::walkable(land.biomeAt(boulder->x, boulder->y)));
    CHECK(std::max(std::abs(boulder->x - grass.x), std::abs(boulder->y - grass.y)) <= 2); // the nearest ring that holds it
    CHECK(sim::findConflicts(land, edits).empty());
}

TEST_CASE("US-204 Properties: a placed entry sets only what its group allows, and they round-trip in the file") {
    CHECK(sim::placedPropertyProblem(sim::EditGroup::Person, "hp", "80").empty());
    CHECK_FALSE(sim::placedPropertyProblem(sim::EditGroup::Person, "hp", "0").empty());
    CHECK_FALSE(sim::placedPropertyProblem(sim::EditGroup::Person, "hp", "many").empty());
    CHECK(sim::placedPropertyProblem(sim::EditGroup::Person, "attitude", "friendly").empty());
    CHECK_FALSE(sim::placedPropertyProblem(sim::EditGroup::Person, "colour", "red").empty());
    CHECK(sim::placedPropertyProblem(sim::EditGroup::Thing, "regrow.duration", "60").empty());
    CHECK_FALSE(sim::placedPropertyProblem(sim::EditGroup::Thing, "regrow.speed", "60").empty());
    CHECK(sim::placedPropertyProblem(sim::EditGroup::Place, "tags", "forage,shelter").empty());
    CHECK_FALSE(sim::placedPropertyProblem(sim::EditGroup::Place, "hp", "5").empty());

    sim::Region land(1, config());
    sim::RegionEdits edits;
    const sim::Tile grass = findTile(land, sim::Biome::Steppe);
    std::string problem;
    sim::PlacedEdit person = entryOf(sim::EditGroup::Person, "elder", grass, "Old Mara");
    person.npcClass = "elder";
    REQUIRE(sim::addPlaced(land, edits, person, problem));
    REQUIRE(sim::setPlacedProperty(edits, "p-0001", "hp", "80", problem));
    REQUIRE(sim::setPlacedProperty(edits, "p-0001", "attitude", "friendly", problem));
    CHECK_FALSE(sim::setPlacedProperty(edits, "p-0001", "hp", "-3", problem));
    CHECK_FALSE(problem.empty());
    REQUIRE(sim::setPlacedProperty(edits, "p-0001", "attitude", "", problem)); // an empty value clears it
    CHECK(sim::findPlaced(edits, "p-0001")->properties.size() == 1);

    Folder folder;
    sim::WorldFile world;
    world.seed = 1;
    world.edits = edits;
    sim::saveWorld(world, folder.path / "w.json", config());
    const sim::WorldFile loaded = sim::loadWorld(folder.path / "w.json", config());
    CHECK(loaded.edits == edits);
    CHECK(sim::findPlaced(loaded.edits, "p-0001")->npcClass == "elder");
    CHECK(sim::worldText(loaded, config()) == sim::worldText(world, config())); // the same text again

    // A bad property in a hand-edited file names the file and the field.
    std::string text = sim::worldText(world, config());
    const std::size_t at = text.find("\"hp\"");
    REQUIRE(at != std::string::npos);
    text.replace(at, 4, "\"mood\"");
    std::ofstream(folder.path / "bad.json") << text;
    try {
        (void)sim::loadWorld(folder.path / "bad.json", config());
        FAIL("a property the person cannot set was accepted");
    } catch (const odysseus::sim::DataError& error) {
        const std::string what = error.what();
        CHECK(what.find("bad.json") != std::string::npos);
        CHECK(what.find("mood") != std::string::npos);
    }
}

TEST_CASE("US-204 Cap: a world keeps at most kMaxWorldEntries edits") {
    sim::Region land(1, config());
    sim::RegionEdits edits;
    for (std::size_t i = 0; i < sim::kMaxWorldEntries; ++i) edits.tiles.push_back({static_cast<int>(i % 200) + 20, static_cast<int>(i / 200) + 20, sim::Biome::Steppe});
    std::string problem;
    CHECK_FALSE(sim::addPlaced(land, edits, entryOf(sim::EditGroup::Thing, "boulder", {30, 30}), problem));
    CHECK(problem.find("at most") != std::string::npos);
}

// ---- US-205 Camps and resources ----

#include "sim/region_save.h"
#include "sim/rivals.h"
#include "sim/world.h"

namespace {

// A tile at least `far` tiles from the start where the site check passes and the ground is open.
sim::Tile goodFarSite(sim::Region& land, int far, int skipFrom = 0) {
    for (int y = 30; y < 226; y += 3) {
        for (int x = 30 + skipFrom; x < 226; x += 3) {
            if (std::max(std::abs(x - land.start().x), std::abs(y - land.start().y)) < far) continue;
            const sim::Biome biome = land.biomeAt(x, y);
            if ((biome == sim::Biome::Steppe || biome == sim::Biome::Forest) && land.goodSite({x, y})) return {x, y};
        }
    }
    return {-1, -1};
}

sim::Tile findResource(sim::Region& land, sim::ResourceKind kind) {
    for (int cy = 1; cy < 7; ++cy) {
        for (int cx = 1; cx < 7; ++cx) {
            for (const sim::Resource& resource : land.chunk(cx, cy).resources) {
                if (resource.kind == kind) return {resource.x, resource.y};
            }
        }
    }
    return {-1, -1};
}

} // namespace

TEST_CASE("US-205 Rules: a camp in water, on poor ground, too close or doubled is refused with the reason; placing anyway is allowed") {
    sim::Region land(1, config());
    sim::RegionEdits edits;
    const sim::Tile water = findTile(land, sim::Biome::Water);
    const sim::Tile site = goodFarSite(land, 60);
    REQUIRE(site.x >= 0);
    std::string problem;
    sim::PlacedEdit rival = entryOf(sim::EditGroup::Camp, "rival", water, "the Crow Clan");
    CHECK_FALSE(sim::addPlaced(land, edits, rival, problem));
    CHECK(problem.find("water") != std::string::npos);

    rival.x = site.x;
    rival.y = site.y;
    REQUIRE(sim::addPlaced(land, edits, rival, problem));
    CHECK(sim::findPlaced(edits, "c-0001")->name == "the Crow Clan");
    CHECK_FALSE(sim::addPlaced(land, edits, entryOf(sim::EditGroup::Camp, "rival", {site.x + 3, site.y}), problem));
    CHECK(problem.find("too close") != std::string::npos);
    CHECK_FALSE(sim::addPlaced(land, edits, entryOf(sim::EditGroup::Camp, "tribe", {site.x + 40, site.y}), problem));
    REQUIRE(sim::addPlaced(land, edits, entryOf(sim::EditGroup::Camp, "player", land.start()), problem));
    CHECK_FALSE(sim::addPlaced(land, edits, entryOf(sim::EditGroup::Camp, "player", {site.x, site.y + 30}), problem)); // one player camp
    CHECK(problem.find("already") != std::string::npos);

    // A tile with no water and food within reach: refused, unless placed anyway.
    sim::Tile bare{-1, -1};
    for (int y = 30; y < 226 && bare.x < 0; y += 5) {
        for (int x = 30; x < 226 && bare.x < 0; x += 5) {
            if (land.biomeAt(x, y) == sim::Biome::Steppe && !land.goodSite({x, y}) && std::abs(x - site.x) > 20 && std::abs(x - land.start().x) > 20) bare = {x, y};
        }
    }
    REQUIRE(bare.x >= 0);
    sim::PlacedEdit poor = entryOf(sim::EditGroup::Camp, "rival", bare);
    CHECK_FALSE(sim::addPlaced(land, edits, poor, problem));
    CHECK(problem.find("water and food") != std::string::npos);
    poor.forced = true;
    const auto forced = sim::addPlaced(land, edits, poor, problem);
    REQUIRE(forced);
    CHECK(forced->after->forced);
    CHECK(sim::findConflicts(land, edits).empty()); // a forced camp is not a conflict
}

TEST_CASE("US-205 Camps: the rival clan lives at the placed camp, the player starts at theirs, and the file round-trips") {
    sim::Region land(1, config());
    sim::RegionEdits edits;
    const sim::Tile site = goodFarSite(land, 60);
    REQUIRE(site.x >= 0);
    std::string problem;
    sim::PlacedEdit rival = entryOf(sim::EditGroup::Camp, "rival", site, "the Crow Clan");
    rival.properties.push_back({"people", "14"});
    REQUIRE(sim::addPlaced(land, edits, rival, problem));
    sim::Tile home{-1, -1};
    for (int y = 30; y < 226 && home.x < 0; y += 3) {
        for (int x = 30; x < 226 && home.x < 0; x += 3) {
            if (std::max(std::abs(x - site.x), std::abs(y - site.y)) >= 20 && land.biomeAt(x, y) == sim::Biome::Steppe && land.goodSite({x, y}) && (x != land.start().x || y != land.start().y)) home = {x, y};
        }
    }
    REQUIRE(home.x >= 0);
    REQUIRE(sim::addPlaced(land, edits, entryOf(sim::EditGroup::Camp, "player", {home.x, home.y}), problem));

    Folder folder;
    sim::WorldFile world;
    world.seed = 1;
    world.edits = edits;
    sim::saveWorld(world, folder.path / "w.json", config());
    const sim::WorldFile loaded = sim::loadWorld(folder.path / "w.json", config());
    CHECK(loaded.edits == edits);

    sim::Region played = sim::makeWorldRegion(loaded, config());
    CHECK(played.start().x == home.x);
    CHECK(played.start().y == home.y);
    const std::vector<sim::CampSite> camps = sim::campsOf(loaded.edits);
    REQUIRE(camps.size() == 2);
    CHECK(camps[0].player); // the player first
    const sim::Rivals rivals(played, played.start(), 1 ^ 0x5151ULL, sim::loadSimConfig(ODYSSEUS_DATA_DIR), camps);
    REQUIRE(rivals.clans().size() >= 2);
    CHECK(rivals.clans()[0].camp.x == site.x);
    CHECK(rivals.clans()[0].camp.y == site.y);
    CHECK(rivals.clans()[0].name == "the Crow Clan");
    CHECK(rivals.clans()[0].world->population() == 14);
    CHECK(sim::Rivals::distanceTiles(rivals.clans()[1].camp, site) >= sim::Rivals::kMinimumBetweenClansTiles); // the other clan keeps away from it

    // Taking the camp off gives the generator start back.
    sim::RegionEdits without = loaded.edits;
    REQUIRE(sim::removePlaced(without, "c-0002", problem));
    sim::applyPlacedToRegion(played, without);
    sim::Region plain(1, config());
    CHECK(played.start().x == plain.start().x);
}

TEST_CASE("US-205 Resources: a flint spot set to 50 gives 50 flint, a hidden spot is gone, a new spot is there") {
    sim::Region probe(1, config());
    const sim::Tile flint = findResource(probe, sim::ResourceKind::Flint);
    const sim::Tile tree = findResource(probe, sim::ResourceKind::Wood);
    REQUIRE(flint.x >= 0);
    REQUIRE(tree.x >= 0);
    sim::RegionEdits edits;
    std::string problem;
    sim::PlacedEdit spot = entryOf(sim::EditGroup::Resource, "flint", flint);
    spot.properties.push_back({"amount", "50"});
    REQUIRE(sim::addPlaced(probe, edits, spot, problem));
    CHECK_FALSE(sim::setPlacedProperty(edits, "r-0001", "amount", "0", problem));
    CHECK_FALSE(sim::setPlacedProperty(edits, "r-0001", "people", "3", problem));
    REQUIRE(sim::hideSeedThing(probe, edits, tree.x, tree.y, problem));
    const sim::Tile grass = findTile(probe, sim::Biome::Steppe);
    sim::PlacedEdit fresh = entryOf(sim::EditGroup::Resource, "berries", {grass.x + 1, grass.y});
    REQUIRE(sim::addPlaced(probe, edits, fresh, problem));
    CHECK_FALSE(sim::addPlaced(probe, edits, entryOf(sim::EditGroup::Resource, "gold", {grass.x + 2, grass.y}), problem));

    Folder folder;
    sim::WorldFile world;
    world.seed = 1;
    world.edits = edits;
    sim::saveWorld(world, folder.path / "w.json", config());
    const sim::WorldFile loaded = sim::loadWorld(folder.path / "w.json", config());
    REQUIRE(loaded.edits.placed.size() == edits.placed.size()); // the file lists the groups in turn, so the order may differ
    for (const sim::PlacedEdit& entry : edits.placed) {
        REQUIRE(sim::findPlaced(loaded.edits, entry.id) != nullptr);
        CHECK(*sim::findPlaced(loaded.edits, entry.id) == entry);
    }
    sim::Region land = sim::makeWorldRegion(loaded, config());

    CHECK(land.resourcesNear(flint, 0).size() == 1);
    CHECK(land.resourcesNear(flint, 0)[0].amount == 50);
    sim::Date today;
    int taken = 0;
    while (land.harvest(flint.x, flint.y, today) && taken < 100) ++taken;
    CHECK(taken == 50);
    CHECK_FALSE(land.harvest(flint.x, flint.y, today)); // nothing left

    CHECK(land.resourcesNear(tree, 0).empty()); // the hidden tree is gone
    const auto berries = land.resourcesNear({grass.x + 1, grass.y}, 0);
    REQUIRE(berries.size() == 1);
    CHECK(berries[0].kind == sim::ResourceKind::Berries);
    CHECK(sim::findConflicts(land, loaded.edits).empty());

    // Taking every entry away gives the seed own resources back.
    sim::applyPlacedToRegion(land, sim::RegionEdits{});
    CHECK(land.resourcesNear(tree, 0).size() == 1);
    CHECK(land.resourcesNear(flint, 0)[0].amount == 1);
}

TEST_CASE("US-205 Saves: what is left of a spot of 50 is kept in the run save") {
    sim::Region probe(1, config());
    const sim::Tile flint = findResource(probe, sim::ResourceKind::Flint);
    REQUIRE(flint.x >= 0);
    sim::Region land(1, config());
    land.setResourceAmount(flint.x, flint.y, 50);
    sim::Date today;
    for (int i = 0; i < 3; ++i) REQUIRE(land.harvest(flint.x, flint.y, today));
    Folder folder;
    sim::saveRegion(land, folder.path / "region.json");
    sim::LoadedRegion loaded = sim::loadRegion(folder.path / "region.json", config());
    int rest = 0;
    while (loaded.region.harvest(flint.x, flint.y, today) && rest < 100) ++rest;
    CHECK(rest == 47);
}
