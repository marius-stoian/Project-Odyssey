#pragma once

#include "boundary.h"

#include "game/level.h"
#include "luna/engine/renderer.h"

#include <map>
#include <string>

namespace odysseus::game {

// How the animals look (US-137, D-21): a single side view each, cut from the owner's animal sheet into the
// content atlas page "animals" (64 x 48, the feet at the middle of the bottom edge). The picture faces east;
// the west side is the same page mirrored.
struct AnimalArt {
    luna::engine::Texture page;     // the page as cut
    luna::engine::Texture mirrored; // the same page turned around, for animals facing west
    int pageWidth = 0;
    std::map<std::string, luna::engine::Rect> sources; // animal name -> its picture in the page
};

inline constexpr int kAnimalWidth = 64;
inline constexpr int kAnimalHeight = 48;

// Whether a facing looks toward the west side of the screen (W, NW, SW).
bool facesWest(Facing facing);

// The animal standing with its feet at (feetX, feetY) on the screen, looking east or west; `flash` brightens
// it for the moment after a hit.
void drawAnimal(luna::engine::Renderer& renderer, const AnimalArt& art, const std::string& animal, int feetX, int feetY, Facing facing, bool flash);
// The animal shrunk to fit inside `area` (palettes).
void drawAnimalIcon(luna::engine::Renderer& renderer, const AnimalArt& art, const std::string& animal, const luna::engine::Rect& area);

} // namespace odysseus::game
