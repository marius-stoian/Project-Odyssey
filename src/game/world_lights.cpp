// US-243: the point lights of the world: fires, burning objects, held weapons and the torches of clan members.
#include "game/odyssey_game.h"

#include "game/lighting.h"

#include <algorithm>

namespace odysseus::game {

namespace {
constexpr double kLightningLift = 0.85; // how far a strike lifts the ambient light toward white
} // namespace

// The lights the level itself holds (they shine whether or not the game is running, so the Editor can preview them): effects with a light, and the
// lights placed with the Editor's Light tool (US-247).
std::vector<OdysseyGame::LightSource> OdysseyGame::levelLightSources() const {
    std::vector<LightSource> sources;
    const auto add = [&](const LightKindDef& kind, double worldX, double worldY, double heightAboveGround, std::uint64_t id) {
        sources.push_back({&kind, worldX, worldY, worldY + heightAboveGround, id});
    };
    // Effects placed in the level that have a light: a camp fire.
    for (const PlacedEffect& placed : level_.effects) {
        const EffectDef* def = catalogs_.effect(placed.name);
        if (def == nullptr || def->light.empty()) continue;
        if (const LightKindDef* kind = lighting_.kind(def->light)) add(*kind, placed.at.x, placed.at.y - 8.0, 8.0, static_cast<std::uint64_t>(placed.id));
    }
    // Lights placed with the Light tool.
    for (const PlacedLight& placed : level_.lights) {
        if (const LightKindDef* kind = lighting_.kind(placed.kind)) add(*kind, placed.at.x, placed.at.y - 8.0, 8.0, 30000U + static_cast<std::uint64_t>(placed.id));
    }
    return sources;
}

std::vector<OdysseyGame::LightSource> OdysseyGame::lightSources(double alpha) const {
    std::vector<LightSource> sources = levelLightSources();
    const auto add = [&](const LightKindDef& kind, double worldX, double worldY, double heightAboveGround, std::uint64_t id) {
        sources.push_back({&kind, worldX, worldY, worldY + heightAboveGround, id});
    };

    // World objects that have a light, while they are in the state that shines (a burning fire pit).
    for (const WorldPlant& plant : plants_) {
        if (!plant.present() || plant.def == nullptr || plant.def->light.empty()) continue;
        if (!plant.def->lightState.empty() && plant.state != plant.def->lightState) continue;
        if (const LightKindDef* kind = lighting_.kind(plant.def->light)) add(*kind, plant.feet.x, plant.feet.y - 8.0, 8.0, 5000U + static_cast<std::uint64_t>(plant.id));
    }
    // Finished buildings with a light shine at night, and a burning piece is a fire (US-255).
    for (const sim::buildings::PlacedBuilding& building : buildings_.store().all()) {
        if (building.state != sim::buildings::State::Finished) continue;
        const sim::buildings::KindDef* kindDef = buildings_.data().kind(building.kind);
        if (kindDef != nullptr && !kindDef->light.empty() && !sky().sunUp) {
            if (const LightKindDef* kind = lighting_.kind(kindDef->light)) {
                const auto [cx, cy] = buildings_.store().centre(building);
                add(*kind, cx * kTileSize + kTileSize / 2.0, cy * kTileSize + kTileSize / 2.0, 24.0, 40000U + static_cast<std::uint64_t>(building.id));
            }
        }
        for (std::size_t p = 0; p < building.pieces.size(); ++p) {
            if (!building.pieces[p].burning) continue;
            if (const LightKindDef* kind = lighting_.kind("campfire")) {
                add(*kind, building.pieces[p].x * kTileSize + kTileSize / 2.0, building.pieces[p].y * kTileSize + kTileSize / 2.0, 24.0, 60000U + static_cast<std::uint64_t>(building.id * 64 + static_cast<int>(p)));
            }
        }
    }
    // The weapon in the hero's hand, when it has a light.
    if (const WeaponDef* weapon = heldWeapon(); weapon != nullptr && !weapon->light.empty()) {
        if (const LightKindDef* kind = lighting_.kind(weapon->light)) add(*kind, hero_.feetX(alpha), hero_.feetY(alpha) - 20.0, 20.0, 9000U);
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
                add(*kind, x, y - 18.0, 18.0, 20000U + static_cast<std::uint64_t>(i));
            }
        }
    }
    return sources;
}

std::vector<luna::engine::PointLight> OdysseyGame::worldLights(const luna::engine::Rect& view, double alpha, double darkness) const {
    const double seconds = (static_cast<double>(ticks_) + alpha) / 20.0; // the flicker follows the game's own time, not the simulation's randomness
    return pointLights(lightSources(alpha), view, seconds, darkness);
}

