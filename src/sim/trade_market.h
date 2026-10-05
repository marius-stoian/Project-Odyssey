#pragma once

#include "boundary.h"

#include "sim/economy.h"
#include "sim/npc_extras.h"
#include "sim/opinion.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace odysseus::sim {

// Every number of the trade rules, from assets/data/sim/trade.json (D-54 Q3, ADR-023). Whole numbers only; percents are whole percents.
struct PriceConfig {
    // Stock (US-281).
    int minimumCap = 10;     // a trader never restocks a good beyond max(its starting stock * capFactor, this)
    int capFactor = 2;
    int defaultTarget = 6;   // the target of a good the trader's profile does not stock
    int deliveryAmount = 1;  // pieces one weighted pick delivers
    int catchUpDays = 7;     // after a long absence at most this many days of deliveries arrive at once
    // The stock-ratio curve (US-282): the price follows target / stock, clamped.
    int curveMinPercent = 50;
    int curveMaxPercent = 200;
    // The drift (US-282): each trade nudges the price of that good, and it decays back every day.
    int driftPerTrade = 3;
    int driftMaxPercent = 40;
    int driftDecayPercent = 25;
    // Reputation (US-282): the percent added to what the hero pays, by the trader's attitude to the hero; some attitudes refuse to trade at all.
    std::array<int, kAttitudeCount> reputation{-10, 0, 25, 0, 10, 25, -20, -20, 10}; // in the order of the Attitude enum
    std::array<bool, kAttitudeCount> refuses{false, false, false, true, false, false, false, false, false};
    // What a trader pays for the hero's goods (D-52 Q-17): its wants at full value, any other good at half.
    int wantPercent = 100;
    int otherPercent = 50;
    // Haggle (US-283): chance = clamp(base + opinion / opinionDivisor + persuasion * perPersuasion, min, max) percent.
    int haggleBase = 20;
    int haggleOpinionDivisor = 4;
    int hagglePerPersuasion = 3;
    int haggleMin = 5;
    int haggleMax = 85;
    int haggleDiscountPercent = 10;
    int haggleFailureOpinion = 5;
    // The purse (US-283): the money a trader can pay out in a currency region, in value units. It starts at startPurse, grows by purseRestock a day up to purseCap, takes
    // in what the hero pays from his balance and pays out the surplus of a deal.
    int startPurse = 30;
    int purseRestock = 5;
    int purseCap = 60;
};

PriceConfig loadPriceConfig(const std::filesystem::path& file);

// The stock of the traders (US-281): placed NPCs with a trade profile (and, from US-283, the hero's balance). Limited stock, a delivery every day (fixed pieces and
// weighted random picks from the trader's goods and the region's), what each trader wants, saved with the game. Deterministic: ordered maps, integers, and a random stream
// per (seed, day, trader) so the same days always bring the same goods (Charter rule 6). Far traders restock here too, in the one daily pass, whatever their distance
// from the hero (US-263): the cost is the number of traders, not of persons.
class TradeMarket {
public:
    struct Trader {
        rules::TradeProfile profile; // data: read again at every start, not saved
        ItemCounts stock;
        ItemCounts drift;            // US-282: item -> percent, signed
        int purse = 0;               // US-283: what it can pay out in a currency region, in value units
        std::int64_t restockDay = 0; // the last day whose delivery has arrived
        std::int64_t haggleDay = -1; // US-283: the day of the last try, -1 never
        bool haggleWon = false;      // that try succeeded: its discount holds for that day
    };

    explicit TradeMarket(PriceConfig config = {}, std::uint64_t seed = 0) : config_(config), seed_(seed) {}

    const PriceConfig& config() const { return config_; }
    void setConfig(const PriceConfig& config) { config_ = config; }
    void setSeed(std::uint64_t seed) { seed_ = seed; }
    std::uint64_t seed() const { return seed_; }

