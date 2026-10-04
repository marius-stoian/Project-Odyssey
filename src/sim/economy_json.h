#pragma once

#include "boundary.h"

#include "sim/economy.h"

#include <nlohmann/json_fwd.hpp>

#include <filesystem>
#include <string>

namespace odysseus::sim {

// Level files (docs/guides/npc-data.md): {"currencies": {...}, "prices": {...}, "resources": {...}}, every table optional. A mistake is a DataError naming file and field.
// Kept apart from economy.h so that only the files that read JSON need the JSON library.
RegionEconomy economyFromJson(const nlohmann::json& value, const std::filesystem::path& file, const std::string& where);
nlohmann::json economyToJson(const RegionEconomy& economy);

} // namespace odysseus::sim
