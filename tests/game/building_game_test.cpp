// US-250 .. US-252: buildings in the game: their own list in the level, the Build menu and placing, building from a blueprint with materials, cancelling,
// pieces and rooms.
#include "camp.h"

#include "game/game_rules.h"

#include <optional>
#include <string>

using namespace camp_support;
namespace buildings = odysseus::sim::buildings;

namespace {

// The pointer is on a world point (the game is at zoom 1 here, so a virtual pixel is a world pixel) and, when `press`, the left button goes down.
luna::engine::Intents pointerAt(game::OdysseyGame& odyssey, double worldX, double worldY, bool press) {
    luna::engine::Intents intents;
    luna::engine::Pointer pointer;
    const luna::engine::Rect view = odyssey.cameraView();
    pointer.x = static_cast<int>(worldX) - view.x;
    pointer.y = static_cast<int>(worldY) - view.y;
    pointer.held[static_cast<std::size_t>(luna::engine::PointerButton::Left)] = press;
    pointer.pressed[static_cast<std::size_t>(luna::engine::PointerButton::Left)] = press;
    intents.setPointer(pointer);
    return intents;
}

std::pair<int, int> heroCell(const game::OdysseyGame& odyssey) {
    return game::BuildingLayer::cellOf(odyssey.hero().feetX(), odyssey.hero().feetY());
}

// A top-left cell near the hero where `kind` stands and whose middle is within `reach` cells of the hero.
std::optional<std::pair<int, int>> spotNear(game::OdysseyGame& odyssey, const std::string& kind, int reach, int turns = 0) {
    const auto [hx, hy] = heroCell(odyssey);
    const auto [w, h] = odyssey.buildings().store().sizeOf(kind, turns);
    for (int ay = hy - 8; ay <= hy + 8; ++ay) {
        for (int ax = hx - 8; ax <= hx + 8; ++ax) {
            const int mx = ax + w / 2;
            const int my = ay + h / 2;
            if (std::abs(mx - hx) > reach || std::abs(my - hy) > reach) continue;
            if (odyssey.buildings().whyNot(odyssey, kind, ax, ay, turns).empty()) return std::make_pair(ax, ay);
        }
    }
    return std::nullopt;
}

int place(game::OdysseyGame& odyssey, const std::string& kind, int ax, int ay, int turns = 0) {
    odyssey.buildings().select(kind);
    odyssey.buildings().setTurns(turns);
    const std::string problem = odyssey.buildings().placeSelected(odyssey, ax, ay);
    REQUIRE_MESSAGE(problem.empty(), problem);
    odyssey.buildings().stopPlacing();
    int found = 0;
    for (const buildings::PlacedBuilding& b : odyssey.buildings().store().all()) found = std::max(found, b.id);
    return found;
}

const buildings::PlacedBuilding& get(game::OdysseyGame& odyssey, int id) {
    const buildings::PlacedBuilding* building = odyssey.buildings().store().find(id);
    REQUIRE(building != nullptr);
    return *building;
}

// The hero works (or brings materials) on a building, as the right-click menu does, and waits for the ring.
void doInteraction(Camp& camp, const std::string& interaction, int id, int ticks) {
    const auto subject = game::buildingSubject(camp.odyssey, id);
    REQUIRE(subject.has_value());
    REQUIRE(game::startInteraction(camp.odyssey, interaction, *subject));
    camp.play(ticks);
}

} // namespace

