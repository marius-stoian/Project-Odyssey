#pragma once

#include "boundary.h"

#include <filesystem>
#include <string>
#include <vector>

namespace odysseus::game {

// The first ten minutes (US-090): an elder asks for a few actions in order, one at a time. The script is data
// (assets/data/hero/tutorial.json), so the words can change without touching code.
struct TutorialStep {
    std::string goal; // what the player must do: gather, eat, tend
    std::string say;
    std::string hint;
};

struct TutorialScript {
    int hintAfterSeconds = 120;
    std::string elder = "Elder";
    std::vector<TutorialStep> steps;
    std::string done;
};

// Every problem is a DataError naming the file and the field.
TutorialScript loadTutorial(const std::filesystem::path& file);

class Tutorial {
public:
    // 20 game ticks make a second.
    static constexpr int kTicksPerSecond = 20;

    void start(const TutorialScript& script);
    void stop() { active_ = false; }
    bool active() const { return active_; }
    bool finished() const { return finished_; }
    std::size_t step() const { return step_; }

    // The player did something ("gather", "eat", "tend"). Only the current step's goal moves the tutorial on.
    // Returns true when this finished a step.
    bool notify(const std::string& goal);
    // One game tick with no screen open: after hintAfterSeconds without progress the elder hints.
    void tick();

    // What the elder says now: the step's words, its hint once the player is stuck, or the closing line. Empty when off.
    std::string text() const;
    bool hinting() const { return active_ && !finished_ && idleTicks_ >= hintTicks(); }
    const std::string& elder() const { return script_.elder; }

private:
    int hintTicks() const { return script_.hintAfterSeconds * kTicksPerSecond; }
    TutorialScript script_;
    bool active_ = false;
    bool finished_ = false;
    std::size_t step_ = 0;
    int idleTicks_ = 0;
    int closingTicks_ = 0;
};

} // namespace odysseus::game
