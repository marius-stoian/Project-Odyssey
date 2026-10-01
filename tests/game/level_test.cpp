// US-122 Levels as data.
#include "game/level.h"
#include "game/odyssey_game.h"
#include "game/test_map.h"

#include "luna/engine/physics_view.h"
#include "sim/data.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
namespace game = odysseus::game;

namespace {

fs::path valleyFile() {
    return ODYSSEUS_DEMO_LEVEL; // the demo level the tests play (the owner edits valley.json)
}

fs::path freshFolder(const std::string& name) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us122" / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    return folder;
}

std::string readAll(const fs::path& file) {
    std::ifstream in(file, std::ios::binary);
    std::stringstream text;
    text << in.rdbuf();
    return text.str();
}

void writeAll(const fs::path& file, const std::string& text) {
    std::ofstream(file, std::ios::binary | std::ios::trunc) << text;
}

// The demo as it was built in code before levels were data: the test map, the hero at the
// crossing, the two straw targets and the goblin two tiles east of the hero.
game::Level oldDemo() {
    const luna::engine::TileMap map = game::makeTestMap();
    game::Level level = game::makeLevel("The Valley", map.width(), map.height(), 0);
    for (int y = 0; y < map.height(); ++y)
        for (int x = 0; x < map.width(); ++x) level.set(x, y, map.at(x, y)); // tile numbers kept their meaning
    level.heroStart = {1040, 1048};
    level.targets = {{784, 1048}, {1040, 784}}; // (24.5, 32.75) m and (32.5, 24.5) m
    level.characters.push_back({1, "goblin", {1104, 1048}, game::Facing::South, "Goblin", 100, 4});
    level.pickups = {{2, "Spear throw", {1034, 1044}}, {3, "Sword", {1046, 1044}}}; // US-134: the two demo weapons by the hero
    level.nextId = 4;
    return level;
}

} // namespace

TEST_CASE("US-122 Load") {
    const game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
    CHECK(definitions.tileNumber("grass") == 0); // the first four kinds keep the old tile numbers
    CHECK(definitions.tileNumber("water") == 3);
    CHECK(definitions.tiles.size() >= 12);
    CHECK(definitions.character("goblin") != nullptr);
    if (!fs::exists(valleyFile())) {
        const fs::path made = freshFolder("valley") / "demo.json";
        game::saveLevel(oldDemo(), definitions, made);
        FAIL("assets/levels/demo.json is missing; the old demo was written to ", made.string());
    }
    const game::Level valley = game::loadLevel(valleyFile(), definitions).level;
    CHECK(valley == oldDemo()); // the valley is exactly the demo that used to be code

    // The game plays it: map, hero start, targets and the goblin all come from the file.
    const game::OdysseyGame odyssey(ODYSSEUS_DATA_DIR, ODYSSEUS_DEMO_LEVEL);
    CHECK(odyssey.level() == valley);
    CHECK(odyssey.hero().feetX() == doctest::Approx(1040.0));
    CHECK(odyssey.hero().feetY() == doctest::Approx(1048.0));
    REQUIRE(odyssey.range().targets().size() == 2);
    CHECK(odyssey.range().targets()[0].base == game::OdysseyGame::openTargetBase());
    CHECK(odyssey.range().targets()[1].base == game::OdysseyGame::blockedTargetBase());
    REQUIRE(odyssey.enemies().size() == 1);
    CHECK(odyssey.enemies().front().name == "Goblin");
    CHECK(odyssey.enemies().front().frames == "goblin");
    CHECK(odyssey.enemies().front().feetX() == doctest::Approx(1104.0));

    // Another level file: other ground, other hero start, other characters.
    const fs::path folder = freshFolder("other");
    game::Level other = game::makeLevel("Snowfield", 20, 12, definitions.tileNumber("snow"));
    other.heroStart = {100, 100};
    other.characters.push_back({1, "wolf", {200, 120}, game::Facing::West, "Grey Wolf", 50, 6});
    other.characters.push_back({2, "wanderer", {260, 120}, game::Facing::South, "Old Man", 100, 5}); // not an enemy
    other.nextId = 3;
    game::saveLevel(other, definitions, folder / "snow.json");
    const game::OdysseyGame snowy(ODYSSEUS_DATA_DIR, folder / "snow.json");
    CHECK(snowy.level().name == "Snowfield");
    CHECK(snowy.hero().feetX() == doctest::Approx(100.0));
    REQUIRE(snowy.enemies().size() == 1);
    CHECK(snowy.enemies().front().name == "Grey Wolf");
    CHECK(snowy.range().targets().empty());
}

