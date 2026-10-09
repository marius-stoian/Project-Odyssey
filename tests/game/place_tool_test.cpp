// US-204 Things, people and places in the region: the Place, Move and Take away tools of the Region view, the world file they write, the level the game makes from it,
// and the pickers and quest markers that name the places and people put on the land.
#include "camp.h"

#include "game/catalogs.h"
#include "game/game_rules.h"
#include "game/level.h"
#include "game/region_level.h"
#include "game/region_view.h"
#include "luna/engine/renderer.h"
#include "sim/world_file.h"
#include "sim/world_places.h"

#include <algorithm>
#include <string>

using namespace camp_support;
namespace sim = odysseus::sim;
namespace eng = luna::engine;

namespace {

sim::RegionConfig config() { return sim::loadRegionConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "region.json"); }

eng::Intents click(int x, int y) {
    eng::Intents intents;
    eng::Pointer pointer;
    pointer.x = x;
    pointer.y = y;
    pointer.held[static_cast<std::size_t>(eng::PointerButton::Left)] = true;
    pointer.pressed[static_cast<std::size_t>(eng::PointerButton::Left)] = true;
    intents.setPointer(pointer);
    return intents;
}

eng::Intents undoPressed() {
    eng::Intents intents;
    intents.set(eng::Intent::Undo, true, true);
    return intents;
}

struct Rig {
    fs::path folder;
    game::RegionView view{960, 540, [this](const std::string& text) { said.push_back(text); }};
    std::vector<std::string> said;
    game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
    game::Catalogs catalogs = game::loadCatalogs(ODYSSEUS_DATA_DIR);
    explicit Rig(const std::string& name) : folder(fs::temp_directory_path() / "odysseus-us204" / name) {
        fs::remove_all(folder);
        fs::create_directories(folder);
        view.setWorldsFolder(folder);
        view.open(1, config());
    }
    // A steppe tile in a row of `run` steppe tiles, well inside the region.
    sim::Tile openRun(int run = 8) const {
        const sim::Region& land = *view.region();
        for (int y = 40; y < 200; ++y) {
            for (int x = 40; x < 200; ++x) {
                bool open = true;
                for (int i = 0; i < run && open; ++i) open = land.biomeAt(x + i, y) == sim::Biome::Steppe;
                if (open) return {x, y};
            }
        }
        return {-1, -1};
    }
    std::string objectKind() const { return !definitions.objects.empty() ? definitions.objects.front() : definitions.plants.front(); }
};

} // namespace

TEST_CASE("US-204 Place: a fire pit and an NPC placed with the tools are saved as overrides and appear in the game") {
    Rig rig("place");
    const sim::Tile open = rig.openRun();
    REQUIRE(open.x >= 0);

    rig.view.setTool(game::RegionTool::Place);
    rig.view.setPlaceGroup(sim::EditGroup::Thing);
    rig.view.setPlaceKind(rig.objectKind());
    const std::string pit = rig.view.placeEntry(open.x, open.y);
    CHECK(pit == "t-0001");
    rig.view.setPlaceGroup(sim::EditGroup::Person);
    CHECK(rig.view.placeKind().empty()); // a kind of one group means nothing in another
    rig.view.setPlaceKind("goblin");
    rig.view.setPlaceName("Old Mara");
    const std::string person = rig.view.placeEntry(open.x + 2, open.y);
    CHECK(person == "p-0001");
    rig.view.setPlaceGroup(sim::EditGroup::Place);
    rig.view.setPlaceKind("shrine");
    rig.view.setPlaceName("Red Cliff");
    CHECK(rig.view.placeEntry(open.x + 4, open.y) == "l-0001");
    CHECK(rig.view.placeName().empty()); // the next place needs a name of its own
    CHECK(rig.view.edits().placed.size() == 3);
    CHECK(rig.view.worldDirty());

    REQUIRE(rig.view.saveWorld());
    const sim::WorldFile saved = sim::loadWorld(rig.view.worldFile(), config());
    CHECK(saved.edits.placed == rig.view.edits().placed);
    CHECK(saved.edits.tiles.empty()); // only the three entries differ from the seed

    // The game makes its level from the world file: the pit, the person and the place are there.
    sim::Region land = sim::makeWorldRegion(saved, config());
    std::vector<std::string> problems;
    const game::Level level = game::levelFromRegion(land, rig.definitions, rig.catalogs, saved.edits, &problems);
    CHECK(problems.empty());
    const auto plant = std::find_if(level.plants.begin(), level.plants.end(), [&](const game::PlacedPlant& p) { return p.kind == rig.objectKind() && p.feet.x == open.x * game::kTileSize + game::kTileSize / 2; });
    CHECK(plant != level.plants.end());
    const auto mara = std::find_if(level.characters.begin(), level.characters.end(), [](const game::PlacedCharacter& c) { return c.name == "Old Mara"; });
    REQUIRE(mara != level.characters.end());
    CHECK(mara->kind == "goblin");
    REQUIRE(level.places.size() == 1);
    CHECK(level.places[0].name == "red-cliff"); // a level names its places with words
    CHECK(level.places[0].tags.front() == "shrine");

    // Loading the same file twice gives the same level, ids and all.
    sim::Region again = sim::makeWorldRegion(saved, config());
    CHECK(game::levelFromRegion(again, rig.definitions, rig.catalogs, saved.edits).plants == level.plants);
    CHECK(game::levelFromRegion(again, rig.definitions, rig.catalogs, saved.edits).characters == level.characters);
}

