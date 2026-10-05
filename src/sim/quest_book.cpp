#include "sim/quest_book.h"

#include "sim/rule_json.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <format>

namespace odysseus::sim::rules {

namespace {

constexpr int kMaxStepsPerUpdate = 64; // a quest that loops through steps whose objectives already hold must not freeze the game
constexpr int kSaveVersion = 1;

// The world the quest conditions see: quest(id) and step(id) are answered by the book, everything else by the game.
class BookContext final : public RuleContext {
public:
    BookContext(const RuleContext& inner, const QuestBook& book) : inner_(inner), book_(book) {}
    Value path(const std::string& dotted) const override { return inner_.path(dotted); }
    Value call(const std::string& name, const std::vector<Value>& args) const override {
        if (name == "quest" && args.size() == 1) return Value::ofText(statusWord(book_.status(args[0].text)));
        if (name == "step" && args.size() == 1) return Value::ofText(book_.activeStep(args[0].text));
        return inner_.call(name, args);
    }

private:
    const RuleContext& inner_;
    const QuestBook& book_;
};

bool allTrue(const std::vector<QuestCondition>& list, const RuleContext& context) {
    return std::all_of(list.begin(), list.end(), [&](const QuestCondition& c) { return isTrue(*c.expr, context); });
}

bool anyTrue(const std::vector<QuestCondition>& list, const RuleContext& context) {
    return std::any_of(list.begin(), list.end(), [&](const QuestCondition& c) { return isTrue(*c.expr, context); });
}

std::int64_t waitTicks(const QuestObjective& o, int ticksPerDay) {
    switch (o.waitUnit) {
    case DelayUnit::Seconds: return static_cast<std::int64_t>(o.amount) * ActionRunner::kTicksPerSecond;
    case DelayUnit::Minutes: return static_cast<std::int64_t>(o.amount) * 60 * ActionRunner::kTicksPerSecond;
    case DelayUnit::Days: return static_cast<std::int64_t>(o.amount) * ticksPerDay;
    }
    return 0;
}

bool isCounted(QuestObjective::Kind kind) {
    return kind == QuestObjective::Kind::Gather || kind == QuestObjective::Kind::Give || kind == QuestObjective::Kind::Craft || kind == QuestObjective::Kind::Defeat;
}

bool isHappening(QuestObjective::Kind kind) { return kind != QuestObjective::Kind::Wait && kind != QuestObjective::Kind::Flag; }

QuestStatus statusFromWord(const std::string& word, bool& ok) {
    ok = true;
    for (QuestStatus s : {QuestStatus::Locked, QuestStatus::Available, QuestStatus::Active, QuestStatus::Done, QuestStatus::Failed}) {
        if (word == statusWord(s)) return s;
    }
    ok = false;
    return QuestStatus::Locked;
}

} // namespace

const char* statusWord(QuestStatus status) {
    switch (status) {
    case QuestStatus::Locked: return "locked";
    case QuestStatus::Available: return "available";
    case QuestStatus::Active: return "active";
    case QuestStatus::Done: return "done";
    case QuestStatus::Failed: return "failed";
    }
    return "locked";
}

QuestBook::QuestBook(std::vector<Quest> quests) : quests_(std::move(quests)) {
    std::sort(quests_.begin(), quests_.end(), [](const Quest& a, const Quest& b) { return a.id < b.id; });
}


void QuestBook::replaceQuests(std::vector<Quest> quests) {
    quests_ = std::move(quests);
    std::sort(quests_.begin(), quests_.end(), [](const Quest& a, const Quest& b) { return a.id < b.id; });
    for (auto it = states_.begin(); it != states_.end();) {
        const Quest* q = find(it->first);
        // A quest that is gone, or whose step is gone, loses its progress rather than pointing at nothing.
        if (q == nullptr || (it->second.status == QuestStatus::Active && q->find(it->second.step) == nullptr)) it = states_.erase(it);
        else ++it;
    }
}

const Quest* QuestBook::find(std::string_view id) const {
    for (const Quest& q : quests_) {
        if (q.id == id) return &q;
    }
    return nullptr;
}

const QuestState* QuestBook::state(std::string_view id) const {
    const auto it = states_.find(std::string(id));
    return it == states_.end() ? nullptr : &it->second;
}

QuestState* QuestBook::stateOf(std::string_view id) {
    if (find(id) == nullptr) return nullptr;
    return &states_[std::string(id)];
}

QuestStatus QuestBook::status(std::string_view id) const {
    const QuestState* s = state(id);
    return s == nullptr ? QuestStatus::Locked : s->status;
}

std::string QuestBook::activeStep(std::string_view id) const {
    const QuestState* s = state(id);
    return s != nullptr && s->status == QuestStatus::Active ? s->step : std::string();
}

void QuestBook::enter(const Quest& quest, QuestState& state, const std::string& step, std::int64_t now) {
    (void)quest;
    state.step = step;
    state.stepStartTick = now;
    state.lastProgressTick = now;
    state.progress = 0;
}

void QuestBook::finish(const Quest& quest, QuestState& state, std::int64_t now, ActionRunner* runner, EffectHost* host, const ThingRef& self) {
    state.status = QuestStatus::Done;
    if (runner != nullptr && host != nullptr && !quest.rewards.empty()) runner->runEffects(quest.rewards, 0, self, now, *host);
}

bool QuestBook::start(std::string_view id, std::int64_t now) {
    const Quest* quest = find(id);
    QuestState* st = stateOf(id);
    if (quest == nullptr || st == nullptr || st->status != QuestStatus::Available || quest->steps.empty()) return false;
    st->status = QuestStatus::Active;
    enter(*quest, *st, quest->start, now);
    return true;
}

bool QuestBook::complete(std::string_view id, std::int64_t now, ActionRunner* runner, EffectHost* host, const ThingRef& self) {
    const Quest* quest = find(id);
    QuestState* st = stateOf(id);
    if (quest == nullptr || st == nullptr || st->status == QuestStatus::Done) return false;
    finish(*quest, *st, now, runner, host, self);
    return true;
}

bool QuestBook::fail(std::string_view id) {
    QuestState* st = stateOf(id);
    if (st == nullptr || st->status == QuestStatus::Done || st->status == QuestStatus::Failed) return false;
    st->status = QuestStatus::Failed;
    return true;
}

bool QuestBook::reset(std::string_view id) {
    if (find(id) == nullptr) return false;
    states_.erase(std::string(id));
    return true;
}

bool QuestBook::jumpTo(std::string_view id, std::string_view step, std::int64_t now) {
    const Quest* quest = find(id);
    QuestState* st = stateOf(id);
    if (quest == nullptr || st == nullptr || quest->find(step) == nullptr) return false;
    if (st->status == QuestStatus::Locked) st->status = QuestStatus::Available;
    if (st->status == QuestStatus::Available) st->status = QuestStatus::Active;
    if (st->status != QuestStatus::Active) st->status = QuestStatus::Active; // the debugger may revive a finished quest
    enter(*quest, *st, std::string(step), now);
    return true;
}

bool QuestBook::objectiveMet(const Quest& quest, QuestState& state, const RuleContext& world, std::int64_t now, int ticksPerDay) {
    const QuestStep* step = quest.find(state.step);
    if (step == nullptr) return false;
    const QuestObjective& o = step->objective;
    switch (o.kind) {
    case QuestObjective::Kind::Wait: return now - state.stepStartTick >= waitTicks(o, ticksPerDay);
    case QuestObjective::Kind::Flag: return world.call("flag", {Value::ofText(o.subject)}).number > 0;
    default: return state.progress >= o.amount;
    }
}

std::vector<QuestChange> QuestBook::update(const RuleContext& world, std::int64_t now, int ticksPerDay, ActionRunner& runner, EffectHost& host, const ThingRef& self) {
    std::vector<QuestChange> changes;
    const BookContext context(world, *this);

    for (const Quest& quest : quests_) {
        QuestState& st = states_[quest.id];

        if (st.status == QuestStatus::Locked && allTrue(quest.requires_, context)) {
            st.status = QuestStatus::Available;
            changes.push_back({QuestChange::Kind::Available, quest.id, {}});
        }
        if (st.status == QuestStatus::Available && quest.giver == "none" && start(quest.id, now)) {
            changes.push_back({QuestChange::Kind::Started, quest.id, st.step});
        }
        if (st.status != QuestStatus::Active) continue;

        if (anyTrue(quest.fail, context)) {
            st.status = QuestStatus::Failed;
            changes.push_back({QuestChange::Kind::Failed, quest.id, {}});
            continue;
        }

        // Events first, in the order they happened; each counts for the step the quest is on at that moment.
        std::size_t next = 0;
        int guard = 0;
        while (st.status == QuestStatus::Active && guard < kMaxStepsPerUpdate) {
            const QuestStep* step = quest.find(st.step);
            if (step == nullptr) break;
            bool moved = false;
            for (; next < events_.size() && !moved; ++next) {
                const QuestEvent& e = events_[next];
                const QuestObjective& o = step->objective;
                if (e.tick < st.stepStartTick || e.kind != o.kind || e.subject != o.subject) continue;
                if (isCounted(o.kind)) st.progress = std::min(o.amount, st.progress + e.amount);
                else if (isHappening(o.kind)) st.progress = o.amount;
                st.lastProgressTick = now;
                if (objectiveMet(quest, st, context, now, ticksPerDay)) moved = true;
            }
            if (!moved && !objectiveMet(quest, st, context, now, ticksPerDay)) break;

            // The step is done: the first branch that holds, otherwise `next`.
            std::string target = step->next;
            for (const QuestBranch& b : step->branches) {
                if (isTrue(*b.condition.expr, context)) {
                    target = b.to;
                    break;
                }
            }
            if (target == kQuestEnd) {
                finish(quest, st, now, &runner, &host, self);
                changes.push_back({QuestChange::Kind::Done, quest.id, {}});
            } else {
                enter(quest, st, target, now);
                changes.push_back({QuestChange::Kind::Step, quest.id, target});
            }
            ++guard;
        }
    }
    events_.clear();
    return changes;
}

bool QuestBook::holds(const RuleContext& world, const QuestCondition& condition) const {
    const BookContext context(world, *this);
    return condition.expr != nullptr && isTrue(*condition.expr, context);
}

bool QuestBook::hintDue(std::string_view id, std::int64_t now) const {
    const Quest* quest = find(id);
    const QuestState* st = state(id);
    if (quest == nullptr || st == nullptr || st->status != QuestStatus::Active) return false;
    const QuestStep* step = quest->find(st->step);
    if (step == nullptr || !step->hint) return false;
    return now - st->lastProgressTick >= static_cast<std::int64_t>(step->hint->afterSeconds) * ActionRunner::kTicksPerSecond;
}

std::string QuestBook::save(std::int64_t now) const {
    nlohmann::json quests = nlohmann::json::object();
    for (const auto& [id, st] : states_) {
        if (st.status == QuestStatus::Locked) continue;
        quests[id] = {{"status", statusWord(st.status)},
                      {"step", st.step},
                      {"age", now - st.stepStartTick},
                      {"idle", now - st.lastProgressTick},
                      {"progress", st.progress}};
    }
    return nlohmann::json{{"version", kSaveVersion}, {"quests", quests}}.dump(1);
}

std::vector<std::string> QuestBook::load(const std::string& text, std::int64_t now) {
    std::vector<std::string> notes;
    states_.clear();
    events_.clear();
    try {
        const nlohmann::json data = nlohmann::json::parse(text);
        if (data.at("version").get<int>() != kSaveVersion) return {"the quest state was saved by another version and was not loaded"};
        for (const auto& [id, item] : data.at("quests").items()) {
            const Quest* quest = find(id);
            if (quest == nullptr) {
                notes.push_back(std::format("quest \"{}\" is not in the data any more; its progress was dropped", id));
                continue;
            }
            bool ok = false;
            QuestState st;
            st.status = statusFromWord(item.at("status").get<std::string>(), ok);
            if (!ok) {
                notes.push_back(std::format("quest \"{}\" has an unknown status and was left locked", id));
                continue;
            }
            st.step = item.at("step").get<std::string>();
            if (st.status == QuestStatus::Active && quest->find(st.step) == nullptr) {
                notes.push_back(std::format("quest \"{}\" was on step \"{}\", which is not in the data any more; it restarts at its first step", id, st.step));
                st.step = quest->start;
                st.stepStartTick = now;
                st.lastProgressTick = now;
                st.progress = 0;
            } else {
                st.stepStartTick = now - item.at("age").get<std::int64_t>();
                st.lastProgressTick = now - item.at("idle").get<std::int64_t>();
                st.progress = item.at("progress").get<int>();
            }
            states_[id] = st;
        }
    } catch (const std::exception& error) {
        notes.push_back(std::string("the quest state could not be read: ") + error.what());
        states_.clear();
    }
    return notes;
}

std::uint64_t QuestBook::hash() const {
    std::uint64_t h = 1469598103934665603ULL;
    const auto mix = [&h](std::uint64_t v) {
        h ^= v;
        h *= 1099511628211ULL;
    };
    const auto mixText = [&](const std::string& s) {
        for (unsigned char c : s) mix(c);
        mix(0xff);
    };
    for (const auto& [id, st] : states_) {
        mixText(id);
        mix(static_cast<std::uint64_t>(st.status));
        mixText(st.step);
        mix(static_cast<std::uint64_t>(st.stepStartTick));
        mix(static_cast<std::uint64_t>(st.lastProgressTick));
        mix(static_cast<std::uint64_t>(st.progress));
    }
    return h;
}

void QuestBook::clearStates() {
    states_.clear();
    events_.clear();
}

} // namespace odysseus::sim::rules