    // Registers a trader: a placed NPC with a trade profile. It starts with the profile's stock unless a saved state for the id was restored; `today` is the day it
    // joins (its first delivery is tomorrow). False when the id is already a trader (its profile is replaced, its stock kept).
    bool addTrader(int id, const rules::TradeProfile& profile, std::int64_t today);
    bool isTrader(int id) const { return traders_.count(id) != 0; }
    const Trader* find(int id) const;
    std::size_t traderCount() const { return traders_.size(); }

    int stock(int id, const std::string& item) const;
    // What the trader aims to keep of a good: its starting stock, or `defaultTarget` for a good it does not stock.
    int target(int id, const std::string& item) const;
    // The most a restock brings a good up to.
    int cap(int id, const std::string& item) const;
    bool wants(int id, const std::string& item) const;
    // What the trader pays for the hero's good, in percent of its price: wantPercent for a want, otherPercent for the rest.
    int wantPercent(int id, const std::string& item) const;

    // ---- Prices (US-282, ADR-023): whole numbers; a price is in thousandths ("milli") of a value unit, so a bundle is added up exactly and rounded once.
    // The price the region market asks for a good before the stock, the drift and the attitude: the region prices table, else the item value, at least 1 (value units).
    static int basePrice(const RegionEconomy& economy, const ItemCounts& itemValues, const std::string& item);
    // 100 * target / max(stock, 1), clamped to the curve: 100 at the target, 200 for an empty shelf, 50 for a shelf at twice the target.
    int stockRatioPercent(int id, const std::string& item) const;
    // The price of one piece after the stock curve and the drift, before the attitude. A currency item is worth its value anywhere: never repriced.
    long long marketMilli(int id, const std::string& item, const RegionEconomy& economy, const ItemCounts& itemValues) const;
    // What the hero pays for one piece, and what the trader pays the hero for one piece (a want at wantPercent, any other good at otherPercent), with the attitude of the
    // trader to the hero applied last. The prices of an id that is not a trader are 0.
    long long heroPaysMilli(int id, const std::string& item, Attitude attitude, const RegionEconomy& economy, const ItemCounts& itemValues) const;
    long long traderPaysMilli(int id, const std::string& item, Attitude attitude, const RegionEconomy& economy, const ItemCounts& itemValues) const;
    static long long toUnits(long long milli) { return (milli + 500) / 1000; } // rounded half up, for showing
    // Whether this attitude refuses to trade at all (hostile), and the percent it adds to what the hero pays.
    bool refuses(Attitude attitude) const { return config_.refuses[static_cast<std::size_t>(attitude)]; }
    int reputationPercent(Attitude attitude) const { return config_.reputation[static_cast<std::size_t>(attitude)]; }
    // The drift of a good (percent, signed): the hero buying raises it, selling lowers it, each piece by driftPerTrade, never beyond driftMaxPercent either way; it decays daily.
    int drift(int id, const std::string& item) const;
    void nudge(int id, const std::string& item, int pieces, bool heroBuys);
    // Rare goods (D-54 Q4): offered only at an opinion at least as high as the band word of the rare table of the trader.
    bool isRare(int id, const std::string& item) const;
    bool rareUnlocked(int id, const std::string& item, int opinion, const OpinionConfig& opinions) const;
    // The goods the trader offers the hero now (in stock, and rare ones only when unlocked), and the rare goods it has in stock but keeps back, both in item order.
    std::vector<std::string> offeredGoods(int id, int opinion, const OpinionConfig& opinions) const;
    std::vector<std::string> lockedGoods(int id, int opinion, const OpinionConfig& opinions) const;
    // The band word a rare good needs ("friendly"), or empty.
    std::string rareWord(int id, const std::string& item) const;

