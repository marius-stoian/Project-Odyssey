#pragma once

#include "boundary.h"

#include "sim/hero_data.h"

#include <filesystem>
#include <string>
#include <vector>

namespace odysseus::sim {

// The switches of whole systems (US-195, EDT-03): each is read once when a game starts, never per frame. A system that is off does not happen at all.
struct PlaySystems {
    bool weather = true;   // the weather cycle and its sky
    bool combat = true;    // enemies, and the hero's weapons against them
    bool rivals = true;    // the rival clans of the region
    bool tutorial = true;  // the first-day quest of a new game
    bool markers = true;   // the arrow that points at the tracked quest step
    bool chronicle = true; // the lines the clan's chronicle gets from conversations
    bool politics = true;  // read by the politics of M13
};

// One rules file of assets/data/rules/: how a game is set up (the growing periods and comforts of the New Game screen), when it is won or lost, and which systems run.
struct PlayRules {
    std::string name = "standard"; // the file's name without .json
    std::string title;             // shown by the picker; the name when empty
    PlaySystems systems;
    // Each part is used when the file gives it; a part it leaves out is the one of rules/standard.json (or of hero/hero.json, the older place, read for one more version).
    bool hasVictory = false;
    int winPercent = 60;           // Trade or Religion at this percent of the region wins
    int combinedWinPercent = 50;   // or the two together at this
    int loseBelowPeople = 3;       // the clan below this many people is defeat
    int rivalFollowerPercent = 60; // the share of a rival clan's people that can follow the hero
    std::vector<PresetConfig> presets;
    std::vector<ComfortConfig> comforts;
};

// Reads assets/data/rules/<name>.json. A mistake is a DataError naming the file and the field; a name with no file is one too.
PlayRules loadPlayRules(const std::filesystem::path& dataDirectory, const std::string& name);

// The names of the rules files, "standard" first, the others by name.
std::vector<std::string> playRuleNames(const std::filesystem::path& dataDirectory);

// Puts the rules over the hero's numbers: the presets and comforts and the thresholds the file gives.
void applyRules(HeroConfig& config, const PlayRules& rules);

// One line for the Game Rules page: "weather on, combat off, ..." and what is won at.
std::string describeRules(const PlayRules& rules);

} // namespace odysseus::sim
