#pragma once

#include "boundary.h"

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace odysseus::sim {

// A count of each kind of item, by item id. An ordered map, so every walk over it, every saved text and every hash is the same on every run (ADR-011).
using ItemCounts = std::map<std::string, int>;

// What the owner sets for a region in the Editor (US-280, D-54 Q1-Q2, docs/plans/M9-npc-design.md): which items are money here, what the market asks for goods, and which
// goods the region delivers to its traders. Whole numbers only: money is an integer (Charter rule 6).
struct RegionEconomy {
    static constexpr int kMaxValue = 100000;

    ItemCounts currencies; // item id -> its value in value units: any currency item is worth its value anywhere
    ItemCounts prices;     // item id -> the market's base price in value units (overrides the item's own value)
    ItemCounts resources;  // item id -> a delivery weight added to every trader's own weights at the daily restock

    bool empty() const { return currencies.empty() && prices.empty() && resources.empty(); }
    // A region with no currency trades by barter only.
    bool hasCurrency() const { return !currencies.empty(); }
    bool isCurrency(const std::string& item) const { return currencies.count(item) != 0; }
    int currencyValue(const std::string& item) const; // 0 when the item is not a currency here
    friend bool operator==(const RegionEconomy&, const RegionEconomy&) = default;
};

// An item id is lower-case letters, digits and "-" (at most 32, or `maxLength`), as in the item files. Editor file names use the same rule.
bool validItemId(std::string_view id, std::size_t maxLength = 32);

// "shells=1 gold=10" and back: the text the Editor shows and reads for a table of item and number. Every name is checked as an item id and every number to the
// range. On a mistake nothing comes back and `problem` (when given) says what is wrong.
std::string formatPairs(const ItemCounts& pairs);
std::optional<ItemCounts> parsePairs(std::string_view text, int minimum, int maximum, std::string* problem = nullptr);

// The money in a bag: the total value of its currency items.
long long coinValue(const RegionEconomy& economy, const ItemCounts& goods);
// Takes every currency item out of `goods` and returns their total value (the balance the trade screen opens with).
int takeCoins(const RegionEconomy& economy, ItemCounts& goods);
// `amount` as coins, the highest value first; `remainder` is what no coin can make (less than the smallest coin value). With no currency the whole amount is the remainder.
ItemCounts makeChange(const RegionEconomy& economy, int amount, int& remainder);

} // namespace odysseus::sim
