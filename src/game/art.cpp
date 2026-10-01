#include "game/art.h"

#include "game/placeholder_art.h"

#include "luna/engine/image_io.h"
#include "luna/engine/image_ops.h"
#include "sim/data.h"
#include "sim/json_data.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <format>
#include <fstream>
#include <tuple>

namespace odysseus::game {

using luna::engine::Image;
using nlohmann::json;

namespace {

core::Rect cell(int index, int width, int height) {
    return {index % kAtlasColumns * width, index / kAtlasColumns * height, width, height};
}

// Frames laid out in rows of kAtlasColumns cells.
Image pack(const std::vector<Image>& frames, int width, int height) {
    const int rows = std::max(1, (static_cast<int>(frames.size()) + kAtlasColumns - 1) / kAtlasColumns);
    Image atlas(kAtlasColumns * width, rows * height);
    for (std::size_t i = 0; i < frames.size(); ++i) {
        const core::Rect at = cell(static_cast<int>(i), width, height);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                atlas.set(at.x + x, at.y + y, frames[i].get(x, y));
            }
        }
    }
    return atlas;
}

} // namespace

core::Rect Atlas::characterFrame(int index) const {
    return cell(index, kAtlasCharacterWidth, kAtlasCharacterHeight);
}

core::Rect Atlas::tileFrame(int index) const {
    return cell(index, kAtlasTileSize, kAtlasTileSize);
}

CutList loadCuts(const std::filesystem::path& file) {
    const json data = sim::readJsonFile(file);
    CutList list;
    list.tolerance = sim::requireInt(data, file, "tolerance", 0, 255);
    list.tileInset = sim::requireInt(data, file, "tileInset", 0, 64);
    if (!data.contains("cuts") || !data.at("cuts").is_array()) {
        throw sim::DataError(file, "cuts", "must be a list of frames");
    }
    std::map<std::string, CutKind> known;
    for (std::size_t i = 0; i < data.at("cuts").size(); ++i) {
        const json& entry = data.at("cuts").at(i);
        const std::string where = std::format("cuts[{}]", i);
        Cut cut;
        if (!entry.contains("name") || !entry.at("name").is_string() || entry.at("name").get<std::string>().empty()) {
            throw sim::DataError(file, where + ".name", "must be a name in quotes");
        }
        cut.name = entry.at("name").get<std::string>();
        if (known.contains(cut.name)) {
            throw sim::DataError(file, where + ".name", "\"" + cut.name + "\" is listed twice");
        }
        const std::string kind = entry.value("kind", std::string());
        if (kind != "tile" && kind != "character") {
            throw sim::DataError(file, where + ".kind", "must be \"tile\" or \"character\"");
        }
        cut.kind = kind == "tile" ? CutKind::Tile : CutKind::Character;
        if (entry.contains("mirrorOf")) {
            cut.mirrorOf = entry.at("mirrorOf").get<std::string>();
            if (!known.contains(cut.mirrorOf) || known.at(cut.mirrorOf) != cut.kind) {
                throw sim::DataError(file, where + ".mirrorOf", "\"" + cut.mirrorOf + "\" must be an earlier frame of the same kind");
            }
        } else {
            if (!entry.contains("sheet") || !entry.at("sheet").is_string()) {
                throw sim::DataError(file, where + ".sheet", "must be the file name of a sheet in assets/sprites");
            }
            cut.sheet = entry.at("sheet").get<std::string>();
            const json& rect = entry.value("rect", json());
            if (!rect.is_array() || rect.size() != 4 || !std::all_of(rect.begin(), rect.end(), [](const json& v) { return v.is_number_integer() && v.get<int>() >= 0; }) ||
                rect.at(2).get<int>() == 0 || rect.at(3).get<int>() == 0) {
                throw sim::DataError(file, where + ".rect", "must be [x, y, width, height] in whole pixels, width and height above 0");
            }
            cut.rect = {rect.at(0).get<int>(), rect.at(1).get<int>(), rect.at(2).get<int>(), rect.at(3).get<int>()};
        }
        known[cut.name] = cut.kind;
        list.cuts.push_back(cut);
    }
    return list;
}

