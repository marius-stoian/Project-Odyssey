#pragma once

#include "boundary.h"

#include <filesystem>

namespace odysseus::sim {

// Numbers of the story engine (M2b), from assets/data/sim/story.json, one section per kind
// of interaction. Whole numbers only (Charter rule 6); percentages are 0..100.

// A failed harvest now and then gives the clan a hard winter to live through (US-110).
struct SeasonStory {
    int leanAutumnPercent = 20; // chance each autumn that the harvest fails
    int leanForagePercent = 45; // the land then gives this share of its usual autumn food
};

// How far back an event looks for its causes, and how much of it a person remembers (US-110).
struct CauseStory {
    int theftWindowDays = 20;       // thefts this recent are blamed for an empty store
    int starvationWindowDays = 60;  // an empty store this recent is blamed for a death by hunger
    int maxCauses = 4;              // an event lists at most this many causes
    int grudgeLimit = 16;           // a person holds at most this many grudges
};

// Every morning each person meets someone; people who dislike each other may quarrel (US-111).
struct QuarrelStory {
    int grudgeMeetPercent = 40;   // chance that the one they meet is someone they hold a grudge against
    int dislikeOpinion = -10;     // both must think this badly of each other or worse
    int irritableBelow = 60;      // Hunger or Energy below this makes a person short-tempered
    int irritablePercent = 40;    // chance of a quarrel when short-tempered
    int calmPercent = 6;          // chance of a quarrel otherwise
    int opinionLoss = 12;         // each thinks this much less of the other
    int feeling = -40;            // how badly each remembers it
};

// Grief looks for someone to blame (US-111).
struct BlameStory {
    int opinionLoss = 40;         // the griever thinks this much less of the one blamed
    int feeling = -70;            // a memory kept for life
};

// A feud that keeps worsening ends in a fight, or the clan drives the aggressor out (US-111).
struct RevengeStory {
    int minFeudDays = 10;         // a feud must be this old
    int revengeOpinion = -60;     // and one side must think this badly of the other or worse
    int percentPerDay = 8;        // chance per day that revenge is taken
    int cooldownDays = 60;        // days before the same feud can lead to revenge again
    int exileClanOpinion = -25;   // the clan exiles an aggressor it thinks this badly of (on average)
    int fightDeathPercent = 20;   // chance the loser of a fight dies
    int winnerHurtPercent = 30;   // chance the winner is hurt too
    int satisfaction = 30;        // the aggressor thinks better of the victim after taking revenge
    int victimOpinionLoss = 30;   // the victim thinks worse of the aggressor
    int feeling = -80;            // the victim's memory of the attack
};

// Wounds (US-111) and, later, sickness (US-112).
struct HealthStory {
    int woundDaysMin = 5;         // an injury lasts this many days at least...
    int woundDaysMax = 12;        // ...and at most this many
    int woundDeathPerMille = 15;  // chance per day, in thousandths, that a wound kills
};

struct StoryConfig {
    SeasonStory season;
    CauseStory causes;
    QuarrelStory quarrel;
    BlameStory blame;
    RevengeStory revenge;
    HealthStory health;
};

StoryConfig loadStoryConfig(const std::filesystem::path& file);

} // namespace odysseus::sim
