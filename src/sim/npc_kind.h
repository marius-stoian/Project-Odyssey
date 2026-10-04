#pragma once

#include "boundary.h"

#include "sim/interaction.h"
#include "sim/npc_class.h"
#include "sim/npc_extras.h"
#include "sim/opinion.h"

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace odysseus::sim::rules {

// The nine attitude words (D-52). US-264 derives them from an integer opinion per pair; here they are only names.
const std::vector<std::string>& attitudeNames();
bool validAttitude(const std::string& word);

// What one layer says about an NPC (US-261): a kind file, or the fields a placed NPC sets itself. A field that is not set (an empty
// optional, an empty list) leaves the layer below it untouched.
struct NpcLayer {
    std::optional<std::vector<std::string>> classes; // set: replaces the classes of the layer below
    std::optional<std::string> attitude;             // set: replaces the attitude of the layer below
    std::vector<std::string> tags;                   // added to the tags of the layers below
    std::vector<std::pair<std::string, std::string>> dialogues; // partner type -> .dlg; replaces that partner type of the layers below
    std::vector<std::string> allow;
    std::vector<std::string> deny;
    NpcExtras extras;                                // trade now; schedule, actions and partner defaults later (M9b, M9c)
    friend bool operator==(const NpcLayer&, const NpcLayer&) = default;
};

// assets/data/npcs/<kind>.json: the defaults of every NPC of one kind (docs/guides/npc-data.md).
struct NpcKind {
    std::string kind; // also the file name; a name of characters.json or animals.json
    NpcLayer layer;
    std::string file; // "npcs/goblin.json"
    friend bool operator==(const NpcKind&, const NpcKind&) = default;
};

class NpcKindCatalog {
public:
    // Like the classes: each file stands alone, a mistake skips that file and is reported as "file:line: message"; a missing folder is no error.
    static NpcKindCatalog load(const std::filesystem::path& folder, LoadReport& report);
    static std::optional<NpcKind> parse(std::string_view text, const std::string& name, LoadReport& report, const std::string& expectedKind = {});

    const std::vector<NpcKind>& all() const { return kinds_; }
    const NpcKind* find(std::string_view kind) const;

private:
    std::vector<NpcKind> kinds_; // sorted by kind
};

// The kind as canonical JSON text in the field order of the guide; parse gives the same kind back.
std::string toJson(const NpcKind& kind);

// Whether an action is allowed for an NPC, after all layers.
enum class ActionState { Unset, Allowed, Denied };

// An NPC after every layer is applied: classes, then the kind, then the placed NPC (D-52).
struct ResolvedNpc {
    std::vector<std::string> classes;
    std::string attitude = "neutral";
    std::vector<std::string> tags;              // sorted, no duplicates
    std::map<std::string, std::string> dialogues; // partner type -> .dlg
    std::map<std::string, ActionState> actions;   // only ids some layer mentioned
    NpcExtras extras;                             // the merge of every layer: classes, then the kind, then the placed NPC
    std::vector<std::string> classActions;        // `does` of its classes (US-291)
    std::vector<std::string> customActions;       // `does` of its kind and of itself

    ActionState action(const std::string& id) const;
    bool denied(const std::string& id) const { return action(id) == ActionState::Denied; }
};

// Layers, lowest first: the default classes (the NpcClass of every class of the NPC, in order), the kind, the placed NPC. Per layer the allow list is
// read first, then the deny list, and a later layer overrides an earlier one: so a placed NPC that denies what its class allows is denied, and a deny
// inside one layer wins over an allow in the same layer. `kind` may be null (a kind without a file).
ResolvedNpc resolveNpc(const NpcClassCatalog& classes, const NpcLayer* kind, const NpcLayer& placed);

} // namespace odysseus::sim::rules
