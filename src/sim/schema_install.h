#pragma once

#include "boundary.h"

#include <filesystem>
#include <optional>
#include <string>

namespace odysseus::sim::schema {

// Reads `<dataRoot>/schemas` and installs it, so every data file is checked against its schema as it loads (US-190, ADR-020). A broken schema folder
// never stops the game: it returns what is wrong (and leaves nothing installed), and the caller logs it. Nothing when it went well. This small header
// is all a program needs; schema.h (with the JSON types) stays inside the Simulation and the Game.
std::optional<std::string> installFromFolder(const std::filesystem::path& dataRoot);

} // namespace odysseus::sim::schema