TEST_CASE("US-204 Place: a click with the Place tool puts the kind on the tile under the pointer") {
    Rig rig("click");
    const sim::Tile open = rig.openRun();
    REQUIRE(open.x >= 0);
    rig.view.setZoom(game::RegionView::kMaxZoom);
    rig.view.centreOn(open.x + 0.5, open.y + 0.5);
    rig.view.setTool(game::RegionTool::Place);
    rig.view.setPlaceGroup(sim::EditGroup::Thing);
    rig.view.setPlaceKind(rig.objectKind());
    rig.view.update(click(480, 270)); // the middle of the screen is the centred tile
    REQUIRE(rig.view.edits().placed.size() == 1);
    CHECK(rig.view.edits().placed[0].x == open.x);
    CHECK(rig.view.edits().placed[0].y == open.y);
    CHECK(rig.view.selected() == "t-0001");

    rig.view.update(undoPressed());
    CHECK(rig.view.edits().placed.empty());
    CHECK(rig.view.selected().empty());
}

TEST_CASE("US-204 Move and take away: each is one step of Undo, a seed tree is hidden by a tombstone that survives the file") {
    Rig rig("move");
    const sim::Tile open = rig.openRun();
    REQUIRE(open.x >= 0);
    rig.view.setPlaceGroup(sim::EditGroup::Thing);
    rig.view.setPlaceKind(rig.objectKind());
    REQUIRE_FALSE(rig.view.placeEntry(open.x, open.y).empty());
    const std::size_t steps = rig.view.history().size();

    rig.view.setTool(game::RegionTool::Move);
    CHECK(rig.view.selectAt(open.x, open.y));
    CHECK(rig.view.moveSelectedTo(open.x + 3, open.y));
    CHECK(rig.view.entryAt(open.x + 3, open.y) != nullptr);
    CHECK(rig.view.entryAt(open.x + 3, open.y)->id == "t-0001"); // a move keeps the id
    CHECK(rig.view.history().size() == steps + 1);
    CHECK(rig.view.undo());
    CHECK(rig.view.entryAt(open.x, open.y) != nullptr);
    CHECK(rig.view.redo());
    CHECK(rig.view.entryAt(open.x + 3, open.y) != nullptr);

    // A tree of the seed.
    sim::Tile tree{-1, -1};
    for (int cy = 2; cy < 6 && tree.x < 0; ++cy) {
        for (int cx = 2; cx < 6 && tree.x < 0; ++cx) {
            for (const sim::Resource& resource : rig.view.region()->chunk(cx, cy).resources) {
                if (resource.kind == sim::ResourceKind::Wood) {
                    tree = {resource.x, resource.y};
                    break;
                }
            }
        }
    }
    REQUIRE(tree.x >= 0);
    sim::Region plain(1, config());
    const game::Level before = game::levelFromRegion(plain, rig.definitions, rig.catalogs);
    const int treeX = tree.x * game::kTileSize + game::kTileSize / 2;
    const auto hasTree = [&](const game::Level& level) {
        return std::any_of(level.plants.begin(), level.plants.end(), [&](const game::PlacedPlant& p) { return p.feet.x == treeX && p.feet.y == tree.y * game::kTileSize + game::kTileSize - 4; });
    };
    REQUIRE(hasTree(before));
    CHECK(rig.view.takeAway(tree.x, tree.y));
    REQUIRE(rig.view.saveWorld());
    const sim::WorldFile saved = sim::loadWorld(rig.view.worldFile(), config());
    sim::Region regenerated = sim::makeWorldRegion(saved, config());
    CHECK_FALSE(hasTree(game::levelFromRegion(regenerated, rig.definitions, rig.catalogs, saved.edits))); // the tree stays away
    sim::Region seedOnly(1, config());
    CHECK(hasTree(game::levelFromRegion(seedOnly, rig.definitions, rig.catalogs)));                       // and without the file it is there
    CHECK(rig.view.undo());
    CHECK(rig.view.edits().placed.size() == 1);

    CHECK(rig.view.takeAway(open.x + 3, open.y)); // an entry of the owner's own is simply removed
    CHECK(rig.view.edits().placed.empty());
}

