#pragma once

#include "boundary.h"

#include "sim/economy.h"
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

    bool empty() const { return trade.empty(); }
    friend bool operator==(const NpcExtras&, const NpcExtras&) = default;
};

// A mistake in a file: the line and the message (the caller adds the file name).
using ExtrasError = std::function<void(int line, const std::string& message)>;

// The Editor's text fields of a trade profile (US-284): the six fields stock, restock, picks, weights, wants and rare as the text the owner types, and back. A mistake
// changes nothing and says what is wrong.
const std::vector<std::string>& tradeFieldNames();
std::string tradeFieldText(const TradeProfile& trade, const std::string& field);
bool setTradeField(TradeProfile& trade, const std::string& field, std::string_view text, std::string& problem);

// The names of the object fields that belong to the extras ("trade", ...), for the unknown-field check of the parsers that know them.
const std::vector<std::string>& extrasFieldNames();
// Reads the extras fields present in `root` (an object); anything wrong goes to `error` and that field is left out.
NpcExtras parseExtras(const JsonValue& root, const ExtrasError& error);
// The extras as JSON field texts in the fixed order of the guide, one per field that is set, for the writers: {"trade", "{ ... }"}.
std::vector<std::pair<std::string, std::string>> extrasFieldTexts(const NpcExtras& extras);
// Applies a higher layer over a lower one (what resolveNpc does for each layer).
void mergeExtras(NpcExtras& base, const NpcExtras& over);

} // namespace odysseus::sim::rules
