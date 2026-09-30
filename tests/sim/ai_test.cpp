#include "sim/ai.h"
#include "sim/world.h"

#include <doctest/doctest.h>

#include <string>

using odysseus::sim::Action;
using odysseus::sim::Need;
using odysseus::sim::Person;
using odysseus::sim::Situation;
namespace sim = odysseus::sim;

namespace {

sim::SimConfig realConfig() {
    return sim::loadSimConfig(ODYSSEUS_DATA_DIR);
}

int scoreOf(const sim::Decision& decision, Action action) {
    return decision.scores[static_cast<std::size_t>(action)];
}

} // namespace

TEST_CASE("US-012 Pick best action") {
    const sim::ActionConfig config = realConfig().actions;
    // A grown-up who is very hungry (10) and slightly tired (70), in the middle of the day.
    Person tok;
    tok.name = "Tok";
    tok.needs[Need::Hunger] = 10;
    tok.needs[Need::Energy] = 70;
    Situation noon;
    noon.hour = 12;
    noon.ageYears = 24;
    odysseus::core::Pcg32 random(1, 1);
    const sim::Decision decision = sim::decide(tok, noon, sim::availableActions(noon, config), config, random);

    CHECK((decision.chosen == Action::Gather || decision.chosen == Action::Hunt));
    const int food = std::max(scoreOf(decision, Action::Gather), scoreOf(decision, Action::Hunt));
    for (std::size_t i = 0; i < sim::kActionCount; ++i) {
        CHECK(decision.scores[i] <= food); // a food action scores highest
    }
    MESSAGE("Gather ", scoreOf(decision, Action::Gather), ", Hunt ", scoreOf(decision, Action::Hunt), ", Sleep ",
            scoreOf(decision, Action::Sleep), ", Rest ", scoreOf(decision, Action::Rest));

    // The same person at night sleeps instead: nobody gathers in the dark.
    Situation night = noon;
    night.hour = 2;
    const sim::Decision atNight = sim::decide(tok, night, sim::availableActions(night, config), config, random);
    CHECK(atNight.chosen == Action::Sleep);
    CHECK(scoreOf(atNight, Action::Gather) == 0);
}

TEST_CASE("US-012 No option") {
    const sim::ActionConfig config = realConfig().actions;
    // A lonely, cold, hungry toddler with no fire and nobody awake: nothing reachable
    // satisfies a need except sleep, and sleep is not what they need most.
    Person child;
    child.name = "Ura";
    child.needs[Need::Hunger] = 5;
    child.needs[Need::Warmth] = 5;
    child.needs[Need::Social] = 5;
    Situation alone;
    alone.hour = 12;
    alone.ageYears = 3; // too young to gather or hunt
    alone.fireLit = false;
    alone.someoneToTalkTo = false;
    odysseus::core::Pcg32 random(1, 1);
    const auto available = sim::availableActions(alone, config);
    CHECK_FALSE(available[static_cast<std::size_t>(Action::Gather)]);
    CHECK_FALSE(available[static_cast<std::size_t>(Action::Hunt)]);
    CHECK_FALSE(available[static_cast<std::size_t>(Action::WarmByFire)]);
    CHECK_FALSE(available[static_cast<std::size_t>(Action::Talk)]);
    std::array<bool, sim::kActionCount> onlyIdle{};
    onlyIdle[static_cast<std::size_t>(Action::Rest)] = true;
    onlyIdle[static_cast<std::size_t>(Action::Wander)] = true;
    const sim::Decision idle = sim::decide(child, alone, onlyIdle, config, random);
    CHECK((idle.chosen == Action::Rest || idle.chosen == Action::Wander));

    // Even with nothing at all possible there is an answer, and no error.
    const sim::Decision nothing = sim::decide(child, alone, std::array<bool, sim::kActionCount>{}, config, random);
    CHECK(nothing.chosen == Action::Wander);
}

TEST_CASE("US-012 Inspectable") {
    sim::World world(42, realConfig());
    world.runTicks(static_cast<std::uint64_t>(world.calendar().ticksPerDay()) / 2); // until noon
    const Person& first = world.people().front();
    const std::string text = sim::describeDecision(first, world.calendar().daysPerYear());
    MESSAGE(text);
    CHECK(text.find(first.name) == 0);
    for (std::size_t i = 0; i < sim::kActionCount; ++i) {
        CHECK(text.find(sim::actionName(static_cast<Action>(i))) != std::string::npos); // every action's score
    }
    CHECK(text.find(std::string(sim::actionName(first.lastDecision.chosen)) + " " +
                    std::to_string(scoreOf(first.lastDecision, first.lastDecision.chosen)) + " (chosen)") != std::string::npos);
    CHECK(world.findPerson(first.name) == &first);
    CHECK(world.findPerson("0") == &first);
}

TEST_CASE("US-012 The clan feeds itself through its first year") {
    sim::World world(42, realConfig());
    world.runTicks(world.calendar().ticksPerYear());
    int starved = 0;
    for (const Person& person : world.people()) {
        starved += person.causeOfDeath == sim::CauseOfDeath::Starvation ? 1 : 0;
    }
    MESSAGE("after one year: population ", world.population(), ", food in store ", world.food());
    CHECK(starved == 0);
    CHECK(world.population() >= 18);
}
