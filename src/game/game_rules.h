#pragma once

#include "boundary.h"

#include "sim/interaction.h"
#include "sim/rule_expr.h"

#include <string>
#include <vector>

namespace odysseus::game {

class OdysseyGame;

// The real world as the rule language sees it (US-150, US-151): what `target.state`, `season`, `has(berries, 2)` mean in the game.
// It is made for one moment and one target (a plant, or nothing), reads the game and changes nothing.
// Answered so far: actor.name, target.name, target.kind, target.state, target.inspect, season, time, distance, has(...) and tag(...).
// need, skill, trait, opinion, kin and flag answer 0 until the stories that give them a meaning (US-154, US-164).
class GameRuleContext : public sim::rules::RuleContext {
public:
    // plantIndex: the plant the hero is acting on, or -1 for none.
    GameRuleContext(const OdysseyGame& game, int plantIndex);

    sim::rules::Value path(const std::string& dotted) const override;
    sim::rules::Value call(const std::string& name, const std::vector<sim::rules::Value>& args) const override;

private:
    const OdysseyGame& game_;
    int plantIndex_;
};

// The part of the day as the files name it: morning (6-11), afternoon (12-17), evening (18-21), night (22-5).
std::string timeOfDayWord(int hour);

} // namespace odysseus::game