TEST_CASE("US-250 Own list: buildings are saved in their own list (level version 6), not as plants, and an older level loads without them") {
    const fs::path data = dataCopy("us250-level");
    const game::Definitions definitions = game::loadDefinitions(data);
    CHECK(definitions.hasBuildingKind("hut"));
    CHECK(definitions.hasBuildingKind("piece:wall-wood"));
    CHECK_FALSE(definitions.hasBuildingKind("castle"));
    game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
    const std::size_t plantsBefore = level.plants.size();
    level.buildings.push_back({level.nextId++, "hut", 5, 5, 1, true, 2, "map", "hut-inside"});
    level.buildings.push_back({level.nextId++, "windbreak", 9, 9, 0, false, -1, "", ""});
    const fs::path file = data / "own-list.json";
    game::saveLevel(level, definitions, file);
    const std::string text = readText(file);
    CHECK(text.find("\"levelVersion\": 7") != std::string::npos);
    CHECK(text.find("\"buildings\"") != std::string::npos);
    CHECK(text.find("\"hut\"") != std::string::npos);
    const game::Level loaded = game::loadLevel(file, definitions).level;
    CHECK(loaded == level);
    CHECK(loaded.plants.size() == plantsBefore); // the buildings are not in the plant list (CI-008)
    CHECK(loaded.buildings.size() == 2);
    CHECK(loaded.buildings[0].interior == "map");
    CHECK_FALSE(loaded.buildings[1].finished);

    // A level without buildings is written without the list, so older levels save as they were.
    level.buildings.clear();
    game::saveLevel(level, definitions, file);
    CHECK(readText(file).find("\"buildings\"") == std::string::npos);

    // A mistake names the field.
    level.buildings.push_back({level.nextId++, "hut", 5, 5, 0, true, -1, "", ""});
    game::saveLevel(level, definitions, file);
    std::string broken = readText(file);
    broken.replace(broken.find("\"hut\""), 5, "\"castle\"");
    writeText(file, broken);
    try {
        game::readLevelFile(file, definitions);
        FAIL("a building kind that does not exist must be refused");
    } catch (const std::exception& error) {
        CHECK(std::string(error.what()).find("buildings[0].kind") != std::string::npos);
    }
}

TEST_CASE("US-250 Game: the buildings of a level stand when the game starts, and a mistake in the data is shown, not fatal") {
    Camp camp("us250-game");
    camp.odyssey.buildings().setClanBuilds(false); // the hero's own work is under test, not the clan's help
    CHECK(camp.odyssey.buildings().notes().empty());
    CHECK(camp.odyssey.buildings().data().kinds().size() >= 5);
    CHECK(camp.odyssey.buildings().knows("hut"));
    CHECK(camp.odyssey.buildings().knows("windbreak"));
    CHECK_FALSE(camp.odyssey.buildings().knows("palisade")); // D-55 Q2: only the hut and the windbreak at the start
    CHECK(camp.odyssey.buildings().knows("piece:wall-wood"));
    camp.odyssey.buildings().learn("palisade");
    CHECK(camp.odyssey.buildings().knows("palisade"));

    const fs::path data = dataCopy("us250-bad");
    std::string kinds = readText(data / "buildings" / "kinds.json");
    kinds.replace(kinds.find("\"footprint\": [3, 3]"), 19, "\"footprint\": [0, 3]");
    writeText(data / "buildings" / "kinds.json", kinds);
    Camp bad("us250-bad", data);
    REQUIRE_FALSE(bad.odyssey.buildings().notes().empty());
    CHECK(bad.odyssey.buildings().notes().front().find("buildings/kinds.json:") != std::string::npos);
    CHECK(bad.odyssey.buildings().notes().front().find("footprint") != std::string::npos);
}

