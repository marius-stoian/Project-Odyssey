#pragma once

#include "boundary.h"

#include "hero_data.h"
#include "world.h"

#include "core/random.h"

#include <functional>
#include <array>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace odysseus::sim {

// The hero's run (M5, US-050..US-073): one person of the clan is the player's hero. A run begins in the Growing Period (ages 12 to
// 26: each year the player picks two activities and faces a crossroads), ends that stage at the Mantle moment with a specialty,
// and then plays on in free time with professions, crafting, trade and a sacred fire, until the clan leads the region, falls
// apart, or the hero dies. All of it is plain data and whole numbers in the Simulation layer, headless and testable; the
// game's screens only show it and ask for choices.

enum class Phase { Growing, Free, Ended };
enum class Outcome { None, Victory, Defeat, Died };

// Seed, Growing Period preset (index into the presets) and Comfort level (index into the comforts).
struct NewGame {
    std::uint64_t seed = 1;
    int preset = 0;
    int comfort = 1;
    std::string rules; // the rules file the run plays under (US-195); empty is "standard"
};

// The clan's world rules for a Comfort level: needs fall faster when harsh, the store starts smaller.
SimConfig configForComfort(const HeroData& data, SimConfig base, int comfort);

struct CraftedItem {
    std::string item;
    int quality = 0;                   // 0 Crude, 1 Fair, 2 Fine, 3 Masterwork
    std::string maker;
    std::int64_t day = 0;
    std::vector<std::string> materials;
    int year = 1;
    Season season = Season::Spring;
};
const char* qualityName(int quality);

struct ActionResult {
    bool ok = false;
    std::string message;
};

// Somebody the hero can barter with: a rival clan, as the run sees it.
struct RivalSummary {
    std::string name;
    int population = 0;
};

using Goods = std::vector<std::pair<std::string, int>>; // item id and count

struct BarterOffer {
    Goods give;              // what the hero hands over (now, or by the due day)
    Goods want;              // what the hero asks for
    bool payLater = false;   // take the goods now and pay within the due days
};

struct BarterResult {
    bool accepted = false;
    std::optional<BarterOffer> counter; // when they refuse: what they would accept
    std::string message;
};

struct Debt {
    int rival = 0;
    Goods owe;
    int value = 0;
    std::int64_t dueDay = 0;
    bool settled = false;
    bool defaulted = false;
};

struct SacredFire {
    bool founded = false;
    bool lit = false;
    std::string name;
    int tileX = 0;
    int tileY = 0;
    int daysUntended = 0;
};

struct RunSummary {
    Outcome outcome = Outcome::None;
    std::string reason;
    std::string heroName;
    int ageYears = 0;
    std::vector<std::string> specialty;
    int tradePercent = 0;
    int religionPercent = 0;
    std::vector<std::string> highlights; // the chronicle lines that matter most to this life
};

class HeroLife {
public:
    // Starts a run in `world`: picks the hero from the clan and brings them to the preset's starting age.
    // `data` and `world` must outlive the HeroLife.
    HeroLife(const HeroData& data, World& world, NewGame game);

    const HeroData& data() const { return *data_; }
    World& world() { return *world_; }
    const World& world() const { return *world_; }
    int personId() const { return personId_; }
    const Person& person() const { return world_->people().at(static_cast<std::size_t>(personId_)); }
    std::string name() const { return person().name; }
    int ageYears() const { return person().ageYears(world_->calendar().daysPerYear()); }
    const NewGame& game() const { return game_; }
    const PresetConfig& preset() const { return data_->config.presets.at(static_cast<std::size_t>(game_.preset)); }
    Phase phase() const { return phase_; }
    Outcome outcome() const { return outcome_; }
    const std::string& origin() const { return origin_; }

    // The imprint multiplier at an age, in percent (US-053): 300, 175 and 50 at 12, 19 and 26; 25 after 26; straight lines between.
    int imprintPercent(int ageYears) const;

