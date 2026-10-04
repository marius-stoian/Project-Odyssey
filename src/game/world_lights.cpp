// US-243: the point lights of the world: fires, burning objects, held weapons and the torches of clan members.
#include "game/odyssey_game.h"

#include "game/lighting.h"

#include <algorithm>

namespace odysseus::game {

std::vector<luna::engine::PointLight> OdysseyGame::worldLights(const luna::engine::Rect& view, double alpha, double darkness) const {
    std::vector<luna::engine::PointLight> lights;
    const double seconds = (static_cast<double>(ticks_) + alpha) / 20.0; // the flicker follows the game's own time, not the simulation's randomness
    const auto add = [&](const LightKindDef& kind, double worldX, double worldY, std::uint64_t id) {
        if (static_cast<int>(lights.size()) >= luna::engine::LightFrame::kMaxLights) return;
        luna::engine::PointLight light;
        light.x = static_cast<float>(worldX - view.x);
        light.y = static_cast<float>(worldY - view.y);
        light.radius = static_cast<float>(kind.radiusTiles * kTileSize);
        const double wobble = kind.flicker > 0.0 ? flickerNoise(id, seconds) : 0.0; // steady fires (D-49) skip the noise altogether
        light.strength = static_cast<float>(kind.strength * darkness * (1.0 - kind.flicker * wobble));
        light.r = static_cast<float>(kind.red / 255.0);
        light.g = static_cast<float>(kind.green / 255.0);
        light.b = static_cast<float>(kind.blue / 255.0);
        light.height = static_cast<float>(kind.height);
        // Off the picture (further than its own radius): nothing to light.
        if (light.x < -light.radius || light.y < -light.radius || light.x > view.width + light.radius || light.y > view.height + light.radius) return;
        lights.push_back(light);
    };

    // Effects placed in the level that have a light: a camp fire.
    for (const PlacedEffect& placed : level_.effects) {
        const EffectDef* def = catalogs_.effect(placed.name);
        if (def == nullptr || def->light.empty()) continue;
        if (const LightKindDef* kind = lighting_.kind(def->light)) add(*kind, placed.at.x, placed.at.y - 8.0, static_cast<std::uint64_t>(placed.id));
    }
    // World objects that have a light, while they are in the state that shines (a burning fire pit).
    for (const WorldPlant& plant : plants_) {
        if (!plant.present() || plant.def == nullptr || plant.def->light.empty()) continue;
        if (!plant.def->lightState.empty() && plant.state != plant.def->lightState) continue;
        if (const LightKindDef* kind = lighting_.kind(plant.def->light)) add(*kind, plant.feet.x, plant.feet.y - 8.0, 5000U + static_cast<std::uint64_t>(plant.id));
    }
    // The weapon in the hero's hand, when it has a light.
    if (const WeaponDef* weapon = heldWeapon(); weapon != nullptr && !weapon->light.empty()) {
        if (const LightKindDef* kind = lighting_.kind(weapon->light)) add(*kind, hero_.feetX(alpha), hero_.feetY(alpha) - 20.0, 9000U);
    }
    // Clan members carry torches at night; the light follows them as they walk.
    if (clan_ && !lighting_.clanTorch.empty() && !sky().sunUp) {
        if (const LightKindDef* kind = lighting_.kind(lighting_.clanTorch)) {
            const auto& figures = clanView_.figures();
            for (std::size_t i = 0; i < figures.size(); ++i) {
                const Figure& figure = figures[i];
                if (!figure.present || !figure.placed) continue;
                const double x = figure.previousX + (figure.x - figure.previousX) * alpha;
                const double y = figure.previousY + (figure.y - figure.previousY) * alpha;
                add(*kind, x, y - 18.0, 20000U + static_cast<std::uint64_t>(i));
            }
        }
    }
    return lights;
}

} // namespace odysseus::game
