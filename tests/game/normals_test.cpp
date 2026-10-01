// US-241 Generated normal maps: odysseus_atlas --normals, hand-made maps that win, and sprites without maps lit flat.
#include "camp.h"

#include "game/art.h"
#include "game/normal_art.h"
#include "luna/engine/image_io.h"
#include "luna/engine/renderer.h"
#include "luna/engine/scaled_renderer.h"
#include "luna/platform/system.h"
#include "luna/platform/window.h"
#include "sim/data.h"

#include <cstdlib>
#include <memory>

using namespace camp_support;

namespace {

// A sprites folder with the committed atlas pictures in it, with or without the normal maps.
fs::path atlasCopy(const std::string& name, bool withNormals) {
    const fs::path sprites = fs::temp_directory_path() / ("odysseus-us241-" + name) / "sprites";
    fs::remove_all(sprites.parent_path());
    fs::create_directories(sprites / "atlas");
    for (const auto& entry : fs::directory_iterator(fs::path(ODYSSEUS_DATA_DIR).parent_path() / "sprites" / "atlas")) {
        const bool normal = entry.path().stem().string().ends_with("_n");
        if (normal && !withNormals) continue;
        fs::copy_file(entry.path(), sprites / "atlas" / entry.path().filename());
    }
    return sprites;
}

const std::vector<std::string> kGround = {"grass", "path", "stone", "water"};

} // namespace

TEST_CASE("US-241 Generate: every atlas gets a matching normal atlas") {
    const fs::path sprites = atlasCopy("generate", false);
    const game::NormalReport report = game::writeNormalAtlases(sprites);
    CHECK(report.atlases == 9); // characters, tiles and seven content pages
    CHECK(report.handMade == 0);
    int checked = 0;
    for (const auto& entry : fs::directory_iterator(sprites / "atlas")) {
        const std::string stem = entry.path().stem().string();
        if (entry.path().extension() != ".png" || stem.ends_with("_n")) continue;
        const fs::path normals = sprites / "atlas" / (stem + "_n.png");
        REQUIRE_MESSAGE(fs::exists(normals), stem);
        std::string problem;
        const auto base = luna::engine::loadPng(entry.path(), problem);
        const auto made = luna::engine::loadPng(normals, problem);
        REQUIRE(base.has_value());
        REQUIRE(made.has_value());
        CHECK(made->width() == base->width());
        CHECK(made->height() == base->height());
        ++checked;
    }
    CHECK(checked == 9);
    // The hero is shaded: some pixel of his first frame leans away from flat.
    std::string problem;
    const auto characters = luna::engine::loadPng(sprites / "atlas" / "characters_n.png", problem);
    REQUIRE(characters.has_value());
    bool leans = false;
    for (int y = 0; y < 48 && !leans; ++y) {
        for (int x = 0; x < 32 && !leans; ++x) leans = std::abs(static_cast<int>(characters->get(x, y).red) - 128) > 20;
    }
    CHECK(leans);
}

TEST_CASE("US-241 The committed normal atlases are the ones the tool makes") {
    const fs::path sprites = atlasCopy("committed", true);
    for (const char* name : {"characters", "tiles", "content-trees", "content-plants-small", "content-animals"}) {
        CHECK_MESSAGE(fs::exists(sprites / "atlas" / (std::string(name) + "_n.png")), name);
    }
    const game::ArtSet art = game::makeArtSet(sprites, kGround);
    CHECK(art.ownArt);
    CHECK(art.heroNormals.width() == art.heroSheet.width()); // the hero sheet has its normal sheet
    CHECK(art.heroNormals.height() == art.heroSheet.height());
    CHECK(art.tileNormals.width() == art.tileStrip.width());
}

TEST_CASE("US-241 Missing: sprites without a normal map are lit flat, with no error") {
    const fs::path sprites = atlasCopy("missing", false);
    std::string note;
    const game::ArtSet art = game::makeArtSet(sprites, kGround);
    CHECK(art.ownArt);
    CHECK(art.heroNormals.width() == 0); // no maps: nothing is given to the renderer, so the shader uses a flat normal
    CHECK(art.charactersNormals.width() == 0);
    CHECK_FALSE(game::loadNormalAtlas(sprites / "atlas", "characters", art.characters, &note).has_value());
    CHECK(note.empty()); // not an error

    // Maps that are there are used; one of the wrong size is left out, with a note.
    game::writeNormalAtlases(sprites);
    const game::ArtSet shaded = game::makeArtSet(sprites, kGround);
    CHECK(shaded.heroNormals.width() == shaded.heroSheet.width());
    CHECK(shaded.charactersNormals.width() == shaded.characters.width());
    luna::engine::savePng(luna::engine::Image(8, 8), sprites / "atlas" / "characters_n.png");
    CHECK_FALSE(game::loadNormalAtlas(sprites / "atlas", "characters", shaded.characters, &note).has_value());
    CHECK_FALSE(note.empty());
}

