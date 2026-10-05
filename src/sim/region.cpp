#include "region.h"

#include "json_data.h"

#include <algorithm>
#include <cstdlib>
#include <queue>
#include <stdexcept>

namespace odysseus::sim {

bool walkable(Biome biome) { return biome == Biome::Steppe || biome == Biome::Forest || biome == Biome::Cave; }

RegionConfig loadRegionConfig(const std::filesystem::path& file) {
    const nlohmann::json json = readJsonFile(file);
    RegionConfig config;
    config.size = requireInt(json, file, "size", 64, 1024);
    config.chunkSize = requireInt(json, file, "chunkSize", 8, 128);
    if (config.size % config.chunkSize != 0) throw DataError(file, "size", "must be a multiple of chunkSize");
    config.lakeLevel = requireInt(json, file, "lakeLevel", 0, 1023);
    config.mountainLevel = requireInt(json, file, "mountainLevel", config.lakeLevel + 100, 1023);
    config.riverBand = requireInt(json, file, "riverBand", 0, 100);
    config.forestMoisture = requireInt(json, file, "forestMoisture", 0, 1023);
    config.caveNoise = requireInt(json, file, "caveNoise", 0, 1023);
    config.edgeWall = requireInt(json, file, "edgeWall", 0, 32);
    config.flintPerMille = requireInt(json, file, "flintPerMille", 0, 1000);
    config.woodPerMille = requireInt(json, file, "woodPerMille", 0, 1000);
    config.berriesPerMille = requireInt(json, file, "berriesPerMille", 0, 1000);
    config.herdPerMille = requireInt(json, file, "herdPerMille", 0, 1000);
    config.herdMinimum = requireInt(json, file, "herdMinimum", 1, 100);
    config.herdMaximum = requireInt(json, file, "herdMaximum", config.herdMinimum, 100);
    config.berryRegrowDays = requireInt(json, file, "berryRegrowDays", 1, 365);
    config.startNeedWithin = requireInt(json, file, "startNeedWithin", 5, 60);
    return config;
}

Region::Region(std::uint64_t seed, RegionConfig config) : seed_(seed), config_(config) {
    start_ = findStart();
}

std::uint32_t Region::hash(int x, int y, std::uint32_t salt) const {
    std::uint64_t h = seed_ ^ (static_cast<std::uint64_t>(salt) * 0x9E3779B97F4A7C15ULL) ^ (static_cast<std::uint64_t>(static_cast<std::uint32_t>(x)) * 0xBF58476D1CE4E5B9ULL) ^
                      (static_cast<std::uint64_t>(static_cast<std::uint32_t>(y)) * 0x94D049BB133111EBULL);
    h ^= h >> 30;
    h *= 0xBF58476D1CE4E5B9ULL;
    h ^= h >> 27;
    h *= 0x94D049BB133111EBULL;
    h ^= h >> 31;
    return static_cast<std::uint32_t>(h >> 16);
}

// Value noise in whole numbers: random heights on a lattice of `cell` tiles, blended with a smooth step. 0..1023.
int Region::noise(int x, int y, int cell, std::uint32_t salt) const {
    const int x0 = x / cell;
    const int y0 = y / cell;
    const std::int64_t fx = (static_cast<std::int64_t>(x - x0 * cell) * 1024) / cell;
    const std::int64_t fy = (static_cast<std::int64_t>(y - y0 * cell) * 1024) / cell;
    const std::int64_t sx = fx * fx * (3 * 1024 - 2 * fx) / (1024 * 1024);
    const std::int64_t sy = fy * fy * (3 * 1024 - 2 * fy) / (1024 * 1024);
    const auto corner = [&](int cx, int cy) { return static_cast<std::int64_t>(hash(cx, cy, salt) & 1023U); };
    const std::int64_t a = corner(x0, y0) + (corner(x0 + 1, y0) - corner(x0, y0)) * sx / 1024;
    const std::int64_t b = corner(x0, y0 + 1) + (corner(x0 + 1, y0 + 1) - corner(x0, y0 + 1)) * sx / 1024;
    return static_cast<int>(std::clamp<std::int64_t>(a + (b - a) * sy / 1024, 0, 1023));
}

int Region::elevationAt(int x, int y) const {
    int e = (noise(x, y, 64, 1) * 4 + noise(x, y, 32, 2) * 2 + noise(x, y, 16, 3)) / 7;
    const int edge = std::min(std::min(x, y), std::min(config_.size - 1 - x, config_.size - 1 - y));
    if (edge < config_.edgeWall) e += (config_.edgeWall - edge) * 70; // the rim of the world rises into mountains
    return std::min(e, 1023);
}

int Region::moistureAt(int x, int y) const { return (noise(x, y, 48, 11) * 2 + noise(x, y, 24, 12)) / 3; }

Biome Region::generatedBiome(int x, int y) const {
    const int e = elevationAt(x, y);
    if (e < config_.lakeLevel) return Biome::Water;
    if (e > config_.mountainLevel) return noise(x, y, 8, 31) > config_.caveNoise ? Biome::Cave : Biome::Mountain;
    if (std::abs(noise(x, y, 96, 21) - 512) < config_.riverBand && e < config_.mountainLevel - 50) return Biome::Water; // a river
    if (moistureAt(x, y) > config_.forestMoisture && e > 200 && e < 780) return Biome::Forest;
    return Biome::Steppe;
}

Biome Region::biomeAt(int x, int y) const {
    if (x < 0 || y < 0 || x >= config_.size || y >= config_.size) return Biome::Mountain;
    if (!overrides_.empty()) {
        const auto found = overrides_.find(key(x, y));
        if (found != overrides_.end()) return found->second;
    }
    return generatedBiome(x, y);
}

std::optional<Resource> Region::resourceAt(int x, int y) const {
    const Biome here = biomeAt(x, y);
    const int first = static_cast<int>(hash(x, y, 77) % 1000U);
    const int second = static_cast<int>(hash(x, y, 78) % 1000U);
    const auto near = [&](Biome wanted, int reach) {
        for (int dy = -reach; dy <= reach; ++dy)
            for (int dx = -reach; dx <= reach; ++dx)
                if (biomeAt(x + dx, y + dy) == wanted) return true;
        return false;
    };
    switch (here) {
    case Biome::Forest: {
        const bool edge = biomeAt(x - 1, y) == Biome::Steppe || biomeAt(x + 1, y) == Biome::Steppe || biomeAt(x, y - 1) == Biome::Steppe || biomeAt(x, y + 1) == Biome::Steppe;
        if (edge && second < config_.berriesPerMille) return Resource{ResourceKind::Berries, x, y, 1, -1};   // berries on the forest's edge
        if (first < config_.woodPerMille) return Resource{ResourceKind::Wood, x, y, 1, -1};                   // trees inside it
        return std::nullopt;
    }
    case Biome::Steppe:
        if (first < config_.herdPerMille) {
            const int span = config_.herdMaximum - config_.herdMinimum + 1;
            return Resource{ResourceKind::Herd, x, y, config_.herdMinimum + static_cast<int>(hash(x, y, 79) % static_cast<std::uint32_t>(span)), -1};
        }
        if (second < config_.flintPerMille && (near(Biome::Water, 2) || near(Biome::Cave, 2))) return Resource{ResourceKind::Flint, x, y, 1, -1};
        return std::nullopt;
    case Biome::Mountain:
        if (second < config_.flintPerMille * 2 && near(Biome::Cave, 2)) return Resource{ResourceKind::Flint, x, y, 1, -1};
        return std::nullopt;
    default: return std::nullopt;
    }
}

const Chunk& Region::chunk(int cx, int cy) {
    if (cx < 0 || cy < 0 || cx >= chunksPerSide() || cy >= chunksPerSide()) throw std::out_of_range("chunk outside the region");
    const auto found = chunks_.find(key(cx, cy));
    if (found != chunks_.end()) return found->second;
    Chunk made;
    made.cx = cx;
    made.cy = cy;
    const int n = config_.chunkSize;
    made.biomes.reserve(static_cast<std::size_t>(n) * static_cast<std::size_t>(n));
    for (int y = cy * n; y < (cy + 1) * n; ++y) {
        for (int x = cx * n; x < (cx + 1) * n; ++x) {
            made.biomes.push_back(biomeAt(x, y));
            if (const auto resource = resourceAt(x, y)) made.resources.push_back(*resource);
        }
    }
    for (const Resource& extra : extraResources_) {
        if (extra.x / n == cx && extra.y / n == cy) made.resources.push_back(extra);
    }
    return chunks_.emplace(key(cx, cy), std::move(made)).first->second;
}

std::vector<Resource> Region::resourcesNear(Tile around, int radius) {
    std::vector<Resource> found;
    const int n = config_.chunkSize;
    const int firstX = std::max(0, (around.x - radius) / n);
    const int lastX = std::min(chunksPerSide() - 1, (around.x + radius) / n);
    const int firstY = std::max(0, (around.y - radius) / n);
    const int lastY = std::min(chunksPerSide() - 1, (around.y + radius) / n);
    for (int cy = firstY; cy <= lastY; ++cy) {
        for (int cx = firstX; cx <= lastX; ++cx) {
            for (const Resource& resource : chunk(cx, cy).resources) {
                const int dx = resource.x - around.x;
                const int dy = resource.y - around.y;
                if (dx * dx + dy * dy <= radius * radius) found.push_back(resource);
            }
        }
    }
    std::sort(found.begin(), found.end(), [&](const Resource& a, const Resource& b) {
        const int da = (a.x - around.x) * (a.x - around.x) + (a.y - around.y) * (a.y - around.y);
        const int db = (b.x - around.x) * (b.x - around.x) + (b.y - around.y) * (b.y - around.y);
        return da != db ? da < db : (a.y != b.y ? a.y < b.y : a.x < b.x);
    });
    return found;
}

bool Region::available(const Resource& resource, const Date& today) const {
    if (resource.takenDay < 0) return true;
    if (resource.kind != ResourceKind::Berries) return false; // trees, flint and herds are taken for good
    const bool bearingSeason = today.season == Season::Summer || today.season == Season::Autumn;
    return bearingSeason && today.day - resource.takenDay >= config_.berryRegrowDays;
}

bool Region::harvest(int x, int y, const Date& today) {
    if (x < 0 || y < 0 || x >= config_.size || y >= config_.size) return false;
    const Tile where = chunkOf(x, y);
    chunk(where.x, where.y);
    Chunk& owner = chunks_.at(key(where.x, where.y));
    for (Resource& resource : owner.resources) {
        if (resource.x == x && resource.y == y) {
            if (!available(resource, today)) return false;
            resource.takenDay = today.day;
            owner.changed = true;
            return true;
        }
    }
    return false;
}

void Region::restoreTaken(int x, int y, std::int64_t takenDay) {
    const Tile where = chunkOf(x, y);
    chunk(where.x, where.y);
    Chunk& owner = chunks_.at(key(where.x, where.y));
    for (Resource& resource : owner.resources) {
        if (resource.x == x && resource.y == y) {
            resource.takenDay = takenDay;
            owner.changed = true;
        }
    }
}

std::vector<const Chunk*> Region::changedChunks() const {
    std::vector<const Chunk*> changed;
    for (const auto& [id, chunk] : chunks_) {
        if (chunk.changed) changed.push_back(&chunk);
    }
    return changed;
}

std::uint64_t Region::fingerprint() {
    std::uint64_t h = 14695981039346656037ULL;
    const auto mix = [&h](std::uint64_t value) {
        h ^= value;
        h *= 1099511628211ULL;
    };
    for (int cy = 0; cy < chunksPerSide(); ++cy) {
        for (int cx = 0; cx < chunksPerSide(); ++cx) {
            const Chunk& made = chunk(cx, cy);
            for (const Biome biome : made.biomes) mix(static_cast<std::uint64_t>(biome));
            for (const Resource& resource : made.resources) {
                mix(static_cast<std::uint64_t>(resource.kind));
                mix(static_cast<std::uint64_t>(resource.x));
                mix(static_cast<std::uint64_t>(resource.y));
                mix(static_cast<std::uint64_t>(resource.amount));
            }
        }
    }
    return h;
}

// A start is good when the land around it has water and food close by and the people can walk to both.
bool Region::startIsGood(Tile tile) {
    const Biome here = biomeAt(tile.x, tile.y);
    if (here != Biome::Steppe && here != Biome::Forest) return false;
    const int reach = config_.startNeedWithin;
    // Water and food within reach (straight-line square), then a walk to both over walkable tiles.
    const int side = 2 * (reach + 5) + 1;
    std::vector<bool> seen(static_cast<std::size_t>(side) * static_cast<std::size_t>(side), false);
    const auto index = [&](int x, int y) { return static_cast<std::size_t>((y - tile.y + reach + 5) * side + (x - tile.x + reach + 5)); };
    bool water = false;
    bool food = false;
    std::queue<Tile> queue;
    queue.push(tile);
    seen[index(tile.x, tile.y)] = true;
    while (!queue.empty() && !(water && food)) {
        const Tile at = queue.front();
        queue.pop();
        const int dx = at.x - tile.x;
        const int dy = at.y - tile.y;
        if (dx * dx + dy * dy > reach * reach) continue;
        for (const Tile next : {Tile{at.x + 1, at.y}, Tile{at.x - 1, at.y}, Tile{at.x, at.y + 1}, Tile{at.x, at.y - 1}}) {
            if (std::abs(next.x - tile.x) > reach + 4 || std::abs(next.y - tile.y) > reach + 4) continue;
            const Biome biome = biomeAt(next.x, next.y);
            if (biome == Biome::Water) {
                water = true; // standing next to water on a walkable tile: one can drink
                continue;
            }
            if (!walkable(biome) || seen[index(next.x, next.y)]) continue;
            seen[index(next.x, next.y)] = true;
            queue.push(next);
        }
        if (!food) {
            const auto resource = resourceAt(at.x, at.y);
            for (const Resource& extra : extraResources_) {
                if (extra.x == at.x && extra.y == at.y && (extra.kind == ResourceKind::Berries || extra.kind == ResourceKind::Herd)) food = true;
            }
            if (resource && (resource->kind == ResourceKind::Berries || resource->kind == ResourceKind::Herd)) food = true;
        }
    }
    return water && food;
}

Tile Region::findStart() {
    const int centre = config_.size / 2;
    const int limit = config_.size / 2 - config_.edgeWall - config_.startNeedWithin - 6;
    for (int ring = 0; ring <= limit; ring += 4) {
        for (int dy = -ring; dy <= ring; dy += 4) {
            for (int dx = -ring; dx <= ring; dx += 4) {
                if (std::max(std::abs(dx), std::abs(dy)) != ring) continue;
                const Tile candidate{centre + dx, centre + dy};
                if (startIsGood(candidate)) return candidate;
            }
        }
    }
    carveStartArea({centre, centre}); // the land offered nothing good: make a small, fair start (rare)
    return {centre, centre};
}

void Region::carveStartArea(Tile centre) {
    const auto set = [&](int x, int y, Biome biome) { overrides_[key(x, y)] = biome; };
    for (int dy = -9; dy <= 9; ++dy)
        for (int dx = -9; dx <= 9; ++dx) set(centre.x + dx, centre.y + dy, Biome::Steppe);
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = 6; dx <= 8; ++dx) set(centre.x + dx, centre.y + dy, Biome::Water);        // a pond
    for (int dy = 3; dy <= 7; ++dy)
        for (int dx = -8; dx <= -4; ++dx) set(centre.x + dx, centre.y + dy, Biome::Forest);      // a wood
    extraResources_.push_back({ResourceKind::Berries, centre.x - 4, centre.y + 4, 1, -1});
    extraResources_.push_back({ResourceKind::Berries, centre.x - 4, centre.y + 6, 1, -1});
    extraResources_.push_back({ResourceKind::Herd, centre.x + 2, centre.y - 6, config_.herdMinimum, -1});
    extraResources_.push_back({ResourceKind::Flint, centre.x + 5, centre.y + 2, 1, -1});
}

} // namespace odysseus::sim
