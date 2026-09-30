// US-120 Real art in the game.
#include "game/art.h"
#include "game/placeholder_art.h"

#include "luna/engine/image_io.h"
#include "sim/data.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
namespace game = odysseus::game;
using luna::engine::Color;
using luna::engine::Image;

namespace {

fs::path sprites() {
    return fs::path(ODYSSEUS_DATA_DIR).parent_path() / "sprites";
}

fs::path freshFolder(const std::string& name) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us120" / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    return folder;
}

void write(const fs::path& file, const std::string& text) {
    std::ofstream(file, std::ios::binary | std::ios::trunc) << text;
}

bool sameImage(const Image& a, const Image& b) {
    if (a.width() != b.width() || a.height() != b.height()) return false;
    for (int y = 0; y < a.height(); ++y)
        for (int x = 0; x < a.width(); ++x)
            if (!(a.get(x, y) == b.get(x, y))) return false;
    return true;
}

// A small sheet like the owner's: a dark background, a figure standing in it, a framed tile.
fs::path makeSheet(const fs::path& folder) {
    Image sheet(120, 80);
    sheet.fillRect(0, 0, 120, 80, Color{18, 20, 22});
    sheet.fillRect(10, 5, 20, 50, Color{200, 40, 40}); // the figure's body
    sheet.fillRect(12, 55, 6, 10, Color{90, 60, 30});  // a leg on the left only: the figure is not symmetric
    sheet.fillRect(60, 10, 44, 44, Color{230, 200, 120}); // the tile's drawn frame
    sheet.fillRect(64, 14, 36, 36, Color{40, 160, 60});   // the tile itself
    REQUIRE(luna::engine::savePng(sheet, folder / "sheet.png"));
    return folder / "sheet.png";
}

} // namespace

TEST_CASE("US-120 Atlas") {
    const fs::path folder = freshFolder("atlas");
    makeSheet(folder);
    write(folder / "cuts.json", R"({"tolerance": 12, "tileInset": 4, "cuts": [
        {"name": "man", "kind": "character", "sheet": "sheet.png", "rect": [4, 2, 34, 70]},
        {"name": "man.mirrored", "kind": "character", "mirrorOf": "man"},
        {"name": "meadow", "kind": "tile", "sheet": "sheet.png", "rect": [60, 10, 44, 44]}]})");
    const game::Atlas atlas = game::cutAtlas(game::loadCuts(folder / "cuts.json"), folder);
    REQUIRE(atlas.characterCount() == 2);
    REQUIRE(atlas.tileCount() == 1);
    // A character: background gone, 32x48, standing on the bottom row.
    const auto man = atlas.characterFrame(atlas.characterCells.at("man"));
    CHECK(man.width == 32);
    CHECK(man.height == 48);
    CHECK(atlas.characters.get(man.x, man.y).alpha == 0); // the corner is transparent: no background left
    bool feet = false;
    for (int x = 0; x < 32; ++x) feet = feet || atlas.characters.get(man.x + x, man.y + 47).alpha > 0;
    CHECK(feet);
    bool red = false;
    for (int y = 0; y < 48; ++y)
        for (int x = 0; x < 32; ++x) red = red || atlas.characters.get(man.x + x, man.y + y) == Color{200, 40, 40};
    CHECK(red);
    // The mirrored frame is the first one, left and right swapped.
    const auto flipped = atlas.characterFrame(atlas.characterCells.at("man.mirrored"));
    for (int y = 0; y < 48; ++y)
        for (int x = 0; x < 32; ++x) CHECK(atlas.characters.get(flipped.x + x, flipped.y + y) == atlas.characters.get(man.x + 31 - x, man.y + y));
    // A tile: its drawn frame trimmed off, 32x32, fully covered by the tile's own colour.
    const auto meadow = atlas.tileFrame(atlas.tileCells.at("meadow"));
    CHECK(meadow.width == 32);
    for (int y = 0; y < 32; ++y)
        for (int x = 0; x < 32; ++x) CHECK(atlas.tiles.get(meadow.x + x, meadow.y + y) == Color{40, 160, 60});
    // Saved and read back: the same pictures and names.
    game::saveAtlas(atlas, folder / "atlas");
    std::string problem;
    const auto loaded = game::loadAtlas(folder / "atlas", problem);
    REQUIRE(loaded.has_value());
    CHECK(loaded->characterCells == atlas.characterCells);
    CHECK(sameImage(loaded->characters, atlas.characters));
    CHECK(sameImage(loaded->tiles, atlas.tiles));
}

