#pragma once

#include "boundary.h"

#include <string>
#include <vector>

namespace odysseus::sim {

class World;

// One episode of the clan's story (M2b, US-115): a group of linked events with a beginning
// (the cause), a turn and an end. Episodes are derived from the chronicle, never stored, so
// old and new saves tell their stories alike.
struct Episode {
    std::string name;          // "The Hard Winter of year 74"
    std::vector<int> events;   // chronicle ids, oldest first
    int beginning = -1;        // the first event: why it all started
    int turn = -1;             // the most important event in between
    int end = -1;              // the last event: what came of it
    std::vector<int> people;   // the people most involved, most first
    int score = 0;             // how much the episode matters (for choosing the best)
};

// The best episodes of the chronicle so far, in the order they began: at most
// EpisodeStory::maxPerCentury for every hundred years.
std::vector<Episode> findEpisodes(const World& world);

// One short paragraph: the name, how it began, the turn, the end and the people.
std::string formatEpisode(const World& world, const Episode& episode);

// The whole printed story: the episodes, then the births, deaths, pairings, partings and feuds
// (at or above `threshold`) with their reasons.
std::vector<std::string> formatStory(const World& world, int threshold);

} // namespace odysseus::sim