Atlas cutAtlas(const CutList& list, const std::filesystem::path& spritesFolder) {
    std::map<std::string, Image> sheets;
    std::map<std::string, Image> made;
    std::vector<Image> characters;
    std::vector<Image> tiles;
    Atlas atlas;
    for (const Cut& cut : list.cuts) {
        Image frame(0, 0);
        if (!cut.mirrorOf.empty()) {
            frame = luna::engine::mirrored(made.at(cut.mirrorOf));
        } else {
            if (!sheets.contains(cut.sheet)) {
                std::string error;
                auto sheet = luna::engine::loadPng(spritesFolder / cut.sheet, error);
                if (!sheet) {
                    throw sim::DataError(spritesFolder / cut.sheet, "(file)", error);
                }
                sheets.emplace(cut.sheet, std::move(*sheet));
            }
            const Image& sheet = sheets.at(cut.sheet);
            if (cut.kind == CutKind::Tile) {
                // Tiles fill their square: trim the drawn frame around them, then scale.
                const int inset = list.tileInset;
                const Image inner = luna::engine::crop(sheet, {cut.rect.x + inset, cut.rect.y + inset, cut.rect.width - 2 * inset, cut.rect.height - 2 * inset});
                frame = luna::engine::fitInto(inner, kAtlasTileSize, kAtlasTileSize, false);
            } else {
                // Characters stand on a coloured sheet: clear the background, keep only the figure.
                Image figure = luna::engine::crop(sheet, cut.rect);
                luna::engine::removeBackground(figure, list.tolerance);
                figure = luna::engine::crop(figure, luna::engine::opaqueBounds(figure));
                frame = luna::engine::fitInto(figure, kAtlasCharacterWidth, kAtlasCharacterHeight, true);
            }
        }
        made.emplace(cut.name, frame);
        if (cut.kind == CutKind::Tile) {
            atlas.tileCells[cut.name] = static_cast<int>(tiles.size());
            tiles.push_back(frame);
        } else {
            atlas.characterCells[cut.name] = static_cast<int>(characters.size());
            characters.push_back(frame);
        }
    }
    atlas.characters = pack(characters, kAtlasCharacterWidth, kAtlasCharacterHeight);
    atlas.tiles = pack(tiles, kAtlasTileSize, kAtlasTileSize);
    return atlas;
}

void saveAtlas(const Atlas& atlas, const std::filesystem::path& folder) {
    std::filesystem::create_directories(folder);
    if (!luna::engine::savePng(atlas.characters, folder / "characters.png") || !luna::engine::savePng(atlas.tiles, folder / "tiles.png")) {
        throw sim::DataError(folder, "(files)", "the atlas pictures cannot be written");
    }
    const json index{{"atlasVersion", 1},
                     {"characterSize", {kAtlasCharacterWidth, kAtlasCharacterHeight}},
                     {"tileSize", kAtlasTileSize},
                     {"columns", kAtlasColumns},
                     {"characters", atlas.characterCells},
                     {"tiles", atlas.tileCells}};
    std::ofstream(folder / "atlas.json", std::ios::binary | std::ios::trunc) << index.dump(1) << '\n';
}

std::optional<Atlas> loadAtlas(const std::filesystem::path& folder, std::string& problem) {
    Atlas atlas;
    try {
        const json index = sim::readJsonFile(folder / "atlas.json");
        if (index.value("atlasVersion", 0) != 1 || index.value("columns", 0) != kAtlasColumns) {
            problem = (folder / "atlas.json").string() + ": atlasVersion or columns is not what this game reads";
            return std::nullopt;
        }
        atlas.characterCells = index.at("characters").get<std::map<std::string, int>>();
        atlas.tileCells = index.at("tiles").get<std::map<std::string, int>>();
    } catch (const std::exception& error) {
        problem = error.what();
        return std::nullopt;
    }
    for (const auto& [name, image, width, height, cells] :
         {std::tuple{"characters.png", &atlas.characters, kAtlasCharacterWidth, kAtlasCharacterHeight, &atlas.characterCells},
          std::tuple{"tiles.png", &atlas.tiles, kAtlasTileSize, kAtlasTileSize, &atlas.tileCells}}) {
        std::string error;
        auto picture = luna::engine::loadPng(folder / name, error);
        if (!picture) {
            problem = (folder / name).string() + " " + error;
            return std::nullopt;
        }
        for (const auto& [frame, index] : *cells) {
            const core::Rect at = cell(index, width, height);
            if (index < 0 || at.x + at.width > picture->width() || at.y + at.height > picture->height()) {
                problem = std::format("{}: frame \"{}\" (cell {}) lies outside the picture", (folder / "atlas.json").string(), frame, index);
                return std::nullopt;
            }
        }
        *image = std::move(*picture);
    }
    return atlas;
}

