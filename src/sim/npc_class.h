#pragma once

#include "boundary.h"

#include "sim/interaction.h"
#include "sim/npc_extras.h"
#include "sim/partner_types.h"

#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace odysseus::sim::rules {

// An NPC Class (US-260, D-52): what kind of person someone is, owner-defined in the Editor. One file per class,
// assets/data/npc-classes/<id>.json (docs/guides/npc-data.md). An NPC has one or more classes.
struct NpcClass {
    std::string id;     // also the file name
    std::string label;  // "Trader"
    int colour = 0xFFFFFF; // 0xRRGGBB: the ring round the marker of an NPC of this class
    std::string icon;   // one of npcIconNames()
    std::vector<std::string> tags; // the tags every NPC of this class carries
    // Default dialogue file per partner type, in file order: "player", "animal", "environment" or "class:<id>".
    std::vector<std::pair<std::string, std::string>> dialogues;
    std::vector<std::string> allow; // interaction ids this class may do
    std::vector<std::string> deny;  // interaction ids this class may never do (a deny always wins)
    NpcExtras extras;               // what the class trades, and later its schedule and actions (M9b, M9c): the defaults of every NPC of this class
    std::string file;               // "npc-classes/trader.json"
    friend bool operator==(const NpcClass&, const NpcClass&) = default;
};

// The built-in icon set a class picks from.
const std::vector<std::string>& npcIconNames();


// "#d9a441" -> 0xD9A441; nullopt when it is not # and six hex digits. And back.
std::optional<int> parseColour(const std::string& text);
std::string formatColour(int rgb);

class NpcClassCatalog {
public:
    // Reads every *.json of the folder. Each file stands alone: one with mistakes is left out and listed in the report
    // as "file:line: message"; the others load. A missing folder is not an error (no classes).
    static NpcClassCatalog load(const std::filesystem::path& folder, LoadReport& report);
    // One class from text; `name` is how errors name the file, `expectedId` the stem of the file ("" = no check).
    static std::optional<NpcClass> parse(std::string_view text, const std::string& name, LoadReport& report, const std::string& expectedId = {});

    const std::vector<NpcClass>& all() const { return classes_; }
    const NpcClass* find(std::string_view id) const;
    std::vector<std::string> ids() const;

private:
    std::vector<NpcClass> classes_; // sorted by id
};

// The class as canonical JSON text in the field order of the guide (the Editor saves it); parse gives the same class back.
std::string toJson(const NpcClass& npcClass);

} // namespace odysseus::sim::rules
