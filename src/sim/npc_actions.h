#pragma once

#include "boundary.h"

#include "sim/npc_events.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace odysseus::sim {

// Where an action an NPC may do comes from (US-291, D-54 Q11): its class, its own custom actions, or a world event. There is no quest source: M10 adds one (a new ActionSource)
// and changes the action schema.
enum class ActionOrigin { Class, Custom, Event, Default };

// An interaction an NPC may choose to do, and where it came from. A candidate from an event knows where the event is.
struct ActionCandidate {
    std::string interaction;
    ActionOrigin origin = ActionOrigin::Class;
    int bonus = 0;             // added to the score of the interaction's own `npc` block
    bool atEvent = false;
    int eventX = 0;
    int eventY = 0;
    std::string eventTrigger; // the world event it answers
};

// Everything the director knows about what an NPC is, shared by the persons that are the same (interned): its classes and tags, the actions of its classes and its own, and
// the allow and deny lists and the default actions for each partner type that the resolving of its layers gave it.
struct NpcProfile {
    std::vector<std::string> classes;
    std::vector<std::string> tags;           // the resolved tags, with "npc"
    std::vector<std::string> classActions;   // `does` of its classes
    std::vector<std::string> customActions;  // `does` of its kind and of itself
    std::vector<std::string> allow;
    std::vector<std::string> deny;
    std::map<std::string, std::vector<std::string>> partnerActions; // partner type -> the interactions it prefers when that partner is the target (US-293)
    bool empty() const { return classes.empty() && classActions.empty() && customActions.empty() && partnerActions.empty(); }
    friend bool operator==(const NpcProfile&, const NpcProfile&) = default;
};

// What a source is shown when it is asked for candidates.
struct SourceContext {
    const NpcProfile& profile;
    int x = 0; // where the NPC is, world pixels
    int y = 0;
    std::uint64_t tick = 0;
    const EventBoard* board = nullptr;
    const EventCatalog* events = nullptr;
};

// A strategy object: one place actions come from. The chooser asks every source in turn; adding a source (the quests of M10) is adding one class and one line, nothing else changes.
class ActionSource {
public:
    virtual ~ActionSource() = default;
    virtual void collect(const SourceContext& context, std::vector<ActionCandidate>& out) const = 0;
};

class ClassActionSource final : public ActionSource {
public:
    void collect(const SourceContext& context, std::vector<ActionCandidate>& out) const override;
};

class CustomActionSource final : public ActionSource {
public:
    void collect(const SourceContext& context, std::vector<ActionCandidate>& out) const override;
};

class EventActionSource final : public ActionSource {
public:
    void collect(const SourceContext& context, std::vector<ActionCandidate>& out) const override;
};

// The default actions of the partner types an NPC meets outside other persons (US-293, D-54 Q14, Q16): what the environment offers (forage, rest, fish, pray) and what an NPC does
// with animals (a hunter hunts), from the `partnerActions` of its layers and the defaults-<type>.json files. The chooser adds the preference bonus of the config to these.
class DefaultActionSource final : public ActionSource {
public:
    void collect(const SourceContext& context, std::vector<ActionCandidate>& out) const override;
};

// The sources in the order they are asked. The standard set is the class, custom, event and default sources.
class ActionSources {
public:
    static ActionSources standard();
    void add(std::unique_ptr<ActionSource> source) { sources_.push_back(std::move(source)); }
    std::size_t count() const { return sources_.size(); }
    std::vector<ActionCandidate> collect(const SourceContext& context) const;

private:
    std::vector<std::unique_ptr<ActionSource>> sources_;
};

} // namespace odysseus::sim
