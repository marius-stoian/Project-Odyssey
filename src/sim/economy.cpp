#include "sim/economy.h"
#include "sim/economy_json.h"

#include "sim/data.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <format>
#include <set>
#include <utility>
#include <vector>

namespace odysseus::sim {

namespace {

using nlohmann::json;

// One table of the economy from a JSON object of item id to whole number.
ItemCounts readTable(const json& table, const std::filesystem::path& file, const std::string& where, int minimum, int maximum) {
    ItemCounts out;
    if (!table.is_object()) throw DataError(file, where, "must be an object of item id: number");
    for (const auto& [name, value] : table.items()) {
        if (!validItemId(name)) throw DataError(file, where + "." + name, "is not an item id (lower-case letters, digits and - only)");
        if (!value.is_number_integer()) throw DataError(file, where + "." + name, "must be a whole number");
        const long long number = value.get<long long>();
        if (number < minimum || number > maximum) throw DataError(file, where + "." + name, std::format("must be between {} and {} (is {})", minimum, maximum, number));
        out[name] = static_cast<int>(number);
    }
    return out;
}

json tableToJson(const ItemCounts& table) {
    json out = json::object();
    for (const auto& [name, value] : table) out[name] = value;
    return out;
}

} // namespace

int RegionEconomy::currencyValue(const std::string& item) const {
    const auto found = currencies.find(item);
    return found == currencies.end() ? 0 : found->second;
}

bool validItemId(std::string_view id) {
    if (id.empty() || id.size() > 32) return false;
    return std::all_of(id.begin(), id.end(), [](unsigned char c) { return (std::islower(c) != 0) || (std::isdigit(c) != 0) || c == '-'; });
}

std::string formatPairs(const ItemCounts& pairs) {
    std::string out;
    for (const auto& [name, value] : pairs) {
        if (!out.empty()) out += ' ';
        out += std::format("{}={}", name, value);
    }
    return out;
}

std::optional<ItemCounts> parsePairs(std::string_view text, int minimum, int maximum, std::string* problem) {
    const auto fail = [&](std::string message) -> std::optional<ItemCounts> {
        if (problem != nullptr) *problem = std::move(message);
        return std::nullopt;
    };
    ItemCounts out;
    std::size_t at = 0;
    while (at < text.size()) {
        while (at < text.size() && (text[at] == ' ' || text[at] == ',' || text[at] == ';')) ++at;
        if (at >= text.size()) break;
        std::size_t end = at;
        while (end < text.size() && text[end] != ' ' && text[end] != ',' && text[end] != ';') ++end;
        const std::string_view word = text.substr(at, end - at);
        at = end;
        const std::size_t equals = word.find('=');
        if (equals == std::string_view::npos) return fail(std::format("\"{}\" must be item=number, for example shells=1", word));
        const std::string_view name = word.substr(0, equals);
        const std::string_view digits = word.substr(equals + 1);
        if (!validItemId(name)) return fail(std::format("\"{}\" is not an item id (lower-case letters, digits and -)", name));
        int number = 0;
        const auto [stop, error] = std::from_chars(digits.data(), digits.data() + digits.size(), number);
        if (digits.empty() || error != std::errc() || stop != digits.data() + digits.size()) return fail(std::format("\"{}\": the number after = must be whole", word));
        if (number < minimum || number > maximum) return fail(std::format("\"{}\": the number must be between {} and {}", word, minimum, maximum));
        out[std::string(name)] = number;
    }
    return out;
}

long long coinValue(const RegionEconomy& economy, const ItemCounts& goods) {
    long long total = 0;
    for (const auto& [item, count] : goods) total += static_cast<long long>(economy.currencyValue(item)) * count;
    return total;
}

int takeCoins(const RegionEconomy& economy, ItemCounts& goods) {
    long long total = 0;
    for (auto it = goods.begin(); it != goods.end();) {
        const int value = economy.currencyValue(it->first);
        if (value > 0) {
            total += static_cast<long long>(value) * it->second;
            it = goods.erase(it);
        } else {
            ++it;
        }
    }
    return static_cast<int>(std::min<long long>(total, 2'000'000'000LL));
}

ItemCounts makeChange(const RegionEconomy& economy, int amount, int& remainder) {
    // The highest value first; the item id breaks a tie, so the same amount always gives the same coins.
    std::vector<std::pair<int, std::string>> coins;
    for (const auto& [item, value] : economy.currencies) coins.emplace_back(value, item);
    std::sort(coins.begin(), coins.end(), [](const auto& a, const auto& b) { return a.first != b.first ? a.first > b.first : a.second < b.second; });
    ItemCounts out;
    int left = std::max(0, amount);
    for (const auto& [value, item] : coins) {
        if (value <= 0 || left < value) continue;
        out[item] += left / value;
        left %= value;
    }
    remainder = left;
    return out;
}

RegionEconomy economyFromJson(const json& value, const std::filesystem::path& file, const std::string& where) {
    if (!value.is_object()) throw DataError(file, where, "must be an object with currencies, prices and resources");
    static const std::set<std::string> known = {"currencies", "prices", "resources"};
    for (const auto& [name, ignored] : value.items()) {
        (void)ignored;
        if (known.count(name) == 0) throw DataError(file, where + "." + name, "is not a field of the economy (currencies, prices, resources)");
    }
    RegionEconomy out;
    if (value.contains("currencies")) out.currencies = readTable(value.at("currencies"), file, where + ".currencies", 1, RegionEconomy::kMaxValue);
    if (value.contains("prices")) out.prices = readTable(value.at("prices"), file, where + ".prices", 1, RegionEconomy::kMaxValue);
    if (value.contains("resources")) out.resources = readTable(value.at("resources"), file, where + ".resources", 1, 1000);
    return out;
}

json economyToJson(const RegionEconomy& economy) {
    json out = json::object();
    if (!economy.currencies.empty()) out["currencies"] = tableToJson(economy.currencies);
    if (!economy.prices.empty()) out["prices"] = tableToJson(economy.prices);
    if (!economy.resources.empty()) out["resources"] = tableToJson(economy.resources);
    return out;
}

} // namespace odysseus::sim
