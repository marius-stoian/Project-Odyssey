#pragma once

#include "boundary.h"

#include "sim/action_runner.h"
#include "sim/conversation.h"
#include "sim/dialogue_script.h"
#include "sim/needs.h"
#include "sim/rule_expr.h"

#include <array>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace odysseus::sim::rules {

// Test-play (US-174, D-56 Q15 and Q16): a conversation played in the Editor on a world of its own, with the values the owner chose, so every branch can be
// checked without playing for hours. The world is thrown away at the end: nothing here touches a level, a save or a data file.

// What a test-play knows. Needs are 0 (empty) to 100 (full), as in `Needs`.
struct TestState {
    int opinion = 0; // what the NPC thinks of the hero, -100..100
    std::array<int, kNeedCount> needs{100, 100, 100, 100};
    std::map<std::string, int> items;  // the hero's bag
    std::map<std::string, int> skills; // professions the hero has a level in
    std::set<std::string> traits;
    std::map<std::string, int> flags;  // story notes
    std::set<std::string> tags;        // tags the NPC carries
    bool kin = false;                  // the NPC is family of the hero
    std::string time = "afternoon";    // morning, afternoon, evening, night
    std::string season = "summer";
    std::string heroName = "Hero";
    std::string npcName = "Elder";
    friend bool operator==(const TestState&, const TestState&) = default;
};

// Changes a state from words: `opinion=25 hunger=40 item.berries=2 skill.hunter=3 trait.diligent flag.met-elder tag.trader kin time=night season=winter
// hero=Joro npc=Ama`. A word without `=` is a switch (`trait.x`, `flag.x`, `tag.x`, `kin`). False, with the problem in `problem`, and the state unchanged, on a
// word it does not understand.
bool applyTestState(TestState& state, std::string_view text, std::string& problem);
// The state as the same words, only what differs from a fresh state.
std::string testStateText(const TestState& state);

// The world of one test-play: answers the questions of conditions from the state and carries out the effects on the state, writing each into a log.
class TestWorld final : public RuleContext, public EffectHost {
public:
    explicit TestWorld(TestState state) : state_(std::move(state)) {}

    Value path(const std::string& dotted) const override;
    Value call(const std::string& name, const std::vector<Value>& args) const override;
    void setState(const ThingRef& target, const std::string& state) override;
    void apply(const Effect& effect, int actor, const ThingRef& target) override;
    int ticksPerDay() const override { return 24 * 60 * 20; }

    const TestState& state() const { return state_; }
    const std::vector<std::string>& log() const { return log_; }

private:
    TestState state_;
    std::vector<std::string> log_;
};

// One conversation under test: the script (the graph's current one, saved or not), the world, and the talk. `startNode` begins it anywhere (Play from here).
class TestPlay {
public:
    TestPlay(DlgScript script, const TestState& state, const std::string& startNode = {});

    bool started() const { return started_; } // false when the start node does not exist
    const std::string& nodeId() const { return conversation_.nodeId(); }
    bool finished() const { return conversation_.finished(); }
    ConversationView view() const { return conversation_.view(world_); }
    bool choose(int visibleIndex);
    void leave() { conversation_.leave(); }
    const TestState& state() const { return world_.state(); }
    // What happened so far, one line each: the effects that ran, and (marked "later") the ones that wait.
    std::vector<std::string> log() const;

private:
    TestWorld world_;
    Conversation conversation_;
    ActionRunner runner_;
    bool started_ = true;
};

} // namespace odysseus::sim::rules