TEST_CASE("US-241 Own map: a hand-made normal map named like the sprite is kept") {
    const fs::path sprites = atlasCopy("own", false);
    luna::engine::Image mine(32, 48);
    mine.fillRect(0, 0, 32, 48, luna::engine::Color{255, 0, 128, 255});
    REQUIRE(luna::engine::savePng(mine, sprites / "hero.S.0_n.png"));
    const game::NormalReport report = game::writeNormalAtlases(sprites);
    CHECK(report.handMade == 1);
    std::string problem;
    const auto atlas = game::loadAtlas(sprites / "atlas", problem);
    REQUIRE(atlas.has_value());
    const auto normals = luna::engine::loadPng(sprites / "atlas" / "characters_n.png", problem);
    REQUIRE(normals.has_value());
    const odysseus::core::Rect cell = atlas->characterFrame(atlas->characterCells.at("hero.S.0"));
    CHECK(normals->get(cell.x + 5, cell.y + 5).red == 255); // exactly the hand-made pixel
    CHECK(normals->get(cell.x + 5, cell.y + 5).green == 0);
    const odysseus::core::Rect other = atlas->characterFrame(atlas->characterCells.at("hero.S.2"));
    CHECK(normals->get(other.x + 16, other.y + 24).green != 0); // the other frames are generated

    // A map of the wrong size is an error that names the file.
    REQUIRE(luna::engine::savePng(luna::engine::Image(10, 10), sprites / "goblin_n.png"));
    try {
        game::writeNormalAtlases(sprites);
        FAIL("a wrong-sized map should be refused");
    } catch (const odysseus::sim::DataError& error) {
        CHECK(std::string(error.what()).find("goblin_n.png") != std::string::npos);
    }
}

TEST_CASE("US-241 The hero is shaded by the generated normals: the side toward the light is brighter") {
    luna::platform::System system;
    std::unique_ptr<luna::platform::Window> window;
    try {
        window = std::make_unique<luna::platform::Window>(luna::platform::WindowSettings{"US-241 test", 1280, 720, 480, 270, true, luna::platform::RendererChoice::Gpu});
    } catch (const std::exception& error) {
        MESSAGE("no GPU renderer here: ", error.what());
        return;
    }
    const game::ArtSet art = game::makeArtSet(fs::path(ODYSSEUS_DATA_DIR).parent_path() / "sprites", kGround);
    REQUIRE(art.ownArt);
    REQUIRE(art.heroNormals.width() > 0);
    luna::engine::WindowRenderer renderer(*window);
    luna::engine::ScaledRenderer big(renderer, 4);
    const luna::engine::Texture sheet = renderer.createTexture(art.heroSheet);
    renderer.setNormalMap(sheet, renderer.createTexture(art.heroNormals));
    const odysseus::core::Rect frame{0, static_cast<int>(game::Facing::South) * game::kCharacterHeight, game::kCharacterWidth, game::kCharacterHeight};

    window->clear(40, 40, 50);
    luna::engine::LightFrame frame1;
    frame1.ambientR = frame1.ambientG = frame1.ambientB = 0.25F;
    luna::engine::PointLight light;
    light.radius = 45.0F;
    light.strength = 1.2F;
    light.r = 1.0F; light.g = 0.78F; light.b = 0.45F;
    light.height = 10.0F;
    // One hero with a fire on his left, one with a fire on his right (positions in the 4x picture: pixels of 4 screen pixels).
    light.x = 14.0F; light.y = 40.0F;
    frame1.lights = {light};
    big.setLighting(&frame1);
    big.draw(sheet, frame, {20, 12});
    luna::engine::LightFrame frame2 = frame1;
    frame2.lights[0].x = 100.0F;
    big.setLighting(&frame2);
    big.draw(sheet, frame, {68, 12});
    big.setLighting(nullptr);
    const luna::platform::Pixels pixels = window->readPixels();
#pragma warning(suppress : 4996) // getenv is fine here: a test reads one optional setting
    if (const char* folder = std::getenv("ODYSSEUS_EVIDENCE_DIR")) window->saveScreenshot(fs::path(folder) / "hero-lit.bmp");
    window->present();

    // Mean brightness of the left and right halves of each hero's body.
    const auto brightness = [&](int heroX, bool leftHalf) {
        double total = 0.0;
        int count = 0;
        for (int y = 24; y < 52; ++y) {
            for (int x = leftHalf ? 6 : 17; x < (leftHalf ? 15 : 26); ++x) {
                const int sx = 160 + (heroX + x) * 8 + 2; // the 4x picture is shown 2x larger by the window
                const int sy = 90 + (12 + y) * 8 + 2;
                const std::size_t at = (static_cast<std::size_t>(sy) * static_cast<std::size_t>(pixels.width) + static_cast<std::size_t>(sx)) * 4;
                total += pixels.rgba[at] + pixels.rgba[at + 1] + pixels.rgba[at + 2];
                ++count;
            }
        }
        return total / count;
    };
    // Hero one is lit from his left, hero two from his right.
    CHECK(brightness(20, true) > brightness(20, false));
    CHECK(brightness(68, false) > brightness(68, true));
}
