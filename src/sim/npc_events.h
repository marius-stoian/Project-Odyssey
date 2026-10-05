#pragma once

#include "boundary.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace odysseus::sim {

// World events that offer actions to the NPCs that match them (US-291, D-54 Q11): "a fire starts" offers "help put out the fire" to the villagers within 12 m. One entry of
// assets/data/sim/events.json says which event (`trigger`), which interaction it offers (`action`), to which classes (none listed: to everyone), how near (`withinMetres`),
// for how long it is on offer (`forMinutes` of game time) and how much it is wanted (`score` is added to the score of the interaction's own `npc` block).
struct EventDef {
    std::string id;
    std::string label;
    std::string trigger; // the name of the world event: "fire"
    std::string action;  // the interaction it offers
    std::vector<std::string> classes;
    int withinMetres = 12;
    int forMinutes = 30;
    int bonus = 30;
    friend bool operator==(const EventDef&, const EventDef&) = default;
};

struct EventCatalog {
    std::vector<EventDef> events;
    const EventDef* find(const std::string& id) const;
};

// Reads events.json: { "events": [ { "id", "label", "trigger", "action", "classes", "withinMetres", "forMinutes", "bonus" } ] }. A mistake is a DataError naming file and field.
EventCatalog loadEventCatalog(const std::filesystem::path& file);

// What has happened lately: an event that was posted at a spot and is on offer until a tick.
struct PostedEvent {
    std::string trigger;
    int x = 0; // world pixels
    int y = 0;
    std::uint64_t untilTick = 0;
    friend bool operator==(const PostedEvent&, const PostedEvent&) = default;
};

// An event on offer to a person: the definition and where it happens.
struct EventOffer {
    const EventDef* def = nullptr;
    int x = 0;
    int y = 0;
};

// The events on offer now, in the order they were posted. A board is small (a few events at a time) and is cleaned of what has expired every time it is looked at.
class EventBoard {
public:
    void post(const std::string& trigger, int x, int y, std::uint64_t untilTick) { posted_.push_back({trigger, x, y, untilTick}); }
    // The offers a person of these classes at (x, y) has at `tick`: every posted event whose definition names one of the classes (or none) and whose spot is within reach.
    std::vector<EventOffer> offersFor(const EventCatalog& catalog, const std::vector<std::string>& classes, int x, int y, std::uint64_t tick) const;
    void expire(std::uint64_t tick);
    const std::vector<PostedEvent>& posted() const { return posted_; }
    void clear() { posted_.clear(); }
    friend bool operator==(const EventBoard&, const EventBoard&) = default;

private:
    std::vector<PostedEvent> posted_;
};

} // namespace odysseus::sim
