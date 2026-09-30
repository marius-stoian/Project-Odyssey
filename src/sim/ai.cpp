#include "sim/ai.h"

#include <algorithm>
#include <format>
#include <vector>

namespace odysseus::sim {

int urgency(int needValue) {
    const int missing = 100 - std::clamp(needValue, 0, 100);
    return missing * missing / 100;
}

std::array<bool, kActionCount> availableActions(const Situation& situation, const ActionConfig& config) {
    std::array<bool, kActionCount> available{};
    const bool daylight = !isNight(config, situation.hour);
    available[static_cast<std::size_t>(Action::Gather)] = daylight && situation.ageYears >= config.workAgeYears;
    available[static_cast<std::size_t>(Action::Hunt)] = daylight && situation.ageYears >= config.huntAgeYears;
    available[static_cast<std::size_t>(Action::Sleep)] = true;
    available[static_cast<std::size_t>(Action::WarmByFire)] = situation.fireLit;
    available[static_cast<std::size_t>(Action::Talk)] = situation.someoneToTalkTo;
    available[static_cast<std::size_t>(Action::GiveGift)] = daylight && situation.canGiveGift;
    available[static_cast<std::size_t>(Action::Steal)] = situation.canSteal;
    available[static_cast<std::size_t>(Action::Rest)] = true;   // always possible: nobody freezes
    available[static_cast<std::size_t>(Action::Wander)] = true; // the last resort
    return available;
}

std::array<int, kActionCount> scoreActions(const Person& person, const Situation& situation,
                                           const std::array<bool, kActionCount>& available, const ActionConfig& config) {
    const int hunger = urgency(person.needs[Need::Hunger]);
    const int tiredness = urgency(person.needs[Need::Energy]);
    const int cold = urgency(person.needs[Need::Warmth]);
    const int loneliness = urgency(person.needs[Need::Social]);
    const int trait = config.traitBonus;
    const int pressure = situation.storePressure * config.storePressureWeight;
    const bool winter = situation.season == Season::Winter;
    const bool night = isNight(config, situation.hour);

    std::array<int, kActionCount> scores{};
    auto set = [&scores](Action action, int score) { scores[static_cast<std::size_t>(action)] = std::max(1, score); };
    // Food: only an empty belly or an empty store sends people out to gather or hunt. With a
    // reason to work, skill and character decide who goes and where.
    // The evening meal covers ordinary hunger, so a belly only counts once it is really
    // empty (urgency above 10, that is Hunger below about 68).
    const int needForFood = std::max(std::max(0, 2 * hunger - 20), pressure);
    int gather = 0;
    int hunt = 0;
    if (needForFood > 0) {
        gather = needForFood + person.gatherSkill / 5 + (person.has(Trait::Diligent) ? trait : 0);
        hunt = needForFood + person.huntSkill / 5 - 20 + (person.has(Trait::Brave) ? trait : 0) -
               (person.has(Trait::Timid) ? trait : 0) + (person.has(Trait::Diligent) ? trait / 2 : 0);
    }
    if (config.gatherYield[static_cast<std::size_t>(situation.season)] == 0) {
        gather /= 4; // little grows in winter
    }
    set(Action::Gather, gather);
    set(Action::Hunt, hunt);
    set(Action::Sleep, 3 * tiredness + (night ? config.nightSleepBonus : 0));
    set(Action::WarmByFire, 3 * cold + (winter ? 10 : 0));
    set(Action::Talk, 2 * loneliness + (person.has(Trait::Talkative) ? trait : 0));
    // Kind people give small gifts (a carved bead, the best berries) when they feel close
    // to others; Greedy people steal from the store when they are hungry.
    set(Action::GiveGift, person.has(Trait::Kind) ? 15 + loneliness : 0);
    set(Action::Steal, person.has(Trait::Greedy) ? 11 + 2 * hunger : 0); // greed tempts even the fed
    set(Action::Rest, 10 + tiredness / 2 + (person.has(Trait::Timid) ? 5 : 0) - (person.has(Trait::Diligent) ? 5 : 0));
    set(Action::Wander, 5);
    for (std::size_t i = 0; i < kActionCount; ++i) {
        if (!available[i]) {
            scores[i] = 0; // cannot be done here and now
        }
    }
    return scores;
}

Decision decide(const Person& person, const Situation& situation, const std::array<bool, kActionCount>& available,
                const ActionConfig& config, core::Pcg32& random) {
    Decision decision;
    decision.scores = scoreActions(person, situation, available, config);
    int best = 0;
    for (const int score : decision.scores) {
        best = std::max(best, score);
    }
    if (best == 0) {
        decision.chosen = Action::Wander; // nothing at all is possible: drift about, never freeze
        return decision;
    }
    // Several actions may share the best score: the seeded stream picks one, always the same.
    std::vector<Action> tied;
    for (std::size_t i = 0; i < kActionCount; ++i) {
        if (decision.scores[i] == best) {
            tied.push_back(static_cast<Action>(i));
        }
    }
    decision.chosen = tied.size() == 1 ? tied.front() : tied[random.below(static_cast<std::uint32_t>(tied.size()))];
    return decision;
}

std::string describeDecision(const Person& person, int daysPerYear) {
    std::string traits;
    for (std::size_t i = 0; i < kTraitCount; ++i) {
        if (person.has(static_cast<Trait>(i))) {
            traits += std::string(traits.empty() ? "" : " ") + traitName(static_cast<Trait>(i));
        }
    }
    std::string text = std::format("{} ({}{}{}): Hunger {}, Energy {}, Warmth {}, Social {} ->", person.name,
                                   person.ageYears(daysPerYear), traits.empty() ? "" : ", ", traits,
                                   person.needs[Need::Hunger], person.needs[Need::Energy], person.needs[Need::Warmth],
                                   person.needs[Need::Social]);
    for (std::size_t i = 0; i < kActionCount; ++i) {
        const auto action = static_cast<Action>(i);
        const int score = person.lastDecision.scores[i];
        text += std::format("{} {} {}{}", i == 0 ? "" : ",", actionName(action), score == 0 ? std::string("-") : std::to_string(score),
                            action == person.lastDecision.chosen ? " (chosen)" : "");
    }
    return text;
}

} // namespace odysseus::sim
