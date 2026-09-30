#pragma once

#include "boundary.h"

#include "world.h"

#include <filesystem>
#include <string>
#include <vector>

namespace odysseus::sim {

// Saves (US-016, ADR-010): versioned JSON. Version 1 was the needs-only world of US-011;
// version 2 adds traits, skills, memories, opinions, kinship and the life events.
inline constexpr int kSaveVersion = 2;
inline constexpr int kSaveBackups = 3;

// Writes the whole world to `file`, safely: first to "<file>.tmp", then the old save moves to
// .bak1 (and .bak1 to .bak2, .bak2 to .bak3), and only then the new file takes its name. A crash
// at any moment leaves the previous complete save in place.
void saveWorld(const World& world, const std::filesystem::path& file);

struct LoadedWorld {
    World world;
    std::filesystem::path loadedFrom;  // the save, or the backup that had to be used
    std::vector<std::string> notes;    // what happened: upgrades, damaged files skipped
};

// Loads a save made by saveWorld. A damaged or half-written file is skipped for the newest
// backup that is intact; an older save version is upgraded step by step. If nothing can be
// loaded, a DataError explains why. `config` is today's content (assets/data).
LoadedWorld loadWorld(const std::filesystem::path& file, const SimConfig& config);

// The backup names, for tests and tools: "<file>.bak1" .. "<file>.bak3".
std::filesystem::path backupPath(const std::filesystem::path& file, int number);

} // namespace odysseus::sim
