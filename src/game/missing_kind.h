#pragma once

#include "boundary.h"

#include "luna/engine/ui.h"

#include <string>

namespace odysseus::game {

// A thing the level places whose kind the data no longer has (US-303, D-58 Q9): a plant or object that was deleted from plants.json or
// objects.json, a light kind that left lights.json, a character kind that left characters.json. It is skipped by play, a warning names the level
// entry, and a red "?" stands where it was placed so the owner can see and fix it.
struct MissingKind {
    std::string what; // the warning: `level "valley": plant #12 "oak" has no kind in plants.json`
    double x = 0.0;   // world pixels
    double y = 0.0;
};

// The red "?" of a missing kind, its foot at screen point (x, y).
inline void drawMissingMarker(luna::engine::UiPainter& painter, int x, int y) {
    painter.fill({x - 5, y - 13, 11, 13}, luna::engine::UiColor::Dark);
    painter.outline({x - 5, y - 13, 11, 13}, luna::engine::UiColor::Red);
    painter.text(x - 2, y - 10, "?", luna::engine::UiColor::Red);
}

} // namespace odysseus::game
