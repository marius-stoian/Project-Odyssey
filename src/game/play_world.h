#pragma once

#include "boundary.h"

#include <filesystem>
#include <string>
#include <vector>

namespace odysseus::game {

// Where the world files live (US-207, D-68): assets/worlds, next to assets/data. A world file is `<name>.json`; its name is what New Game offers, what a run save records and what
// the Region view saves under.
std::filesystem::path worldsFolder(const std::filesystem::path& dataDirectory);
std::filesystem::path worldFilePath(const std::filesystem::path& dataDirectory, const std::string& name);
// The names of the world files there are, in alphabetical order (the .json files of the folder, without the ending). Backups (`.bak1`...) and other files are not listed.
std::vector<std::string> worldNames(const std::filesystem::path& dataDirectory);

} // namespace odysseus::game
