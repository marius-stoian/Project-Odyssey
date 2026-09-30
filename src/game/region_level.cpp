#include "game/region_level.h"

#include <algorithm>
#include <format>

namespace odysseus::game {

namespace {

std::vector<std::string> plantNames(const Catalogs& catalogs, bool blocks, bool edible, const std::string& size) {
    std::vector<std::string> names;
    for (const PlantDef& plant : catalogs.plants) {
        if (plant.blocks == blocks && plant.edible == edible && plant.size == size) names.push_back(plant.name);
    }
    return names;
}

} // namespace

Level levelFromRegion(sim::Region& region, const Definitions& definitions, const Catalogs& catalogs) {
    const int grass = std::max(0, definitions.tileNumber("grass"));
    const int water = std::max(0, definitions.tileNumber("water"));
    const int stone = std::max(0, definitions.tileNumber("stone"));
    const int path = std::max(0, definitions.tileNumber("path"));
    Level level = makeLevel(std::format("Region {}", region.seed()), region.size(), region.size(), grass);
    for (int y = 0; y < region.size(); ++y) {
        for (int x = 0; x < region.size(); ++x) {
            switch (region.biomeAt(x, y)) {
            case sim::Biome::Water: level.set(x, y, water); break;
            case sim::Biome::Mountain: level.set(x, y, stone); break;
            case sim::Biome::Cave: level.set(x, y, path); break;
            default: break;
            }
        }
    }
    const std::vector<std::string> trees = plantNames(catalogs, true, false, "tree");
    std::vector<std::string> berries = plantNames(catalogs, false, true, "small");
    if (berries.size() > 6) berries.resize(6);
    std::string flintPlant = "moss";
    if (catalogs.plant(flintPlant) == nullptr && !catalogs.plants.empty()) flintPlant = catalogs.plants.front().name;
    std::string herdAnimal = "deer";
    if (definitions.character(herdAnimal) == nullptr) herdAnimal.clear();

    const auto footOf = [](int tile) { return tile * kTileSize + kTileSize / 2; };
    for (int cy = 0; cy < region.chunksPerSide(); ++cy) {
        for (int cx = 0; cx < region.chunksPerSide(); ++cx) {
            for (const sim::Resource& resource : region.chunk(cx, cy).resources) {
                const int id = level.nextId;
                const std::uint32_t pick = static_cast<std::uint32_t>(resource.x * 73856093 ^ resource.y * 19349663);
                if (resource.kind == sim::ResourceKind::Wood && !trees.empty()) {
                    level.plants.push_back({id, trees[pick % trees.size()], {footOf(resource.x), resource.y * kTileSize + kTileSize - 4}});
                    ++level.nextId;
                } else if (resource.kind == sim::ResourceKind::Berries && !berries.empty()) {
                    level.plants.push_back({id, berries[pick % berries.size()], {footOf(resource.x), resource.y * kTileSize + kTileSize - 4}});
                    ++level.nextId;
                } else if (resource.kind == sim::ResourceKind::Flint && !flintPlant.empty() && catalogs.plant(flintPlant) != nullptr) {
                    level.plants.push_back({id, flintPlant, {footOf(resource.x), resource.y * kTileSize + kTileSize - 4}});
                    ++level.nextId;
                } else if (resource.kind == sim::ResourceKind::Herd && !herdAnimal.empty()) {
                    const CharacterKindDef* kind = definitions.character(herdAnimal);
                    level.characters.push_back({id, herdAnimal, {footOf(resource.x), resource.y * kTileSize + kTileSize - 4}, (pick & 1U) != 0U ? Facing::East : Facing::West,
                                                herdAnimal, kind->hp, kind->swordDamage});
                    ++level.nextId;
                }
            }
        }
    }
    const sim::Tile start = region.start();
    level.heroStart = {footOf(start.x), footOf(start.y)};
    // The fire of the player's camp burns where the hero starts.
    if (definitions.hasLoopingEffect("flame")) level.effects.push_back({level.nextId++, "flame", level.heroStart});
    level.clan = true;
    return level;
}

} // namespace odysseus::game
