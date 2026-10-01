#pragma once

#include "boundary.h"

#include "sim/rule_effect.h"
#include "sim/rule_expr.h"
#include "sim/rule_json.h"

#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace odysseus::sim::rules {

// One condition of an interaction and what to tell the player when it fails.
struct Requirement {
    std::string source; // "target.state == ripe"
    ExprPtr condition;
    std::string otherwise; // "Nothing to pick yet"
};

// How clan members and animals decide to do this on their own (US-154).
struct NpcRule {
    std::string scoreSource; // "need(hunger) * 2 + trait(diligent) * 10"
    ExprPtr score;
    int cooldownSeconds = 0;
};

// One interaction from assets/data/interactions/<id>.json (INT-01, INT-03; docs/guides/interaction-data.md).
// Numbers are whole: the range and the duration are kept in thousandths of a metre and of a second.
struct Interaction {
    std::string id;
    std::string label; // "Gather {target.name}"
    std::string note;  // the owner's own words; survives saves
    std::vector<std::string> actors;     // hero, person, animal, or the name of a kind
    std::vector<std::string> targetTags; // the target must have all of these
    std::vector<std::string> targetKinds; // when not empty, the target must be one of these kinds
    int rangeMilli = 1500;
    int durationMilli = 0;
    int order = 100; // in a menu, lower comes first
    std::vector<Requirement> requires_;
    std::vector<Effect> effects;
    std::optional<NpcRule> npc;
    std::optional<std::string> chronicle;
    std::string file; // "interactions/gather.json"
};

// What the matcher needs to know about the two things involved.
struct ThingInfo {
    std::string kind;                // "bush", "person", "deer"
    std::vector<std::string> tags;   // "edible", "plant", "hero", "animal"...
};

struct Offer {
    const Interaction* interaction = nullptr;
    bool enabled = true;
    std::string reason; // why it is disabled ("Too far away", "Nothing to pick yet")
};

struct LoadReport {
    std::vector<Diagnostic> errors;   // each one is "file:line: message"; a file with errors is not loaded
    std::vector<Diagnostic> warnings; // loaded, but probably a mistake (a tag nothing uses)
    int filesRead = 0;
    int loaded = 0;
};

struct LoadOptions {
    // The tags the catalogs use. When not empty, an interaction that targets any other tag gets a warning.
    std::set<std::string> knownTags;
    // Interaction ids that exist outside this folder (never needed yet; kept for tests).
    std::set<std::string> extraIds;
};

class InteractionRegistry {
public:
    // Reads every *.json in the folder (comments allowed). Each file stands alone: one with mistakes is left out and
    // listed in the report, the others load. The same text always loads the same way.
    static InteractionRegistry load(const std::filesystem::path& folder, LoadReport& report, const LoadOptions& options = {});

    // For tests and the Editor: one interaction from text. `name` is how errors name the file.
    static std::optional<Interaction> parse(std::string_view text, const std::string& name, LoadReport& report, const std::string& expectedId = {});

    const std::vector<Interaction>& all() const { return interactions_; }
    const Interaction* find(std::string_view id) const;

    // Everything this actor may do to this target, in menu order. Disabled ones come with their reason, so a menu
    // can show them greyed out. `distanceMilli` is the distance between the two in thousandths of a metre.
    std::vector<Offer> offered(const ThingInfo& actor, const ThingInfo& target, long long distanceMilli, const RuleContext& context) const;

private:
    std::vector<Interaction> interactions_; // sorted by order, then id
};

// The interaction as canonical JSON text, the form the Editor saves. Loading it again gives the same interaction.
std::string toJson(const Interaction& interaction);

// Replaces {target.name}-style tokens in `text` using the context (unknown tokens stay as written).
std::string fillTokens(const std::string& text, const RuleContext& context);

} // namespace odysseus::sim::rules
