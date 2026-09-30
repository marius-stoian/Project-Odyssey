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

struct StoryConfig {
    SeasonStory season;
    CauseStory causes;
};

StoryConfig loadStoryConfig(const std::filesystem::path& file);

} // namespace odysseus::sim
