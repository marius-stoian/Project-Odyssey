#include "sim/npc_events.h"

#include "sim/data.h"
#include "sim/economy.h"
#include "sim/json_data.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>

namespace odysseus::sim {

const EventDef* EventCatalog::find(const std::string& id) const {
    for (const EventDef& event : events) {
        if (event.id == id) return &event;
    }
    return nullptr;
}

EventCatalog loadEventCatalog(const std::filesystem::path& file) {
    using nlohmann::json;
    EventCatalog catalog;
    const json data = readJsonFile(file);
    if (requireInt(data, file, "version", 1, 1) != 1) throw DataError(file, "version", "must be 1");
    if (!data.contains("events") || !data.at("events").is_array()) throw DataError(file, "events", "must be a list");
    for (std::size_t i = 0; i < data.at("events").size(); ++i) {
        const json& entry = data.at("events").at(i);
        const std::string where = "events[" + std::to_string(i) + "]";
        if (!entry.is_object()) throw DataError(file, where, "must be an object");
        const auto word = [&](const char* field, bool required) {
            if (!entry.contains(field)) {
                if (required) throw DataError(file, where + "." + field, "is missing");
                return std::string();
            }
            if (!entry.at(field).is_string() || !validItemId(entry.at(field).get<std::string>())) throw DataError(file, where + "." + field, "must be a word (lower-case letters, digits and -)");
            return entry.at(field).get<std::string>();
        };
        EventDef def;
        def.id = word("id", true);
        def.trigger = word("trigger", true);
        def.action = word("action", true);
        if (entry.contains("label")) {
            if (!entry.at("label").is_string()) throw DataError(file, where + ".label", "must be text");
            def.label = entry.at("label").get<std::string>();
        }
        if (entry.contains("classes")) {
            if (!entry.at("classes").is_array()) throw DataError(file, where + ".classes", "must be a list of class ids");
            for (const json& id : entry.at("classes")) {
                if (!id.is_string() || !validItemId(id.get<std::string>())) throw DataError(file, where + ".classes", "must hold class ids in quotes");
                def.classes.push_back(id.get<std::string>());
            }
        }
        if (entry.contains("withinMetres")) def.withinMetres = requireInt(entry, file, where, "withinMetres", 1, 200);
        if (entry.contains("forMinutes")) def.forMinutes = requireInt(entry, file, where, "forMinutes", 1, 1440);
        if (entry.contains("bonus")) def.bonus = requireInt(entry, file, where, "bonus", 0, 1000);
        if (catalog.find(def.id) != nullptr) throw DataError(file, where + ".id", "\"" + def.id + "\" is used twice");
        catalog.events.push_back(def);
    }
    return catalog;
}

std::vector<EventOffer> EventBoard::offersFor(const EventCatalog& catalog, const std::vector<std::string>& classes, int x, int y, std::uint64_t tick) const {
    std::vector<EventOffer> out;
    for (const PostedEvent& event : posted_) {
        if (event.untilTick < tick) continue;
        for (const EventDef& def : catalog.events) {
            if (def.trigger != event.trigger) continue;
            const bool classMatches = def.classes.empty() || std::any_of(def.classes.begin(), def.classes.end(), [&](const std::string& id) { return std::find(classes.begin(), classes.end(), id) != classes.end(); });
            if (!classMatches) continue;
            const std::int64_t dx = event.x - x;
            const std::int64_t dy = event.y - y;
            const std::int64_t reach = static_cast<std::int64_t>(def.withinMetres) * 32; // one tile is one metre, 32 pixels
            if (dx * dx + dy * dy > reach * reach) continue;
            out.push_back({&def, event.x, event.y});
        }
    }
    return out;
}

void EventBoard::expire(std::uint64_t tick) {
    std::erase_if(posted_, [tick](const PostedEvent& event) { return event.untilTick < tick; });
}

} // namespace odysseus::sim
