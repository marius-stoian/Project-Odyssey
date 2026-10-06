#include "sim/test_play.h"

#include "core/text.h"

#include "sim/rule_effect.h"

#include <algorithm>
#include <charconv>
#include <format>
#include <sstream>

namespace odysseus::sim::rules {

namespace {

std::optional<int> wholeNumber(const std::string& text) {
    int value = 0;
    const char* first = text.data();
    const char* last = text.data() + text.size();
    const auto [end, error] = std::from_chars(first, last, value);
    if (error != std::errc() || end != last) return std::nullopt;
    return value;
}

bool isHero(const Value& who) { return who.isText && (who.text == "hero" || who.text == "actor"); }
bool isNpc(const Value& who) { return who.isText && (who.text == "npc" || who.text == "target"); }

} // namespace

bool applyTestState(TestState& state, std::string_view text, std::string& problem) {
    TestState next = state;
    std::istringstream in{std::string(text)};
    for (std::string word; in >> word;) {
        const std::size_t equals = word.find('=');
        const std::string key = word.substr(0, equals);
        const std::string value = equals == std::string::npos ? std::string() : word.substr(equals + 1);
        const auto number = wholeNumber(value);
        const auto dot = key.find('.');
        const std::string head = dot == std::string::npos ? key : key.substr(0, dot);
        const std::string tail = dot == std::string::npos ? std::string() : key.substr(dot + 1);
        const auto needNumber = [&](int low, int high) -> bool {
            if (!number || *number < low || *number > high) {
                problem = std::format("\"{}\": the value must be a whole number from {} to {}", word, low, high);
                return false;
            }
            return true;
        };
        if (key == "opinion") {
            if (!needNumber(-100, 100)) return false;
            next.opinion = *number;
        } else if (const auto need = needFromName(key)) {
            if (!needNumber(0, 100)) return false;
            next.needs[static_cast<std::size_t>(*need)] = *number;
        } else if (head == "item" && !tail.empty()) {
            if (!needNumber(0, 9999)) return false;
            next.items[tail] = *number;
        } else if (head == "skill" && !tail.empty()) {
            if (!needNumber(0, 100)) return false;
            next.skills[tail] = *number;
        } else if (head == "flag" && !tail.empty()) {
            if (equals == std::string::npos) next.flags[tail] = 1;
            else if (!needNumber(-1000000, 1000000)) return false;
            else next.flags[tail] = *number;
        } else if ((head == "trait" || head == "tag") && !tail.empty()) {
            const bool on = equals == std::string::npos || value != "0";
            auto& set = head == "trait" ? next.traits : next.tags;
            if (on) set.insert(tail);
            else set.erase(tail);
        } else if (key == "kin") {
            next.kin = equals == std::string::npos || value != "0";
        } else if (key == "time" && !value.empty()) {
            next.time = value;
        } else if (key == "season" && !value.empty()) {
            next.season = value;
        } else if (key == "hero" && !value.empty()) {
            next.heroName = value;
        } else if (key == "npc" && !value.empty()) {
            next.npcName = value;
        } else {
            problem = std::format("\"{}\" is not one of opinion=, hunger= energy= warmth= social=, item.x=, skill.x=, flag.x, trait.x, tag.x, kin, time=, season=, hero=, npc=", word);
            return false;
        }
    }
    state = std::move(next);
    problem.clear();
    return true;
}

std::string testStateText(const TestState& state) {
    const TestState fresh;
    std::string out;
    const auto add = [&](const std::string& word) { out += (out.empty() ? "" : " ") + word; };
    if (state.opinion != fresh.opinion) add(std::format("opinion={}", state.opinion));
    for (std::size_t i = 0; i < kNeedCount; ++i) {
        if (state.needs[i] != fresh.needs[i]) add(std::format("{}={}", core::lowered(needName(static_cast<Need>(i))), state.needs[i]));
    }
    for (const auto& [item, n] : state.items) add(std::format("item.{}={}", item, n));
    for (const auto& [skill, n] : state.skills) add(std::format("skill.{}={}", skill, n));
    for (const auto& [flag, n] : state.flags) add(n == 1 ? "flag." + flag : std::format("flag.{}={}", flag, n));
    for (const std::string& trait : state.traits) add("trait." + trait);
    for (const std::string& tag : state.tags) add("tag." + tag);
    if (state.kin) add("kin");
    if (state.time != fresh.time) add("time=" + state.time);
    if (state.season != fresh.season) add("season=" + state.season);
    if (state.heroName != fresh.heroName) add("hero=" + state.heroName);
    if (state.npcName != fresh.npcName) add("npc=" + state.npcName);
    return out;
}

Value TestWorld::path(const std::string& dotted) const {
    if (dotted == "actor" || dotted == "target" || dotted == "npc" || dotted == "hero") return Value::ofText(dotted); // a name for the call functions to look up
    if (dotted == "actor.name" || dotted == "hero.name") return Value::ofText(state_.heroName);
    if (dotted == "target.name" || dotted == "npc.name") return Value::ofText(state_.npcName);
    if (dotted == "hero.kind" || dotted == "actor.kind") return Value::ofText("hero");
    if (dotted == "npc.kind" || dotted == "target.kind") return Value::ofText("person");
    if (dotted == "season") return Value::ofText(state_.season);
    if (dotted == "time") return Value::ofText(state_.time);
    return Value::ofNumber(0);
}

Value TestWorld::call(const std::string& name, const std::vector<Value>& args) const {
    if (name == "need" && args.size() == 1 && args[0].isText) {
        const auto need = needFromName(args[0].text);
        return Value::ofNumber(need ? 100 - state_.needs[static_cast<std::size_t>(*need)] : 0); // how much is missing, as the game reads it
    }
    if (name == "has") {
        const std::size_t base = args.size() == 3 ? 1 : 0;
        if (args.size() < base + 2 || !args[base].isText) return Value::ofNumber(0);
        if (base == 1 && !isHero(args[0])) return Value::ofNumber(0);
        const auto found = state_.items.find(args[base].text);
        return Value::ofNumber((found == state_.items.end() ? 0 : found->second) >= args[base + 1].number ? 1 : 0);
    }
    if (name == "skill" && args.size() == 1 && args[0].isText) {
        const auto found = state_.skills.find(args[0].text);
        return Value::ofNumber(found == state_.skills.end() ? 0 : found->second);
    }
    if (name == "trait" && args.size() == 1 && args[0].isText) return Value::ofNumber(state_.traits.count(args[0].text) != 0 ? 1 : 0);
    if (name == "opinion" && args.size() == 2) return Value::ofNumber(isNpc(args[0]) && isHero(args[1]) ? state_.opinion : 0);
    if (name == "mood" && args.size() == 1) {
        Needs needs;
        for (std::size_t i = 0; i < kNeedCount; ++i) needs.values[i] = state_.needs[i];
        return Value::ofText(moodWord(state_.opinion, needs));
    }
    if (name == "kin" && args.size() == 2) return Value::ofNumber(state_.kin ? 1 : 0);
    if (name == "flag" && args.size() == 1 && args[0].isText) {
        const auto found = state_.flags.find(args[0].text);
        return Value::ofNumber(found == state_.flags.end() ? 0 : found->second);
    }
    if (name == "tag" && args.size() == 2 && args[0].isText && args[1].isText) {
        if (isNpc(args[0])) return Value::ofNumber(state_.tags.count(args[1].text) != 0 ? 1 : 0);
        if (isHero(args[0])) return Value::ofNumber(args[1].text == "hero" ? 1 : 0);
    }
    return Value::ofNumber(0);
}

void TestWorld::setState(const ThingRef& /*target*/, const std::string& state) { log_.push_back("target state: " + state); }

void TestWorld::apply(const Effect& effect, int /*actor*/, const ThingRef& /*target*/) {
    const auto argText = [&](std::size_t i) {
        if (i >= effect.args.size() || !effect.args[i]) return std::string();
        const Value v = evaluate(*effect.args[i], *this);
        return v.isText ? v.text : std::format("{}", v.number);
    };
    const auto argNumber = [&](std::size_t i) -> long long {
        return i < effect.args.size() && effect.args[i] ? evaluate(*effect.args[i], *this).number : 0;
    };
    const std::string who = argText(0);
    const bool toHero = who == "hero" || who == "actor";
    if ((effect.verb == "give" || effect.verb == "take") && effect.args.size() == 3) {
        const std::string item = argText(1);
        const int n = static_cast<int>(argNumber(2));
        if (toHero) {
            int& held = state_.items[item];
            held = std::max(0, held + (effect.verb == "give" ? n : -n));
            if (held == 0) state_.items.erase(item);
        }
        log_.push_back(std::format("{} {} {} {}", effect.verb == "give" ? "gave" : "took", n, item, effect.verb == "give" ? "to " + who : "from " + who));
    } else if (effect.verb == "opinion" && effect.args.size() == 3) {
        const int n = static_cast<int>(argNumber(2));
        if ((who == "npc" || who == "target") && argText(1) == "hero") state_.opinion = std::clamp(state_.opinion + n, -100, 100);
        log_.push_back(std::format("opinion of {} about {} {:+}", who, argText(1), n));
    } else if (effect.verb == "flag" && !effect.argSources.empty()) {
        state_.flags[effect.argSources[0]] = effect.args.size() >= 2 ? static_cast<int>(argNumber(1)) : 1;
        log_.push_back("flag " + effect.argSources[0]);
    } else if (effect.verb == "remember") {
        log_.push_back("remembered: " + fillDialogueTokens(argText(1), *this));
    } else if (effect.verb == "say" || effect.verb == "chronicle") {
        log_.push_back(effect.verb + ": " + fillDialogueTokens(argText(0), *this));
    } else {
        log_.push_back("effect: " + effect.source); // does nothing in a test-play, but shows the conversation reached it
    }
}

TestPlay::TestPlay(DlgScript script, const TestState& state, const std::string& startNode)
    : world_(state), conversation_(std::move(script), 0, ThingRef{}) {
    if (!startNode.empty()) started_ = conversation_.jumpTo(startNode);
}

bool TestPlay::choose(int visibleIndex) { return conversation_.choose(visibleIndex, world_, runner_, 0, world_); }

std::vector<std::string> TestPlay::log() const {
    std::vector<std::string> lines = world_.log();
    for (const PendingEffect& pending : runner_.pending()) {
        lines.push_back(std::format("later ({} s): {}", pending.dueTick / ActionRunner::kTicksPerSecond, pending.effect.source));
    }
    return lines;
}

} // namespace odysseus::sim::rules
