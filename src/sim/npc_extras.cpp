#include "sim/npc_extras.h"

#include "sim/partner_types.h"

#include <algorithm>
#include <charconv>
#include <format>
#include <set>

namespace odysseus::sim::rules {

namespace {

bool wholeNumber(const JsonValue& value, int minimum, int maximum, int& out) {
    if (!value.isNumber()) return false;
    const std::string& text = value.text;
    int parsed = 0;
    const auto [stop, error] = std::from_chars(text.data(), text.data() + text.size(), parsed);
    if (error != std::errc() || stop != text.data() + text.size() || parsed < minimum || parsed > maximum) return false;
    out = parsed;
    return true;
}

// {"flint": 6, ...} with item ids as keys and whole numbers in the range as values.
ItemCounts readTable(const JsonValue* value, const std::string& where, int minimum, int maximum, const ExtrasError& error) {
    ItemCounts out;
    if (value == nullptr) return out;
    if (!value->isObject()) {
        error(value->line, std::format("{} must be an object of item: number", where));
        return out;
    }
    for (std::size_t i = 0; i < value->keys.size(); ++i) {
        const std::string& name = value->keys[i];
        int number = 0;
        if (!validItemId(name)) {
            error(value->keyLines[i], std::format("{}: \"{}\" is not an item id (lower-case letters, digits and -)", where, name));
        } else if (!wholeNumber(value->items[i], minimum, maximum, number)) {
            error(value->items[i].line, std::format("{}.{} must be a whole number from {} to {}", where, name, minimum, maximum));
        } else {
            out[name] = number;
        }
    }
    return out;
}

TradeProfile readTrade(const JsonValue& block, const ExtrasError& error) {
    TradeProfile out;
    if (!block.isObject()) {
        error(block.line, "trade must be { \"stock\": {...}, \"restockPerDay\": {...}, \"deliveries\": n, \"weights\": {...}, \"wants\": [...], \"rare\": {...} }");
        return out;
    }
    static const std::set<std::string> known = {"stock", "restockPerDay", "deliveries", "weights", "wants", "rare"};
    for (std::size_t i = 0; i < block.keys.size(); ++i) {
        if (known.count(block.keys[i]) == 0) {
            error(block.keyLines[i], std::format("unknown field \"{}\" in trade (known: stock, restockPerDay, deliveries, weights, wants, rare)", block.keys[i]));
        }
    }
    out.stock = readTable(block.find("stock"), "trade.stock", 0, 9999, error);
    out.restockPerDay = readTable(block.find("restockPerDay"), "trade.restockPerDay", 0, 999, error);
    out.weights = readTable(block.find("weights"), "trade.weights", 0, 1000, error);
    if (const JsonValue* deliveries = block.find("deliveries")) {
        int number = 0;
        if (!wholeNumber(*deliveries, 0, 20, number)) error(deliveries->line, "trade.deliveries must be a whole number from 0 to 20");
        else out.deliveries = number;
    }
    if (const JsonValue* wants = block.find("wants")) {
        if (!wants->isArray()) {
            error(wants->line, "trade.wants must be a list of item ids");
        } else {
            for (const JsonValue& item : wants->items) {
                if (!item.isString() || !validItemId(item.text)) error(item.line, "trade.wants must hold item ids in quotes");
                else if (std::find(out.wants.begin(), out.wants.end(), item.text) == out.wants.end()) out.wants.push_back(item.text);
            }
        }
    }
    if (const JsonValue* rare = block.find("rare")) {
        if (!rare->isObject()) {
            error(rare->line, "trade.rare must be an object of item: band word");
        } else {
            const std::vector<std::string>& words = rareBandWords();
            for (std::size_t i = 0; i < rare->keys.size(); ++i) {
                const JsonValue& word = rare->items[i];
                if (!validItemId(rare->keys[i])) {
                    error(rare->keyLines[i], std::format("trade.rare: \"{}\" is not an item id", rare->keys[i]));
                } else if (!word.isString() || std::find(words.begin(), words.end(), word.text) == words.end()) {
                    error(word.line, std::format("trade.rare.{} must be one of: hostile, wary, suspicious, neutral, friendly, enchanted, lovingly", rare->keys[i]));
                } else {
                    out.rare[rare->keys[i]] = word.text;
                }
            }
        }
    }
    return out;
}

std::vector<ScheduleBlock> readBlocks(const JsonValue& list, const std::string& where, const ExtrasError& error) {
    std::vector<ScheduleBlock> out;
    if (!list.isArray()) {
        error(list.line, where + " must be a list of blocks like { \"from\": \"06:00\", \"do\": \"work\", \"at\": \"market\" }");
        return out;
    }
    for (const JsonValue& entry : list.items) {
        if (!entry.isObject()) {
            error(entry.line, where + " must hold blocks like { \"from\": \"06:00\", \"do\": \"work\", \"at\": \"market\" }");
            continue;
        }
        for (std::size_t i = 0; i < entry.keys.size(); ++i) {
            if (entry.keys[i] != "from" && entry.keys[i] != "do" && entry.keys[i] != "at") error(entry.keyLines[i], std::format("unknown field \"{}\" in a schedule block (from, do, at)", entry.keys[i]));
        }
        const JsonValue* from = entry.find("from");
        const JsonValue* doing = entry.find("do");
        const JsonValue* at = entry.find("at");
        const std::optional<int> minute = from != nullptr && from->isString() ? parseClock(from->text) : std::nullopt;
        if (!minute) {
            error(from != nullptr ? from->line : entry.line, "a schedule block needs \"from\": \"HH:MM\", for example \"06:00\"");
            continue;
        }
        if (doing == nullptr || !doing->isString() || !validItemId(doing->text)) {
            error(doing != nullptr ? doing->line : entry.line, "a schedule block needs \"do\": an activity word (work, eat, sleep, ...) or an interaction id");
            continue;
        }
        if (at != nullptr && (!at->isString() || !validItemId(at->text))) {
            error(at->line, "\"at\" must be the name of a place of the level, or home");
            continue;
        }
        out.push_back({*minute, doing->text, at != nullptr ? at->text : std::string("home")});
    }
    std::stable_sort(out.begin(), out.end(), [](const ScheduleBlock& a, const ScheduleBlock& b) { return a.minute < b.minute; });
    for (std::size_t i = 1; i < out.size(); ++i) {
        if (out[i].minute == out[i - 1].minute) error(list.line, std::format("{}: two blocks begin at {}", where, formatClock(out[i].minute)));
    }
    return out;
}

Schedule readSchedule(const JsonValue& block, const ExtrasError& error) {
    Schedule out;
    if (block.isArray()) {
        out.day = readBlocks(block, "schedule", error);
    } else if (block.isObject()) {
        for (std::size_t i = 0; i < block.keys.size(); ++i) {
            if (block.keys[i] != "day" && block.keys[i] != "night") error(block.keyLines[i], std::format("unknown field \"{}\" in schedule (day, night)", block.keys[i]));
        }
        if (const JsonValue* day = block.find("day")) out.day = readBlocks(*day, "schedule.day", error);
        if (const JsonValue* night = block.find("night")) out.night = readBlocks(*night, "schedule.night", error);
    } else {
        error(block.line, "schedule must be a list of blocks, or { \"day\": [...], \"night\": [...] }");
    }
    return out;
}

std::string blocksText(const std::vector<ScheduleBlock>& blocks) {
    std::string out = "[";
    for (std::size_t i = 0; i < blocks.size(); ++i) {
        out += std::format("{}{{ \"from\": \"{}\", \"do\": {}, \"at\": {} }}", i != 0 ? ", " : "", formatClock(blocks[i].minute), quoteJson(blocks[i].activity), quoteJson(blocks[i].place));
    }
    return out + "]";
}

std::string scheduleJsonText(const Schedule& schedule) {
    if (schedule.night.empty()) return blocksText(schedule.day);
    return "{ \"day\": " + blocksText(schedule.day) + ", \"night\": " + blocksText(schedule.night) + " }";
}

std::string tableText(const ItemCounts& table) {
    std::string out = "{";
    bool first = true;
    for (const auto& [name, value] : table) {
        out += std::format("{} {}: {}", first ? "" : ",", quoteJson(name), value);
        first = false;
    }
    return out + (table.empty() ? "}" : " }");
}

std::string tradeText(const TradeProfile& trade) {
    std::vector<std::string> parts;
    if (!trade.stock.empty()) parts.push_back("\"stock\": " + tableText(trade.stock));
    if (!trade.restockPerDay.empty()) parts.push_back("\"restockPerDay\": " + tableText(trade.restockPerDay));
    if (trade.deliveries >= 0) parts.push_back(std::format("\"deliveries\": {}", trade.deliveries));
    if (!trade.weights.empty()) parts.push_back("\"weights\": " + tableText(trade.weights));
    if (!trade.wants.empty()) {
        std::string list = "[";
        for (std::size_t i = 0; i < trade.wants.size(); ++i) list += (i != 0 ? ", " : "") + quoteJson(trade.wants[i]);
        parts.push_back("\"wants\": " + list + "]");
    }
    if (!trade.rare.empty()) {
        std::string object = "{";
        bool first = true;
        for (const auto& [item, word] : trade.rare) {
            object += std::format("{} {}: {}", first ? "" : ",", quoteJson(item), quoteJson(word));
            first = false;
        }
        parts.push_back("\"rare\": " + object + " }");
    }
    std::string out = "{";
    for (std::size_t i = 0; i < parts.size(); ++i) out += (i != 0 ? ", " : " ") + parts[i];
    return out + " }";
}

} // namespace

bool validPartnerKey(const std::string& key) { return key == "class" || validPartnerType(key); }

std::string partnerActionsText(const NpcExtras& extras, const std::string& partnerType) {
    const auto found = extras.partnerActions.find(partnerType);
    return found == extras.partnerActions.end() ? std::string() : doesText(found->second);
}

bool setPartnerActions(NpcExtras& extras, const std::string& partnerType, std::string_view text, std::string& problem) {
    if (!validPartnerKey(partnerType)) {
        problem = std::format("\"{}\" is not a partner type", partnerType);
        return false;
    }
    std::vector<std::string> ids;
    if (!setDoes(ids, text, problem)) return false;
    if (ids.empty()) extras.partnerActions.erase(partnerType);
    else extras.partnerActions[partnerType] = ids;
    return true;
}

std::string doesText(const std::vector<std::string>& does) {
    std::string out;
    for (const std::string& id : does) out += (out.empty() ? "" : " ") + id;
    return out;
}

bool setDoes(std::vector<std::string>& does, std::string_view text, std::string& problem) {
    std::vector<std::string> ids;
    std::size_t at = 0;
    while (at < text.size()) {
        while (at < text.size() && (text[at] == ' ' || text[at] == ',' || text[at] == ';')) ++at;
        std::size_t end = at;
        while (end < text.size() && text[end] != ' ' && text[end] != ',' && text[end] != ';') ++end;
        if (end == at) break;
        const std::string id(text.substr(at, end - at));
        at = end;
        if (!validItemId(id)) {
            problem = std::format("\"{}\" is not an interaction id (lower-case letters, digits and -)", id);
            return false;
        }
        if (std::find(ids.begin(), ids.end(), id) == ids.end()) ids.push_back(id);
    }
    does = ids;
    return true;
}

const std::vector<std::string>& scheduleFieldNames() {
    static const std::vector<std::string> names = {"day", "night"};
    return names;
}

std::string scheduleFieldText(const Schedule& schedule, const std::string& field) {
    if (field == "day") return scheduleText(schedule.day);
    if (field == "night") return scheduleText(schedule.night);
    return {};
}

bool setScheduleField(Schedule& schedule, const std::string& field, std::string_view text, std::string& problem) {
    if (field != "day" && field != "night") {
        problem = std::format("\"{}\" is not a schedule field (day, night)", field);
        return false;
    }
    const std::optional<std::vector<ScheduleBlock>> blocks = parseScheduleText(text, problem);
    if (!blocks) return false;
    (field == "day" ? schedule.day : schedule.night) = *blocks;
    return true;
}

const std::vector<std::string>& tradeFieldNames() {
    static const std::vector<std::string> names = {"stock", "restock", "picks", "weights", "wants", "rare"};
    return names;
}

std::string tradeFieldText(const TradeProfile& trade, const std::string& field) {
    if (field == "stock") return formatPairs(trade.stock);
    if (field == "restock") return formatPairs(trade.restockPerDay);
    if (field == "picks") return trade.deliveries < 0 ? std::string() : std::to_string(trade.deliveries);
    if (field == "weights") return formatPairs(trade.weights);
    std::string out;
    if (field == "wants") {
        for (const std::string& item : trade.wants) out += (out.empty() ? "" : " ") + item;
    } else if (field == "rare") {
        for (const auto& [item, word] : trade.rare) out += std::format("{}{}={}", out.empty() ? "" : " ", item, word);
    }
    return out;
}

bool setTradeField(TradeProfile& trade, const std::string& field, std::string_view text, std::string& problem) {
    if (field == "stock" || field == "restock" || field == "weights") {
        const int maximum = field == "stock" ? 9999 : (field == "restock" ? 999 : 1000);
        const std::optional<ItemCounts> parsed = parsePairs(text, 0, maximum, &problem);
        if (!parsed) return false;
        (field == "stock" ? trade.stock : (field == "restock" ? trade.restockPerDay : trade.weights)) = *parsed;
        return true;
    }
    if (field == "picks") {
        if (text.find_first_not_of(' ') == std::string_view::npos) {
            trade.deliveries = -1; // an empty field: not set
            return true;
        }
        int number = 0;
        std::size_t first = text.find_first_not_of(' ');
        std::size_t last = text.find_last_not_of(' ');
        const std::string_view digits = text.substr(first, last - first + 1);
        const auto [stop, error] = std::from_chars(digits.data(), digits.data() + digits.size(), number);
        if (error != std::errc() || stop != digits.data() + digits.size() || number < 0 || number > 20) {
            problem = "the number of picks a day must be a whole number from 0 to 20";
            return false;
        }
        trade.deliveries = number;
        return true;
    }
    // wants and rare are lists of words separated by spaces, commas or semicolons.
    std::vector<std::string> words;
    std::size_t at = 0;
    while (at < text.size()) {
        while (at < text.size() && (text[at] == ' ' || text[at] == ',' || text[at] == ';')) ++at;
        std::size_t end = at;
        while (end < text.size() && text[end] != ' ' && text[end] != ',' && text[end] != ';') ++end;
        if (end > at) words.emplace_back(text.substr(at, end - at));
        at = end;
    }
    if (field == "wants") {
        std::vector<std::string> wants;
        for (const std::string& word : words) {
            if (!validItemId(word)) {
                problem = std::format("\"{}\" is not an item id (lower-case letters, digits and -)", word);
                return false;
            }
            if (std::find(wants.begin(), wants.end(), word) == wants.end()) wants.push_back(word);
        }
        trade.wants = wants;
        return true;
    }
    if (field == "rare") {
        std::map<std::string, std::string> rare;
        const std::vector<std::string>& bands = rareBandWords();
        for (const std::string& word : words) {
            const std::size_t equals = word.find('=');
            const std::string item = word.substr(0, equals);
            const std::string band = equals == std::string::npos ? std::string() : word.substr(equals + 1);
            if (equals == std::string::npos || !validItemId(item) || std::find(bands.begin(), bands.end(), band) == bands.end()) {
                problem = std::format("\"{}\" must be item=band, for example obsidian=friendly (bands: hostile, wary, suspicious, neutral, friendly, enchanted, lovingly)", word);
                return false;
            }
            rare[item] = band;
        }
        trade.rare = rare;
        return true;
    }
    problem = std::format("\"{}\" is not a trade field", field);
    return false;
}

const std::vector<std::string>& rareBandWords() {
    static const std::vector<std::string> words = {"hostile", "wary", "suspicious", "neutral", "friendly", "enchanted", "lovingly"};
    return words;
}

void mergeTrade(TradeProfile& base, const TradeProfile& over) {
    for (const auto& [item, count] : over.stock) base.stock[item] = count;
    for (const auto& [item, count] : over.restockPerDay) base.restockPerDay[item] = count;
    if (over.deliveries >= 0) base.deliveries = over.deliveries;
    for (const auto& [item, weight] : over.weights) base.weights[item] = weight;
    for (const std::string& item : over.wants) {
        if (std::find(base.wants.begin(), base.wants.end(), item) == base.wants.end()) base.wants.push_back(item);
    }
    for (const auto& [item, word] : over.rare) base.rare[item] = word;
}

void mergeExtras(NpcExtras& base, const NpcExtras& over) {
    mergeTrade(base.trade, over.trade);
    for (const std::string& id : over.does) {
        if (std::find(base.does.begin(), base.does.end(), id) == base.does.end()) base.does.push_back(id);
    }
    for (const auto& [type, ids] : over.partnerActions) base.partnerActions[type] = ids; // the list of the highest layer for a partner type wins
    if (!over.schedule.empty()) base.schedule = over.schedule; // the schedule of the highest layer that has one
}

const std::vector<std::string>& extrasFieldNames() {
    static const std::vector<std::string> names = {"trade", "schedule", "does", "partnerActions"};
    return names;
}

NpcExtras parseExtras(const JsonValue& root, const ExtrasError& error) {
    NpcExtras out;
    if (const JsonValue* trade = root.find("trade")) out.trade = readTrade(*trade, error);
    if (const JsonValue* schedule = root.find("schedule")) out.schedule = readSchedule(*schedule, error);
    if (const JsonValue* partner = root.find("partnerActions")) {
        if (!partner->isObject()) {
            error(partner->line, "partnerActions must be { \"animal\": [\"hunt\"], ... }");
        } else {
            for (std::size_t i = 0; i < partner->keys.size(); ++i) {
                const JsonValue& list = partner->items[i];
                if (!validPartnerKey(partner->keys[i])) {
                    error(partner->keyLines[i], std::format("unknown partner type \"{}\" (player, animal, environment, class, class:<id> or a type of partner-types.json)", partner->keys[i]));
                } else if (!list.isArray()) {
                    error(list.line, std::format("partnerActions.{} must be a list of interaction ids", partner->keys[i]));
                } else {
                    std::vector<std::string> ids;
                    for (const JsonValue& item : list.items) {
                        if (!item.isString() || !validItemId(item.text)) error(item.line, "partnerActions must hold interaction ids in quotes");
                        else if (std::find(ids.begin(), ids.end(), item.text) == ids.end()) ids.push_back(item.text);
                    }
                    out.partnerActions[partner->keys[i]] = ids;
                }
            }
        }
    }
    if (const JsonValue* does = root.find("does")) {
        if (!does->isArray()) {
            error(does->line, "does must be a list of interaction ids, like [\"patrol\"]");
        } else {
            for (const JsonValue& item : does->items) {
                if (!item.isString() || !validItemId(item.text)) error(item.line, "does must hold interaction ids in quotes");
                else if (std::find(out.does.begin(), out.does.end(), item.text) == out.does.end()) out.does.push_back(item.text);
            }
        }
    }
    return out;
}

std::vector<std::pair<std::string, std::string>> extrasFieldTexts(const NpcExtras& extras) {
    std::vector<std::pair<std::string, std::string>> out;
    if (!extras.trade.empty()) out.emplace_back("trade", tradeText(extras.trade));
    if (!extras.schedule.empty()) out.emplace_back("schedule", scheduleJsonText(extras.schedule));
    if (!extras.partnerActions.empty()) {
        std::string object = "{";
        bool first = true;
        for (const auto& [type, ids] : extras.partnerActions) {
            std::string list = "[";
            for (std::size_t i = 0; i < ids.size(); ++i) list += (i != 0 ? ", " : "") + quoteJson(ids[i]);
            object += std::format("{} {}: {}]", first ? "" : ",", quoteJson(type), list);
            first = false;
        }
        out.emplace_back("partnerActions", object + " }");
    }
    if (!extras.does.empty()) {
        std::string list = "[";
        for (std::size_t i = 0; i < extras.does.size(); ++i) list += (i != 0 ? ", " : "") + quoteJson(extras.does[i]);
        out.emplace_back("does", list + "]");
    }
    return out;
}

} // namespace odysseus::sim::rules
