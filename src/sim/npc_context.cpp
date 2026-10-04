#include "sim/npc_context.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>

namespace odysseus::sim {

std::string timeWord(int hour) {
    if (hour >= 6 && hour <= 11) return "morning";
    if (hour >= 12 && hour <= 17) return "afternoon";
    if (hour >= 18 && hour <= 21) return "evening";
    return "night";
}

NpcRuleContext::NpcRuleContext(const NpcPopulation& population, int actorIndex, const std::vector<std::string>& actorTags, const ActionTarget& target, int hour)
    : population_(population), actorIndex_(actorIndex), actorTags_(actorTags), target_(target), hour_(hour) {}

int NpcRuleContext::personOf(const std::string& word) const {
    if (word == "hero") return NpcPopulation::kHero;
    if (word == "actor") return population_.id(actorIndex_);
    if (word == "target" || word == "npc") return target_.kind == ActionTarget::Kind::Person ? target_.id : -1;
    return -1;
}

rules::Value NpcRuleContext::path(const std::string& dotted) const {
    using rules::Value;
    if (dotted == "actor.name") return Value::ofText(population_.kind(actorIndex_));
    if (dotted == "target.name" || dotted == "npc.name") return Value::ofText(target_.name);
    if (dotted == "target.kind" || dotted == "npc.kind") return Value::ofText(target_.kindName);
    if (dotted == "target.state") return Value::ofText({});
    if (dotted == "time") return Value::ofText(timeWord(hour_));
    if (dotted == "distance") {
        const double dx = target_.x - population_.x(actorIndex_);
        const double dy = target_.y - population_.y(actorIndex_);
        return Value::ofNumber(static_cast<long long>(std::llround(std::hypot(dx, dy) / 32.0 * 1000.0)));
    }
    return Value::ofNumber(0);
}

rules::Value NpcRuleContext::call(const std::string& name, const std::vector<rules::Value>& args) const {
    using rules::Value;
    const auto text = [&](std::size_t at) { return at < args.size() && args[at].isText ? args[at].text : std::string(); };
    if (name == "need" && args.size() == 1) {
        // How much of a need is missing: 0 (full) to 100 (desperate).
        for (std::size_t n = 0; n < kNeedCount; ++n) {
            std::string lower = needName(static_cast<Need>(n));
            std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (lower == text(0)) return Value::ofNumber(population_.needsConfig().maximum - population_.need(actorIndex_, static_cast<Need>(n)));
        }
        return Value::ofNumber(0);
    }
    if (name == "opinion" && args.size() == 2) {
        const int holder = personOf(text(0));
        const int about = personOf(text(1));
        if (holder < 0 || about < 0 || !population_.hasOpinions(holder)) return Value::ofNumber(0);
        return Value::ofNumber(population_.opinion(holder, about));
    }
    if (name == "mood" && args.size() == 1) {
        const int who = personOf(text(0));
        if (who < 0 || !population_.hasOpinions(who)) return Value::ofText("neutral");
        // How the person thinks of the one they act on (or of the actor, when it is the target that is asked about).
        const int other = who == population_.id(actorIndex_) ? (target_.kind == ActionTarget::Kind::Person ? target_.id : NpcPopulation::kHero) : population_.id(actorIndex_);
        return Value::ofText(attitudeName(population_.attitude(who, other)));
    }
    if (name == "kin" && args.size() == 2) {
        const int a = personOf(text(0));
        const int b = personOf(text(1));
        const int ia = population_.indexOf(a);
        const int ib = population_.indexOf(b);
        return Value::ofNumber(ia >= 0 && ib >= 0 && population_.family(ia) != 0 && population_.family(ia) == population_.family(ib) ? 1 : 0);
    }
    if (name == "tag" && args.size() == 2) {
        const std::string who = text(0);
        const std::string tag = text(1);
        if (who == "actor") return Value::ofNumber(std::find(actorTags_.begin(), actorTags_.end(), tag) != actorTags_.end() ? 1 : 0);
        if (who == "target" || who == "npc") return Value::ofNumber(std::find(target_.tags.begin(), target_.tags.end(), tag) != target_.tags.end() ? 1 : 0);
    }
    return Value::ofNumber(0); // has, skill, trait, flag: nothing yet for an NPC actor
}

} // namespace odysseus::sim
