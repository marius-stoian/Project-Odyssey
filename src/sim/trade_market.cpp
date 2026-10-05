#include "sim/trade_market.h"

#include "core/random.h"
#include "sim/data.h"
#include "sim/json_data.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <format>
#include <iterator>
#include <optional>

namespace odysseus::sim {

namespace {

using nlohmann::json;

void mix(std::uint64_t& hash, std::uint64_t value) {
    for (int i = 0; i < 8; ++i) {
        hash ^= (value >> (i * 8)) & 0xFFU;
        hash *= 0x100000001B3ULL; // FNV-1a
    }
}

void mixText(std::uint64_t& hash, const std::string& text) {
    for (const char c : text) mix(hash, static_cast<unsigned char>(c));
    mix(hash, 0xFF);
}

json tableJson(const ItemCounts& table) {
    json out = json::object();
    for (const auto& [name, value] : table) out[name] = value;
    return out;
}

ItemCounts tableFrom(const json& value, const std::string& where) {
    ItemCounts out;
    if (!value.is_object()) throw DataError("trade.json", where, "must be an object");
    for (const auto& [name, count] : value.items()) {
        if (!count.is_number_integer()) throw DataError("trade.json", where + "." + name, "must be a whole number");
        out[name] = count.get<int>();
    }
    return out;
}

} // namespace

PriceConfig loadPriceConfig(const std::filesystem::path& file) {
    PriceConfig config;
    const json data = readJsonFile(file);
    if (requireInt(data, file, "version", 1, 1) != 1) throw DataError(file, "version", "must be 1");
    const auto section = [&](const char* name) -> const json* {
        if (!data.contains(name)) return nullptr;
        if (!data.at(name).is_object()) throw DataError(file, name, "must be an object");
        return &data.at(name);
    };
    if (const json* stock = section("stock")) {
        const auto read = [&](const char* field, int& into, int low, int high) {
            if (stock->contains(field)) into = requireInt(data, file, "stock", field, low, high);
        };
        read("minimumCap", config.minimumCap, 1, 9999);
        read("capFactor", config.capFactor, 1, 100);
        read("defaultTarget", config.defaultTarget, 1, 9999);
        read("deliveryAmount", config.deliveryAmount, 1, 999);
        read("catchUpDays", config.catchUpDays, 1, 365);
    }
    if (const json* curve = section("curve")) {
        if (curve->contains("minPercent")) config.curveMinPercent = requireInt(data, file, "curve", "minPercent", 1, 100);
        if (curve->contains("maxPercent")) config.curveMaxPercent = requireInt(data, file, "curve", "maxPercent", 100, 1000);
    }
    if (const json* drift = section("drift")) {
        if (drift->contains("percentPerTrade")) config.driftPerTrade = requireInt(data, file, "drift", "percentPerTrade", 0, 100);
        if (drift->contains("maxPercent")) config.driftMaxPercent = requireInt(data, file, "drift", "maxPercent", 0, 500);
        if (drift->contains("decayPercentPerDay")) config.driftDecayPercent = requireInt(data, file, "drift", "decayPercentPerDay", 0, 100);
    }
    if (const json* reputation = section("reputation")) {
        if (reputation->contains("percent")) {
            if (!reputation->at("percent").is_object()) throw DataError(file, "reputation.percent", "must be an object of attitude word: percent");
            for (const auto& [word, value] : reputation->at("percent").items()) {
                const std::optional<Attitude> attitude = attitudeFromName(word);
                if (!attitude) throw DataError(file, "reputation.percent." + word, "is not an attitude word");
                if (!value.is_number_integer() || value.get<int>() < -90 || value.get<int>() > 500) throw DataError(file, "reputation.percent." + word, "must be a whole number from -90 to 500");
                config.reputation[static_cast<std::size_t>(*attitude)] = value.get<int>();
            }
        }
        if (reputation->contains("refuse")) {
            if (!reputation->at("refuse").is_array()) throw DataError(file, "reputation.refuse", "must be a list of attitude words");
            config.refuses.fill(false);
            for (const json& word : reputation->at("refuse")) {
                const std::optional<Attitude> attitude = word.is_string() ? attitudeFromName(word.get<std::string>()) : std::nullopt;
                if (!attitude) throw DataError(file, "reputation.refuse", "must hold attitude words in quotes");
                config.refuses[static_cast<std::size_t>(*attitude)] = true;
            }
        }
    }
    if (const json* wants = section("wants")) {
        if (wants->contains("wantPercent")) config.wantPercent = requireInt(data, file, "wants", "wantPercent", 1, 1000);
        if (wants->contains("otherPercent")) config.otherPercent = requireInt(data, file, "wants", "otherPercent", 1, 1000);
    }
    if (const json* haggle = section("haggle")) {
        const auto read = [&](const char* field, int& into, int low, int high) {
            if (haggle->contains(field)) into = requireInt(data, file, "haggle", field, low, high);
        };
        read("baseChance", config.haggleBase, 0, 100);
        read("opinionDivisor", config.haggleOpinionDivisor, 1, 100);
        read("perPersuasion", config.hagglePerPersuasion, 0, 100);
        read("minChance", config.haggleMin, 0, 100);
        read("maxChance", config.haggleMax, 0, 100);
        read("discountPercent", config.haggleDiscountPercent, 0, 90);
        read("failureOpinion", config.haggleFailureOpinion, 0, 100);
        if (config.haggleMin > config.haggleMax) throw DataError(file, "haggle.minChance", "must not be higher than maxChance");
    }
    if (const json* purse = section("purse")) {
        const auto read = [&](const char* field, int& into, int low, int high) {
            if (purse->contains(field)) into = requireInt(data, file, "purse", field, low, high);
        };
        read("start", config.startPurse, 0, 100000);
        read("restockPerDay", config.purseRestock, 0, 100000);
        read("cap", config.purseCap, 0, 1000000);
    }
    if (config.curveMinPercent > config.curveMaxPercent) throw DataError(file, "curve.minPercent", "must not be higher than maxPercent");
    return config;
}

bool TradeMarket::addTrader(int id, const rules::TradeProfile& profile, std::int64_t today) {
    if (const auto existing = traders_.find(id); existing != traders_.end()) {
        existing->second.profile = profile; // the data changed (F5, the Editor): the stock stays
        return false;
    }
    Trader trader;
    trader.profile = profile;
    trader.restockDay = today;
    trader.purse = config_.startPurse;
    today_ = std::max(today_, today);
    if (const auto saved = pending_.find(id); saved != pending_.end()) {
        trader.stock = saved->second.stock;
        trader.drift = saved->second.drift;
        trader.purse = saved->second.purse;
        trader.restockDay = saved->second.restockDay;
        trader.haggleDay = saved->second.haggleDay;
        trader.haggleWon = saved->second.haggleWon;
        pending_.erase(saved);
    } else {
        trader.stock = profile.stock;
        std::erase_if(trader.stock, [](const auto& entry) { return entry.second <= 0; });
    }
    traders_.emplace(id, std::move(trader));
    return true;
}

const TradeMarket::Trader* TradeMarket::find(int id) const {
    const auto found = traders_.find(id);
    return found == traders_.end() ? nullptr : &found->second;
}

int TradeMarket::stock(int id, const std::string& item) const {
    const Trader* trader = find(id);
    if (trader == nullptr) return 0;
    const auto found = trader->stock.find(item);
    return found == trader->stock.end() ? 0 : found->second;
}

int TradeMarket::target(int id, const std::string& item) const {
    const Trader* trader = find(id);
    if (trader == nullptr) return config_.defaultTarget;
    const auto found = trader->profile.stock.find(item);
    return found != trader->profile.stock.end() && found->second > 0 ? found->second : config_.defaultTarget;
}

int TradeMarket::cap(int id, const std::string& item) const {
    const Trader* trader = find(id);
    int start = 0;
    if (trader != nullptr) {
        const auto found = trader->profile.stock.find(item);
        if (found != trader->profile.stock.end()) start = found->second;
    }
    return std::max(start * config_.capFactor, config_.minimumCap);
}

bool TradeMarket::wants(int id, const std::string& item) const {
    const Trader* trader = find(id);
    return trader != nullptr && std::find(trader->profile.wants.begin(), trader->profile.wants.end(), item) != trader->profile.wants.end();
}

int TradeMarket::wantPercent(int id, const std::string& item) const { return wants(id, item) ? config_.wantPercent : config_.otherPercent; }

int TradeMarket::addUpToCap(int id, Trader& trader, const std::string& item, int count) {
    const int room = cap(id, item) - stock(id, item);
    const int added = std::clamp(count, 0, std::max(0, room));
    if (added > 0) trader.stock[item] += added;
    return added;
}

// One day's delivery: the fixed pieces, then the weighted random picks. The picks come from the trader's own weights plus the region's; the stream depends only on the
// world seed, the day and the trader, so the same day always brings the same goods wherever the hero is.
void TradeMarket::deliver(int id, Trader& trader, std::int64_t day, const RegionEconomy& economy) {
    for (const auto& [item, count] : trader.profile.restockPerDay) addUpToCap(id, trader, item, count);
    const int picks = std::max(0, trader.profile.deliveries);
    if (picks == 0) return;
    ItemCounts weights = trader.profile.weights;
    for (const auto& [item, weight] : economy.resources) weights[item] += weight;
    long long total = 0;
    for (const auto& entry : weights) total += entry.second;
    if (total <= 0) return;
    core::Pcg32 random(seed_ + static_cast<std::uint64_t>(day) * 0x9E3779B97F4A7C15ULL, static_cast<std::uint64_t>(id) * 2ULL + 1ULL);
    for (int pick = 0; pick < picks; ++pick) {
        long long roll = static_cast<long long>(random.below(static_cast<std::uint32_t>(total)));
        for (const auto& [item, weight] : weights) {
            if (roll < weight) {
                addUpToCap(id, trader, item, config_.deliveryAmount);
                break;
            }
            roll -= weight;
        }
    }
}

void TradeMarket::dailyUpdate(std::int64_t today, const RegionEconomy& economy) {
    today_ = std::max(today_, today);
    for (auto& [id, trader] : traders_) {
        if (today <= trader.restockDay) continue;
        const std::int64_t first = std::max(trader.restockDay + 1, today - config_.catchUpDays + 1);
        for (std::int64_t day = first; day <= today; ++day) deliver(id, trader, day, economy);
        trader.purse = static_cast<int>(std::min<std::int64_t>(std::max(trader.purse, config_.purseCap), static_cast<std::int64_t>(trader.purse) + (today - first + 1) * config_.purseRestock));
        // Every day the drift falls back toward 0 by a share of itself, at least one point (US-282); a long absence counts as at most a month of it.
        const std::int64_t days = std::min<std::int64_t>(today - trader.restockDay, 30);
        for (std::int64_t day = 0; day < days; ++day) {
            for (auto it = trader.drift.begin(); it != trader.drift.end();) {
                const int step = std::max(1, std::abs(it->second) * config_.driftDecayPercent / 100);
                it->second = it->second > 0 ? std::max(0, it->second - step) : std::min(0, it->second + step);
                it = it->second == 0 ? trader.drift.erase(it) : std::next(it);
            }
        }
        trader.restockDay = today;
    }
}

int TradeMarket::basePrice(const RegionEconomy& economy, const ItemCounts& itemValues, const std::string& item) {
    if (const auto set = economy.prices.find(item); set != economy.prices.end()) return std::max(1, set->second);
    if (const auto value = itemValues.find(item); value != itemValues.end()) return std::max(1, value->second);
    return 1;
}

int TradeMarket::stockRatioPercent(int id, const std::string& item) const {
    const long long ratio = 100LL * target(id, item) / std::max(1, stock(id, item));
    return static_cast<int>(std::clamp<long long>(ratio, config_.curveMinPercent, config_.curveMaxPercent));
}

int TradeMarket::drift(int id, const std::string& item) const {
    const Trader* trader = find(id);
    if (trader == nullptr) return 0;
    const auto found = trader->drift.find(item);
    return found == trader->drift.end() ? 0 : found->second;
}

void TradeMarket::nudge(int id, const std::string& item, int pieces, bool heroBuys) {
    const auto found = traders_.find(id);
    if (found == traders_.end() || pieces <= 0) return;
    int& value = found->second.drift[item];
    value = std::clamp(value + (heroBuys ? 1 : -1) * pieces * config_.driftPerTrade, -config_.driftMaxPercent, config_.driftMaxPercent);
    if (value == 0) found->second.drift.erase(item);
}

// market = base x ratio% x (100 + drift)%, in thousandths, rounded half up once at the end.
long long TradeMarket::marketMilli(int id, const std::string& item, const RegionEconomy& economy, const ItemCounts& itemValues) const {
    if (economy.isCurrency(item)) return static_cast<long long>(economy.currencyValue(item)) * 1000;
    const long long numerator = 1000LL * basePrice(economy, itemValues, item) * stockRatioPercent(id, item) * (100 + drift(id, item));
    return std::max<long long>(1, (numerator + 5000) / 10000);
}

// The attitude is applied last, to the exact figure, so that the rounding happens once: market x (100 + percent)%.
long long TradeMarket::heroPaysMilli(int id, const std::string& item, Attitude attitude, const RegionEconomy& economy, const ItemCounts& itemValues) const {
    if (!isTrader(id)) return 0;
    if (economy.isCurrency(item)) return marketMilli(id, item, economy, itemValues);
    const long long numerator = 1000LL * basePrice(economy, itemValues, item) * stockRatioPercent(id, item) * (100 + drift(id, item)) * (100 + reputationPercent(attitude));
    long long milli = std::max<long long>(1, (numerator + 500000) / 1000000);
    if (const int discount = discountPercent(id); discount > 0) milli = std::max<long long>(1, (milli * (100 - discount) + 50) / 100); // a won haggle: the hero pays less today
    return milli;
}

long long TradeMarket::traderPaysMilli(int id, const std::string& item, Attitude attitude, const RegionEconomy& economy, const ItemCounts& itemValues) const {
    if (!isTrader(id)) return 0;
    if (economy.isCurrency(item)) return marketMilli(id, item, economy, itemValues);
    // The trader never pays a premium for scarcity (the curve is capped at 100 on this side): otherwise selling to an empty shelf and buying the goods back from the full
    // one would make money from nothing. A full shelf still pays less.
    const long long numerator = 1000LL * basePrice(economy, itemValues, item) * std::min(100, stockRatioPercent(id, item)) * (100 + drift(id, item)) * std::max(0, 100 - reputationPercent(attitude)) * wantPercent(id, item);
    long long milli = std::max<long long>(1, (numerator + 50000000) / 100000000);
    if (const int discount = discountPercent(id); discount > 0) milli = (milli * (100 + discount) + 50) / 100; // a won haggle: the trader pays more today
    return milli;
}

bool TradeMarket::isRare(int id, const std::string& item) const {
    const Trader* trader = find(id);
    return trader != nullptr && trader->profile.rare.count(item) != 0;
}

std::string TradeMarket::rareWord(int id, const std::string& item) const {
    const Trader* trader = find(id);
    if (trader == nullptr) return {};
    const auto found = trader->profile.rare.find(item);
    return found == trader->profile.rare.end() ? std::string() : found->second;
}

bool TradeMarket::rareUnlocked(int id, const std::string& item, int opinion, const OpinionConfig& opinions) const {
    const std::string word = rareWord(id, item);
    if (word.empty()) return true; // not a rare good
    const std::optional<Attitude> band = attitudeFromName(word);
    for (const OpinionConfig::Band& entry : opinions.bands) {
        if (band && entry.word == *band) return opinion >= entry.from;
    }
    return false; // a word that is no band of the scale unlocks nothing
}

std::vector<std::string> TradeMarket::offeredGoods(int id, int opinion, const OpinionConfig& opinions) const {
    std::vector<std::string> out;
    const Trader* trader = find(id);
    if (trader == nullptr) return out;
    for (const auto& entry : trader->stock) {
        if (entry.second > 0 && rareUnlocked(id, entry.first, opinion, opinions)) out.push_back(entry.first);
    }
    return out;
}

std::vector<std::string> TradeMarket::lockedGoods(int id, int opinion, const OpinionConfig& opinions) const {
    std::vector<std::string> out;
    const Trader* trader = find(id);
    if (trader == nullptr) return out;
    for (const auto& entry : trader->stock) {
        if (entry.second > 0 && !rareUnlocked(id, entry.first, opinion, opinions)) out.push_back(entry.first);
    }
    return out;
}

int TradeMarket::purse(int id) const {
    const Trader* trader = find(id);
    return trader == nullptr ? 0 : trader->purse;
}

int TradeMarket::discountPercent(int id) const {
    const Trader* trader = find(id);
    return trader != nullptr && trader->haggleWon && trader->haggleDay == today_ ? config_.haggleDiscountPercent : 0;
}

bool TradeMarket::canHaggle(int id) const {
    const Trader* trader = find(id);
    return trader != nullptr && trader->haggleDay != today_;
}

int TradeMarket::haggleChance(int opinion, int persuasion) const {
    const long long chance = static_cast<long long>(config_.haggleBase) + opinion / config_.haggleOpinionDivisor + static_cast<long long>(persuasion) * config_.hagglePerPersuasion;
    return static_cast<int>(std::clamp<long long>(chance, config_.haggleMin, config_.haggleMax));
}

// A seeded roll: the same world, trader and day always give the same number, however many other rolls came before.
TradeMarket::Haggle TradeMarket::haggle(int id, int opinion, int persuasion) {
    Haggle result;
    const auto found = traders_.find(id);
    if (found == traders_.end()) {
        result.message = "There is nobody to haggle with.";
        return result;
    }
    if (found->second.haggleDay == today_) {
        result.message = "You have already haggled today: try again tomorrow.";
        return result;
    }
    result.tried = true;
    result.chance = haggleChance(opinion, persuasion);
    core::Pcg32 random(seed_ ^ 0x484147474C45ULL, (static_cast<std::uint64_t>(static_cast<std::uint32_t>(id)) << 32) ^ static_cast<std::uint64_t>(today_));
    result.roll = static_cast<int>(random.below(100));
    result.won = result.roll < result.chance;
    found->second.haggleDay = today_;
    found->second.haggleWon = result.won;
    if (result.won) {
        result.message = std::format("You haggle well: {}% off today.", config_.haggleDiscountPercent);
    } else {
        result.opinionChange = -config_.haggleFailureOpinion;
        result.message = "They are not amused: they think a little less of you.";
    }
    return result;
}

TradeMarket::Quote TradeMarket::quote(int id, const Deal& deal, Attitude attitude, int opinion, const OpinionConfig& opinions, const RegionEconomy& economy, const ItemCounts& itemValues) const {
    Quote out;
    const Trader* trader = find(id);
    if (trader == nullptr) {
        out.problem = "There is nobody to trade with.";
        return out;
    }
    if (refuses(attitude)) {
        out.refused = true;
        out.problem = "They will not trade with you.";
        return out;
    }
    const auto fail = [&](std::string problem) {
        if (out.problem.empty()) out.problem = std::move(problem);
    };
    for (const auto& [item, count] : deal.heroGets) {
        if (count <= 0) continue;
        if (!rareUnlocked(id, item, opinion, opinions)) fail(std::format("{} is kept for people they like better.", item));
        else if (count > stock(id, item)) fail(std::format("They have only {} of {}.", stock(id, item), item));
        out.givesMilli += count * heroPaysMilli(id, item, attitude, economy, itemValues);
    }
    for (const auto& [item, count] : deal.heroGives) {
        if (count <= 0) continue;
        if (economy.isCurrency(item)) fail("Coins are in your balance: pay from it.");
        out.receivesMilli += count * traderPaysMilli(id, item, attitude, economy, itemValues);
    }
    if (deal.balancePays < 0 || deal.balancePays > heroBalance_) fail(std::format("You have only {} in your balance.", heroBalance_));
    out.receivesMilli += static_cast<long long>(std::max(0, deal.balancePays)) * 1000;
    if (out.problem.empty() && out.receivesMilli == 0 && out.givesMilli == 0) fail("Offer something, and ask for something.");
    if (out.problem.empty() && deal.heroGets.empty() && deal.balancePays > 0 && deal.heroGives.empty()) fail("Ask for something in return for your money.");
    if (out.problem.empty() && out.receivesMilli < out.givesMilli) fail(std::format("They want {} more in value.", rules::formatMilli(out.givesMilli - out.receivesMilli)));
    if (out.problem.empty() && out.receivesMilli > out.givesMilli) {
        const long long surplus = (out.receivesMilli - out.givesMilli) / 1000;
        if (economy.hasCurrency()) out.surplusUnits = static_cast<int>(std::min<long long>(surplus, trader->purse));
        if (deal.heroGets.empty() && out.surplusUnits == 0) fail("They cannot pay you now: ask for goods, or come back tomorrow.");
    }
    out.acceptable = out.problem.empty();
    return out;
}

TradeMarket::Outcome TradeMarket::execute(int id, const Deal& deal, Attitude attitude, int opinion, const OpinionConfig& opinions, const RegionEconomy& economy, const ItemCounts& itemValues) {
    Outcome out;
    const Quote priced = quote(id, deal, attitude, opinion, opinions, economy, itemValues);
    if (!priced.acceptable) {
        out.message = priced.problem;
        return out;
    }
    Trader& trader = traders_.at(id);
    for (const auto& [item, count] : deal.heroGets) {
        if (count <= 0) continue;
        removeStock(id, item, count);
        nudge(id, item, count, true);
        out.heroGot[item] += count;
    }
    for (const auto& [item, count] : deal.heroGives) {
        if (count <= 0) continue;
        addStock(id, item, count);
        nudge(id, item, count, false);
        out.heroGave[item] += count;
    }
    heroBalance_ -= deal.balancePays;
    trader.purse += deal.balancePays;
    trader.purse -= priced.surplusUnits;
    heroBalance_ += priced.surplusUnits;
    out.balancePaid = deal.balancePays;
    out.balanceGained = priced.surplusUnits;
    out.done = true;
    out.message = "It is a deal.";
    return out;
}

int TradeMarket::addStock(int id, const std::string& item, int count) {
    const auto found = traders_.find(id);
    if (found == traders_.end() || count <= 0) return 0;
    found->second.stock[item] += count;
    return count;
}

int TradeMarket::removeStock(int id, const std::string& item, int count) {
    const auto found = traders_.find(id);
    if (found == traders_.end() || count <= 0) return 0;
    const auto have = found->second.stock.find(item);
    if (have == found->second.stock.end()) return 0;
    const int taken = std::min(count, have->second);
    have->second -= taken;
    if (have->second <= 0) found->second.stock.erase(have);
    return taken;
}

std::string TradeMarket::toText() const {
    json traders = json::array();
    const auto describe = [&](int id, const ItemCounts& stock, const ItemCounts& drift, int purse, std::int64_t restockDay, std::int64_t haggleDay, bool won) {
        json entry{{"id", id}, {"stock", tableJson(stock)}, {"restock", restockDay}, {"purse", purse}};
        if (!drift.empty()) entry["drift"] = tableJson(drift);
        if (haggleDay >= 0) {
            entry["haggle"] = haggleDay;
            entry["won"] = won;
        }
        traders.push_back(entry);
    };
    // Registered traders and the restored states that no trader has claimed yet, in the order of their ids.
    auto live = traders_.begin();
    auto saved = pending_.begin();
    while (live != traders_.end() || saved != pending_.end()) {
        if (saved == pending_.end() || (live != traders_.end() && live->first < saved->first)) {
            describe(live->first, live->second.stock, live->second.drift, live->second.purse, live->second.restockDay, live->second.haggleDay, live->second.haggleWon);
            ++live;
        } else {
            describe(saved->first, saved->second.stock, saved->second.drift, saved->second.purse, saved->second.restockDay, saved->second.haggleDay, saved->second.haggleWon);
            ++saved;
        }
    }
    json data{{"version", kSaveVersion}, {"balance", heroBalance_}, {"traders", traders}};
    return data.dump();
}

void TradeMarket::restoreState(std::string_view text) {
    json data;
    try {
        data = json::parse(text);
    } catch (const json::exception& error) {
        throw DataError("trade.json", "(file)", error.what());
    }
    if (!data.is_object() || !data.contains("version") || !data.at("version").is_number_integer()) throw DataError("trade.json", "version", "is missing");
    if (data.at("version").get<int>() > kSaveVersion) throw DataError("trade.json", "version", "was saved by a newer version of the game");
    if (!data.contains("traders") || !data.at("traders").is_array()) throw DataError("trade.json", "traders", "must be a list");
    if (data.contains("balance") && data.at("balance").is_number_integer()) heroBalance_ = std::max(0, data.at("balance").get<int>());
    for (const json& entry : data.at("traders")) {
        if (!entry.is_object() || !entry.contains("id") || !entry.at("id").is_number_integer() || !entry.contains("stock") || !entry.contains("restock")) {
            throw DataError("trade.json", "traders", "an entry needs id, stock and restock");
        }
        SavedState state;
        state.stock = tableFrom(entry.at("stock"), "stock");
        if (entry.contains("drift")) state.drift = tableFrom(entry.at("drift"), "drift");
        state.restockDay = entry.at("restock").get<std::int64_t>();
        state.purse = entry.value("purse", config_.startPurse);
        if (entry.contains("haggle")) {
            state.haggleDay = entry.at("haggle").get<std::int64_t>();
            state.haggleWon = entry.value("won", false);
        }
        const int id = entry.at("id").get<int>();
        if (const auto live = traders_.find(id); live != traders_.end()) {
            live->second.stock = state.stock; // already registered: the saved state is applied at once
            live->second.drift = state.drift;
            live->second.purse = state.purse;
            live->second.restockDay = state.restockDay;
            live->second.haggleDay = state.haggleDay;
            live->second.haggleWon = state.haggleWon;
        } else {
            pending_[id] = std::move(state);
        }
    }
}

std::uint64_t TradeMarket::hash() const {
    std::uint64_t h = 0xCBF29CE484222325ULL;
    mix(h, seed_);
    mix(h, static_cast<std::uint64_t>(heroBalance_));
    mix(h, traders_.size());
    for (const auto& [id, trader] : traders_) {
        mix(h, static_cast<std::uint64_t>(id));
        for (const auto& [item, count] : trader.stock) {
            mixText(h, item);
            mix(h, static_cast<std::uint64_t>(count));
        }
        for (const auto& [item, percent] : trader.drift) {
            mixText(h, item);
            mix(h, static_cast<std::uint64_t>(static_cast<std::int64_t>(percent)));
        }
        mix(h, static_cast<std::uint64_t>(trader.purse));
        mix(h, static_cast<std::uint64_t>(trader.restockDay));
        mix(h, static_cast<std::uint64_t>(trader.haggleDay));
        mix(h, trader.haggleWon ? 1ULL : 0ULL);
    }
    return h;
}

} // namespace odysseus::sim
