#pragma once

#include "boundary.h"

#include "game/catalogs.h"
#include "game/level.h"
#include "luna/engine/renderer.h"

#include <map>
#include <string>

namespace odysseus::game {

// Plants in the world (US-136, D-21). The pictures come from the content atlas, in one page per size:
// "plants-small" (32 x 32), "plants-tall" (32 x 64) and "trees" (64 x 96); the feet are at the middle
// of the bottom edge of the picture.
struct PlantArt {
    struct Source {
        std::string page;
        luna::engine::Rect rect; // the picture in its page
    };
    std::map<std::string, luna::engine::Texture> pages; // page name -> texture
    std::map<std::string, Source> sources;              // plant name -> where its picture is
};

// How tall a big plant is to shots and arrows, metres: 1.5 (bushes and tall plants), 3 (trees).
// Small plants block nothing, so 0.
double plantObstacleHeight(const PlantDef& plant);

// A plant's picture at its natural size, standing with its feet at (feetX, feetY) on the screen.
void drawPlant(luna::engine::Renderer& renderer, const PlantArt& art, const std::string& plant, int feetX, int feetY);
// The same stretched to fit inside `area` (keeping its proportions), for palettes.
void drawPlantIcon(luna::engine::Renderer& renderer, const PlantArt& art, const std::string& plant, const luna::engine::Rect& area);
// A plant's size on the screen in pixels (for picking it with the pointer); 32 x 32 when unknown.
luna::engine::Rect plantExtent(const PlantArt& art, const std::string& plant);

// The map cell a plant's feet stand in.
inline PixelPoint plantCell(PixelPoint feet) { return {feet.x >= 0 ? feet.x / kTileSize : -1, feet.y >= 0 ? feet.y / kTileSize : -1}; }

// A plant in the play state: growing, or destroyed and waiting to grow back.
struct WorldPlant {
    int id = 0;
    std::string kind;
    const PlantDef* def = nullptr;
    PixelPoint feet;
    bool alive = true;
    int regrowTicks = 0; // destroyed: ticks left before the same kind grows back somewhere else
};

inline constexpr int kInspectReachPixels = 48;  // 1.5 m: Interact looks at a plant this close
inline constexpr int kInspectTicks = 60;        // the text stays 3 s
inline constexpr int kRegrowTicks = 300;        // 15 s (D-21)
inline constexpr int kPlantHeal = 10;           // HP from an edible plant
inline constexpr int kRegrowTries = 64;         // random spots tried before waiting a second
inline constexpr int kRegrowRetryTicks = 20;

} // namespace odysseus::game