std::vector<luna::engine::PointLight> OdysseyGame::pointLights(const std::vector<LightSource>& sources, const luna::engine::Rect& view, double seconds,
                                                                double darkness) const {
    std::vector<luna::engine::PointLight> lights;
    for (const LightSource& source : sources) {
        if (static_cast<int>(lights.size()) >= luna::engine::LightFrame::kMaxLights) break;
        const LightKindDef& kind = *source.kind;
        luna::engine::PointLight light;
        light.x = static_cast<float>(source.x - view.x);
        light.y = static_cast<float>(source.y - view.y);
        light.radius = static_cast<float>(kind.radiusTiles * kTileSize);
        const double wobble = kind.flicker > 0.0 ? flickerNoise(source.id, seconds) : 0.0; // steady fires (D-49) skip the noise altogether
        light.strength = static_cast<float>(kind.strength * darkness * (1.0 - kind.flicker * wobble));
        light.r = static_cast<float>(kind.red / 255.0);
        light.g = static_cast<float>(kind.green / 255.0);
        light.b = static_cast<float>(kind.blue / 255.0);
        light.height = static_cast<float>(kind.height);
        // Off the picture (further than its own radius): nothing to light.
        if (light.x < -light.radius || light.y < -light.radius || light.x > view.width + light.radius || light.y > view.height + light.radius) continue;
        lights.push_back(light);
    }
    return lights;
}

// How dark the world is for the fires: 0 in full daylight, 1 on the darkest night (0.45 is how far the darkest night goes, D-49).
double OdysseyGame::darknessOf(const luna::engine::LightFrame& frame) {
    const double ambient = (frame.ambientR + frame.ambientG + frame.ambientB) / 3.0;
    return std::clamp((1.0 - ambient) / 0.45, 0.0, 1.0);
}

// The ambient colour of the world now: the lights.json colour, tinted by the time of day (US-242) and dimmed by an eclipse (US-248).
luna::engine::LightFrame OdysseyGame::ambientLightFrame(double alpha, bool withWeather) const {
    luna::engine::LightFrame frame = lighting_.ambientFrame();
    const SkyState skyNow = sky();
    frame.ambientR *= skyNow.ambientR;
    frame.ambientG *= skyNow.ambientG;
    frame.ambientB *= skyNow.ambientB;
    if (const CelestialLight celestial = celestialLight(alpha); celestial.valid && celestial.dimming < 1.0) {
        frame.ambientR *= static_cast<float>(celestial.dimming);
        frame.ambientG *= static_cast<float>(celestial.dimming);
        frame.ambientB *= static_cast<float>(celestial.dimming);
    }
    if (!withWeather) return frame; // fire shadows follow the sky only: dim weather by day still casts none (US-245)
    // Weather (US-246): rain dims and cools, fog greys, over the same 3 s the weather fades in; a storm's lightning flashes the whole scene.
    const WeatherLight weatherNow = weatherLight(catalogs_.weather, weather_.previous(), weather_.current(), weather_.fade());
    frame.ambientR *= weatherNow.red;
    frame.ambientG *= weatherNow.green;
    frame.ambientB *= weatherNow.blue;
    const double seconds = (static_cast<double>(ticks_) + alpha) / 20.0;
    const double rate = weatherFlashRate(catalogs_.weather, weather_.previous(), weather_.current(), weather_.fade());
    if (const double flash = lightningFlash(weatherSeed_, seconds, rate); flash > 0.0) {
        const auto lift = [flash](float ambient) { return ambient + (1.0F - ambient) * static_cast<float>(flash * kLightningLift); };
        frame.ambientR = lift(frame.ambientR);
        frame.ambientG = lift(frame.ambientG);
        frame.ambientB = lift(frame.ambientB);
    }
    return frame;
}

// What the Editor lights the level with when the owner previews an hour (US-247): the sky of that hour (in the clan's season, or spring) and the level's
// own lights. Nothing here is saved or played; weather and eclipses are left out.
luna::engine::LightFrame OdysseyGame::editorLightFrame(double hour, const luna::engine::Rect& view) const {
    luna::engine::LightFrame frame = lighting_.ambientFrame();
    const SkyState skyThen = skyAt(sky_, hour, gameClock().season);
    frame.ambientR *= skyThen.ambientR;
    frame.ambientG *= skyThen.ambientG;
    frame.ambientB *= skyThen.ambientB;
    frame.normalMaps = settings_.lighting != "Low";
    if (const double darkness = darknessOf(frame); darkness > 0.0) frame.lights = pointLights(levelLightSources(), view, 0.0, darkness);
    return frame;
}

} // namespace odysseus::game
