#pragma once

#include "boundary.h"

#include "game/level.h"
#include "luna/engine/renderer.h"
#include "luna/engine/ui.h"

#include <map>
#include <string>

namespace odysseus::game {

// How a weapon looks as a pickup, in the hotbar and in the Editor's weapon palette (US-134):
// its icon from the content atlas. The demo weapons of M1b and US-029 have no icon, so they
// get a small gold badge with two letters.
struct WeaponArt {
    luna::engine::Texture icons;
    std::map<std::string, luna::engine::Rect> sources; // weapon name -> its icon in the atlas
};

// Two letters for a weapon without an icon: "Sp" for the spear throw, "Sw" for the sword.
std::string badgeLetters(const std::string& weapon);

// Draws the weapon's icon stretched into `area`, or its badge when it has no icon.
void drawWeaponIcon(luna::engine::Renderer& renderer, luna::engine::UiPainter& painter, const WeaponArt& art, const std::string& weapon,
                    const luna::engine::Rect& area);

// Pickups are picked up when the hero's feet come within this many pixels of their middle.
inline constexpr int kPickupReach = 16;
// A pickup lying in the world is drawn this many pixels across.
inline constexpr int kPickupSize = 20;

} // namespace odysseus::game