    // ---- Trading (US-283, D-54 Q1-Q5): a deal is what the hero hands over, what he takes, and how much of his balance he pays. The market checks it against the stock,
    // the prices and the attitude, and moves the stock, the drift and the money; the caller moves the hero's bag.
    struct Deal {
        ItemCounts heroGives; // goods the hero hands over (never coins: coins are in the balance)
        ItemCounts heroGets;  // goods the hero takes from the stock of the trader
        int balancePays = 0;  // units of the hero's balance paid into the deal
    };
    struct Quote {
        bool refused = false;        // the trader will not trade with this hero at all (hostile)
        long long receivesMilli = 0; // what the trader receives: the hero's goods at what it pays, plus the balance
        long long givesMilli = 0;    // what the trader gives: its goods at what the hero pays
        int surplusUnits = 0;        // the balance the hero gets back for what he gave beyond what he took (a currency region and a purse that can pay)
        bool acceptable = false;
        std::string problem;         // the first thing wrong, in plain words; empty when acceptable
    };
    struct Outcome {
        bool done = false;
        std::string message;
        ItemCounts heroGave; // what left the hero's bag
        ItemCounts heroGot;  // what came into it
        int balancePaid = 0;
        int balanceGained = 0;
    };
    // The live balance bar: received minus given. execute() does the deal when the quote is acceptable (received at least given).
    Quote quote(int id, const Deal& deal, Attitude attitude, int opinion, const OpinionConfig& opinions, const RegionEconomy& economy, const ItemCounts& itemValues) const;
    Outcome execute(int id, const Deal& deal, Attitude attitude, int opinion, const OpinionConfig& opinions, const RegionEconomy& economy, const ItemCounts& itemValues);
    // The hero's balance: the value of the coins he brought to the trade screen, kept here so that it survives a save (US-283).
    int heroBalance() const { return heroBalance_; }
    void setHeroBalance(int units) { heroBalance_ = std::max(0, units); }
    int purse(int id) const;
    std::int64_t today() const { return today_; }

    // Haggle (D-54 Q5): one try per trader per in-game day, a seeded roll of the world seed, the trader and the day; a win is a discount for the rest of the day, a loss costs
    // opinion. `persuasion` is the hero's Trade affinity divided by 10.
    struct Haggle {
        bool tried = false;     // false: nothing was rolled (already tried today, or not a trader)
        bool won = false;
        int chance = 0;         // percent
        int roll = 0;           // 0..99; a win is a roll below the chance
        int opinionChange = 0;  // what the caller adds to the opinion of the trader of the hero (negative after a loss)
        std::string message;
    };
    int haggleChance(int opinion, int persuasion) const;
    bool canHaggle(int id) const;
    Haggle haggle(int id, int opinion, int persuasion);
    int discountPercent(int id) const; // the percent a won haggle takes off today, else 0

    // A new day has begun: every trader gets the deliveries of the days since its last one (at most `catchUpDays`). Calling it twice for the same day changes nothing.
    void dailyUpdate(std::int64_t today, const RegionEconomy& economy);

    // Stock moves with trades (US-283). Both return what really moved.
    int addStock(int id, const std::string& item, int count);
    int removeStock(int id, const std::string& item, int count);

    // The saved state (versioned JSON; the data of the profiles is read again at every start) and its reader: `restoreState` keeps the states until the traders are
    // registered with addTrader. A damaged text is a DataError.
    std::string toText() const;
    // `skip` lists persons whose saved state is not wanted (the level changed or removed them: US-305).
    void restoreState(std::string_view text, const std::set<int>& skip = {});
    std::uint64_t hash() const;

    static constexpr int kSaveVersion = 1;

private:
    struct SavedState {
        ItemCounts stock;
        ItemCounts drift;
        int purse = 0;
        std::int64_t restockDay = 0;
        std::int64_t haggleDay = -1;
        bool haggleWon = false;
    };
    void deliver(int id, Trader& trader, std::int64_t day, const RegionEconomy& economy);
    int addUpToCap(int id, Trader& trader, const std::string& item, int count);

    PriceConfig config_;
    std::uint64_t seed_;
    std::map<int, Trader> traders_;
    std::map<int, SavedState> pending_; // restored states of traders that are not registered (yet): they are kept in the next save
    int heroBalance_ = 0;
    std::int64_t today_ = 0;
};

} // namespace odysseus::sim