    // ---- the Growing Period (US-051, US-052, US-054)
    const AffinityValues& affinities() const { return affinity_; }
    // Scripted starts and tests: set an affinity, a skill, or the trade standing directly.
    void setAffinity(Affinity affinity, int value) { affinity_[static_cast<std::size_t>(affinity)] = value; }
    void setSkillPoints(int profession, int points) { skill_.at(static_cast<std::size_t>(profession)) = points; }
    void addTradePoints(int points) { tradePoints_ += points; }
    const SkillValues& skills() const { return skill_; }
    const std::vector<Activity>& activities() const { return data_->activities; }
    // Chooses this year's two activities (indexes into activities(), different); false when the choice is not valid.
    bool chooseFocus(int first, int second);
    // Lives the year: the two activities teach (gains times the imprint of the age the year began at), the clan's world runs a
    // whole year, and one crossroads event may come up. Returns the event, or nullptr when none matches (or the hero died).
    const CrossroadsEvent* liveYear();
    const CrossroadsEvent* pendingEvent() const { return pendingEvent_ >= 0 ? &data_->events.at(static_cast<std::size_t>(pendingEvent_)) : nullptr; }
    // Applies an option's effects (affinities times the imprint, the opinion of the person involved, a trait) and records it in
    // the chronicle. At the mantle age the Growing Period ends by itself.
    bool resolveEvent(int option);
    // Who in the clan an event involves (the role of the event), or -1.
    int rolePerson(const std::string& role) const;
    // The Mantle moment: the top affinity (and the second when it is within 15 percent, or tied) of the five professions become
    // the specialty. Runs by itself when the hero reaches the mantle age; also when the preset is Off.
    const std::vector<int>& specialty() const { return specialty_; }
    std::vector<std::string> specialtyNames() const;
    void mantle();
    const std::string& lastNote() const { return lastNote_; }

    // ---- professions, skills and things (US-060..US-063)
    int skillPoints(int profession) const { return skill_.at(static_cast<std::size_t>(profession)); }
    int skillLevel(int profession) const { return skillPoints(profession) / data_->config.skillLevelPoints; }
    // A success or a try gives experience; a master beside the hero doubles it (masterBonusPercent). Returns the points gained.
    int useSkill(int profession, bool success);
    int count(const std::string& item) const;
    void give(const std::string& item, int count);
    // Told whenever items enter the bag (US-181): what, how many, and whether they were just crafted. Quests count gathering and crafting from it.
    // Decides a story event's `trigger` (US-185): the game answers it in the rule language. Without one every trigger counts as true.
    using TriggerCheck = std::function<bool(const std::string& condition)>;
    void setEventTrigger(TriggerCheck check) { eventTrigger_ = std::move(check); }
    using ItemObserver = std::function<void(const std::string& item, int amount, bool crafted)>;
    void setItemObserver(ItemObserver observer) { itemObserver_ = std::move(observer); }
    bool take(const std::string& item, int count);
    const std::map<std::string, int>& inventory() const { return inventory_; }
    bool hasToolFor(int profession) const;
    ActionResult gatherBerries();
    ActionResult pickFlint();
    ActionResult knapFlint();       // needs a hammerstone
    ActionResult chopWood();
    ActionResult gatherHerbs();
    // Why a recipe cannot be made now (empty: it can): missing materials or tools, named.
    std::string craftBlockedReason(const Recipe& recipe) const;
    ActionResult craft(const std::string& recipeId);
    const std::vector<CraftedItem>& crafted() const { return crafted_; }
    std::string describe(const CraftedItem& item) const;

    // ---- apprenticeship (US-063)
    int masterOf(int profession) const;      // the clan member who teaches this profession, or -1
    int apprenticeOf(int profession) const { return master_.at(static_cast<std::size_t>(profession)); }
    // Asks the master of a profession to teach the hero: refused, with the reason, unless they trust the hero enough.
    ActionResult askToApprentice(int profession);
    ActionResult talkTo(int person);          // the clan member talks with the hero: they like each other more
    ActionResult giveBerriesTo(int person);   // a gift of berries

