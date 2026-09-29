#pragma once

#include "boundary.h"

#include "needs.h"

#include <cstdint>
#include <string>

namespace odysseus::sim {

using PersonId = int; // index into World::people(), never reused: the dead keep their place

enum class Sex { Female, Male };

enum class CauseOfDeath { None, Starvation, Cold, OldAge, Hunting, Childbirth };

const char* causeName(CauseOfDeath cause);

// One member of the clan. Plain data: the systems (needs, AI, memory) are functions that
// read and change it. This "struct as component" style is what an ECS formalises later.
struct Person {
    PersonId id = 0;
    std::string name;
    Sex sex = Sex::Female;
    int ageDays = 0;
    bool alive = true;
    CauseOfDeath causeOfDeath = CauseOfDeath::None;
    Needs needs;
    int daysAtZeroHunger = 0; // consecutive days that started with Hunger at 0
    int daysAtZeroWarmth = 0;
};

} // namespace odysseus::sim
