#pragma once

#include "boundary.h"

#include "data.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>

namespace odysseus::sim {

// Helpers for reading content files. Simulation-internal: other layers never see JSON.

// Reads and parses a JSON file; a missing file or a syntax error becomes a DataError.
nlohmann::json readJsonFile(const std::filesystem::path& file);

// Reads a whole number field and checks its range.
int requireInt(const nlohmann::json& object, const std::filesystem::path& file, const std::string& field, int minimum,
               int maximum);

// The same for a field inside a named section of the file; errors say "section.field".
int requireInt(const nlohmann::json& object, const std::filesystem::path& file, const std::string& section, const std::string& field,
               int minimum, int maximum);

} // namespace odysseus::sim
