#pragma once

#include "boundary.h"

#include "luna/engine/image.h"
#include "sim/npc_class.h"
#include "sim/npc_kind.h"

#include <optional>
#include <string>
#include <vector>

namespace odysseus::game {

// The Editor's marker under a placed NPC (US-269): a ring in the colour of its class with the class icon inside. With several classes the ring is split in equal
// arcs (the first arc at the top, going clockwise, one per class in the NPC's order) and the icon is the first class's. Drawn only in the Editor.
struct NpcMarker {
    std::vector<int> colours; // 0xRRGGBB, one per class that has a file
    std::string icon;         // the first such class's icon
    friend bool operator==(const NpcMarker&, const NpcMarker&) = default;
};

constexpr int kMarkerSize = 18; // pixels, square

// What the marker of an NPC with these classes looks like; nullopt when none of its classes has a file (no class, no marker).
std::optional<NpcMarker> markerFor(const sim::rules::ResolvedNpc& resolved, const sim::rules::NpcClassCatalog& catalog);

// The marker's picture (kMarkerSize square, transparent outside the ring); the same marker always gives the same picture.
luna::engine::Image markerImage(const NpcMarker& marker);

// The 8x8 picture of a built-in icon: eight strings of eight characters, '#' for a lit pixel. An unknown name gives the picture of "person".
const std::vector<std::string>& iconBitmap(const std::string& name);

} // namespace odysseus::game