TEST_CASE("US-204 Properties: a property of the picked entry wins over the kind in the game, a wrong one is refused") {
    Rig rig("properties");
    const sim::Tile open = rig.openRun();
    REQUIRE(open.x >= 0);
    rig.view.setPlaceGroup(sim::EditGroup::Person);
    rig.view.setPlaceKind("goblin");
    rig.view.setPlaceName("Tough Gob");
    REQUIRE_FALSE(rig.view.placeEntry(open.x, open.y).empty());
    CHECK(rig.view.setProperty("hp", "321"));
    CHECK(rig.view.setProperty("tags", "guard,loud"));
    CHECK_FALSE(rig.view.setProperty("hp", "lots"));
    CHECK_FALSE(rig.view.setProperty("colour", "red"));

    sim::Region land(1, config());
    std::vector<std::string> problems;
    const game::Level level = game::levelFromRegion(land, rig.definitions, rig.catalogs, rig.view.edits(), &problems);
    const auto gob = std::find_if(level.characters.begin(), level.characters.end(), [](const game::PlacedCharacter& c) { return c.name == "Tough Gob"; });
    REQUIRE(gob != level.characters.end());
    CHECK(gob->hp == 321);
    CHECK(gob->tags == std::vector<std::string>{"guard", "loud"});
    const game::CharacterKindDef* kind = rig.definitions.character("goblin");
    REQUIRE(kind != nullptr);
    CHECK(gob->swordDamage == kind->swordDamage); // what the entry does not set comes from the kind

    CHECK(rig.view.setProperty("hp", "")); // cleared: the kind decides again
    sim::Region cleared(1, config());
    const game::Level plain = game::levelFromRegion(cleared, rig.definitions, rig.catalogs, rig.view.edits());
    const auto back = std::find_if(plain.characters.begin(), plain.characters.end(), [](const game::PlacedCharacter& c) { return c.name == "Tough Gob"; });
    REQUIRE(back != plain.characters.end());
    CHECK(back->hp == kind->hp);
}

TEST_CASE("US-204 Conflicts: Fix: move puts an entry the changed land no longer holds on the nearest tile that can") {
    Rig rig("fix");
    const sim::Tile open = rig.openRun();
    REQUIRE(open.x >= 0);
    rig.view.setPlaceGroup(sim::EditGroup::Thing);
    rig.view.setPlaceKind(rig.objectKind());
    REQUIRE_FALSE(rig.view.placeEntry(open.x + 2, open.y).empty());
    REQUIRE(rig.view.region()->setTileEdit(open.x + 2, open.y, sim::Biome::Water)); // the land changes under it
    CHECK(sim::findConflicts(*rig.view.region(), rig.view.edits()).size() == 1);
    CHECK(rig.view.settleConflicts(sim::ConflictChoice::Move) == 1);
    CHECK(sim::findConflicts(*rig.view.region(), rig.view.edits()).empty());
    const sim::PlacedEdit* moved = sim::findPlaced(rig.view.edits(), "t-0001");
    REQUIRE(moved != nullptr);
    CHECK(sim::walkable(rig.view.region()->biomeAt(moved->x, moved->y)));
    CHECK(rig.view.undo()); // one step of Undo takes the move back
    CHECK(sim::findPlaced(rig.view.edits(), "t-0001")->x == open.x + 2);
    CHECK(rig.view.settleConflicts(sim::ConflictChoice::Remove) == 1);
    CHECK(rig.view.edits().placed.empty());
}

