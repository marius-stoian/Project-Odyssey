#include "game/normal_art.h"

#include "game/art.h"
#include "game/content_art.h"
#include "sim/data.h"

#include "luna/engine/image_io.h"
#include "luna/engine/image_ops.h"

#include <format>

namespace odysseus::game {

namespace {

using luna::engine::Image;

constexpr double kBodyStrength = 2.0;   // characters, plants, animals: rounded bodies
constexpr double kGroundStrength = 0.6; // ground tiles: a hint of grain, never a bump

// The hand-made map of `frame`, when `<sprites>/<frame>_n.png` exists: pasted at `at` (the frame's rectangle) into `normals`. A map of another size
// than the frame is a DataError, so a wrong file is found at once and not as a smear in the game.
bool pasteHandMade(Image& normals, const std::filesystem::path& sprites, const std::string& frame, const core::Rect& at) {
    const std::filesystem::path file = sprites / (frame + "_n.png");
    if (!std::filesystem::exists(file)) return false;
    std::string error;
    const auto made = luna::engine::loadPng(file, error);
    if (!made) throw sim::DataError(file, "(file)", error);
    if (made->width() != at.width || made->height() != at.height) {
        throw sim::DataError(file, "(size)", std::format("the frame \"{}\" is {}x{} pixels, this map is {}x{}", frame, at.width, at.height, made->width(), made->height()));
    }
    for (int y = 0; y < at.height; ++y) {
        for (int x = 0; x < at.width; ++x) normals.set(at.x + x, at.y + y, made->get(x, y));
    }
    return true;
}

void save(const Image& normals, const std::filesystem::path& file) {
    if (!luna::engine::savePng(normals, file)) throw sim::DataError(file, "(file)", "the normal atlas cannot be written");
}

} // namespace

NormalReport writeNormalAtlases(const std::filesystem::path& spritesFolder) {
    const std::filesystem::path folder = spritesFolder / "atlas";
    NormalReport report;
    std::string problem;
    const auto atlas = loadAtlas(folder, problem);
    if (!atlas) throw sim::DataError(folder, "(atlas)", problem);

    Image characters = luna::engine::normalAtlas(atlas->characters, kAtlasCharacterWidth, kAtlasCharacterHeight, kBodyStrength);
    for (const auto& [name, cell] : atlas->characterCells) {
        if (pasteHandMade(characters, spritesFolder, name, atlas->characterFrame(cell))) ++report.handMade;
    }
    save(characters, folder / "characters_n.png");
    ++report.atlases;

    Image tiles = luna::engine::normalAtlas(atlas->tiles, kAtlasTileSize, kAtlasTileSize, kGroundStrength);
    for (const auto& [name, cell] : atlas->tileCells) {
        if (pasteHandMade(tiles, spritesFolder, name, atlas->tileFrame(cell))) ++report.handMade;
    }
    save(tiles, folder / "tiles_n.png");
    ++report.atlases;

    if (const auto content = loadContent(folder, problem)) {
        for (const ContentPage& page : content->pages) {
            Image normals = luna::engine::normalAtlas(content->pictures.at(page.name), page.cellWidth, page.cellHeight, kBodyStrength);
            for (const auto& [name, frame] : content->frames) {
                if (frame.page != page.name) continue;
                if (const auto rect = content->rect(name)) {
                    if (pasteHandMade(normals, spritesFolder, name, *rect)) ++report.handMade;
                }
            }
            save(normals, folder / ("content-" + page.name + "_n.png"));
            ++report.atlases;
        }
    }
    return report;
}

std::optional<Image> loadNormalAtlas(const std::filesystem::path& folder, const std::string& name, const Image& base, std::string* note) {
    const std::filesystem::path file = folder / (name + "_n.png");
    if (!std::filesystem::exists(file)) return std::nullopt; // no map: flat, no error
    std::string error;
    auto normals = luna::engine::loadPng(file, error);
    if (!normals) {
        if (note != nullptr) *note = file.string() + " " + error;
        return std::nullopt;
    }
    if (normals->width() != base.width() || normals->height() != base.height()) {
        if (note != nullptr) *note = std::format("{} is {}x{} but its atlas is {}x{}: not used", file.string(), normals->width(), normals->height(), base.width(), base.height());
        return std::nullopt;
    }
    return normals;
}

} // namespace odysseus::game
