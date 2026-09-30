#pragma once

#include "boundary.h"

#include "person.h"

#include "core/random.h"

#include <filesystem>
#include <string>
#include <vector>

namespace odysseus::sim {

// From assets/data/sim/clan.json: who lives in the world at the start.
struct ClanConfig {
    int startingPeople = 20;
    int minimumStartingAgeYears = 2;
    int maximumStartingAgeYears = 45;
    int startingFood = 300; // meals in the clan's shared store
};

ClanConfig loadClanConfig(const std::filesystem::path& file);

// From assets/data/sim/names.json: the names people are given.
struct NameList {
    std::vector<std::string> female;
    std::vector<std::string> male;
};

NameList loadNameList(const std::filesystem::path& file);

// Picks a name for a new person, preferring names nobody has carried yet. When every name has
// been used, an old one returns with an ordinal, "Mira the Second", so the chronicle never
// mixes two people up.
std::string pickName(const NameList& names, Sex sex, const std::vector<Person>& everyone, core::Pcg32& random);

// One or two random traits (never Brave and Timid together).
void giveRandomTraits(Person& person, core::Pcg32& random);

// The founding clan: startingPeople people of random sex and age with full needs.
std::vector<Person> makeStartingClan(const ClanConfig& config, const NameList& names, int daysPerYear, core::Pcg32& random);

} // namespace odysseus::sim
