#pragma once

#include "boundary.h"

#include "sim/interaction.h"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace odysseus::sim::rules {

// The kinds of partner an NPC deals with (US-293, D-54 Q14): the player, animals, the environment, and one `class:<id>` for every NPC class. The list comes from
// assets/data/sim/partner-types.json so the owner can add more ("buildings"); `class:<id>` is always valid. The game reads the file at start and tells the data readers.
const std::vector<std::string>& partnerTypeNames(); // the types of the file, in file order (the three built-in ones until a file is read)
void setPartnerTypes(const std::vector<std::string>& types); // an empty list gives the three built-in types again
bool validPartnerType(const std::string& type);               // a registered type, or class:<id>

// Reads partner-types.json: { "types": ["player", "animal", "environment", "buildings"] }. The three built-in types are always included. A mistake is a DataError.
std::vector<std::string> loadPartnerTypes(const std::filesystem::path& file);

// What a partner type gives every NPC by default (US-293, D-54 Q14): assets/data/interactions/defaults-<type>.json, one file per type (class, animal, environment): { "partnerType":
// "animal", "actions": ["hunt"] }. `class` is the default for meeting any NPC class. A class, a kind or an NPC overrides the actions of a type with its own `partnerActions`.
struct PartnerDefaults {
    std::map<std::string, std::vector<std::string>> actions; // partner type -> interaction ids
    const std::vector<std::string>* find(const std::string& type) const {
        const auto found = actions.find(type);
        return found == actions.end() ? nullptr : &found->second;
    }
};

// Reads every defaults-*.json of the folder. Each file stands alone: one with mistakes is left out and reported as "file:line: message".
PartnerDefaults loadPartnerDefaults(const std::filesystem::path& folder, LoadReport& report);

} // namespace odysseus::sim::rules