    // ---- dominion, trade and the sacred fire (US-070..US-072)
    void setRivals(std::vector<RivalSummary> rivals);
    const std::vector<RivalSummary>& rivals() const { return rivals_; }
    int tradePercent() const;
    int religionPercent() const;
    int tradePoints() const { return tradePoints_; }
    int followers() const;
    int relation(int rival) const { return rival >= 0 && static_cast<std::size_t>(rival) < relations_.size() ? relations_[static_cast<std::size_t>(rival)] : 0; }
    // What each rival wants (the goods it pays more for).
    bool rivalNeeds(int rival, const std::string& item) const;
    BarterResult barter(int rival, const BarterOffer& offer);
    const std::vector<Debt>& debts() const { return debts_; }
    // The owner's setup of the world (US-206): the store holds exactly `count` of an item (false: not an item of the game), and a debt to rival `rival` is on the books,
    // due in `days` days. Neither is a trade, so no quest event, tradePoints or message follows.
    bool stockItem(const std::string& item, int count);
    void addDebt(int rival, Goods owe, int value, int days);
    ActionResult payDebt(std::size_t index);
    std::string foundFireBlockedReason() const;
    ActionResult foundFire(const std::string& name, int tileX, int tileY);
    ActionResult holdRitual(const std::vector<int>& attendees);
    ActionResult tendFire();
    // The first day (US-090): eat a handful of berries (the hero's hunger is fed), and warm yourself and the clan at the camp fire.
    ActionResult eatBerries();
    ActionResult tendCampFire();
    const SacredFire& fire() const { return fire_; }
    int faith(int person) const;

    // ---- the days (US-055, US-070, US-073)
    // Call every game tick: when a day has ended in the world, the run does its daily work (aging, the fire, debts, dominion,
    // the end of the run).
    void update();
    // The run's end so far: the outcome, and the summary for the end screen.
    RunSummary summary() const;
    const std::vector<std::string>& news() const { return news_; }
    // Extra Energy decay a hero of this age suffers, in percent (US-055).
    int agingPercent(int ageYears) const;

    // ---- saving the run (US-080): one JSON file next to the clan's world, written safely with backups
    void save(const std::filesystem::path& file) const;
    static HeroLife load(const HeroData& data, World& world, const std::filesystem::path& file);
    // The rules a saved run plays under ("" for a save from before US-195, or one that cannot be read): the hero data must be read under them before the run is loaded.
    static std::string savedRules(const std::filesystem::path& file);

private:
    struct RestoreTag {};
    HeroLife(const HeroData& data, World& world, RestoreTag) : data_(&data), world_(&world), rng_(1, 61) {}

    ItemObserver itemObserver_;
    TriggerCheck eventTrigger_;
    bool crafting_ = false;

    void dayEnded();
    void end(Outcome outcome, const std::string& reason);
    void say(const std::string& line);
    int valueOf(const Goods& goods, int rival, bool asGiven) const;
    int unitValue(const std::string& item) const;
    void addAffinity(const AffinityValues& gains, int percent);
    void pickHero();

    const HeroData* data_;
    World* world_;
    NewGame game_;
    core::Pcg32 rng_;
    int personId_ = 0;
    std::string origin_;
    Phase phase_ = Phase::Growing;
    Outcome outcome_ = Outcome::None;
    std::string reason_;
    AffinityValues affinity_{};
    SkillValues skill_{};
    int focusFirst_ = -1;
    int focusSecond_ = -1;
    int pendingEvent_ = -1;
    int pendingAge_ = 12;                 // the age the pending event's year began at
    std::set<std::string> seen_;          // crossroads events already met
    std::vector<int> specialty_;
    std::string lastNote_;
    std::map<std::string, int> inventory_;
    std::vector<CraftedItem> crafted_;
    std::array<int, kProfessionCount> master_{{-1, -1, -1, -1, -1}};
    std::vector<RivalSummary> rivals_;
    std::vector<int> relations_;
    std::vector<int> converts_;
    int tradePoints_ = 0;
    std::vector<Debt> debts_;
    SacredFire fire_;
    std::map<int, int> faith_;
    std::int64_t lastDay_ = 0;
    std::vector<std::string> news_;
};

} // namespace odysseus::sim
