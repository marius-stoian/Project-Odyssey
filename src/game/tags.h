#pragma once

#include "boundary.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace odysseus::game {

// Tags and states of catalog entries (US-151, INT-02). A tag says what a thing is ("edible", "plant", "hostile"); an interaction
// targets tags, not kinds. A state says what condition it is in ("ripe", "picked"); the first listed is the starting one.
// Both are optional in the JSON: when an entry has no "tags" (or "states"), the loader derives them from the entry's other
// fields, so the catalogs written before US-151 keep working. A written list replaces the derived one.

// The entry's "tags" list, or `derived` when it has none. Words of letters, digits, '-' and '_', no repeats; errors name file and field.
std::vector<std::string> readTags(const nlohmann::json& entry, const std::filesystem::path& file, const std::string& where, std::vector<std::string> derived);

// The same for "states".
std::vector<std::string> readStates(const nlohmann::json& entry, const std::filesystem::path& file, const std::string& where, std::vector<std::string> derived);

} // namespace odysseus::game