TEST_CASE("US-122 Round trip") {
    const game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
    game::Level level = game::makeLevel("Mixed Ground", 40, 30, definitions.tileNumber("dirt"));
    for (int y = 0; y < level.height; ++y)
        for (int x = 0; x < level.width; ++x) level.set(x, y, (x * 7 + y * 3) % static_cast<int>(definitions.tiles.size()));
    level.heroStart = {33, 47};
    level.targets = {{64, 64}};
    level.characters.push_back({7, "troll", {320, 400}, game::Facing::NorthEast, "Bridge Troll", 250, 12});
    level.characters.push_back({3, "bat", {500, 90}, game::Facing::South, "Bat", 20, 2});
    level.nextId = 8;
    const fs::path file = freshFolder("round-trip") / "mixed.json";
    game::saveLevel(level, definitions, file);
    const game::LoadedLevel loaded = game::loadLevel(file, definitions);
    CHECK(loaded.loadedFrom == file);
    CHECK(loaded.notes.empty());
    CHECK(loaded.level == level); // exactly the same
    // Saving again keeps the previous file as a backup.
    level.name = "Mixed Ground, second draft";
    game::saveLevel(level, definitions, file);
    CHECK(fs::exists(fs::path(file.string() + ".bak1")));
    CHECK(game::loadLevel(file, definitions).level.name == "Mixed Ground, second draft");
}

TEST_CASE("US-122 Damaged") {
    const game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
    const fs::path folder = freshFolder("damaged");
    const fs::path file = folder / "level.json";
    game::Level level = game::makeLevel("Good", 10, 10, 0);
    level.characters.push_back({1, "goblin", {40, 60}, game::Facing::South, "Goblin", 60, 4});
    level.pickups.push_back({2, "Sword", {80, 60}});
    level.nextId = 3;
    game::saveLevel(level, definitions, file);
    level.name = "Better";
    game::saveLevel(level, definitions, file); // "Good" is now level.json.bak1

    SUBCASE("a damaged file: the last good backup is used, and the problem is named") {
        writeAll(file, "{ \"levelVersion\": 1, \"name\": ");
        const game::LoadedLevel loaded = game::loadLevel(file, definitions);
        CHECK(loaded.loadedFrom == fs::path(file.string() + ".bak1"));
        CHECK(loaded.level.name == "Good");
        REQUIRE_FALSE(loaded.notes.empty());
        MESSAGE(loaded.notes.front());
        CHECK(loaded.notes.front().find("level.json") != std::string::npos);
    }
    SUBCASE("each broken field is named") {
        const std::string good = readAll(file);
        auto problem = [&](const std::string& from, const std::string& to) {
            std::string text = good;
            const auto at = text.find(from);
            REQUIRE_MESSAGE(at != std::string::npos, from);
            text.replace(at, from.size(), to);
            const fs::path broken = folder / "broken.json";
            writeAll(broken, text);
            try {
                (void)game::readLevelFile(broken, definitions);
            } catch (const odysseus::sim::DataError& error) {
                return std::string(error.what());
            }
            return std::string("accepted");
        };
        CHECK(problem("\"width\": 10", "\"width\": 3").find("width") != std::string::npos);
        CHECK(problem("\"kind\": \"goblin\"", "\"kind\": \"dragon\"").find("characters[0].kind") != std::string::npos);
        CHECK(problem("\"facing\": \"S\"", "\"facing\": \"Up\"").find("characters[0].facing") != std::string::npos);
        CHECK(problem("\"defaultGround\": \"grass\"", "\"defaultGround\": \"cheese\"").find("defaultGround") != std::string::npos);
        CHECK(problem("\"weapon\": \"Sword\"", "\"weapon\": \"Laser\"").find("pickups[0].weapon") != std::string::npos);
        CHECK(problem("\"levelVersion\": 2", "\"levelVersion\": 9").find("newer version") != std::string::npos);
        CHECK(problem("[\n    \"grass\",\n    10\n   ]", "[\n    \"grass\",\n    9\n   ]").find("ground[0]") != std::string::npos);
    }
}
