#include "region_edits.h"

#include <format>
#include <string>

namespace odysseus::sim {

namespace {

const char* biomeWord(Biome biome) {
    switch (biome) {
    case Biome::Steppe: return "steppe";
    case Biome::Forest: return "forest";
    case Biome::Water: return "water";
    case Biome::Mountain: return "mountain";
    case Biome::Cave: return "a cave mouth";
    }
    return "?";
}

bool inside(const Region& land, int x, int y) { return x >= 0 && y >= 0 && x < land.size() && y < land.size(); }

bool seedHasResource(Region& land, int x, int y) {
    const Tile chunk = land.chunkOf(x, y);
    for (const Resource& resource : land.chunk(chunk.x, chunk.y).resources) {
        if (resource.x == x && resource.y == y) return true;
    }
    return false;
}

} // namespace

Biome effectiveBiome(Region& land, const RegionEdits& edits, int x, int y) {
    Biome biome = land.biomeAt(x, y);
    for (const TileEdit& edit : edits.tiles) { // the last edit of a tile wins
        if (edit.x == x && edit.y == y) biome = edit.biome;
    }
    return biome;
}

std::vector<EditConflict> findConflicts(Region& land, const RegionEdits& edits) {
    std::vector<EditConflict> conflicts;
    for (const PlacedEdit& entry : edits.placed) {
        if (!inside(land, entry.x, entry.y)) {
            conflicts.push_back({entry.id, std::format("({}, {}) is outside the region", entry.x, entry.y)});
            continue;
        }
        if (entry.removal) {
            if (!seedHasResource(land, entry.x, entry.y)) conflicts.push_back({entry.id, std::format("nothing is left at ({}, {}) to take away", entry.x, entry.y)});
            continue;
        }
        const Biome biome = effectiveBiome(land, edits, entry.x, entry.y);
        if (!walkable(biome)) {
            conflicts.push_back({entry.id, std::format("{} '{}' at ({}, {}) now stands on {}", entry.group == EditGroup::Camp ? "camp" : "entry", entry.kind, entry.x, entry.y, biomeWord(biome))});
        } else if (entry.group == EditGroup::Camp && !entry.forced && !land.goodSite({entry.x, entry.y})) {
            conflicts.push_back({entry.id, std::format("camp '{}' at ({}, {}) no longer has water and food within reach", entry.kind, entry.x, entry.y)});
        }
    }
    return conflicts;
}

std::vector<std::string> landWarnings(Region& land) {
    std::vector<std::string> warnings;
    const Tile start = land.start();
    if (!walkable(land.biomeAt(start.x, start.y))) {
        warnings.push_back(std::format("the start ({}, {}) is now on {}", start.x, start.y, biomeWord(land.biomeAt(start.x, start.y))));
    } else if (!land.goodSite(start)) {
        warnings.push_back(std::format("the start ({}, {}) no longer has water and food within reach", start.x, start.y));
    }
    for (const auto& [tile, biome] : land.tileEditList()) {
        if (biome != Biome::Cave) continue;
        bool open = false; // a cave mouth is entered from a walkable tile next to it
        for (const Tile next : {Tile{tile.x + 1, tile.y}, Tile{tile.x - 1, tile.y}, Tile{tile.x, tile.y + 1}, Tile{tile.x, tile.y - 1}}) {
            const Biome there = land.biomeAt(next.x, next.y);
            if (walkable(there) && there != Biome::Cave) open = true;
        }
        if (!open) warnings.push_back(std::format("the cave mouth at ({}, {}) has no walkable tile next to it", tile.x, tile.y));
    }
    return warnings;
}

} // namespace odysseus::sim
