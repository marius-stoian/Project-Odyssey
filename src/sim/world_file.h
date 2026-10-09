#pragma once

#include "boundary.h"

#include "region.h"
#include "region_edits.h"
#include "world_setup.h"

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace odysseus::sim {

// The world file (US-202, ADR-010, ADR-020, brief section 4.6, design M12 section 3): `assets/worlds/<name>.json`, the seed plus only what differs from it:
// the generator settings that differ from assets/data/sim/region.json, tiles painted another biome (in lists per chunk, tile coordinates inside the chunk,
// in tile order, so the file reads and diffs well), and the things, people, places, camps and resources put on the land or taken off it. The land itself is
// never stored: it is made again from the seed, and then the edits are laid over it.
inline constexpr int kWorldVersion = 1;
// A world file holds at most this many edits in all (design Q6; it becomes a Game Rules value when Game Rules reaches the world).
inline constexpr std::size_t kMaxWorldEntries = 20000;

struct WorldFile {
    int version = kWorldVersion;
    std::uint64_t seed = 1;
    std::vector<std::pair<std::string, int>> generator; // settings that differ from region.json, by their key in that file
    RegionEdits edits;
    WorldSetup setup; // the clans and people sections (US-206)
    friend bool operator==(const WorldFile&, const WorldFile&) = default;
};

// region.json with the world's own settings laid over it. A key that is not a generator setting is a DataError.
RegionConfig worldConfig(const WorldFile& world, RegionConfig base);
// The settings of `config` that differ from `base`, in the order of region.json.
std::vector<std::pair<std::string, int>> generatorDifferences(const RegionConfig& config, const RegionConfig& base);

// A region as the world file describes it: the seed and settings, then the painted tiles laid over the generated land.
Region makeWorldRegion(const WorldFile& world, const RegionConfig& base);

// Writes the file safely (a temporary file renamed over it, three backups kept). Tile edits are written in tile order and the lists in the order they have,
// so the same world always gives the same text. Throws DataError when it holds more than kMaxWorldEntries edits.
void saveWorld(const WorldFile& world, const std::filesystem::path& file, const RegionConfig& base);
// Reads and checks a world file. A newer version than this game reads, a missing field, an unknown biome or a tile outside its chunk is a DataError that
// names the file and the field.
WorldFile loadWorld(const std::filesystem::path& file, const RegionConfig& base);

// The text saveWorld writes (for tests and for the size check).
std::string worldText(const WorldFile& world, const RegionConfig& base);

} // namespace odysseus::sim
