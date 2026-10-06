#include "hero_life.h"

#include "core/text.h"

#include "json_data.h"
#include "save.h"

#include <algorithm>
#include <format>

namespace odysseus::sim {

namespace {

using nlohmann::json;

constexpr int kMaxAffinity = 100;
constexpr int kMaxSkill = 1000;
constexpr int kApprenticePointsPerDay = 2;

std::optional<Trait> traitFromName(const std::string& name) {
    if (name == "brave") return Trait::Brave;
    if (name == "timid") return Trait::Timid;
    if (name == "kind") return Trait::Kind;
    if (name == "greedy") return Trait::Greedy;
    if (name == "talkative") return Trait::Talkative;
    if (name == "diligent") return Trait::Diligent;
    return std::nullopt;
}

std::string goodsText(const HeroData& data, const Goods& goods) {
    std::string out;
    for (const auto& [id, count] : goods) {
        const Item* item = data.item(id);
        out += std::format("{}{} x {}", out.empty() ? "" : ", ", count, item != nullptr ? item->name : id);
    }
    return out;
}

} // namespace

const char* qualityName(int quality) {
    static constexpr const char* kNames[] = {"crude", "fair", "fine", "masterwork"};
    return kNames[std::clamp(quality, 0, 3)];
}

SimConfig configForComfort(const HeroData& data, SimConfig base, int comfort) {
    const ComfortConfig& level = data.config.comforts.at(static_cast<std::size_t>(std::clamp(comfort, 0, static_cast<int>(data.config.comforts.size()) - 1)));
    for (int& decay : base.needs.dailyDecay) decay = std::max(1, decay * level.needsPercent / 100);
    base.needs.winterWarmthDecay = std::max(1, base.needs.winterWarmthDecay * level.needsPercent / 100);
    base.clan.startingFood = std::max(0, base.clan.startingFood * level.foodPercent / 100);
    return base;
}

HeroLife::HeroLife(const HeroData& data, World& world, NewGame game) : data_(&data), world_(&world), game_(game), rng_(game.seed, 61) {
    game_.preset = std::clamp(game.preset, 0, static_cast<int>(data.config.presets.size()) - 1);
    game_.comfort = std::clamp(game.comfort, 0, static_cast<int>(data.config.comforts.size()) - 1);
    pickHero();
    const OriginConfig& origin = data.config.origins.at(rng_.below(static_cast<std::uint32_t>(data.config.origins.size())));
    origin_ = origin.name;
    const bool off = preset().mantleAge <= preset().startAge;
    for (std::size_t i = 0; i < kAffinityCount; ++i) {
        // A youth starts with half of what the family gives; an adult of the Off preset with all of it and a little luck.
        affinity_[i] = off ? std::min(kMaxAffinity, origin.affinity[i] + static_cast<int>(rng_.below(11))) : origin.affinity[i] / 2;
    }
    for (std::size_t p = 0; p < kProfessionCount; ++p) skill_[p] = off ? affinity_[p] / 2 : 0;
    relations_.clear();
    lastDay_ = world_->date().day;
    world_->note(std::format("{}, {}, is the clan's hero: {} years old.", name(), origin_, ageYears()), kImportanceHero, EventKind::Hero, personId_);
    if (off) {
        mantle();
    } else {
        phase_ = Phase::Growing;
    }
}

void HeroLife::pickHero() {
    std::vector<int> candidates;
    for (const Person& person : world_->people()) {
        if (person.alive && !person.exiled) candidates.push_back(person.id);
    }
    personId_ = candidates.empty() ? 0 : candidates[rng_.below(static_cast<std::uint32_t>(candidates.size()))];
    if (Person* hero = world_->personMutable(personId_)) {
        hero->ageDays = preset().startAge * world_->calendar().daysPerYear() + static_cast<int>(rng_.below(static_cast<std::uint32_t>(world_->calendar().daysPerYear())));
        hero->partner = -1;
        hero->pregnantDays = 0;
    }
}

int HeroLife::imprintPercent(int ageYears) const {
    const ImprintConfig& c = data_->config.imprint;
    if (ageYears > c.endAge) return c.settledPercent;
    if (ageYears <= c.startAge) return c.peakPercent;
    if (ageYears <= c.midAge) return c.peakPercent + (c.midPercent - c.peakPercent) * (ageYears - c.startAge) / std::max(1, c.midAge - c.startAge);
    return c.midPercent + (c.endPercent - c.midPercent) * (ageYears - c.midAge) / std::max(1, c.endAge - c.midAge);
}

void HeroLife::say(const std::string& line) {
    news_.push_back(line);
    if (news_.size() > 40) news_.erase(news_.begin());
}

void HeroLife::addAffinity(const AffinityValues& gains, int percent) {
    for (std::size_t i = 0; i < kAffinityCount; ++i) {
        if (gains[i] == 0) continue;
        const int scaled = gains[i] > 0 ? (gains[i] * percent + 99) / 100 : (gains[i] * percent - 99) / 100;
        affinity_[i] = std::clamp(affinity_[i] + scaled, 0, kMaxAffinity);
    }
}

bool HeroLife::chooseFocus(int first, int second) {
    const int n = static_cast<int>(data_->activities.size());
    if (phase_ != Phase::Growing || first < 0 || second < 0 || first >= n || second >= n || first == second) return false;
    focusFirst_ = first;
    focusSecond_ = second;
    return true;
}

int HeroLife::rolePerson(const std::string& role) const {
    const int adultYears = world_->config().life.adultAgeYears;
    const int daysPerYear = world_->calendar().daysPerYear();
    int best = -1;
    for (const Person& other : world_->people()) {
        if (!other.alive || other.exiled || other.id == personId_) continue;
        if (role == "elder") {
            if (other.ageYears(daysPerYear) < adultYears) continue;
            if (best < 0 || other.ageDays > world_->people()[static_cast<std::size_t>(best)].ageDays) best = other.id;
        } else {
            const int mine = person().ageDays;
            if (best < 0 || std::abs(other.ageDays - mine) < std::abs(world_->people()[static_cast<std::size_t>(best)].ageDays - mine)) best = other.id;
        }
    }
    return best;
}

const CrossroadsEvent* HeroLife::liveYear() {
    if (phase_ != Phase::Growing || focusFirst_ < 0) return nullptr;
    const int age = ageYears();
    const int imprint = imprintPercent(age);
    for (const int index : {focusFirst_, focusSecond_}) {
        const Activity& activity = data_->activities[static_cast<std::size_t>(index)];
        addAffinity(activity.affinity, imprint);
        for (std::size_t p = 0; p < kProfessionCount; ++p) skill_[p] = std::min(kMaxSkill, skill_[p] + (activity.skill[p] * imprint + 99) / 100);
    }
    pendingAge_ = age;
    focusFirst_ = focusSecond_ = -1;
    // The year passes for the whole clan.
    world_->runTicks(world_->calendar().ticksPerYear());
    lastDay_ = world_->date().day;
    if (!person().alive) {
        end(Outcome::Defeat, std::format("{} died at {}, before the mantle.", name(), ageYears()));
        return nullptr;
    }
    // One crossroads event: among those whose conditions match, the ones not yet met first.
    std::vector<int> matching;
    std::vector<int> fresh;
    for (std::size_t i = 0; i < data_->events.size(); ++i) {
        const CrossroadsEvent& event = data_->events[i];
        if (age < event.minAge || age > event.maxAge) continue;
        if (event.needsAffinity && affinity_[static_cast<std::size_t>(*event.needsAffinity)] < event.needsMinimum) continue;
        if (!event.trigger.empty() && eventTrigger_ && !eventTrigger_(event.trigger)) continue;
        matching.push_back(static_cast<int>(i));
        if (!seen_.contains(event.id)) fresh.push_back(static_cast<int>(i));
    }
    const std::vector<int>& pool = fresh.empty() ? matching : fresh;
    pendingEvent_ = pool.empty() ? -1 : pool[rng_.below(static_cast<std::uint32_t>(pool.size()))];
    if (pendingEvent_ >= 0) {
        seen_.insert(data_->events[static_cast<std::size_t>(pendingEvent_)].id);
        return &data_->events[static_cast<std::size_t>(pendingEvent_)];
    }
    if (ageYears() >= preset().mantleAge) mantle();
    return nullptr;
}

bool HeroLife::resolveEvent(int option) {
    if (pendingEvent_ < 0) return false;
    const CrossroadsEvent& event = data_->events[static_cast<std::size_t>(pendingEvent_)];
    if (option < 0 || option >= static_cast<int>(event.options.size())) return false;
    const CrossroadsOption& chosen = event.options[static_cast<std::size_t>(option)];
    addAffinity(chosen.affinity, imprintPercent(pendingAge_));
    const int other = rolePerson(event.role);
    if (other >= 0 && chosen.opinion != 0) {
        world_->adjustOpinion(other, personId_, chosen.opinion);
        world_->adjustOpinion(personId_, other, chosen.opinion / 2);
    }
    if (const auto trait = traitFromName(chosen.trait)) {
        if (Person* hero = world_->personMutable(personId_)) hero->give(*trait);
    }
    std::string line = core::replaceAll(core::replaceAll(chosen.note, "{hero}", name()), "{other}", other >= 0 ? world_->people()[static_cast<std::size_t>(other)].name : "someone");
    world_->note(line, kImportanceHero, EventKind::Hero, personId_, other);
    lastNote_ = line;
    pendingEvent_ = -1;
    if (ageYears() >= preset().mantleAge) mantle();
    return true;
}

std::vector<std::string> HeroLife::specialtyNames() const {
    std::vector<std::string> names;
    for (const int index : specialty_) names.push_back(data_->professions[static_cast<std::size_t>(index)].name);
    return names;
}

void HeroLife::mantle() {
    std::vector<int> order{0, 1, 2, 3, 4};
    std::stable_sort(order.begin(), order.end(), [this](int a, int b) { return affinity_[static_cast<std::size_t>(a)] > affinity_[static_cast<std::size_t>(b)]; });
    specialty_.clear();
    specialty_.push_back(order[0]);
    const int top = affinity_[static_cast<std::size_t>(order[0])];
    if (affinity_[static_cast<std::size_t>(order[1])] * 100 >= top * 85) specialty_.push_back(order[1]); // close behind, or tied
    phase_ = Phase::Free;
    lastDay_ = world_->date().day;
    std::string names;
    for (const std::string& n : specialtyNames()) names += (names.empty() ? "" : " and ") + n;
    const std::string line = std::format("{} took up the mantle of {} at {}.", name(), names, ageYears());
    world_->note(line, kImportanceHero, EventKind::Hero, personId_);
    lastNote_ = line;
    say(line);
}

// ---- things

int HeroLife::useSkill(int profession, bool success) {
    if (profession < 0 || profession >= static_cast<int>(kProfessionCount)) return 0;
    int points = success ? 2 : 1;
    const int master = master_[static_cast<std::size_t>(profession)];
    if (master >= 0 && world_->people()[static_cast<std::size_t>(master)].alive) points = points * (100 + data_->config.masterBonusPercent) / 100;
    skill_[static_cast<std::size_t>(profession)] = std::min(kMaxSkill, skill_[static_cast<std::size_t>(profession)] + points);
    return points;
}

int HeroLife::count(const std::string& item) const {
    const auto found = inventory_.find(item);
    return found == inventory_.end() ? 0 : found->second;
}

void HeroLife::give(const std::string& item, int amount) {
    if (amount <= 0) return;
    inventory_[item] += amount;
    if (itemObserver_) itemObserver_(item, amount, crafting_);
}

bool HeroLife::take(const std::string& item, int amount) {
    if (count(item) < amount) return false;
    inventory_[item] -= amount;
    if (inventory_[item] == 0) inventory_.erase(item);
    return true;
}

bool HeroLife::hasToolFor(int profession) const {
    const Profession& p = data_->professions.at(static_cast<std::size_t>(profession));
    return std::any_of(p.tools.begin(), p.tools.end(), [this](const std::string& tool) { return count(tool) > 0; });
}

ActionResult HeroLife::gatherBerries() {
    give("berries", 2);
    useSkill(1, true);
    return {true, "You gather 2 berries."};
}

ActionResult HeroLife::pickFlint() {
    give("flint", 1);
    useSkill(2, false);
    return {true, "You pick up a piece of flint by hand."};
}

ActionResult HeroLife::knapFlint() {
    if (count("hammerstone") == 0) return {false, "Needs a hammerstone"};
    give("flint", 2);
    useSkill(2, true);
    return {true, "You knap 2 pieces of flint from the nodule."};
}

ActionResult HeroLife::chopWood() {
    give("wood", 1);
    useSkill(1, true);
    return {true, "You cut a piece of wood."};
}

ActionResult HeroLife::gatherHerbs() {
    give("herbs", 1);
    useSkill(4, true);
    return {true, "You gather a handful of herbs."};
}

std::string HeroLife::craftBlockedReason(const Recipe& recipe) const {
    for (const std::string& tool : recipe.tools) {
        if (count(tool) == 0) return std::format("Needs a {}", data_->item(tool) != nullptr ? data_->item(tool)->name : tool);
    }
    for (const auto& [id, needed] : recipe.inputs) {
        if (count(id) < needed) return std::format("Needs {} x {} (you have {})", needed, data_->item(id) != nullptr ? data_->item(id)->name : id, count(id));
    }
    return {};
}

ActionResult HeroLife::craft(const std::string& recipeId) {
    const Recipe* recipe = data_->recipe(recipeId);
    if (recipe == nullptr) return {false, "Unknown recipe"};
    if (const std::string why = craftBlockedReason(*recipe); !why.empty()) return {false, why};
    CraftedItem made;
    made.item = recipe->output;
    made.maker = name();
    made.day = world_->date().day;
    made.year = world_->date().year;
    made.season = world_->date().season;
    for (const auto& [id, needed] : recipe->inputs) {
        take(id, needed);
        for (int i = 0; i < needed; ++i) made.materials.push_back(id);
    }
    // Quality: skill plus a roll; more skill gives a better tier more often.
    const int score = skillPoints(recipe->profession) + static_cast<int>(rng_.below(41));
    made.quality = std::min(3, score / 25);
    crafting_ = true;
    give(recipe->output, recipe->count);
    crafting_ = false;
    crafted_.push_back(made);
    useSkill(recipe->profession, true);
    return {true, std::format("You make a {} {}.", qualityName(made.quality), data_->item(recipe->output) != nullptr ? data_->item(recipe->output)->name : recipe->output)};
}

std::string HeroLife::describe(const CraftedItem& item) const {
    std::string materials;
    std::map<std::string, int> counts;
    for (const std::string& id : item.materials) ++counts[id];
    for (const auto& [id, n] : counts) materials += std::format("{}{} x {}", materials.empty() ? "" : ", ", n, data_->item(id) != nullptr ? data_->item(id)->name : id);
    const Item* kind = data_->item(item.item);
    return std::format("{} ({}), made by {} from {} on day {} of {}, year {}", kind != nullptr ? kind->name : item.item, qualityName(item.quality), item.maker,
                       materials.empty() ? "nothing" : materials, item.day + 1, seasonName(item.season), item.year);
}

// ---- apprenticeship

int HeroLife::masterOf(int profession) const {
    if (profession < 0 || profession >= static_cast<int>(kProfessionCount)) return -1;
    const int adultYears = world_->config().life.adultAgeYears;
    const int daysPerYear = world_->calendar().daysPerYear();
    std::vector<int> elders;
    for (const Person& other : world_->people()) {
        if (other.alive && !other.exiled && other.id != personId_ && other.ageYears(daysPerYear) >= adultYears) elders.push_back(other.id);
    }
    if (elders.empty()) return -1;
    std::sort(elders.begin(), elders.end(), [this](int a, int b) {
        const Person& pa = world_->people()[static_cast<std::size_t>(a)];
        const Person& pb = world_->people()[static_cast<std::size_t>(b)];
        return pa.ageDays != pb.ageDays ? pa.ageDays > pb.ageDays : pa.id < pb.id;
    });
    return elders[static_cast<std::size_t>(profession) % elders.size()];
}

ActionResult HeroLife::askToApprentice(int profession) {
    const int master = masterOf(profession);
    if (master < 0) return {false, "Nobody in the clan can teach that."};
    const std::string& teacher = world_->people()[static_cast<std::size_t>(master)].name;
    const int trust = data_->config.trustOpinion;
    const int opinion = world_->opinion(master, personId_);
    if (opinion < trust) {
        return {false, std::format("{} will not teach you yet: they think {} of you and want at least {}.", teacher, opinion, trust)};
    }
    master_[static_cast<std::size_t>(profession)] = master;
    const std::string line = std::format("{} took {} as an apprentice in {}.", teacher, name(), data_->professions[static_cast<std::size_t>(profession)].name);
    world_->note(line, kImportanceApprentice, EventKind::Apprentice, master, personId_);
    say(line);
    return {true, line};
}

ActionResult HeroLife::talkTo(int other) {
    if (other < 0 || static_cast<std::size_t>(other) >= world_->people().size() || !world_->people()[static_cast<std::size_t>(other)].alive) return {false, "Nobody there."};
    world_->talk(personId_, other);
    return {true, std::format("You talk with {}.", world_->people()[static_cast<std::size_t>(other)].name)};
}

ActionResult HeroLife::giveBerriesTo(int other) {
    if (other < 0 || static_cast<std::size_t>(other) >= world_->people().size() || !world_->people()[static_cast<std::size_t>(other)].alive) return {false, "Nobody there."};
    if (!take("berries", 1)) return {false, "You have no berries to give."};
    world_->giveGift(personId_, other);
    return {true, std::format("You give {} some berries.", world_->people()[static_cast<std::size_t>(other)].name)};
}

// ---- dominion, trade and the sacred fire

void HeroLife::setRivals(std::vector<RivalSummary> rivals) {
    rivals_ = std::move(rivals);
    if (relations_.size() < rivals_.size()) relations_.resize(rivals_.size(), 0);
    if (converts_.size() < rivals_.size()) converts_.resize(rivals_.size(), 0);
}

int HeroLife::tradePercent() const {
    const DominionConfig& d = data_->config.dominion;
    long long mine = tradePoints_ + static_cast<long long>(world_->population()) * d.tradePerPerson;
    long long theirs = 0;
    for (const RivalSummary& rival : rivals_) theirs += static_cast<long long>(rival.population) * d.rivalTradePerPerson + world_->date().day * d.rivalTradeGrowthPerDay;
    return mine + theirs <= 0 ? 0 : static_cast<int>(mine * 100 / (mine + theirs));
}

int HeroLife::faith(int who) const {
    const auto found = faith_.find(who);
    return found == faith_.end() ? 0 : found->second;
}

int HeroLife::followers() const {
    int total = 0;
    for (const auto& [who, value] : faith_) {
        if (value >= data_->config.fire.followerFaith && static_cast<std::size_t>(who) < world_->people().size() && world_->people()[static_cast<std::size_t>(who)].alive) ++total;
    }
    for (const int converts : converts_) total += converts;
    return total;
}

int HeroLife::religionPercent() const {
    if (!fire_.founded) return 0;
    const DominionConfig& d = data_->config.dominion;
    long long theirs = 0;
    for (std::size_t i = 0; i < rivals_.size(); ++i) {
        theirs += std::max(0, rivals_[i].population * d.rivalFollowerPercent / 100 - (i < converts_.size() ? converts_[i] : 0));
    }
    const long long mine = followers();
    return mine + theirs <= 0 ? 0 : static_cast<int>(mine * 100 / (mine + theirs));
}

bool HeroLife::rivalNeeds(int rival, const std::string& item) const {
    if (rival % 2 == 0) return item == "fur" || item == "flint-tool";
    return item == "spear" || item == "berries" || item == "basket";
}

int HeroLife::unitValue(const std::string& item) const {
    const Item* found = data_->item(item);
    return found != nullptr ? found->value : 1;
}

int HeroLife::valueOf(const Goods& goods, int rival, bool asGiven) const {
    int total = 0;
    for (const auto& [item, n] : goods) {
        int value = unitValue(item) * n;
        if (asGiven && rivalNeeds(rival, item)) value = value * data_->config.trade.needPercent / 100;
        total += value;
    }
    return total;
}

BarterResult HeroLife::barter(int rival, const BarterOffer& offer) {
    BarterResult result;
    if (rival < 0 || static_cast<std::size_t>(rival) >= rivals_.size()) {
        result.message = "There is nobody to trade with.";
        return result;
    }
    const std::string& clanName = rivals_[static_cast<std::size_t>(rival)].name;
    if (offer.give.empty() || offer.want.empty()) {
        result.message = "Offer something, and ask for something.";
        return result;
    }
    const TradeConfig& trade = data_->config.trade;
    const int rel = relation(rival);
    if (offer.payLater) {
        int open = 0;
        for (const Debt& debt : debts_) open += (!debt.settled && !debt.defaulted) ? 1 : 0;
        if (open >= trade.maxDebts || rel < -10) {
            result.message = std::format("{} will not trade on credit: {}.", clanName, open >= trade.maxDebts ? "you owe them too much already" : "they do not trust you");
            return result;
        }
    } else {
        for (const auto& [item, n] : offer.give) {
            if (count(item) < n) {
                result.message = std::format("You do not have {} x {}.", n, data_->item(item) != nullptr ? data_->item(item)->name : item);
                return result;
            }
        }
    }
    const int giveValue = valueOf(offer.give, rival, true);
    const int wantValue = valueOf(offer.want, rival, false);
    const int demand = rel < 0 ? 120 : (rel > 30 ? 90 : 100);
    const int creditPercent = offer.payLater ? 120 : 100; // credit costs a fifth more
    const long long required = static_cast<long long>(wantValue) * demand * creditPercent; // x 10000
    if (static_cast<long long>(giveValue) * 10000 < required) {
        // A counter: more of the first thing offered.
        const auto& [firstItem, firstCount] = offer.give.front();
        const int unit = std::max(1, unitValue(firstItem) * (rivalNeeds(rival, firstItem) ? trade.needPercent : 100) / 100);
        const int requiredValue = static_cast<int>((required + 9999) / 10000);
        const int extra = (requiredValue - giveValue + unit - 1) / unit;
        BarterOffer counter = offer;
        counter.give.front().second = firstCount + std::max(1, extra);
        result.counter = counter;
        result.message = std::format("{} want more: {} for {}.", clanName, goodsText(*data_, counter.give), goodsText(*data_, counter.want));
        return result;
    }
    result.accepted = true;
    if (offer.payLater) {
        for (const auto& [item, n] : offer.want) give(item, n);
        debts_.push_back({rival, offer.give, giveValue, world_->date().day + trade.dueDays, false, false});
        tradePoints_ += wantValue;
        result.message = std::format("{} give you {} now and expect {} within {} days.", clanName, goodsText(*data_, offer.want), goodsText(*data_, offer.give), trade.dueDays);
        say("A debt was recorded with " + clanName + ".");
    } else {
        for (const auto& [item, n] : offer.give) take(item, n);
        for (const auto& [item, n] : offer.want) give(item, n);
        tradePoints_ += std::max(giveValue, wantValue);
        result.message = std::format("{} agree: {} for {}.", clanName, goodsText(*data_, offer.give), goodsText(*data_, offer.want));
    }
    relations_[static_cast<std::size_t>(rival)] = std::clamp(rel + trade.relationshipPerDeal, -100, 100);
    return result;
}

ActionResult HeroLife::payDebt(std::size_t index) {
    if (index >= debts_.size() || debts_[index].settled || debts_[index].defaulted) return {false, "Nothing to pay."};
    Debt& debt = debts_[index];
    for (const auto& [item, n] : debt.owe) {
        if (count(item) < n) return {false, std::format("You do not have {} x {}.", n, data_->item(item) != nullptr ? data_->item(item)->name : item)};
    }
    for (const auto& [item, n] : debt.owe) take(item, n);
    debt.settled = true;
    relations_[static_cast<std::size_t>(debt.rival)] = std::clamp(relation(debt.rival) + data_->config.trade.relationshipPerDeal * 2, -100, 100);
    tradePoints_ += debt.value / 4;
    return {true, "The debt is paid."};
}

std::string HeroLife::foundFireBlockedReason() const {
    if (fire_.founded) return "You have already founded a sacred fire.";
    const bool shaman = std::find(specialty_.begin(), specialty_.end(), 4) != specialty_.end();
    if (!shaman && skillLevel(3) < data_->config.fire.skillLevelToFound) {
        return std::format("Needs the Shaman-healer specialty or Fire-keeping level {} (you have {}).", data_->config.fire.skillLevelToFound, skillLevel(3));
    }
    return {};
}

ActionResult HeroLife::foundFire(const std::string& fireName, int tileX, int tileY) {
    if (const std::string why = foundFireBlockedReason(); !why.empty()) return {false, why};
    if (fireName.empty()) return {false, "A sacred fire needs a name."};
    fire_ = {true, true, fireName, tileX, tileY, 0};
    const std::string line = std::format("{} lit a sacred fire and named it {}.", name(), fireName);
    world_->note(line, 70, EventKind::Hero, personId_);
    say(line);
    return {true, line};
}

ActionResult HeroLife::holdRitual(const std::vector<int>& attendees) {
    if (!fire_.founded) return {false, "There is no sacred fire."};
    if (!fire_.lit) return {false, std::format("{} has gone out: tend it first.", fire_.name)};
    const FireConfig& fc = data_->config.fire;
    if (static_cast<int>(attendees.size()) < fc.ritualMinimumAttendees) {
        return {false, std::format("A ritual needs {} people at the fire ({} are here).", fc.ritualMinimumAttendees, attendees.size())};
    }
    for (const int who : attendees) faith_[who] = std::min(100, faith(who) + fc.ritualFaith);
    faith_[personId_] = std::min(100, faith(personId_) + fc.ritualFaith);
    fire_.daysUntended = 0;
    int converted = 0;
    for (std::size_t i = 0; i < rivals_.size(); ++i) {
        // The rivals who think well of the hero listen more readily.
        const int chance = fc.conversionPerMille * (100 + std::clamp(relation(static_cast<int>(i)), -50, 100)) / 100;
        const int pool = rivals_[i].population * data_->config.dominion.rivalFollowerPercent / 100 - converts_[i];
        if (pool > 0 && static_cast<int>(rng_.below(1000)) < chance) {
            ++converts_[i];
            ++converted;
        }
    }
    const std::string line = converted > 0 ? std::format("The ritual at {} drew {} from the other clans to the flame.", fire_.name, converted == 1 ? "one listener" : std::format("{} listeners", converted))
                                           : std::format("The ritual at {} strengthened the faith of the clan.", fire_.name);
    say(line);
    return {true, line};
}

ActionResult HeroLife::tendFire() {
    if (!fire_.founded) return {false, "There is no sacred fire."};
    if (!fire_.lit) {
        if (!take("wood", 1)) return {false, "Needs a piece of wood to light it again."};
        fire_.lit = true;
    }
    fire_.daysUntended = 0;
    return {true, std::format("You tend {}.", fire_.name)};
}

ActionResult HeroLife::eatBerries() {
    if (!take("berries", 1)) return {false, "You have no berries."};
    if (Person* hero = world_->personMutable(personId_)) {
        int& hunger = hero->needs[Need::Hunger];
        hunger = std::min(100, hunger + world_->config().needs.mealValue);
    }
    return {true, "You eat the berries."};
}

ActionResult HeroLife::tendCampFire() {
    if (Person* hero = world_->personMutable(personId_)) {
        int& warmth = hero->needs[Need::Warmth];
        warmth = std::min(100, warmth + 20);
    }
    return {true, "You feed the clan's fire."};
}

int HeroLife::agingPercent(int years) const {
    const AgingConfig& a = data_->config.aging;
    return years < a.fromYears ? 0 : a.startPercent + a.perYearPercent * (years - a.fromYears);
}

// ---- the days

void HeroLife::update() {
    if (phase_ != Phase::Free) {
        lastDay_ = world_->date().day;
        return;
    }
    while (phase_ == Phase::Free && lastDay_ < world_->date().day) {
        ++lastDay_;
        dayEnded();
    }
}

void HeroLife::end(Outcome outcome, const std::string& reason) {
    phase_ = Phase::Ended;
    outcome_ = outcome;
    reason_ = reason;
    world_->note(reason, 90, EventKind::Hero, personId_);
    say(reason);
}

void HeroLife::dayEnded() {
    Person* hero = world_->personMutable(personId_);
    // The hero's death ends the run (US-055); before the mantle it is a loss.
    if (hero == nullptr || !hero->alive) {
        if (ageYears() < preset().mantleAge) {
            end(Outcome::Defeat, std::format("{} died at {}, before the mantle.", name(), ageYears()));
        } else {
            end(Outcome::Died, std::format("{} died at {}.", name(), ageYears()));
        }
        return;
    }
    // Age takes its toll on Energy.
    const int aging = agingPercent(ageYears());
    if (aging > 0) {
        const int extra = world_->config().needs.dailyDecay[static_cast<std::size_t>(Need::Energy)] * aging / 100;
        hero->needs[Need::Energy] = std::max(0, hero->needs[Need::Energy] - extra);
    }
    // Working beside a master teaches.
    for (std::size_t p = 0; p < kProfessionCount; ++p) {
        if (master_[p] >= 0 && world_->people()[static_cast<std::size_t>(master_[p])].alive) {
            skill_[p] = std::min(kMaxSkill, skill_[p] + kApprenticePointsPerDay * (100 + data_->config.masterBonusPercent) / 100);
        }
    }
    // The sacred fire goes out when nobody tends it, and the faithful drift away.
    if (fire_.founded) {
        if (fire_.lit && ++fire_.daysUntended > data_->config.fire.untendedDaysBeforeOut) {
            fire_.lit = false;
            const std::string line = std::format("The sacred fire {} went out for want of tending.", fire_.name);
            world_->note(line, 60, EventKind::Hero, personId_);
            say(line);
        }
        if (!fire_.lit) {
            for (auto& [who, value] : faith_) value = std::max(0, value - data_->config.fire.neglectFaithLoss);
            for (int& converts : converts_) {
                if (converts > 0) {
                    --converts;
                    break;
                }
            }
        }
    }
    // Debts fall due.
    for (Debt& debt : debts_) {
        if (!debt.settled && !debt.defaulted && world_->date().day > debt.dueDay) {
            debt.defaulted = true;
            relations_[static_cast<std::size_t>(debt.rival)] = std::clamp(relation(debt.rival) - data_->config.trade.defaultRelationshipPenalty, -100, 100);
            tradePoints_ = std::max(0, tradePoints_ - debt.value * data_->config.trade.defaultTradePenaltyPercent / 100);
            const std::string line = std::format("{} failed to pay a debt to {}.", name(), debt.rival < static_cast<int>(rivals_.size()) ? rivals_[static_cast<std::size_t>(debt.rival)].name : "a rival clan");
            world_->note(line, 60, EventKind::Hero, personId_);
            say(line);
        }
    }
    // Winning and losing (US-073).
    const DominionConfig& d = data_->config.dominion;
    const int trade = tradePercent();
    const int religion = religionPercent();
    if (trade >= d.winPercent || religion >= d.winPercent || (trade + religion) / 2 >= d.combinedWinPercent) {
        end(Outcome::Victory, std::format("Your clan leads the region: Trade {}%, Religion {}%.", trade, religion));
    } else if (world_->population() < d.loseBelowPeople) {
        end(Outcome::Defeat, std::format("Your clan has dwindled below {} people.", d.loseBelowPeople));
    }
}

RunSummary HeroLife::summary() const {
    RunSummary s;
    s.outcome = outcome_;
    s.reason = reason_;
    s.heroName = name();
    s.ageYears = ageYears();
    s.specialty = specialtyNames();
    s.tradePercent = tradePercent();
    s.religionPercent = religionPercent();
    std::vector<std::string> lines;
    for (const ChronicleEntry& entry : world_->chronicle().entries()) {
        if (entry.who == personId_ || entry.other == personId_ || entry.importance >= 75) lines.push_back(formatEntry(entry));
    }
    const std::size_t from = lines.size() > 14 ? lines.size() - 14 : 0;
    s.highlights.assign(lines.begin() + static_cast<std::ptrdiff_t>(from), lines.end());
    return s;
}

// ---- saving

void HeroLife::save(const std::filesystem::path& file) const {
    json goods = json::object();
    for (const auto& [item, n] : inventory_) goods[item] = n;
    json crafted = json::array();
    for (const CraftedItem& c : crafted_) crafted.push_back({{"item", c.item}, {"quality", c.quality}, {"maker", c.maker}, {"day", c.day}, {"materials", c.materials}, {"year", c.year}, {"season", static_cast<int>(c.season)}});
    json rivals = json::array();
    for (const RivalSummary& r : rivals_) rivals.push_back({{"name", r.name}, {"population", r.population}});
    const auto goodsJson = [](const Goods& g) {
        json out = json::array();
        for (const auto& [item, n] : g) out.push_back({item, n});
        return out;
    };
    json debts = json::array();
    for (const Debt& d : debts_) debts.push_back({{"rival", d.rival}, {"owe", goodsJson(d.owe)}, {"value", d.value}, {"dueDay", d.dueDay}, {"settled", d.settled}, {"defaulted", d.defaulted}});
    json faith = json::array();
    for (const auto& [who, value] : faith_) faith.push_back({who, value});
    const json data{{"version", 2}, // version 2 (US-195) adds the rules the run plays under
                    {"seed", game_.seed}, {"preset", game_.preset}, {"comfort", game_.comfort}, {"rules", game_.rules},
                    {"rng", {{"state", rng_.state()}, {"increment", rng_.increment()}}},
                    {"person", personId_}, {"origin", origin_}, {"phase", static_cast<int>(phase_)}, {"outcome", static_cast<int>(outcome_)}, {"reason", reason_},
                    {"affinity", affinity_}, {"skill", skill_}, {"focus", {focusFirst_, focusSecond_}}, {"pendingEvent", pendingEvent_}, {"pendingAge", pendingAge_},
                    {"seen", seen_}, {"specialty", specialty_}, {"inventory", goods}, {"crafted", crafted}, {"master", master_},
                    {"rivals", rivals}, {"relations", relations_}, {"converts", converts_}, {"tradePoints", tradePoints_}, {"debts", debts},
                    {"fire", {{"founded", fire_.founded}, {"lit", fire_.lit}, {"name", fire_.name}, {"x", fire_.tileX}, {"y", fire_.tileY}, {"untended", fire_.daysUntended}}},
                    {"faith", faith}, {"lastDay", lastDay_}};
    writeSaveText(file, data.dump(1));
}

std::string HeroLife::savedRules(const std::filesystem::path& file) {
    try {
        const json j = readJsonFile(file);
        return j.value("rules", std::string());
    } catch (const std::exception&) {
        return {};
    }
}

HeroLife HeroLife::load(const HeroData& data, World& world, const std::filesystem::path& file) {
    const json j = readJsonFile(file);
    HeroLife life(data, world, RestoreTag{});
    try {
        life.game_ = {j.at("seed").get<std::uint64_t>(), j.at("preset").get<int>(), j.at("comfort").get<int>(), j.value("rules", std::string())}; // a version 1 save has no rules: standard
        life.rng_.restore(j.at("rng").at("state").get<std::uint64_t>(), j.at("rng").at("increment").get<std::uint64_t>());
        life.personId_ = j.at("person").get<int>();
        life.origin_ = j.at("origin").get<std::string>();
        life.phase_ = static_cast<Phase>(j.at("phase").get<int>());
        life.outcome_ = static_cast<Outcome>(j.at("outcome").get<int>());
        life.reason_ = j.at("reason").get<std::string>();
        life.affinity_ = j.at("affinity").get<AffinityValues>();
        life.skill_ = j.at("skill").get<SkillValues>();
        life.focusFirst_ = j.at("focus").at(0).get<int>();
        life.focusSecond_ = j.at("focus").at(1).get<int>();
        life.pendingEvent_ = j.at("pendingEvent").get<int>();
        life.pendingAge_ = j.at("pendingAge").get<int>();
        life.seen_ = j.at("seen").get<std::set<std::string>>();
        life.specialty_ = j.at("specialty").get<std::vector<int>>();
        life.inventory_ = j.at("inventory").get<std::map<std::string, int>>();
        for (const json& c : j.at("crafted")) {
            CraftedItem item;
            item.item = c.at("item").get<std::string>();
            item.quality = c.at("quality").get<int>();
            item.maker = c.at("maker").get<std::string>();
            item.day = c.at("day").get<std::int64_t>();
            item.materials = c.at("materials").get<std::vector<std::string>>();
            item.year = c.at("year").get<int>();
            item.season = static_cast<Season>(c.at("season").get<int>());
            life.crafted_.push_back(item);
        }
        life.master_ = j.at("master").get<std::array<int, kProfessionCount>>();
        for (const json& r : j.at("rivals")) life.rivals_.push_back({r.at("name").get<std::string>(), r.at("population").get<int>()});
        life.relations_ = j.at("relations").get<std::vector<int>>();
        life.converts_ = j.at("converts").get<std::vector<int>>();
        life.tradePoints_ = j.at("tradePoints").get<int>();
        for (const json& d : j.at("debts")) {
            Debt debt;
            debt.rival = d.at("rival").get<int>();
            for (const json& g : d.at("owe")) debt.owe.push_back({g.at(0).get<std::string>(), g.at(1).get<int>()});
            debt.value = d.at("value").get<int>();
            debt.dueDay = d.at("dueDay").get<std::int64_t>();
            debt.settled = d.at("settled").get<bool>();
            debt.defaulted = d.at("defaulted").get<bool>();
            life.debts_.push_back(debt);
        }
        const json& f = j.at("fire");
        life.fire_ = {f.at("founded").get<bool>(), f.at("lit").get<bool>(), f.at("name").get<std::string>(), f.at("x").get<int>(), f.at("y").get<int>(), f.at("untended").get<int>()};
        for (const json& pair : j.at("faith")) life.faith_[pair.at(0).get<int>()] = pair.at(1).get<int>();
        life.lastDay_ = j.at("lastDay").get<std::int64_t>();
    } catch (const json::exception& error) {
        throw DataError(file, "(contents)", std::string("is damaged: ") + error.what());
    }
    return life;
}

} // namespace odysseus::sim