TEST_CASE("US-120 The committed atlas is the owner's sheets, cut") {
    const game::Atlas fresh = game::cutAtlas(game::loadCuts(sprites() / "cuts.json"), sprites());
    std::string problem;
    const auto committed = game::loadAtlas(sprites() / "atlas", problem);
    REQUIRE_MESSAGE(committed.has_value(), problem);
    CHECK(committed->characterCells == fresh.characterCells); // run odysseus_atlas after changing cuts.json
    CHECK(committed->tileCells == fresh.tileCells);
    CHECK(sameImage(committed->characters, fresh.characters));
    CHECK(sameImage(committed->tiles, fresh.tiles));
    MESSAGE(fresh.characterCount(), " character frames, ", fresh.tileCount(), " tiles");
    CHECK(fresh.tileCount() >= 12);
    for (const char* monster : {"goblin", "skeleton", "wolf", "spider", "slime", "orc", "troll", "bat", "plant_monster", "ghost"}) {
        CHECK(fresh.characterCells.contains(monster));
    }
    for (const char* view : {"S", "E", "N", "W"}) {
        for (int frame = 0; frame < 8; ++frame) {
            CHECK(fresh.characterCells.contains(std::string("hero.") + view + "." + std::to_string(frame)));
        }
    }
    // Every figure stands free of its sheet (transparent corners) and every tile is whole.
    for (const auto& [name, cell] : fresh.characterCells) {
        const auto r = fresh.characterFrame(cell);
        CHECK_MESSAGE(fresh.characters.get(r.x, r.y).alpha == 0, name);
        CHECK_MESSAGE(fresh.characters.get(r.x + r.width - 1, r.y).alpha == 0, name);
    }
    for (const auto& [name, cell] : fresh.tileCells) {
        const auto r = fresh.tileFrame(cell);
        for (int y = 0; y < r.height; ++y)
            for (int x = 0; x < r.width; ++x) REQUIRE_MESSAGE(fresh.tiles.get(r.x + x, r.y + y).alpha == 255, name);
    }
}

TEST_CASE("US-120 Heroes and ground") {
    const game::ArtSet art = game::makeArtSet(sprites());
    REQUIRE_MESSAGE(art.ownArt, art.problem);
    CHECK(art.heroSheet.width() == game::kWalkFrames * game::kCharacterWidth);
    CHECK(art.heroSheet.height() == static_cast<int>(game::Facing::Count) * game::kCharacterHeight);
    CHECK(art.tileStrip.width() == static_cast<int>(game::TileKind::Count) * game::kTileSize);
    // The owner's art, not the programmer art.
    CHECK_FALSE(sameImage(art.heroSheet, game::makeCharacterSheet()));
    CHECK_FALSE(sameImage(art.tileStrip, game::makeTileSheet()));
    CHECK(art.enemy.width() == game::kCharacterWidth);
    CHECK_FALSE(sameImage(art.enemy, art.enemyHit)); // the hit flash is red
}

TEST_CASE("US-120 Missing art") {
    SUBCASE("no atlas at all") {
        const fs::path folder = freshFolder("missing");
        const game::ArtSet art = game::makeArtSet(folder);
        CHECK_FALSE(art.ownArt);
        MESSAGE(art.problem);
        CHECK(art.problem.find("atlas.json") != std::string::npos);
        CHECK(sameImage(art.heroSheet, game::makeCharacterSheet())); // the programmer art stands in
    }
    SUBCASE("a damaged picture") {
        const fs::path folder = freshFolder("damaged");
        fs::copy(sprites() / "atlas", folder / "atlas");
        write(folder / "atlas" / "characters.png", "not a picture");
        const game::ArtSet art = game::makeArtSet(folder);
        CHECK_FALSE(art.ownArt);
        MESSAGE(art.problem);
        CHECK(art.problem.find("characters.png") != std::string::npos);
    }
}

TEST_CASE("US-120 Cut list errors name the field") {
    const fs::path folder = freshFolder("errors");
    auto problem = [&](const std::string& text) {
        write(folder / "cuts.json", text);
        try {
            (void)game::loadCuts(folder / "cuts.json");
        } catch (const odysseus::sim::DataError& error) {
            return std::string(error.what());
        }
        return std::string("accepted");
    };
    CHECK(problem(R"({"tolerance": 12, "tileInset": 4, "cuts": [{"name": "a", "kind": "person", "sheet": "s.png", "rect": [0,0,1,1]}]})").find("cuts[0].kind") != std::string::npos);
    CHECK(problem(R"({"tolerance": 12, "tileInset": 4, "cuts": [{"name": "a", "kind": "tile", "sheet": "s.png", "rect": [0,0,0,1]}]})").find("cuts[0].rect") != std::string::npos);
    CHECK(problem(R"({"tolerance": 12, "tileInset": 4, "cuts": [{"name": "a", "kind": "tile", "mirrorOf": "b"}]})").find("cuts[0].mirrorOf") != std::string::npos);
    CHECK(problem(R"({"tileInset": 4, "cuts": []})").find("tolerance") != std::string::npos);
}