Image contactSheet(const Atlas& atlas) {
    constexpr int kZoom = 3;
    constexpr int kGap = 6;
    auto section = [&](const Image& source, const std::map<std::string, int>& cells, int w, int h) {
        const int count = static_cast<int>(cells.size());
        const int cols = kAtlasColumns;
        const int rows = std::max(1, (count + cols - 1) / cols);
        Image out(cols * (w * kZoom + kGap) + kGap, rows * (h * kZoom + kGap) + kGap);
        for (int y = 0; y < out.height(); ++y) {
            for (int x = 0; x < out.width(); ++x) {
                const bool light = ((x / 8) + (y / 8)) % 2 == 0;
                out.set(x, y, light ? luna::engine::Color{200, 200, 200} : luna::engine::Color{150, 150, 150});
            }
        }
        for (int i = 0; i < count; ++i) {
            const core::Rect at = cell(i, w, h);
            const int ox = kGap + (i % cols) * (w * kZoom + kGap);
            const int oy = kGap + (i / cols) * (h * kZoom + kGap);
            for (int y = 0; y < h * kZoom; ++y) {
                for (int x = 0; x < w * kZoom; ++x) {
                    const auto c = source.get(at.x + x / kZoom, at.y + y / kZoom);
                    if (c.alpha > 0) out.set(ox + x, oy + y, c);
                }
            }
        }
        return out;
    };
    const Image people = section(atlas.characters, atlas.characterCells, kAtlasCharacterWidth, kAtlasCharacterHeight);
    const Image ground = section(atlas.tiles, atlas.tileCells, kAtlasTileSize, kAtlasTileSize);
    Image sheet(std::max(people.width(), ground.width()), people.height() + ground.height());
    for (int y = 0; y < people.height(); ++y)
        for (int x = 0; x < people.width(); ++x) sheet.set(x, y, people.get(x, y));
    for (int y = 0; y < ground.height(); ++y)
        for (int x = 0; x < ground.width(); ++x) sheet.set(x, people.height() + y, ground.get(x, y));
    return sheet;
}

namespace {

void paste(Image& target, int left, int top, const Image& source, const core::Rect& from) {
    for (int y = 0; y < from.height; ++y) {
        for (int x = 0; x < from.width; ++x) {
            target.set(left + x, top + y, source.get(from.x + x, from.y + y));
        }
    }
}

// The same figure in red: how the enemy looks for a moment after a hit.
Image redTint(const Image& source) {
    Image out = source;
    for (int y = 0; y < out.height(); ++y) {
        for (int x = 0; x < out.width(); ++x) {
            const auto c = out.get(x, y);
            if (c.alpha > 0) {
                out.set(x, y, {static_cast<std::uint8_t>(std::min(255, c.red / 2 + 150)), static_cast<std::uint8_t>(c.green / 4),
                               static_cast<std::uint8_t>(c.blue / 4), c.alpha});
            }
        }
    }
    return out;
}

// A flat, speckled tile for ground kinds the programmer art never drew.
Image plainTile(const std::string& name) {
    std::uint32_t h = 2166136261U;
    for (const char c : name) h = (h ^ static_cast<unsigned char>(c)) * 16777619U;
    const luna::engine::Color base{static_cast<std::uint8_t>(80 + h % 120), static_cast<std::uint8_t>(80 + (h >> 8) % 120),
                                   static_cast<std::uint8_t>(80 + (h >> 16) % 120)};
    Image tile(kTileSize, kTileSize);
    tile.fillRect(0, 0, kTileSize, kTileSize, base);
    for (int i = 0; i < 24; ++i) {
        tile.set((i * 7 + 3) % kTileSize, (i * 13 + 5) % kTileSize,
                 {static_cast<std::uint8_t>(base.red * 3 / 4), static_cast<std::uint8_t>(base.green * 3 / 4), static_cast<std::uint8_t>(base.blue * 3 / 4)});
    }
    return tile;
}

ArtSet programmerArt(std::string problem, const std::vector<std::string>& groundFrames) {
    ArtSet art;
    art.heroSheet = makeCharacterSheet();
    // The four grounds the programmer art knows, by their atlas names; plain tiles for the rest.
    const Image drawn = makeTileSheet();
    const std::map<std::string, int> known = {{"grass", 0}, {"path", 1}, {"stone", 2}, {"water", 3}};
    art.tileStrip = Image(std::max<int>(1, static_cast<int>(groundFrames.size())) * kTileSize, kTileSize);
    for (std::size_t i = 0; i < groundFrames.size(); ++i) {
        const auto it = known.find(groundFrames[i]);
        if (it != known.end()) {
            paste(art.tileStrip, static_cast<int>(i) * kTileSize, 0, drawn, {it->second * kTileSize, 0, kTileSize, kTileSize});
        } else {
            paste(art.tileStrip, static_cast<int>(i) * kTileSize, 0, plainTile(groundFrames[i]), {0, 0, kTileSize, kTileSize});
        }
    }
    // Every character is the programmer figure; its red flash is the one the prop sheet has.
    art.characters = luna::engine::crop(art.heroSheet, {0, 0, kCharacterWidth, kCharacterHeight});
    art.charactersHit = luna::engine::crop(makePropSheet(), kEnemyHitFrame);
    art.problem = std::move(problem);
    return art;
}

} // namespace

