#pragma once

#include "boundary.h"

#include "sim/economy.h"
#include "sim/npc_schedule.h"
#include "sim/rule_json.h"

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace odysseus::sim::rules {

// What an NPC trades (M9b, docs/plans/M9-npc-design.md): the `trade` block of a class file, a kind file or a placed NPC. Layers merge class, then kind, then the NPC:
// the maps merge per key (the later layer wins), `wants` is the union.
struct TradeProfile {
    ItemCounts stock;          // the starting stock; also the target the stock-ratio curve measures against (0 = the shelf is empty at the start)
    ItemCounts restockPerDay;  // delivered every day as written
    int deliveries = -1;       // weighted random picks a day (each adds one piece); -1 = not set
    ItemCounts weights;        // the picks are drawn from these weights plus the region's `resources`
    std::vector<std::string> wants; // bought at full value; any other good at half (D-52 Q-17)
    std::map<std::string, std::string> rare; // good -> the lowest band word of the trader's opinion that unlocks it ("friendly", "enchanted")

    bool empty() const { return stock.empty() && restockPerDay.empty() && deliveries < 0 && weights.empty() && wants.empty() && rare.empty(); }
    friend bool operator==(const TradeProfile&, const TradeProfile&) = default;
};

// The band words a `rare` entry may name, lowest first.
const std::vector<std::string>& rareBandWords();

// `over` is the higher layer: its entries win per key, its wants are added.
void mergeTrade(TradeProfile& base, const TradeProfile& over);

// The fields every layer of an NPC may add beyond the M9a ones. Later stories add schedules, actions and partner defaults here, so a class, a kind and a placed NPC
// all read, write and merge them the same way.
struct NpcExtras {
    TradeProfile trade;
    Schedule schedule; // the day of the NPC (US-290); the whole schedule of the highest layer that has one wins
    // The actions the NPC does on its own when it is idle on duty (US-291, D-54 Q11): interaction ids. In a class file they are the actions of that class, in a kind file and on a
    // placed NPC its custom actions. Layers add up.
    std::vector<std::string> does;
    // What the NPC prefers to do with each kind of partner it meets (US-293, D-54 Q14): partner type (player, animal, environment, class:<id>, a type of partner-types.json, or `class`
    // for every NPC class) to interaction ids. The chooser gives these a bonus; a layer's list for a type replaces the lower layers' list for it.
    std::map<std::string, std::vector<std::string>> partnerActions;

    bool empty() const { return trade.empty() && schedule.empty() && does.empty() && partnerActions.empty(); }
    friend bool operator==(const NpcExtras&, const NpcExtras&) = default;
};

// A mistake in a file: the line and the message (the caller adds the file name).
using ExtrasError = std::function<void(int line, const std::string& message)>;

// The Editor's text fields of a trade profile (US-284): the six fields stock, restock, picks, weights, wants and rare as the text the owner types, and back. A mistake
// changes nothing and says what is wrong.
const std::vector<std::string>& tradeFieldNames();
std::string tradeFieldText(const TradeProfile& trade, const std::string& field);
bool setTradeField(TradeProfile& trade, const std::string& field, std::string_view text, std::string& problem);

// A schedule from the text of its JSON, { "day": [...], "night": [...] } or a list of day blocks: the one reader of the format, used by the class, kind and NPC files and
// by the professions of hero/professions.json (US-196: a routine is a schedule). Mistakes are added to `problems` as "message" (the lines are those of `jsonText`).
Schedule scheduleFromJson(std::string_view jsonText, std::vector<std::string>& problems);

// The Editor's text fields of a schedule (US-290): "day" and "night", each "06:00 work market; 21:00 sleep home". Empty text clears the list. A mistake changes nothing.
const std::vector<std::string>& scheduleFieldNames();
std::string scheduleFieldText(const Schedule& schedule, const std::string& field);
bool setScheduleField(Schedule& schedule, const std::string& field, std::string_view text, std::string& problem);

// The Editor's line of the default actions with one partner type (US-293): interaction ids separated by spaces or commas; empty text removes the type. A mistake changes nothing.
std::string partnerActionsText(const NpcExtras& extras, const std::string& partnerType);
bool setPartnerActions(NpcExtras& extras, const std::string& partnerType, std::string_view text, std::string& problem);
// A partner type a `partnerActions` key may name: a registered type, class:<id>, or `class`.
bool validPartnerKey(const std::string& key);

// The Editor's line of actions (US-291): interaction ids separated by spaces or commas. Empty text clears the list. A mistake changes nothing.
std::string doesText(const std::vector<std::string>& does);
bool setDoes(std::vector<std::string>& does, std::string_view text, std::string& problem);

// The names of the object fields that belong to the extras ("trade", ...), for the unknown-field check of the parsers that know them.
const std::vector<std::string>& extrasFieldNames();
// Reads the extras fields present in `root` (an object); anything wrong goes to `error` and that field is left out.
NpcExtras parseExtras(const JsonValue& root, const ExtrasError& error);
// The extras as JSON field texts in the fixed order of the guide, one per field that is set, for the writers: {"trade", "{ ... }"}.
std::vector<std::pair<std::string, std::string>> extrasFieldTexts(const NpcExtras& extras);
// Applies a higher layer over a lower one (what resolveNpc does for each layer).
void mergeExtras(NpcExtras& base, const NpcExtras& over);

} // namespace odysseus::sim::rules
