#pragma once

#include "boundary.h"

#include "region.h"

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace odysseus::sim {

// Saving the region (US-043, ADR-010): the seed and the chunks that changed, nothing else. Every other tile and resource
// is made again from the seed when the game loads, so a save stays small however large the land is. The file is versioned
// JSON written safely (a temporary file, three backups), and loading falls back to the newest intact backup.
inline constexpr int kRegionSaveVersion = 1;

void saveRegion(const Region& region, const std::filesystem::path& file);

struct LoadedRegion {
    Region region;
    std::filesystem::path loadedFrom;
    std::vector<std::string> notes; // which files were skipped, and why
};

// `config` is today's content (assets/data/sim/region.json); the save keeps only the seed, so a changed region.json
// would change the land: the save also stores the size and chunk size and refuses to load on a mismatch.
// `makeBase`, when given, makes the region the changes are laid onto (a world file's land with its edits, US-207) instead of the plain land of the seed; its seed must be the saved one.
LoadedRegion loadRegion(const std::filesystem::path& file, const RegionConfig& config, const std::function<Region()>& makeBase = {});

// The number of chunks a save of this region would store (the changed ones).
std::size_t savedChunkCount(const Region& region);

} // namespace odysseus::sim