core::Rect ArtSet::frame(const std::string& frames, int directions, Facing facing, int walkFrame) const {
    // The owner's hero sheet is one turn-around: row S turns from the front (S0, S4) to three-quarter
    // views, row E holds the side views (E0-E3 look left, E5-E7 look right), row N the back and its
    // three-quarter views. Looking left on screen is West, so each of the eight facings is given the
    // frames that really show that direction (picked by eye from the sheet), four per walk cycle.
    struct View {
        char row;
        int frame[4];
    };
    constexpr View kView[] = {
        {'S', {0, 4, 0, 4}}, // South: facing the player
        {'S', {1, 2, 3, 2}}, // SouthWest: front, turned to the left
        {'E', {0, 1, 2, 3}}, // West: side view looking left
        {'N', {1, 2, 1, 2}}, // NorthWest: back, turned to the left
        {'N', {4, 3, 4, 5}}, // North: back
        {'N', {7, 6, 7, 6}}, // NorthEast: back, turned to the right
        {'E', {7, 6, 5, 6}}, // East: side view looking right
        {'S', {7, 6, 5, 6}}, // SouthEast: front, turned to the right
    };
    const std::string name = directions == 8 ? std::format("{}.{}.{}", frames, kView[static_cast<std::size_t>(facing)].row,
                                                           kView[static_cast<std::size_t>(facing)].frame[std::clamp(walkFrame, 0, 3)])
                                             : frames;    const auto it = cells.find(name);
    const int cell = it == cells.end() ? 0 : it->second;
    return {cell % kAtlasColumns * kAtlasCharacterWidth, cell / kAtlasColumns * kAtlasCharacterHeight, kAtlasCharacterWidth, kAtlasCharacterHeight};
}

ArtSet makeArtSet(const std::filesystem::path& spritesFolder, const std::vector<std::string>& groundFrames) {
    std::string problem;
    const auto atlas = loadAtlas(spritesFolder / "atlas", problem);
    if (!atlas) {
        return programmerArt(problem, groundFrames);
    }
    for (const std::string& name : groundFrames) {
        if (!atlas->tileCells.contains(name)) {
            return programmerArt((spritesFolder / "atlas" / "atlas.json").string() + ": no ground frame \"" + name + "\"", groundFrames);
        }
    }
    if (!atlas->characterCells.contains("hero.S.0")) {
        return programmerArt((spritesFolder / "atlas" / "atlas.json").string() + ": no hero frames", groundFrames);
    }
    ArtSet art;
    art.ownArt = true;
    art.cells = atlas->characterCells;
    art.characters = atlas->characters;
    art.charactersHit = redTint(atlas->characters);
    art.heroSheet = Image(kWalkFrames * kCharacterWidth, static_cast<int>(Facing::Count) * kCharacterHeight);
    for (int facing = 0; facing < static_cast<int>(Facing::Count); ++facing) {
        for (int walk = 0; walk < kWalkFrames; ++walk) {
            paste(art.heroSheet, walk * kCharacterWidth, facing * kCharacterHeight, art.characters, art.frame("hero", 8, static_cast<Facing>(facing), walk));
        }
    }
    art.tileStrip = Image(std::max<int>(1, static_cast<int>(groundFrames.size())) * kTileSize, kTileSize);
    for (std::size_t i = 0; i < groundFrames.size(); ++i) {
        paste(art.tileStrip, static_cast<int>(i) * kTileSize, 0, atlas->tiles, atlas->tileFrame(atlas->tileCells.at(groundFrames[i])));
    }
    return art;
}

} // namespace odysseus::game