TEST_CASE("US-251 Place: the Build menu lists what the hero knows; a click places a see-through blueprint; an invalid spot is red and refused") {
    Camp camp("us251-place");
    camp.odyssey.buildings().setClanBuilds(false); // the hero's own work is under test, not the clan's help
    game::OdysseyGame& odyssey = camp.odyssey;
    odyssey.setViewScales(1, 1);
    camp.play(2);
    camp.play(1, [] {
        luna::engine::Intents intents;
        intents.set(luna::engine::Intent::Build, true, true);
        return intents;
    }());
    REQUIRE(odyssey.buildings().menuOpen());
    const std::vector<std::string> listed = odyssey.buildings().menuKinds(false);
    CHECK(listed == std::vector<std::string>{"hut", "windbreak"});
    odyssey.buildings().setPiecesTab(true);
    CHECK(odyssey.buildings().menuKinds(true).size() >= 10);
    odyssey.buildings().setPiecesTab(false);
    // Click the first row of the panel: the hut is chosen and the ghost follows the pointer.
    const luna::engine::Rect panel = odyssey.buildings().panelRect(odyssey);
    luna::engine::Intents row;
    luna::engine::Pointer onRow;
    onRow.x = panel.x + 20;
    onRow.y = panel.y + 34;
    onRow.pressed[static_cast<std::size_t>(luna::engine::PointerButton::Left)] = true;
    onRow.held[static_cast<std::size_t>(luna::engine::PointerButton::Left)] = true;
    row.setPointer(onRow);
    camp.play(1, row);
    REQUIRE(odyssey.buildings().selected() == "hut");
    CHECK(odyssey.buildings().placing());

    const auto spot = spotNear(odyssey, "hut", 4);
    REQUIRE(spot.has_value());
    const double wx = (spot->first + 1) * game::kTileSize + 16.0; // the pointer is on the middle cell of the 3 x 3 footprint
    const double wy = (spot->second + 1) * game::kTileSize + 16.0;
    camp.play(1, pointerAt(odyssey, wx, wy, false));
    CHECK(odyssey.buildings().ghostValid());
    CHECK(odyssey.buildings().ghostCellX() == spot->first);
    camp.play(1, pointerAt(odyssey, wx, wy, true)); // click
    REQUIRE(odyssey.buildings().store().all().size() == 1);
    const buildings::PlacedBuilding& hut = odyssey.buildings().store().all().front();
    CHECK(hut.state == buildings::State::Blueprint);
    CHECK(odyssey.buildings().store().obstacles().empty()); // a ghost does not block walking
    CHECK(odyssey.buildings().store().ruleState(hut) == "waiting"); // the camp has a run: the materials are not free
}

TEST_CASE("US-251 Place: an invalid spot is shown red and nothing is placed there") {
    Camp camp("us251-red");
    camp.odyssey.buildings().setClanBuilds(false); // the hero's own work is under test, not the clan's help
    game::OdysseyGame& odyssey = camp.odyssey;
    odyssey.setViewScales(1, 1);
    odyssey.buildings().select("hut");
    // Find a solid tile (rock or water) in the level.
    const luna::engine::TileMap& map = odyssey.tileMap();
    int sx = -1;
    int sy = -1;
    for (int y = 1; y < map.height() - 1 && sx < 0; ++y) {
        for (int x = 1; x < map.width() - 1; ++x) {
            if (map.isSolid(x, y)) {
                sx = x;
                sy = y;
                break;
            }
        }
    }
    REQUIRE(sx >= 0);
    camp.play(1, pointerAt(odyssey, sx * game::kTileSize + 16.0, sy * game::kTileSize + 16.0, false));
    CHECK_FALSE(odyssey.buildings().ghostValid());
    CHECK_FALSE(odyssey.buildings().ghostProblem().empty());
    camp.play(1, pointerAt(odyssey, sx * game::kTileSize + 16.0, sy * game::kTileSize + 16.0, true));
    CHECK(odyssey.buildings().store().all().empty());
    // Standing in the way of a wall is refused too.
    const auto [hx, hy] = heroCell(odyssey);
    CHECK_FALSE(odyssey.buildings().whyNot(odyssey, "piece:wall-wood", hx, hy, 0).empty());
    // Esc leaves placing, then the menu.
    CHECK(odyssey.buildings().escape());
    CHECK_FALSE(odyssey.buildings().placing());
    CHECK(odyssey.buildings().escape());
    CHECK_FALSE(odyssey.buildings().menuOpen());
    CHECK_FALSE(odyssey.buildings().escape());
}

