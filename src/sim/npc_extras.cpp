#include "sim/npc_extras.h"

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

void mergeExtras(NpcExtras& base, const NpcExtras& over) { mergeTrade(base.trade, over.trade); }

const std::vector<std::string>& extrasFieldNames() {
    static const std::vector<std::string> names = {"trade"};
    return names;
}

NpcExtras parseExtras(const JsonValue& root, const ExtrasError& error) {
    NpcExtras out;
    if (const JsonValue* trade = root.find("trade")) out.trade = readTrade(*trade, error);
    return out;
}

std::vector<std::pair<std::string, std::string>> extrasFieldTexts(const NpcExtras& extras) {
    std::vector<std::pair<std::string, std::string>> out;
    if (!extras.trade.empty()) out.emplace_back("trade", tradeText(extras.trade));
    return out;
}

} // namespace odysseus::sim::rules