TEST_CASE("US-204 Pickers and Places: a quest or dialogue field offers the people and places of the region, and a marker points at a named place") {
    const fs::path data = dataCopy("us204-pickers");
    const game::Definitions definitions = game::loadDefinitions(data);
    game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
    level.characters.clear();
    level.pickups.clear();
    level.places.clear();
    level.places.push_back({"red-cliff", {level.heroStart.x + 96, level.heroStart.y + 32}, {"shrine"}});
    game::saveLevel(level, definitions, data / "pickers-level.json");
    eng::RecordingRenderer renderer;
    game::OdysseyGame odyssey(data, data / "pickers-level.json");
    odyssey.setViewScales(1, 1);
    odyssey.start(renderer);

    // A named place is a spot a quest marker can point at.
    const std::optional<std::pair<double, double>> marker = game::questMarkerPosition(odyssey, "place:Red Cliff");
    REQUIRE(marker);
    CHECK(marker->first == doctest::Approx(level.heroStart.x + 96));
    CHECK(marker->second == doctest::Approx(level.heroStart.y + 32));

    // The places and people put on the region are offered next to those of the level.
    game::RegionView& view = odyssey.editor().regionView();
    view.open(1, config());
    const sim::Region& land = *view.region();
    sim::Tile open{-1, -1};
    for (int y = 40; y < 200 && open.x < 0; ++y) {
        for (int x = 40; x < 200 && open.x < 0; ++x) {
            if (land.biomeAt(x, y) == sim::Biome::Steppe && land.biomeAt(x + 1, y) == sim::Biome::Steppe) open = {x, y};
        }
    }
    REQUIRE(open.x >= 0);
    view.setPlaceGroup(sim::EditGroup::Place);
    view.setPlaceKind("market");
    view.setPlaceName("Salt Market");
    REQUIRE_FALSE(view.placeEntry(open.x, open.y).empty());
    view.setPlaceGroup(sim::EditGroup::Person);
    view.setPlaceKind("goblin");
    view.setPlaceName("Old Mara");
    REQUIRE_FALSE(view.placeEntry(open.x + 1, open.y).empty());

    const auto has = [&](const std::string& catalog, const std::string& name) {
        const std::vector<std::string> names = odyssey.suggestionNames(catalog);
        return std::find(names.begin(), names.end(), name) != names.end();
    };
    CHECK(has("places", "red-cliff"));
    CHECK(has("places", "Salt Market"));
    CHECK(has("markers", "place:Salt Market"));
    CHECK(has("markers", "npc:old-mara"));
    CHECK(has("people", "Old Mara"));
    CHECK(has("goals", "goto salt-market"));
    CHECK(has("goals", "goto red-cliff"));
    CHECK(has("goals", "talk old-mara"));
}

TEST_CASE("US-204 Palette: the Place tool offers the plants, objects, animals, people and classes the data holds") {
    const fs::path data = dataCopy("us204-palette");
    const game::Definitions definitions = game::loadDefinitions(data);
    game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
    game::saveLevel(level, definitions, data / "palette-level.json");
    eng::RecordingRenderer renderer;
    game::OdysseyGame odyssey(data, data / "palette-level.json");
    odyssey.setViewScales(1, 1);
    odyssey.start(renderer);
    const game::RegionView::PlacePalette& palette = odyssey.editor().regionView().palette();
    CHECK_FALSE(palette.things.empty());
    CHECK_FALSE(palette.people.empty());
    CHECK_FALSE(palette.places.empty());
    CHECK(std::find(palette.people.begin(), palette.people.end(), "goblin") != palette.people.end());
    CHECK(std::find(palette.classes.begin(), palette.classes.end(), "trader") != palette.classes.end());
    for (const std::string& object : definitions.objects) CHECK(std::find(palette.things.begin(), palette.things.end(), object) != palette.things.end());
}