TEST_CASE("US-251 Build: bring materials from the bag, work on the site, and the building is finished after its build time") {
    Camp camp("us251-build");
    camp.odyssey.buildings().setClanBuilds(false); // the hero's own work is under test, not the clan's help
    game::OdysseyGame& odyssey = camp.odyssey;
    odyssey.setViewScales(1, 1);
    const auto spot = spotNear(odyssey, "hut", 1);
    REQUIRE(spot.has_value());
    const int id = place(odyssey, "hut", spot->first, spot->second);
    CHECK(odyssey.buildings().store().ruleState(get(odyssey, id)) == "waiting");
    // The menu of the site: bring materials, build (greyed out until the materials are there) and cancel.
    const auto centre = odyssey.buildings().store().centre(get(odyssey, id));
    (void)centre;
    const auto siteSubject = game::buildingSubject(odyssey, id);
    REQUIRE(siteSubject.has_value());
    const auto offers = odyssey.offersFor(*siteSubject);
    REQUIRE(offers.size() == 3);
    CHECK(offers[0].interaction->id == "deliver-materials");
    CHECK(offers[0].enabled);
    CHECK(offers[1].interaction->id == "build-work");
    CHECK_FALSE(offers[1].enabled);
    CHECK(offers[1].reason == "Bring the materials first");
    CHECK(offers[2].interaction->id == "cancel-blueprint");
    CHECK(offers[2].enabled);
    CHECK(siteSubject->title == "Hut: needs materials");

    // With nothing in the bag nothing moves.
    doInteraction(camp, "deliver-materials", id, 15);
    CHECK(odyssey.buildings().store().ruleState(get(odyssey, id)) == "waiting");
    CHECK(get(odyssey, id).delivered.empty());
    // Half of it: work is limited to what was brought.
    odyssey.life()->give("wood", 14);
    odyssey.life()->give("herbs", 10);
    doInteraction(camp, "deliver-materials", id, 15);
    CHECK(odyssey.life()->count("wood") == 0);
    CHECK(odyssey.buildings().store().ruleState(get(odyssey, id)) == "waiting"); // no fur yet for the door flap
    odyssey.life()->give("fur", 1);
    doInteraction(camp, "deliver-materials", id, 15);
    CHECK(odyssey.buildings().store().ruleState(get(odyssey, id)) == "ready");
    CHECK(odyssey.life()->count("fur") == 0);

    int rounds = 0;
    while (get(odyssey, id).state == buildings::State::Blueprint && rounds < 30) {
        doInteraction(camp, "build-work", id, 85); // four seconds of work each time
        ++rounds;
    }
    CHECK(rounds > 3); // it takes several rounds: the build time is 41 seconds
    CHECK(get(odyssey, id).state == buildings::State::Finished);
    CHECK(odyssey.buildings().store().condition(get(odyssey, id)) == 100);
    // The walls stand on the map and block walking; the door and the middle do not.
    const auto& hutBuilt = get(odyssey, id);
    int blockedCells = 0;
    for (int y = hutBuilt.y; y < hutBuilt.y + hutBuilt.height; ++y) {
        for (int x = hutBuilt.x; x < hutBuilt.x + hutBuilt.width; ++x) blockedCells += odyssey.tileMap().obstacleHeight(x, y) > 0.0 ? 1 : 0;
    }
    CHECK(blockedCells == 7);
    CHECK(odyssey.buildings().store().rooms().size() == 1);
    // A finished building has no Build item; it has no Cancel either.
    const auto subject = game::buildingSubject(odyssey, id);
    REQUIRE(subject.has_value());
    for (const auto& offer : odyssey.offersFor(*subject)) {
        CHECK(offer.interaction->id != "build-work");
        CHECK(offer.interaction->id != "cancel-blueprint");
    }
}

