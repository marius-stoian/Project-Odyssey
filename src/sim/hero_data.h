#pragma once

#include "boundary.h"

#include <array>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace odysseus::sim {

// The hero's life (M5): the content that drives it, read from assets/data/hero/ (Charter rule 7). Everything is whole numbers;
// percentages are written as integers (a multiplier of 3.00 is 300).

// What the hero leans toward. The first five are the professions, in the order of professions.json; Trade and Religion are
// the two pillars a clan can lead a region by.
enum class Affinity { Hunter, Gatherer, Knapper, FireKeeper, Shaman, Trade, Religion, Count };
inline constexpr std::size_t kAffinityCount = static_cast<std::size_t>(Affinity::Count);
inline constexpr std::size_t kProfessionCount = 5;
const char* affinityName(Affinity affinity);          // "Hunter", "Trade", ...
std::optional<Affinity> affinityFromKey(const std::string& key); // "hunter", "fireKeeper", ...
const char* affinityKey(Affinity affinity);                      // the same key the other way round

using AffinityValues = std::array<int, kAffinityCount>;
using SkillValues = std::array<int, kProfessionCount>;

struct ImprintConfig {
    int startAge = 12;
    int peakPercent = 300;
    int midAge = 19;
    int midPercent = 175;
    int endAge = 26;
    int endPercent = 50;
    int settledPercent = 25; // after the end age
};

struct PresetConfig {
    std::string name;
    int startAge = 12;
    int mantleAge = 26;
};

struct ComfortConfig {
    std::string name;
    int needsPercent = 100; // how fast needs fall
    int foodPercent = 100;  // the starting store
};

struct AgingConfig {
    int fromYears = 40;
    int startPercent = 10;   // extra Energy decay at that age
    int perYearPercent = 2;  // and each year after
};

struct OriginConfig {
    std::string name;
    AffinityValues affinity{};
};

struct DominionConfig {
    int tradePerPerson = 3;
    int rivalTradePerPerson = 10;
    int rivalTradeGrowthPerDay = 1;
    int winPercent = 60;
    int combinedWinPercent = 50;
    int loseBelowPeople = 3;
    int rivalFollowerPercent = 60;
};

struct TradeConfig {
    int dueDays = 14;
    int maxDebts = 3;
    int defaultRelationshipPenalty = 30;
    int defaultTradePenaltyPercent = 50;
    int relationshipPerDeal = 3;
    int needPercent = 130;
};

struct FireConfig {
    int skillLevelToFound = 3;
    int untendedDaysBeforeOut = 5;
    int ritualMinimumAttendees = 5;
    int ritualFaith = 12;
    int followerFaith = 30;
    int neglectFaithLoss = 10;
    int conversionPerMille = 120;
    int ritualRadiusTiles = 4;
};

struct HeroConfig {
    ImprintConfig imprint;
    std::vector<PresetConfig> presets;
    std::vector<ComfortConfig> comforts;
    AgingConfig aging;
    std::vector<OriginConfig> origins;
    DominionConfig dominion;
    TradeConfig trade;
    FireConfig fire;
    int skillLevelPoints = 10;
    int masterBonusPercent = 100;
    int trustOpinion = 25;
};

// A way to spend a year of youth (US-051): base gains, before the imprint multiplier.
struct Activity {
    std::string id;
    std::string name;
    std::string text;
    AffinityValues affinity{};
    SkillValues skill{};
};

struct CrossroadsOption {
    std::string text;
    AffinityValues affinity{};
    int opinion = 0;          // what the person the event involves now thinks of the hero
    std::string trait;        // "brave", "kind", ...; empty = none
    std::string note;         // the chronicle sentence, with {hero} and {other}
};

struct CrossroadsEvent {
    std::string id;
    std::string title;
    std::string text;
    int minAge = 12;
    int maxAge = 25;
    std::optional<Affinity> needsAffinity;
    int needsMinimum = 0;
    std::string role;         // "elder" or "friend": who in the clan the event involves
    int order = 0;            // where it stands among the events (the draw of a year depends on the order, so it is part of the data)
    std::string trigger;      // a condition in the rule language; empty: always possible (US-185)
    std::vector<CrossroadsOption> options;
};

struct Profession {
    std::string id;
    std::string name;
    std::string skill;
    std::vector<std::string> tools;
    std::vector<std::string> actions;
    std::string pillar;
};

struct Item {
    std::string id;
    std::string name;
    std::string kind; // material, tool, good, food
    int value = 1;
};

struct Recipe {
    std::string id;
    std::string name;
    std::string station; // "knapping-stone" or "fire"
    int profession = 0;  // index of the profession whose skill the crafting trains
    std::vector<std::pair<std::string, int>> inputs;
    std::vector<std::string> tools; // needed, not used up
    std::string output;
    int count = 1;
};

struct HeroData {
    HeroConfig config;
    std::vector<Activity> activities;
    std::vector<CrossroadsEvent> events;
    std::vector<Profession> professions;
    std::vector<Item> items;
    std::vector<Recipe> recipes;

    const Item* item(const std::string& id) const;
    const Profession* profession(const std::string& id) const;
    int professionIndex(const std::string& id) const; // -1 when unknown
    const Recipe* recipe(const std::string& id) const;
};

// Reads every file of assets/data/hero/ and the rules file rules/<rulesName>.json (US-195). Any problem is a DataError naming the file and the field (a profession that needs a
// tool that no item describes names the profession and the tool).
HeroData loadHeroData(const std::filesystem::path& dataDirectory, const std::string& rulesName = "standard");

// One story event as the Editor reads and writes it (US-185): the text of story/events/<id>.json. A mistake comes back in `problem`.
std::optional<CrossroadsEvent> parseStoryEvent(const std::string& jsonText, const std::string& name, std::string& problem);
std::string writeStoryEvent(const CrossroadsEvent& event);

} // namespace odysseus::sim
