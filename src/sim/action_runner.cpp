#include "sim/action_runner.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <format>

namespace odysseus::sim::rules {

namespace {

constexpr int kTicksPerMinute = 60 * ActionRunner::kTicksPerSecond;

std::int64_t delayTicks(const Effect& effect, EffectHost& host) {
    switch (effect.delayUnit) {
    case DelayUnit::Seconds: return static_cast<std::int64_t>(effect.delayAmount) * ActionRunner::kTicksPerSecond;
    case DelayUnit::Minutes: return static_cast<std::int64_t>(effect.delayAmount) * kTicksPerMinute;
    case DelayUnit::Days: return static_cast<std::int64_t>(effect.delayAmount) * host.ticksPerDay();
    }
    return 0;
}

std::uint64_t mix(std::uint64_t hash, std::uint64_t value) {
    hash ^= value + 0x9E3779B97F4A7C15ULL + (hash << 6) + (hash >> 2);
    return hash;
}

std::uint64_t mixText(std::uint64_t hash, const std::string& text) {
    for (const char c : text) hash = mix(hash, static_cast<unsigned char>(c));
    return hash;
}

} // namespace

bool ActionRunner::start(const Interaction& interaction, int actor, const ThingRef& target, std::int64_t now, EffectHost& host) {
    if (running(actor) != nullptr) return false;
    if (interaction.durationMilli <= 0) {
        for (const Effect& effect : interaction.effects) run(effect, actor, target, now, host);
        return true;
    }
    RunningAction action;
    action.actor = actor;
    action.interaction = interaction.id;
    action.target = target;
    action.startTick = now;
    // Whole ticks, never less than one, so a very short action still takes a moment.
    action.endTick = now + std::max<std::int64_t>(1, (static_cast<std::int64_t>(interaction.durationMilli) * kTicksPerSecond + 500) / 1000);
    const auto at = std::lower_bound(running_.begin(), running_.end(), actor, [](const RunningAction& a, int id) { return a.actor < id; });
    running_.insert(at, action);
    return true;
}

void ActionRunner::cancel(int actor) {
    std::erase_if(running_, [actor](const RunningAction& a) { return a.actor == actor; });
}

void ActionRunner::run(const Effect& effect, int actor, const ThingRef& target, std::int64_t now, EffectHost& host) {
    if (effect.verb == "after" && effect.inner) {
        PendingEffect pending;
        pending.dueTick = now + delayTicks(effect, host);
        pending.seq = nextSeq_++;
        pending.actor = actor;
        pending.target = target;
        pending.effect = *effect.inner;
        const auto at = std::upper_bound(pending_.begin(), pending_.end(), pending, [](const PendingEffect& a, const PendingEffect& b) {
            return a.dueTick != b.dueTick ? a.dueTick < b.dueTick : a.seq < b.seq;
        });
        pending_.insert(at, std::move(pending));
        return;
    }
    if (effect.verb == "set" && !effect.args.empty() && effect.args[0]->kind == Expr::Kind::Path && effect.args[0]->text == "target.state" && effect.args.size() == 2 &&
        effect.args[1]->kind == Expr::Kind::Text) {
        host.setState(target, effect.args[1]->text);
        return;
    }
    host.apply(effect, actor, target);
}

void ActionRunner::tick(std::int64_t now, const InteractionRegistry& registry, EffectHost& host) {
    // Effects whose time has come, in the order they were due and made. An effect may make new ones; those wait for their own time.
    while (!pending_.empty() && pending_.front().dueTick <= now) {
        const PendingEffect due = pending_.front();
        pending_.erase(pending_.begin());
        run(due.effect, due.actor, due.target, now, host);
    }
    // Actions whose duration is over, lowest actor first.
    std::vector<RunningAction> finished;
    for (const RunningAction& action : running_) {
        if (now >= action.endTick) finished.push_back(action);
    }
    std::erase_if(running_, [now](const RunningAction& a) { return now >= a.endTick; });
    for (const RunningAction& action : finished) {
        const Interaction* interaction = registry.find(action.interaction);
        if (interaction == nullptr) continue; // taken out of the data while it ran: nothing happens
        for (const Effect& effect : interaction->effects) run(effect, action.actor, action.target, now, host);
    }
}

const RunningAction* ActionRunner::running(int actor) const {
    for (const RunningAction& action : running_) {
        if (action.actor == actor) return &action;
    }
    return nullptr;
}

int ActionRunner::progress(int actor, std::int64_t now) const {
    const RunningAction* action = running(actor);
    if (action == nullptr) return -1;
    const std::int64_t span = action->endTick - action->startTick;
    if (span <= 0) return 100;
    return static_cast<int>(std::clamp<std::int64_t>((now - action->startTick) * 100 / span, 0, 100));
}

std::string ActionRunner::savePending(std::int64_t now) const {
    nlohmann::json list = nlohmann::json::array();
    for (const PendingEffect& p : pending_) {
        list.push_back({{"in", std::max<std::int64_t>(0, p.dueTick - now)}, {"actor", p.actor}, {"kind", p.target.kind}, {"id", p.target.id}, {"effect", p.effect.source}});
    }
    return nlohmann::json{{"version", 1}, {"pending", list}}.dump(1);
}

std::vector<std::string> ActionRunner::loadPending(const std::string& text, std::int64_t now) {
    std::vector<std::string> notes;
    pending_.clear();
    nextSeq_ = 0;
    try {
        const nlohmann::json data = nlohmann::json::parse(text);
        if (data.at("version").get<int>() != 1) {
            notes.push_back("The timers were saved by another version and were not loaded");
            return notes;
        }
        for (const nlohmann::json& entry : data.at("pending")) {
            ParsedEffect parsed = parseEffect(entry.at("effect").get<std::string>());
            if (parsed.problem) {
                notes.push_back(std::format("A saved timer was dropped: {}", parsed.problem->message));
                continue;
            }
            PendingEffect pending;
            pending.dueTick = now + entry.at("in").get<std::int64_t>();
            pending.seq = nextSeq_++; // saved in order: the order is kept
            pending.actor = entry.at("actor").get<int>();
            pending.target = {entry.at("kind").get<int>(), entry.at("id").get<int>()};
            pending.effect = std::move(parsed.effect);
            pending_.push_back(std::move(pending));
        }
        std::stable_sort(pending_.begin(), pending_.end(), [](const PendingEffect& a, const PendingEffect& b) { return a.dueTick < b.dueTick; });
    } catch (const std::exception& error) {
        pending_.clear();
        notes.push_back(std::string("The saved timers could not be read: ") + error.what());
    }
    return notes;
}

std::uint64_t ActionRunner::hash() const {
    std::uint64_t h = 0xCBF29CE484222325ULL;
    for (const RunningAction& a : running_) {
        h = mix(h, static_cast<std::uint64_t>(a.actor));
        h = mixText(h, a.interaction);
        h = mix(h, static_cast<std::uint64_t>(a.target.kind) * 1000003ULL + static_cast<std::uint64_t>(a.target.id));
        h = mix(h, static_cast<std::uint64_t>(a.startTick));
        h = mix(h, static_cast<std::uint64_t>(a.endTick));
    }
    for (const PendingEffect& p : pending_) {
        h = mix(h, static_cast<std::uint64_t>(p.dueTick));
        h = mix(h, static_cast<std::uint64_t>(p.seq));
        h = mix(h, static_cast<std::uint64_t>(p.actor));
        h = mix(h, static_cast<std::uint64_t>(p.target.kind) * 1000003ULL + static_cast<std::uint64_t>(p.target.id));
        h = mixText(h, p.effect.source);
    }
    return h;
}

void ActionRunner::clear() {
    running_.clear();
    pending_.clear();
    nextSeq_ = 0;
}

} // namespace odysseus::sim::rules
