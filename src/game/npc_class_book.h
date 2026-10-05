#pragma once

#include "boundary.h"

#include "game/level.h"
#include "sim/npc_class.h"
#include "sim/npc_kind.h"

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace odysseus::game {

// The NPC Classes of the game (US-260): the catalog read from assets/data/npc-classes/ and the owner's changes to it (create, edit, delete),
// each written to its own file at once. The Editor works on it; the game reads it.
// The layer a placed NPC adds over its kind: only the fields it sets itself.
sim::rules::NpcLayer placedLayer(const PlacedCharacter& placed);

class NpcClassBook {
public:
    // The folder of the classes (assets/data/npc-classes) and, when given, the folder of the kind files (assets/data/npcs).
    explicit NpcClassBook(std::filesystem::path folder, std::filesystem::path kindsFolder = {});

    const sim::rules::NpcClassCatalog& catalog() const { return catalog_; }
    const sim::rules::NpcKindCatalog& kinds() const { return kinds_; }
    // What a placed NPC is after its classes, its kind file and its own fields are applied (US-261).
    sim::rules::ResolvedNpc resolve(const PlacedCharacter& placed) const;
    // The default actions with each partner type (US-293): the defaults-<type>.json files, the lowest layer of what an NPC does with that kind of partner.
    void setPartnerDefaults(sim::rules::PartnerDefaults defaults) { partnerDefaults_ = std::move(defaults); }
    const sim::rules::PartnerDefaults& partnerDefaults() const { return partnerDefaults_; }
    const sim::rules::LoadReport& report() const { return report_; }
    const std::filesystem::path& folder() const { return folder_; }

    // Told after the book writes a class or kind file itself, so the file watcher does not read it a second time (US-304).
    void setWroteFile(std::function<void(const std::filesystem::path&)> wrote) { wrote_ = std::move(wrote); }

    // Reads every file again (F5). All or nothing: when any file has a mistake the classes in use stay and false comes back (the report names the mistakes).
    bool reload();

    // Writes the class to <folder>/<id>.json and reads the folder again. Returns the problem, or nullopt when it is saved.
    std::optional<std::string> save(const sim::rules::NpcClass& npcClass);

    // Writes the kind file <kindsFolder>/<kind>.json (US-269) and reads the kind files again. Returns the problem, or nullopt when it is saved.
    std::optional<std::string> saveKind(const sim::rules::NpcKind& kind);

    // Names of the placed NPCs of the level that have this class.
    static std::vector<std::string> usersOf(const std::string& id, const Level& level);
    // Deletes the file of the class. Refused (returns the reason, naming every NPC that uses it) while placed NPCs have it.
    std::optional<std::string> remove(const std::string& id, const Level& level);

private:
    std::filesystem::path folder_;
    std::function<void(const std::filesystem::path&)> wrote_;
    std::filesystem::path kindsFolder_;
    sim::rules::NpcClassCatalog catalog_;
    sim::rules::NpcKindCatalog kinds_;
    sim::rules::LoadReport report_;
    sim::rules::PartnerDefaults partnerDefaults_;
};

} // namespace odysseus::game