TEST_CASE("US-251 Cancel: the delivered materials are dropped on the site and picked up by walking over them") {
    Camp camp("us251-cancel");
    camp.odyssey.buildings().setClanBuilds(false); // the hero's own work is under test, not the clan's help
    game::OdysseyGame& odyssey = camp.odyssey;
    odyssey.setViewScales(1, 1);
    // A windbreak a few cells away from the hero, along a row nothing stands on (so the hero can walk to the drop).
    const auto [hx, hy] = heroCell(odyssey);
    std::optional<std::pair<int, int>> far;
    int direction = 0;
    for (const int dir : {1, -1}) {
        for (int d = 4; d <= 7 && !far; ++d) {
            bool clear = true;
            for (int k = 1; k <= d + 1; ++k) clear = clear && !odyssey.tileMap().isSolid(hx + dir * k, hy) && odyssey.tileMap().obstacleHeight(hx + dir * k, hy) == 0.0;
            const int ax = dir > 0 ? hx + d - 1 : hx - d - 1;
            if (clear && odyssey.buildings().whyNot(odyssey, "windbreak", ax, hy, 0).empty()) {
                far = std::make_pair(ax, hy);
                direction = dir;
            }
        }
    }
    REQUIRE(far.has_value());
    const int id = place(odyssey, "windbreak", far->first, far->second);
    odyssey.life()->give("fur", 3);
    CHECK(odyssey.buildings().deliver(odyssey, id).find("all materials") != std::string::npos);
    CHECK(odyssey.life()->count("fur") == 0);
    const std::string message = odyssey.buildings().cancel(odyssey, id);
    CHECK(message.find("on the ground") != std::string::npos);
    CHECK(odyssey.buildings().store().find(id) == nullptr);
    REQUIRE(odyssey.buildings().store().drops().size() == 1);
    CHECK(odyssey.buildings().store().drops().front().items.at("fur") == 3);
    camp.play(2);
    CHECK(odyssey.buildings().store().drops().size() == 1); // the hero is not there yet
    CHECK(odyssey.life()->count("fur") == 0);
    // Walk to the drop (the keys): the materials come back into the bag.
    luna::engine::Intents east;
    east.set(direction > 0 ? luna::engine::Intent::MoveRight : luna::engine::Intent::MoveLeft, true, false);
    for (int i = 0; i < 800 && odyssey.buildings().store().drops().size() == 1; ++i) odyssey.update(east);
    INFO("hero at " << odyssey.hero().feetX() << ", " << odyssey.hero().feetY() << "; drops left: " << odyssey.buildings().store().drops().size());
    CHECK(odyssey.buildings().store().drops().empty());
    CHECK(odyssey.life()->count("fur") == 3);
}

TEST_CASE("US-252 Pieces: walls, a door and a roof are placed one by one, each built like a small blueprint, and the room forms when it is closed") {
    Camp camp("us252-pieces");
    camp.odyssey.buildings().setClanBuilds(false); // the hero's own work is under test, not the clan's help
    game::OdysseyGame& odyssey = camp.odyssey;
    odyssey.setViewScales(1, 1);
    // A free 3 x 3 square east of the hero, found by the pieces themselves.
    const auto [hx, hy] = heroCell(odyssey);
    int ox = -1;
    int oy = -1;
    for (int y = hy - 6; y <= hy + 6 && ox < 0; ++y) {
        for (int x = hx + 2; x <= hx + 10; ++x) {
            bool free = true;
            for (int dy = 0; dy < 3 && free; ++dy) {
                for (int dx = 0; dx < 3; ++dx) free = free && odyssey.buildings().whyNot(odyssey, "piece:wall-wood", x + dx, y + dy, 0).empty();
            }
            if (free) {
                ox = x;
                oy = y;
                break;
            }
        }
    }
    REQUIRE(ox >= 0);
    std::vector<int> ids;
    for (int dy = 0; dy < 3; ++dy) {
        for (int dx = 0; dx < 3; ++dx) {
            if (dx == 1 && dy == 1) continue;
            ids.push_back(place(odyssey, dx == 1 && dy == 2 ? "piece:door-wood" : "piece:wall-wood", ox + dx, oy + dy));
        }
    }
    ids.push_back(place(odyssey, "piece:roof-thatch", ox + 1, oy + 1));
    for (const int id : ids) {
        CHECK(get(odyssey, id).pieces.size() == 1);
        CHECK(get(odyssey, id).state == buildings::State::Blueprint);
    }
    CHECK(odyssey.buildings().store().rooms().empty());
    // Each piece is built like a small blueprint (the hero is far from them: the store is worked on directly, as the interaction does when near).
    for (const int id : ids) {
        odyssey.buildings().store().waiveCost(id);
        odyssey.buildings().store().work(id, 100000);
    }
    odyssey.update({}); // the walls reach the map
    REQUIRE(odyssey.buildings().store().rooms().size() == 1);
    const buildings::Room& room = odyssey.buildings().store().rooms().front();
    CHECK(room.doors == 1);
    CHECK(odyssey.buildings().store().sheltered(ox + 1, oy + 1));
    // Paths go through the door only: the walls block, the door cell does not.
    CHECK(odyssey.tileMap().obstacleHeight(ox, oy) > 0.0);
    CHECK(odyssey.tileMap().obstacleHeight(ox + 1, oy + 2) == 0.0);
}
