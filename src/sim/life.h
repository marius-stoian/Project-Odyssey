#pragma once

#include "boundary.h"

#include <filesystem>

namespace odysseus::sim {

// From assets/data/sim/life.json: pairing, births, old age and feuds (D-02, US-014).
struct LifeConfig {
    int adultAgeYears = 16;
    int pairOpinion = 60;          // two adults who both like each other at least this much...
    int courtingBonus = 30;        // how much more an unpaired adult wants to talk to a possible partner
    int pairPercent = 10;          // ...pair with this chance each morning
    int fertileFromYears = 16;
    int fertileToYears = 40;
    int conceptionPerMille = 25;   // chance per morning for a paired woman, in thousandths
    int pregnancyDays = 21;        // three seasons of a 28-day year
    int childbirthDeathPercent = 3;
    int birthSpacingYears = 2;     // a mother nurses this long before the next child
    int famineStorePressure = 50;  // no conceptions while the store is this short (0..100)
    int oldAgeFromYears = 45;
    int oldAgePerMillePerYear = 1; // daily chance of dying of old age, per year past oldAgeFromYears
    int feudOpinion = -50;         // two people who both think this badly of each other feud
    int parentChildOpinion = 60;
    int griefFeeling = -70;        // the memory of losing close kin
};

LifeConfig loadLifeConfig(const std::filesystem::path& file);

} // namespace odysseus::sim
