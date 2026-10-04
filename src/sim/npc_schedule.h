#pragma once

#include "boundary.h"

#include "sim/economy.h"
#include "sim/needs.h"

#include <array>
#include <filesystem>
#include <vector>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace odysseus::sim::rules {

// A day of an NPC (US-290, D-54 Q9): blocks of time, each with an activity and a place. A block lasts until the next one begins and the day wraps round midnight, so
// "22:00 sleep at home" holds until the morning block. Whole minutes of the day; the simulation looks at the schedule on the hour.
struct ScheduleBlock {
    int minute = 0;        // when it begins, 0 to 1439
    std::string activity;  // work, eat, sleep, rest, idle, go, patrol, or the id of an interaction
    std::string place;     // the name of a place of the level, or "home" (where the NPC was placed)
    friend bool operator==(const ScheduleBlock&, const ScheduleBlock&) = default;
};

// The day and, when the night differs, the night variant (D-54 Q9). With no night blocks the day blocks hold at night too.
struct Schedule {
    std::vector<ScheduleBlock> day;
    std::vector<ScheduleBlock> night;

    bool empty() const { return day.empty() && night.empty(); }
    friend bool operator==(const Schedule&, const Schedule&) = default;
};

// "06:00" to 360 and back. Nothing for text that is not HH:MM with a valid hour and minute.
std::optional<int> parseClock(std::string_view text);
std::string formatClock(int minute);

// The block in force at a minute of the day: the last block that begins at or before it; before the first block of the day it is the last block (it came over
// midnight). At night the night blocks are used when there are any. Null for an empty schedule.
const ScheduleBlock* activeBlock(const Schedule& schedule, int minuteOfDay, bool night);

// The Editor's text of a list of blocks: "06:00 work market; 21:00 sleep home" (the place is optional: home), and back. Blocks come back sorted by time; two at the same
// time, a bad time, or an activity or place that is no word is a mistake with the reason.
std::string scheduleText(const std::vector<ScheduleBlock>& blocks);
std::optional<std::vector<ScheduleBlock>> parseScheduleText(std::string_view text, std::string& problem);

// What the schedule names that the level or the data does not have: a place that is no place of the level, an activity that is neither one of the activity words nor an
// interaction. One sentence each, for the log and the Editor.
std::vector<std::string> scheduleProblems(const Schedule& schedule, const std::set<std::string>& places, const std::set<std::string>& activities, const std::set<std::string>& interactions);

// From assets/data/sim/schedule.json (US-290): where the night is, what an interruption does, and what each activity gives back every hour.
struct ScheduleConfig {
    int nightFromHour = 21; // the night is from this hour ...
    int nightToHour = 6;    // ... to this one, round midnight
    int eatBelow = 20;      // hunger under this sends a person to eat (D-54 Q10)
    int eatRestore = 40;    // an hour of eating gives this much hunger back
    std::string eatPlace = "home";
    std::string dangerPlace = "home"; // danger or fear sends a person here
    int scatterPixels = 48; // persons at the same named place stand within this many pixels of it, not on one spot
    // Per activity word, what an hour of it restores of each need (a person near the hero; the daily rules restore the far ones).
    std::map<std::string, std::array<int, kNeedCount>> activities;
    // The activities during which a person is free to do something of their own (US-291): its class, custom and event actions. A person asleep or eating is not.
    std::set<std::string> freeActivities{"idle", "work", "go", "patrol"};
    int maxPerHour = 64; // at most this many persons choose an action on one hour mark (the budget of ADR-022)
    // Persons act on each other (US-292, D-54 Q12, Q13): how close two persons must be to meet, how a fight runs, what the witnesses and the family of the dead feel, how often a far
    // person has a dealing in its daily visit, how much a partner type an NPC prefers adds to the score (US-293), and the combat numbers of a person nobody gave any.
    int meetRadius = 96;         // pixels (3 m)
    int fightRoundsPerHour = 6;  // a fight near the hero: rounds of strike and strike back an hour
    int farFightRounds = 12;     // a far fight is settled at once, in this many rounds at most
    int witnessOpinion = 6;      // witnesses who know the victim think this much less of the attacker
    int griefOpinion = 30;       // the family of the dead think this much less of the killer
    int farPercent = 5;          // the chance in a hundred that a far person has a dealing when it is visited (once a day)
    int preferBonus = 40;        // added to the score of an action the NPC prefers for the kind of partner it meets
    int defaultHp = 100;
    int defaultDamage = 5;
    int chatSocial = 10;         // the Social need a conversation gives back to both
    std::vector<std::string> chatter{"Fine day.", "Have you eaten?", "The nights are getting cold.", "Mind the wolves.", "Did you see the smoke?"};

    bool isNight(int hour) const { return nightFromHour > nightToHour ? (hour >= nightFromHour || hour < nightToHour) : (hour >= nightFromHour && hour < nightToHour); }
    bool knownActivity(const std::string& word) const { return activities.count(word) != 0; }
};

ScheduleConfig loadScheduleConfig(const std::filesystem::path& file);

} // namespace odysseus::sim::rules
