#pragma once

#include "boundary.h"

#include "core/geometry.h"
#include "luna/engine/image.h"

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace odysseus::game {

// The owner's art (D-05) turned into game-ready frames (US-120). His sheets in assets/sprites/
// are never changed: cuts.json lists the rectangles to take from them, and the cutter
// (odysseus_atlas) writes the atlases the game loads.

inline constexpr int kAtlasCharacterWidth = 32;  // D-04
inline constexpr int kAtlasCharacterHeight = 48;
inline constexpr int kAtlasTileSize = 32;        // D-16
inline constexpr int kAtlasColumns = 16;         // cells per atlas row

enum class CutKind { Tile, Character };

// One frame to take from a sheet, or to make by mirroring another frame.
struct Cut {
    std::string name;             // "grass", "hero.S.0", "goblin"
    CutKind kind = CutKind::Tile;
    std::string sheet;            // file name in assets/sprites/
    core::Rect rect{};            // where in the sheet
    std::string mirrorOf;         // not empty: this frame is that frame, left and right swapped
};

struct CutList {
    int tolerance = 24;           // how close to the background colour counts as background
    int tileInset = 4;            // pixels trimmed off each tile's drawn border
    std::vector<Cut> cuts;
};

// Reads cuts.json; every problem is a DataError naming the file and the field.
CutList loadCuts(const std::filesystem::path& file);

// Frames packed in two pictures, with a name for every cell.
struct Atlas {
    luna::engine::Image characters{kAtlasColumns * kAtlasCharacterWidth, kAtlasCharacterHeight};
    luna::engine::Image tiles{kAtlasColumns * kAtlasTileSize, kAtlasTileSize};
    std::map<std::string, int> characterCells; // name -> cell number, row by row
    std::map<std::string, int> tileCells;

    int characterCount() const { return static_cast<int>(characterCells.size()); }
    int tileCount() const { return static_cast<int>(tileCells.size()); }
    core::Rect characterFrame(int cell) const;
    core::Rect tileFrame(int cell) const;
};

// Cuts every frame from the sheets in `spritesFolder`. A sheet that cannot be read is a DataError.
Atlas cutAtlas(const CutList& cuts, const std::filesystem::path& spritesFolder);

// Writes characters.png, tiles.png and atlas.json into `folder`.
void saveAtlas(const Atlas& atlas, const std::filesystem::path& folder);

// Reads what saveAtlas wrote. On any problem returns nothing and says which file and why.
std::optional<Atlas> loadAtlas(const std::filesystem::path& folder, std::string& problem);

// Every frame on a checkerboard, larger, with gaps between them: for the owner to review.
luna::engine::Image contactSheet(const Atlas& atlas);

// The pictures the game draws, in the layouts the drawing code already uses: the hero sheet
// (a row per Facing, kWalkFrames columns), the ground strip (a column per TileKind), and the
// demo enemy with its red hit flash. From the owner's atlas when it loads, otherwise the
// programmer art (US-120 "Missing art").
struct ArtSet {
    luna::engine::Image heroSheet{0, 0};
    luna::engine::Image tileStrip{0, 0};
    luna::engine::Image enemy{0, 0};
    luna::engine::Image enemyHit{0, 0};
    bool ownArt = false;
    std::string problem; // why the programmer art is used, when it is
};

// `spritesFolder` holds atlas/ (written by odysseus_atlas).
ArtSet makeArtSet(const std::filesystem::path& spritesFolder);

} // namespace odysseus::game
