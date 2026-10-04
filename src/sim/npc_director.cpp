#include "sim/npc_director.h"

#include "core/random.h"
#include "sim/data.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <format>
#include <limits>
#include <optional>

namespace odysseus::sim {

namespace {

using nlohmann::json;

constexpr std::int32_t kUnset = std::numeric_limits<std::int32_t>::min(); // a home that has not been given: the person's place when the director first sees them
constexpr int kMinScore = 30; // a score below this is not worth getting up for (as for the clan members and animals of M7)

void mix(std::uint64_t& hash, std::uint64_t value) {
    for (int i = 0; i < 8; ++i) {
        hash ^= (value >> (i * 8)) & 0xFFU;
        hash *= 0x100000001B3ULL; // FNV-1a
    }
}

// A run-length coded array of small numbers: [[value, count], ...].
template <typename T>
json runs(const std::vector<T>& values) {
    json out = json::array();
    std::size_t i = 0;
    while (i < values.size()) {
        std::size_t j = i;
        while (j < values.size() && values[j] == values[i]) ++j;
        out.push_back(json::array({static_cast<int>(values[i]), static_cast<std::int64_t>(j - i)}));
        i = j;
    }
    return out;
}

template <typename T>
std::vector<T> unrun(const json& value, const char* field) {
    std::vector<T> out;
    if (!value.is_array()) throw DataError("npc-life.json", field, "must be a list of [value, count]");
    for (const json& entry : value) {
        if (!entry.is_array() || entry.size() != 2 || !entry.at(0).is_number_integer() || !entry.at(1).is_number_integer() || entry.at(1).get<std::int64_t>() < 0 ||
            entry.at(1).get<std::int64_t>() > 100'000'000) {
            throw DataError("npc-life.json", field, "must be a list of [value, count]");
        }
        out.insert(out.end(), static_cast<std::size_t>(entry.at(1).get<std::int64_t>()), static_cast<T>(entry.at(0).get<int>()));
    }
    return out;
}

json profileJson(const NpcProfile& profile) {
    json partner = json::object();
    for (const auto& [type, ids] : profile.partnerActions) partner[type] = ids;
    return json{{"classes", profile.classes}, {"tags", profile.tags}, {"classActions", profile.classActions}, {"customActions", profile.customActions},
                {"allow", profile.allow}, {"deny", profile.deny}, {"partner", partner}};
}

NpcProfile profileFrom(const json& value) {
    NpcProfile profile;
    profile.classes = value.at("classes").get<std::vector<std::string>>();
    profile.tags = value.at("tags").get<std::vector<std::string>>();
    profile.classActions = value.at("classActions").get<std::vector<std::string>>();
    profile.customActions = value.at("customActions").get<std::vector<std::string>>();
    profile.allow = value.at("allow").get<std::vector<std::string>>();
    profile.deny = value.at("deny").get<std::vector<std::string>>();
    for (const auto& [type, ids] : value.at("partner").items()) profile.partnerActions[type] = ids.get<std::vector<std::string>>();
    return profile;
}

json blocksJson(const std::vector<rules::ScheduleBlock>& blocks) {
    json out = json::array();
    for (const rules::ScheduleBlock& block : blocks) out.push_back(json::array({block.minute, block.activity, block.place}));
    return out;
}

std::vector<rules::ScheduleBlock> blocksFrom(const json& value) {
    std::vector<rules::ScheduleBlock> out;
    if (!value.is_array()) throw DataError("npc-life.json", "schedules", "must be lists of [minute, activity, place]");
    for (const json& entry : value) {
        if (!entry.is_array() || entry.size() != 3 || !entry.at(0).is_number_integer() || !entry.at(1).is_string() || !entry.at(2).is_string()) {
            throw DataError("npc-life.json", "schedules", "a block is [minute, activity, place]");
        }
        out.push_back({entry.at(0).get<int>(), entry.at(1).get<std::string>(), entry.at(2).get<std::string>()});
    }
    return out;
}

} // namespace

NpcDirector::NpcDirector(rules::ScheduleConfig config) : config_(std::move(config)) {
    schedules_.emplace_back();
    profiles_.emplace_back();
}

void NpcDirector::ensure(std::size_t size) {
    if (modes_.size() >= size) return;
    scheduleOf_.resize(size, 0);
    modes_.resize(size, static_cast<std::uint8_t>(Mode::Scheduled));
    dangers_.resize(size, 0);
    homeX_.resize(size, kUnset);
    homeY_.resize(size, kUnset);
    profileOf_.resize(size, 0);
    lastAction_.resize(size, -1);
}

void NpcDirector::setPlaces(std::vector<Place> places) { places_ = std::move(places); }

const Place* NpcDirector::place(const std::string& name) const {
    for (const Place& candidate : places_) {
        if (candidate.name == name) return &candidate;
    }
    return nullptr;
}

int NpcDirector::internSchedule(const rules::Schedule& schedule) {
    if (schedule.empty()) return 0;
    for (std::size_t i = 1; i < schedules_.size(); ++i) {
        if (schedules_[i] == schedule) return static_cast<int>(i);
    }
    schedules_.push_back(schedule);
    return static_cast<int>(schedules_.size() - 1);
}

void NpcDirector::setSchedule(int index, const rules::Schedule& schedule) {
    ensure(static_cast<std::size_t>(index) + 1);
    scheduleOf_[static_cast<std::size_t>(index)] = static_cast<std::uint16_t>(internSchedule(schedule));
}

const rules::Schedule* NpcDirector::schedule(int index) const {
    if (index < 0 || static_cast<std::size_t>(index) >= scheduleOf_.size() || scheduleOf_[static_cast<std::size_t>(index)] == 0) return nullptr;
    return &schedules_[scheduleOf_[static_cast<std::size_t>(index)]];
}

void NpcDirector::setHome(int index, int x, int y) {
    ensure(static_cast<std::size_t>(index) + 1);
    homeX_[static_cast<std::size_t>(index)] = x;
    homeY_[static_cast<std::size_t>(index)] = y;
}

int NpcDirector::homeX(int index) const { return static_cast<std::size_t>(index) < homeX_.size() ? homeX_[static_cast<std::size_t>(index)] : kUnset; }
int NpcDirector::homeY(int index) const { return static_cast<std::size_t>(index) < homeY_.size() ? homeY_[static_cast<std::size_t>(index)] : kUnset; }

int NpcDirector::internProfile(const NpcProfile& profile) {
    if (profile.empty() && profile.tags.empty() && profile.allow.empty() && profile.deny.empty()) return 0;
    for (std::size_t i = 1; i < profiles_.size(); ++i) {
        if (profiles_[i] == profile) return static_cast<int>(i);
    }
    profiles_.push_back(profile);
    return static_cast<int>(profiles_.size() - 1);
}

void NpcDirector::setProfile(int index, const NpcProfile& profile) {
    ensure(static_cast<std::size_t>(index) + 1);
    profileOf_[static_cast<std::size_t>(index)] = static_cast<std::uint16_t>(internProfile(profile));
}

const NpcProfile* NpcDirector::profile(int index) const {
    if (index < 0 || static_cast<std::size_t>(index) >= profileOf_.size() || profileOf_[static_cast<std::size_t>(index)] == 0) return nullptr;
    return &profiles_[profileOf_[static_cast<std::size_t>(index)]];
}

std::string NpcDirector::lastAction(int index) const {
    if (index < 0 || static_cast<std::size_t>(index) >= lastAction_.size() || lastAction_[static_cast<std::size_t>(index)] < 0) return {};
    return actionNames_[static_cast<std::size_t>(lastAction_[static_cast<std::size_t>(index)])];
}

void NpcDirector::setDanger(int index, bool danger) {
    ensure(static_cast<std::size_t>(index) + 1);
    dangers_[static_cast<std::size_t>(index)] = danger ? 1 : 0;
}

NpcDirector::Mode NpcDirector::mode(int index) const {
    return static_cast<std::size_t>(index) < modes_.size() ? static_cast<Mode>(modes_[static_cast<std::size_t>(index)]) : Mode::Scheduled;
}

const rules::ScheduleBlock* NpcDirector::currentBlock(const NpcPopulation& population, int index) const {
    const rules::Schedule* own = schedule(index);
    if (own == nullptr) return nullptr;
    const int hour = population.hourOfDay();
    return rules::activeBlock(*own, hour * 60, config_.isNight(hour));
}

std::string NpcDirector::activity(const NpcPopulation& population, int index) const {
    switch (mode(index)) {
    case Mode::Eating: return "eat";
    case Mode::Fleeing: return "flee";
    case Mode::Fighting: return "fight";
    case Mode::Dead: return "dead";
    case Mode::Scheduled: break;
    }
    const rules::ScheduleBlock* block = currentBlock(population, index);
    return block != nullptr ? block->activity : std::string("idle");
}

std::pair<int, int> NpcDirector::pointFor(const NpcPopulation& population, int index, const std::string& placeName) const {
    const std::size_t at = static_cast<std::size_t>(index);
    const int homeX = at < homeX_.size() && homeX_[at] != kUnset ? homeX_[at] : population.x(index);
    const int homeY = at < homeY_.size() && homeY_[at] != kUnset ? homeY_[at] : population.y(index);
    if (placeName == "home") return {homeX, homeY};
    const Place* spot = place(placeName);
    if (spot == nullptr) return {homeX, homeY}; // an unknown place: stay at home (the log names it when the level loads)
    if (config_.scatterPixels <= 0) return {spot->x, spot->y};
    // A person stands near the place, at a spot that depends only on who they are, so a crowd spreads out instead of standing on one point.
    const std::uint32_t id = static_cast<std::uint32_t>(population.id(index));
    const std::uint32_t h = id * 2654435761U + 0x9E3779B9U;
    const int span = 2 * config_.scatterPixels + 1;
    const int dx = static_cast<int>(h % static_cast<std::uint32_t>(span)) - config_.scatterPixels;
    const int dy = static_cast<int>((h >> 16) % static_cast<std::uint32_t>(span)) - config_.scatterPixels;
    return {spot->x + dx, spot->y + dy};
}

void NpcDirector::goTo(NpcPopulation& population, int index, const std::string& placeName) {
    const auto [x, y] = pointFor(population, index, placeName);
    if (x != population.x(index) || y != population.y(index)) population.move(index, x, y);
}

// The schedule block of this hour: go to its place and, for a person near the hero, get back what an hour of that activity gives.
void NpcDirector::followSchedule(NpcPopulation& population, int index, int hour, bool restoreNeeds) {
    const rules::Schedule* own = schedule(index);
    if (own == nullptr) return;
    const rules::ScheduleBlock* block = rules::activeBlock(*own, hour * 60, config_.isNight(hour));
    if (block == nullptr) return;
    goTo(population, index, block->place);
    if (!restoreNeeds) return;
    const auto effect = config_.activities.find(block->activity);
    if (effect == config_.activities.end()) return;
    for (std::size_t n = 0; n < kNeedCount; ++n) {
        if (effect->second[n] > 0) population.setNeed(index, static_cast<Need>(n), population.need(index, static_cast<Need>(n)) + effect->second[n]);
    }
}

// An hour of a person near the hero: danger sends them home, hunger sends them to eat (D-54 Q10), a fight is left to the fight; otherwise the schedule decides. When the
// interruption is over the next hour simply follows the schedule again, so the person goes back to what they were doing.
void NpcDirector::stepNear(NpcPopulation& population, int index, int hour) {
    const std::size_t at = static_cast<std::size_t>(index);
    const Mode current = static_cast<Mode>(modes_[at]);
    if (current == Mode::Dead || current == Mode::Fighting) return;
    if (dangers_[at] != 0) {
        modes_[at] = static_cast<std::uint8_t>(Mode::Fleeing);
        goTo(population, index, config_.dangerPlace);
        return;
    }
    const bool hungry = population.need(index, Need::Hunger) < config_.eatBelow;
    if (hungry && schedule(index) != nullptr) {
        modes_[at] = static_cast<std::uint8_t>(Mode::Eating);
        goTo(population, index, config_.eatPlace);
        population.setNeed(index, Need::Hunger, population.need(index, Need::Hunger) + config_.eatRestore);
        return;
    }
    modes_[at] = static_cast<std::uint8_t>(Mode::Scheduled);
    followSchedule(population, index, hour, true);
    if (isFree(population, index, hour)) chooseAction(population, index, hour, false); // idle on duty: do something of one's own
}

std::pair<int, int> NpcDirector::pointNear(const NpcPopulation& population, int index, int x, int y) const {
    if (config_.scatterPixels <= 0) return {x, y};
    const std::uint32_t id = static_cast<std::uint32_t>(population.id(index));
    const std::uint32_t h = id * 2654435761U + 0x9E3779B9U;
    const int span = 2 * config_.scatterPixels + 1;
    return {x + static_cast<int>(h % static_cast<std::uint32_t>(span)) - config_.scatterPixels, y + static_cast<int>((h >> 16) % static_cast<std::uint32_t>(span)) - config_.scatterPixels};
}

bool NpcDirector::isFree(const NpcPopulation& population, int index, int hour) const {
    (void)population;
    const rules::Schedule* own = schedule(index);
    if (own == nullptr) return true;
    const rules::ScheduleBlock* block = rules::activeBlock(*own, hour * 60, config_.isNight(hour));
    return block == nullptr || config_.freeActivities.count(block->activity) != 0;
}

// The choice: every candidate interaction (class, custom and event actions) is scored against each thing it could be done to (a place, or the spot of an event) with the `npc`
// block of its file and the rule language, and the best score wins (a tie by a roll of the world seed, the tick and the person); below kMinScore nobody gets up for it.
bool NpcDirector::chooseAction(NpcPopulation& population, int index, int hour, bool eventsOnly) {
    if (interactions_ == nullptr || chosenThisHour_ >= config_.maxPerHour) return false;
    const std::size_t at = static_cast<std::size_t>(index);
    if (modes_[at] == static_cast<std::uint8_t>(Mode::Dead) || modes_[at] == static_cast<std::uint8_t>(Mode::Fighting)) return false;
    const NpcProfile& own = profiles_[profileOf_[at]];
    if (own.classActions.empty() && own.customActions.empty() && board_.posted().empty()) return false;
    const SourceContext context{own, population.x(index), population.y(index), population.ticks(), &board_, &events_};
    std::vector<ActionCandidate> candidates = sources_.collect(context);
    if (eventsOnly) std::erase_if(candidates, [](const ActionCandidate& candidate) { return candidate.origin != ActionOrigin::Event; });
    if (candidates.empty()) return false;

    rules::ThingInfo actor;
    actor.kind = population.kind(index);
    actor.tags = own.tags;
    if (std::find(actor.tags.begin(), actor.tags.end(), "npc") == actor.tags.end()) actor.tags.push_back("npc");
    const std::int64_t now = static_cast<std::int64_t>(population.ticks());
    struct Option {
        const rules::Interaction* interaction;
        ActionTarget target;
    };
    std::vector<Option> options;
    std::vector<int> scores;
    for (const ActionCandidate& candidate : candidates) {
        const rules::Interaction* interaction = interactions_->find(candidate.interaction);
        if (interaction == nullptr || !interaction->npc || !cooldowns_.ready(population.id(index), interaction->id, now)) continue;
        if (std::find(own.deny.begin(), own.deny.end(), interaction->id) != own.deny.end()) continue; // an action the NPC may never do
        std::vector<ActionTarget> targets;
        if (candidate.atEvent) {
            targets.push_back({ActionTarget::Kind::Event, -1, candidate.eventTrigger, "event", candidate.eventX, candidate.eventY, {"event", candidate.eventTrigger}});
        } else {
            for (const Place& place : places_) {
                ActionTarget target{ActionTarget::Kind::Place, -1, place.name, "place", place.x, place.y, place.tags};
                target.tags.push_back("place");
                targets.push_back(std::move(target));
            }
        }
        for (const ActionTarget& target : targets) {
            rules::ThingInfo thing;
            thing.kind = target.kindName;
            thing.tags = target.tags;
            const NpcRuleContext rule(population, index, actor.tags, target, hour);
            for (const rules::Offer& offer : interactions_->offered(actor, thing, 0, rule)) {
                if (offer.interaction != interaction || !offer.enabled) continue;
                const long long score = rules::evaluate(*interaction->npc->score, rule).number + candidate.bonus;
                options.push_back({interaction, target});
                scores.push_back(static_cast<int>(std::clamp<long long>(score, 0, 100000)));
                break;
            }
        }
    }
    if (options.empty()) return false;
    core::Pcg32 random(seed_ + population.ticks() * 0x9E3779B97F4A7C15ULL, static_cast<std::uint64_t>(index) * 2ULL + 1ULL);
    const std::optional<std::size_t> pick = rules::pickBest(scores, kMinScore, random);
    if (!pick) return false;
    carryOut(population, index, *options[*pick].interaction, options[*pick].target);
    ++chosenThisHour_;
    return true;
}

// What an interaction does when an NPC does it: the words the simulation can carry out. `walk-to` takes the person to the thing.
void NpcDirector::carryOut(NpcPopulation& population, int index, const rules::Interaction& interaction, const ActionTarget& target) {
    for (const rules::Effect& effect : interaction.effects) {
        if (effect.verb == "do" && !effect.args.empty() && effect.args[0]->text == "walk-to") {
            const auto [x, y] = pointNear(population, index, target.x, target.y);
            if (x != population.x(index) || y != population.y(index)) population.move(index, x, y);
        }
    }
    const std::int64_t now = static_cast<std::int64_t>(population.ticks());
    cooldowns_.start(population.id(index), interaction.id, now + static_cast<std::int64_t>(interaction.npc->cooldownSeconds) * 20);
    const auto known = std::find(actionNames_.begin(), actionNames_.end(), interaction.id);
    if (known == actionNames_.end()) actionNames_.push_back(interaction.id);
    lastAction_[static_cast<std::size_t>(index)] = static_cast<std::int16_t>(std::find(actionNames_.begin(), actionNames_.end(), interaction.id) - actionNames_.begin());
}

void NpcDirector::postEvent(NpcPopulation& population, const std::string& trigger, int x, int y) {
    int minutes = 0;
    int reach = 0;
    for (const EventDef& def : events_.events) {
        if (def.trigger != trigger) continue;
        minutes = std::max(minutes, def.forMinutes);
        reach = std::max(reach, def.withinMetres * 32);
    }
    if (minutes == 0) return; // nobody listens for this event
    ensure(population.size());
    board_.post(trigger, x, y, population.ticks() + static_cast<std::uint64_t>(minutes) * static_cast<std::uint64_t>(population.ticksPerHour()) / 60U);
    const int hour = population.hourOfDay();
    for (const int index : population.near(x, y, reach)) chooseAction(population, index, hour, true); // the persons it concerns answer at once
}

void NpcDirector::stepFar(NpcPopulation& population, int index, int hour) {
    const std::size_t at = static_cast<std::size_t>(index);
    const Mode current = static_cast<Mode>(modes_[at]);
    if (current == Mode::Dead || current == Mode::Fighting) return;
    modes_[at] = static_cast<std::uint8_t>(Mode::Scheduled); // far away, nobody is eating or running: the day's needs are settled at its end
    followSchedule(population, index, hour, false);
}

// Every person is visited once a day, at the tick of the day that is their index modulo the length of the day: at most population / ticksPerDay persons a tick.
void NpcDirector::farSlice(NpcPopulation& population) {
    const std::uint64_t perDay = static_cast<std::uint64_t>(population.ticksPerDay());
    const std::uint64_t slot = population.ticks() % perDay;
    const int hour = static_cast<int>(slot / static_cast<std::uint64_t>(population.ticksPerHour()));
    for (std::uint64_t i = slot; i < population.size(); i += perDay) {
        const int index = static_cast<int>(i);
        if (scheduleOf_[i] == 0 || population.isNear(index)) continue; // the near ones are looked at every hour
        stepFar(population, index, hour);
    }
}

void NpcDirector::tick(NpcPopulation& population) {
    const std::size_t size = population.size();
    if (modes_.size() < size) ensure(size);
    // A person the director has not seen before takes the place where they stand as their home (unless a home was given). Each person is looked at once.
    for (; adopted_ < size; ++adopted_) {
        if (homeX_[adopted_] != kUnset) continue;
        homeX_[adopted_] = population.x(static_cast<int>(adopted_));
        homeY_[adopted_] = population.y(static_cast<int>(adopted_));
    }
    if (size == 0) return;
    const std::uint64_t perHour = static_cast<std::uint64_t>(population.ticksPerHour());
    if (population.ticks() % perHour == 0) {
        const int hour = population.hourOfDay();
        board_.expire(population.ticks());
        chosenThisHour_ = 0;
        // Everybody near follows their schedule; the budget of choices (maxPerHour) goes round: the hour decides who is first, so nobody is left out hour after hour.
        const std::vector<int> nearby = population.nearFocus();
        const std::size_t start = nearby.empty() ? 0 : static_cast<std::size_t>((population.ticks() / perHour) % nearby.size());
        for (std::size_t k = 0; k < nearby.size(); ++k) stepNear(population, nearby[(start + k) % nearby.size()], hour);
    }
    farSlice(population);
}

std::string NpcDirector::toText() const {
    json schedules = json::array();
    for (std::size_t i = 1; i < schedules_.size(); ++i) schedules.push_back(json{{"day", blocksJson(schedules_[i].day)}, {"night", blocksJson(schedules_[i].night)}});
    json homes = json::array();
    for (std::size_t i = 0; i < homeX_.size(); ++i) {
        homes.push_back(homeX_[i]);
        homes.push_back(homeY_[i]);
    }
    json danger = json::array();
    for (std::size_t i = 0; i < dangers_.size(); ++i) {
        if (dangers_[i] != 0) danger.push_back(i);
    }
    json profiles = json::array();
    for (std::size_t i = 1; i < profiles_.size(); ++i) profiles.push_back(profileJson(profiles_[i]));
    json events = json::array();
    for (const PostedEvent& event : board_.posted()) events.push_back(json::array({event.trigger, event.x, event.y, event.untilTick}));
    json data{{"version", kSaveVersion}, {"schedules", schedules}, {"schedule", runs(scheduleOf_)}, {"mode", runs(modes_)}, {"homes", homes}, {"danger", danger},
              {"profiles", profiles}, {"profile", runs(profileOf_)}, {"events", events}};
    return data.dump() + "\n";
}

NpcDirector NpcDirector::fromText(std::string_view text, rules::ScheduleConfig config) {
    json data;
    try {
        data = json::parse(text);
    } catch (const json::exception& error) {
        throw DataError("npc-life.json", "(syntax)", error.what());
    }
    try {
        if (data.at("version").get<int>() != kSaveVersion) throw DataError("npc-life.json", "version", "is not a version this game reads");
        NpcDirector director(std::move(config));
        for (const json& entry : data.at("schedules")) {
            rules::Schedule schedule;
            schedule.day = blocksFrom(entry.at("day"));
            schedule.night = blocksFrom(entry.at("night"));
            director.schedules_.push_back(schedule);
        }
        director.scheduleOf_ = unrun<std::uint16_t>(data.at("schedule"), "schedule");
        director.modes_ = unrun<std::uint8_t>(data.at("mode"), "mode");
        const json& homes = data.at("homes");
        if (!homes.is_array() || homes.size() != 2 * director.modes_.size() || director.scheduleOf_.size() != director.modes_.size()) throw DataError("npc-life.json", "homes", "does not fit the persons");
        director.dangers_.assign(director.modes_.size(), 0);
        director.homeX_.resize(director.modes_.size());
        director.homeY_.resize(director.modes_.size());
        for (std::size_t i = 0; i < director.modes_.size(); ++i) {
            director.homeX_[i] = homes.at(2 * i).get<std::int32_t>();
            director.homeY_[i] = homes.at(2 * i + 1).get<std::int32_t>();
            if (director.scheduleOf_[i] >= director.schedules_.size()) throw DataError("npc-life.json", "schedule", "names a schedule that is not there");
        }
        for (const json& index : data.at("danger")) {
            const std::size_t at = index.get<std::size_t>();
            if (at < director.dangers_.size()) director.dangers_[at] = 1;
        }
        for (const json& entry : data.at("profiles")) director.profiles_.push_back(profileFrom(entry));
        director.profileOf_ = unrun<std::uint16_t>(data.at("profile"), "profile");
        director.lastAction_.assign(director.modes_.size(), -1);
        if (director.profileOf_.size() != director.modes_.size()) throw DataError("npc-life.json", "profile", "does not fit the persons");
        for (const std::uint16_t id : director.profileOf_) {
            if (id >= director.profiles_.size()) throw DataError("npc-life.json", "profile", "names a profile that is not there");
        }
        for (const json& entry : data.at("events")) {
            if (!entry.is_array() || entry.size() != 4) throw DataError("npc-life.json", "events", "an event is [trigger, x, y, until]");
            director.board_.post(entry.at(0).get<std::string>(), entry.at(1).get<int>(), entry.at(2).get<int>(), entry.at(3).get<std::uint64_t>());
        }
        return director;
    } catch (const json::exception& error) {
        throw DataError("npc-life.json", "(content)", error.what());
    }
}

std::uint64_t NpcDirector::hash() const {
    std::uint64_t h = 0xCBF29CE484222325ULL;
    mix(h, schedules_.size());
    for (const rules::Schedule& schedule : schedules_) {
        for (const std::vector<rules::ScheduleBlock>* blocks : {&schedule.day, &schedule.night}) {
            mix(h, blocks->size());
            for (const rules::ScheduleBlock& block : *blocks) {
                mix(h, static_cast<std::uint64_t>(block.minute));
                for (const char c : block.activity) mix(h, static_cast<unsigned char>(c));
                for (const char c : block.place) mix(h, static_cast<unsigned char>(c));
            }
        }
    }
    mix(h, profiles_.size());
    for (const NpcProfile& profile : profiles_) {
        for (const std::vector<std::string>* names : {&profile.classes, &profile.tags, &profile.classActions, &profile.customActions, &profile.allow, &profile.deny}) {
            mix(h, names->size());
            for (const std::string& name : *names) {
                for (const char c : name) mix(h, static_cast<unsigned char>(c));
            }
        }
        for (const auto& [type, ids] : profile.partnerActions) {
            for (const char c : type) mix(h, static_cast<unsigned char>(c));
            for (const std::string& id : ids) {
                for (const char c : id) mix(h, static_cast<unsigned char>(c));
            }
        }
    }
    for (const PostedEvent& event : board_.posted()) {
        for (const char c : event.trigger) mix(h, static_cast<unsigned char>(c));
        mix(h, static_cast<std::uint64_t>(event.x));
        mix(h, static_cast<std::uint64_t>(event.y));
        mix(h, event.untilTick);
    }
    mix(h, modes_.size());
    for (std::size_t i = 0; i < modes_.size(); ++i) {
        mix(h, profileOf_[i]);
        mix(h, scheduleOf_[i]);
        mix(h, modes_[i]);
        mix(h, dangers_[i]);
        mix(h, static_cast<std::uint64_t>(static_cast<std::int64_t>(homeX_[i])));
        mix(h, static_cast<std::uint64_t>(static_cast<std::int64_t>(homeY_[i])));
    }
    return h;
}

} // namespace odysseus::sim