// ---- US-205 Camps and resources ----

TEST_CASE("US-205 Camps: a rival camp placed with the tool is refused in water, allowed anyway on poor ground, and moved with Move") {
    Rig rig("camps");
    sim::Region& land = *rig.view.region();
    sim::Tile site{-1, -1};
    for (int y = 30; y < 226 && site.x < 0; y += 3) {
        for (int x = 30; x < 226 && site.x < 0; x += 3) {
            if (std::max(std::abs(x - land.start().x), std::abs(y - land.start().y)) >= 60 && land.biomeAt(x, y) == sim::Biome::Steppe && land.goodSite({x, y})) site = {x, y};
        }
    }
    REQUIRE(site.x >= 0);
    sim::Tile water{-1, -1};
    for (int y = 30; y < 226 && water.x < 0; ++y) {
        for (int x = 30; x < 226 && water.x < 0; ++x) {
            if (land.biomeAt(x, y) == sim::Biome::Water) water = {x, y};
        }
    }
    rig.view.setPlaceGroup(sim::EditGroup::Camp);
    rig.view.setPlaceKind("rival");
    rig.view.setPlaceName("the Crow Clan");
    CHECK(rig.view.placeEntry(water.x, water.y).empty()); // refused
    REQUIRE_FALSE(rig.said.empty());
    CHECK(rig.said.back().find("water") != std::string::npos);
    CHECK(rig.view.edits().placed.empty());

    REQUIRE(rig.view.placeEntry(site.x, site.y) == "c-0001");
    rig.view.setTool(game::RegionTool::Move);
    CHECK(rig.view.selectAt(site.x, site.y));
    int step = 1; // the next tile along that passes the site check too (a moved camp is checked like a new one)
    while (step < 12 && !(land.biomeAt(site.x + step, site.y) == sim::Biome::Steppe && land.goodSite({site.x + step, site.y}))) ++step;
    CHECK(rig.view.moveSelectedTo(site.x + step, site.y)); // moved: the rival clan starts there now
    REQUIRE_FALSE(sim::campsOf(rig.view.edits()).empty());
    CHECK(sim::campsOf(rig.view.edits())[0].at.x == site.x + step);
    CHECK(rig.view.undo());
    REQUIRE_FALSE(sim::campsOf(rig.view.edits()).empty());
    CHECK(sim::campsOf(rig.view.edits())[0].at.x == site.x);
}

TEST_CASE("US-205 Resources: Set amount=50 on a flint spot of the seed gives 50, the view and Undo follow") {
    Rig rig("resources");
    sim::Region& land = *rig.view.region();
    sim::Tile flint{-1, -1};
    for (int cy = 1; cy < 7 && flint.x < 0; ++cy) {
        for (int cx = 1; cx < 7 && flint.x < 0; ++cx) {
            for (const sim::Resource& resource : land.chunk(cx, cy).resources) {
                if (resource.kind == sim::ResourceKind::Flint) flint = {resource.x, resource.y};
            }
        }
    }
    REQUIRE(flint.x >= 0);
    rig.view.setPlaceGroup(sim::EditGroup::Resource);
    rig.view.setPlaceKind("flint");
    REQUIRE(rig.view.placeEntry(flint.x, flint.y) == "r-0001");
    CHECK(rig.view.setProperty("amount", "50"));
    CHECK_FALSE(rig.view.setProperty("amount", "many"));
    CHECK(land.resourcesNear(flint, 0)[0].amount == 50);
    REQUIRE(rig.view.saveWorld());
    const sim::WorldFile saved = sim::loadWorld(rig.view.worldFile(), config());
    sim::Region played = sim::makeWorldRegion(saved, config());
    sim::Date today;
    int taken = 0;
    while (played.harvest(flint.x, flint.y, today) && taken < 100) ++taken;
    CHECK(taken == 50);

    CHECK(rig.view.undo()); // the amount
    CHECK(land.resourcesNear(flint, 0)[0].amount == 1);
    CHECK(rig.view.undo()); // the entry
    CHECK(rig.view.edits().placed.empty());
    CHECK(land.resourcesNear(flint, 0)[0].amount == 1);
}
