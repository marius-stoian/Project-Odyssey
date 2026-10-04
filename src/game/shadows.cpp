// US-244: sun and moon shadows of the people, animals, plants and objects of the world; US-245: faint shadows away from the nearest fires at night.
// Presentation only: the simulation never reads them.
#include "game/odyssey_game.h"

#include "game/clan_art.h"
#include "luna/engine/image_ops.h"
#include "luna/engine/shadow_draw.h"

#include <algorithm>
#include <cmath>

namespace odysseus::game {

namespace {
constexpr double kFireShadowMinDistance = 12.0; // pixels: closer than this, a light is the thing's own torch
constexpr double kFireShadowShortest = 0.4;     // shadow length per height at the edge of a fire's light ...
constexpr double kFireShadowLongest = 1.5;      // ... and right beside it (the cap the moon has too, D-49)
} // namespace

OdysseyGame::ShadowCast OdysseyGame::shadowCast(double alpha) const {
    ShadowCast cast;
    const CelestialLight light = celestialLight(alpha);
    if (!light.valid || light.lengthPerHeight <= 0.0) return cast;
    // How dark: the sky's shadow strength of the hour (sky.json), what an eclipse leaves of the light, and what the weather takes away (fog and
    // heavy cloud fade the shadows).
    const auto& weather = catalogs_.weather;
    const auto fadeOf = [&](int index) { return index >= 0 && index < static_cast<int>(weather.size()) ? weather[static_cast<std::size_t>(index)].shadowFade : 0.0; };
    const double fade = fadeOf(weather_.previous()) + (fadeOf(weather_.current()) - fadeOf(weather_.previous())) * weather_.fade();
    const double strength = std::clamp(sky().shadowStrength * std::min(1.0, light.strength) * (1.0 - fade), 0.0, 1.0);
    cast.alpha = static_cast<std::uint8_t>(std::lround(strength * 255.0));
    cast.on = cast.alpha > 0;
    cast.dirX = light.dirX;
    cast.dirY = light.dirY;
    cast.lengthPerHeight = light.lengthPerHeight;
    return cast;
}

void OdysseyGame::castShadow(luna::engine::Renderer& renderer, const ShadowCast& cast, const luna::engine::Texture& texture, const luna::engine::Rect& source,
                             int feetX, int feetY, double heightMetres, double worldX, double worldY) const {
    if (heightMetres <= 0.0) return;
    const auto found = silhouettes_.find(texture.id);
    if (found == silhouettes_.end()) return;
    // The shadow is as long as the thing is tall (its catalog height) times the light's factor, however many pixels its picture has.
    const double metresToPixels = kTileSize * heightMetres / std::max(1, source.height);
    if (cast.on) luna::engine::drawShadow(renderer, found->second, source, {feetX, feetY}, cast.dirX, cast.dirY, cast.lengthPerHeight * metresToPixels, cast.alpha);

    // Fires (US-245): of the lights that reach this thing, only the nearest few (lights.json fireShadows.maxPerObject) throw a shadow, and a thing
    // right beside its own torch (the one it carries) throws none from it. Nearer is longer and darker; the shadow falls straight away from the fire.
    struct Near {
        double distance;
        const LightSource* light;
    };
    std::vector<Near> reaching;
    for (const LightSource& light : fireShadows_.lights) {
        const double distance = std::hypot(worldX - light.x, worldY - light.groundY);
        if (distance >= light.kind->radiusTiles * kTileSize || distance < kFireShadowMinDistance) continue;
        reaching.push_back({distance, &light});
    }
    const std::size_t count = std::min(reaching.size(), static_cast<std::size_t>(lighting_.shadowLightsPerObject));
    std::partial_sort(reaching.begin(), reaching.begin() + static_cast<std::ptrdiff_t>(count), reaching.end(), [](const Near& a, const Near& b) {
        return a.distance != b.distance ? a.distance < b.distance : a.light->id < b.light->id; // a tie goes the same way every frame
    });
    for (std::size_t i = 0; i < count; ++i) {
        const LightSource& light = *reaching[i].light;
        const double nearness = 1.0 - reaching[i].distance / (light.kind->radiusTiles * kTileSize); // 1 beside the fire, 0 at the edge of its light
        const double darkness = lighting_.fireShadowStrength * fireShadows_.darkness * std::min(1.0, light.kind->strength) * nearness;
        const auto alpha = static_cast<std::uint8_t>(std::lround(std::clamp(darkness, 0.0, 1.0) * 255.0));
        if (alpha == 0) continue;
        const double lengthPerHeight = kFireShadowShortest + (kFireShadowLongest - kFireShadowShortest) * nearness;
        const double dirX = (worldX - light.x) / reaching[i].distance;
        const double dirY = (worldY - light.groundY) / reaching[i].distance;
        luna::engine::drawShadow(renderer, found->second, source, {feetX, feetY}, dirX, dirY, lengthPerHeight * metresToPixels, alpha);
    }
}
luna::engine::Texture OdysseyGame::lookTexture(luna::engine::Renderer& renderer, const LookSpec& look) {
    const auto found = lookTextures_.find(look);
    if (found != lookTextures_.end()) return found->second;
    const luna::engine::Image sheet = composeLook(*layerSheets_, look);
    const luna::engine::Texture texture = renderer.createTexture(sheet);
    silhouettes_[texture.id] = renderer.createTexture(luna::engine::silhouette(sheet));
    lookTextures_.emplace(look, texture);
    return texture;
}

void OdysseyGame::drawShadows(luna::engine::Renderer& renderer, const luna::engine::Rect& view, double alpha) {
    const ShadowCast cast = shadowCast(alpha);
    // The fires that may shade things tonight; the Low lighting preset turns fire shadows off (US-245). Fires too far off the picture to reach into it are left out.
    fireShadows_.lights.clear();
    fireShadows_.darkness = darknessOf(ambientLightFrame(alpha));
    if (settings_.lighting != "Low" && lighting_.shadowLightsPerObject > 0 && fireShadows_.darkness > 0.0) {
        for (const LightSource& light : lightSources(alpha)) {
            if (!light.kind->shadows) continue;
            const double reach = light.kind->radiusTiles * kTileSize;
            if (light.x < view.x - reach || light.x > view.x + view.width + reach || light.groundY < view.y - reach || light.groundY > view.y + view.height + reach) continue;
            fireShadows_.lights.push_back(light);
        }
    }
    if (!cast.on && fireShadows_.lights.empty()) return;
    const auto screenX = [&view](double worldX) { return static_cast<int>(std::lround(worldX)) - view.x; };
    const auto screenY = [&view](double worldY) { return static_cast<int>(std::lround(worldY)) - view.y; };
    const auto heightOf = [this](const std::string& kind, bool& casts) {
        const CharacterKindDef* def = definitions_.character(kind);
        casts = def == nullptr || def->shadow;
        return def != nullptr ? def->height : 1.7;
    };
    const auto isOnScreen = [&](double worldX, double worldY) {
        const double margin = 4.0 * kTileSize; // a long shadow can reach in from outside the picture
        return worldX > view.x - margin && worldX < view.x + view.width + margin && worldY > view.y - margin && worldY < view.y + view.height + margin;
    };

    // Plants and objects.
    for (const WorldPlant& plant : plants_) {
        if (!plant.present() || plant.def == nullptr || !plant.def->shadow || plant.def->height <= 0.0) continue;
        if (!isOnScreen(plant.feet.x, plant.feet.y)) continue;
        const auto source = plantArt_.sources.find(plant.kind);
        if (source == plantArt_.sources.end()) continue;
        const auto page = plantArt_.pages.find(source->second.page);
        if (page == plantArt_.pages.end()) continue;
        castShadow(renderer, cast, page->second, source->second.rect, screenX(plant.feet.x), screenY(plant.feet.y), plant.def->height, plant.feet.x, plant.feet.y);
    }
    // The clan.
    if (clan_) {
        bool casts = true;
        const double personHeight = heightOf("hero", casts);
        for (const Figure& figure : clanView_.figures()) {
            if (!casts || !figure.present || !isOnScreen(figure.feetX(alpha), figure.feetY(alpha))) continue;
            const luna::engine::Texture texture = lookTexture(renderer, figure.look);
            const luna::engine::Rect source{figure.animationFrame() * kCharacterWidth, static_cast<int>(figure.facing) * kCharacterHeight, kCharacterWidth, kCharacterHeight};
            castShadow(renderer, cast, texture, source, screenX(figure.feetX(alpha)), screenY(figure.feetY(alpha)), personHeight * (figure.child ? 0.75 : 1.0), figure.feetX(alpha), figure.feetY(alpha));
        }
    }
    // Placed characters who are not enemies, and enemies.
    const auto character = [&](const std::string& kindName, const std::string& frames, int directions, Facing facing, double feetX, double feetY) {
        bool casts = true;
        const double height = heightOf(kindName, casts);
        if (!casts || !isOnScreen(feetX, feetY)) return;
        const CharacterKindDef* kind = definitions_.character(kindName);
        if (kind != nullptr && kind->animal) {
            const auto picture = animalArt_.sources.find(kindName);
            if (picture == animalArt_.sources.end()) return;
            luna::engine::Rect source = picture->second;
            const bool west = facesWest(facing);
            if (west) source.x = animalArt_.pageWidth - source.x - source.width;
            castShadow(renderer, cast, west ? animalArt_.mirrored : animalArt_.page, source, screenX(feetX), screenY(feetY), height, feetX, feetY);
            return;
        }
        castShadow(renderer, cast, charactersAtlas_, art_.frame(frames, directions, facing, 0), screenX(feetX), screenY(feetY), height, feetX, feetY);
    };
    for (const PlacedCharacter& placed : bystanders_) {
        if (const CharacterKindDef* kind = definitions_.character(placed.kind)) character(kind->name, kind->frames, kind->directions, placed.facing, placed.feet.x, placed.feet.y);
    }
    for (const Enemy& enemy : enemies_) {
        if (enemy.isAlive()) character(enemy.kindName, enemy.frames, enemy.directions, enemy.facing, enemy.feetX(), enemy.feetY());
    }
    // The hero.
    bool casts = true;
    const double heroHeight = heightOf("hero", casts);
    if (casts) castShadow(renderer, cast, characters_, hero_.spriteFrame(), screenX(hero_.feetX(alpha)), screenY(hero_.feetY(alpha)), heroHeight, hero_.feetX(alpha), hero_.feetY(alpha));
}

} // namespace odysseus::game
