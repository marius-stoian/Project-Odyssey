#pragma once

#include "boundary.h"

#include "sim/npc_population.h"
#include "sim/rule_expr.h"

#include <string>
#include <vector>

namespace odysseus::sim {

// What an NPC may act on (US-291, US-292): a place of the level, another person, an animal, or the spot of an event. Plain data: the director makes one for each thing near an
// NPC that it might do something to.
struct ActionTarget {
    enum class Kind { Place, Person, Animal, Event };
    Kind kind = Kind::Place;
    int id = -1;           // the person's id in the population, or the animal's id in the level; -1 for a place or an event
    std::string name;      // "market", a person's kind, an animal's kind
    std::string kindName;  // what `target.kind` says
    int x = 0;             // world pixels
    int y = 0;
    std::vector<std::string> tags;
};

// The rule language over two NPCs of a population (the game implements it over the hero and the real world; this one is for NPC actors, US-291 and US-292): `actor` is who acts,
// `target` is what they act on, `npc` means the target as in the files of the hero's interactions. It answers: actor.name, target.name, target.kind, target.state (empty), time
// (the part of the day), distance (thousandths of a metre), need(hunger) of the actor, opinion(a, b) between two persons, mood(x), kin(a, b), tag(x, tag) and the rest as 0.
class NpcRuleContext final : public rules::RuleContext {
public:
    NpcRuleContext(const NpcPopulation& population, int actorIndex, const std::vector<std::string>& actorTags, const ActionTarget& target, int hour);

    rules::Value path(const std::string& dotted) const override;
    rules::Value call(const std::string& name, const std::vector<rules::Value>& args) const override;

private:
    // The id of the person a word of the rules means: actor, target (or npc), hero (0), or -1.
    int personOf(const std::string& word) const;

    const NpcPopulation& population_;
    int actorIndex_;
    const std::vector<std::string>& actorTags_;
    const ActionTarget& target_;
    int hour_;
};

// "morning" (6-11), "afternoon" (12-17), "evening" (18-21), "night" (22-5): the words the interaction files use for the time of day.
std::string timeWord(int hour);

} // namespace odysseus::sim
