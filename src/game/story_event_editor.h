#pragma once

#include "boundary.h"

#include "luna/engine/input.h"
#include "luna/engine/ui.h"
#include "sim/hero_data.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace odysseus::game {

// The Story events list of the Editor (US-185, EDT-02): the crossroads events of assets/data/story/events/ edited as forms. A list of events on the left; on
// the right the event's id (the file name), title, text, ages, who it involves, its trigger (a condition in the rule language), the affinity it needs, and
// each option with its words, affinities, what the other person thinks afterwards, a trait and the chronicle note. Save checks the event with the game's
// own reader and writes the file (keeping the older one as `.bak`). It takes the whole screen like the graph editor; Esc comes back.
class StoryEventEditor {
public:
    using Say = std::function<void(const std::string&)>;
    StoryEventEditor(int viewWidth, int viewHeight, Say say);

    void setFolder(std::filesystem::path folder, std::function<void()> saved = {}) {
        folder_ = std::move(folder);
        saved_ = std::move(saved);
    }

    bool shown() const { return shown_; }
    void show(bool shown);
    bool typing() const;
    std::vector<std::string> files() const; // event ids, in file order of the folder
    bool open(const std::string& id);
    bool createNew(const std::string& id);
    bool save();
    bool dirty() const { return open_ && formText() != savedText_; }
    const sim::CrossroadsEvent* current() const { return open_ ? &event_ : nullptr; }
    sim::CrossroadsEvent& edit() { return event_; } // for tests: the form writes through the same fields
    // The affinities of an option as the form shows them: "fireKeeper=6 hunter=2", and back (false for a word that is not an affinity or not a whole number).
    static std::string affinityText(const sim::AffinityValues& values);
    static bool affinityFromText(const std::string& text, sim::AffinityValues& values);

    void update(const luna::engine::Intents& intents);
    void draw(luna::engine::UiPainter& painter) const;
    void drawOverlay(luna::engine::UiPainter& painter) const;

private:
    void rebuild();
    std::string formText() const { return sim::writeStoryEvent(event_); }
    std::filesystem::path fileOf(const std::string& id) const { return folder_ / (id + ".json"); }

    int viewWidth_;
    int viewHeight_;
    Say say_;
    std::function<void()> saved_;
    std::filesystem::path folder_;
    bool shown_ = false;
    bool open_ = false;
    bool rebuild_ = true;
    sim::CrossroadsEvent event_;
    std::string savedText_;
    std::string newName_;
    std::string affinityNeeded_;
    std::vector<std::string> optionAffinity_;
    std::unique_ptr<luna::engine::Panel> panel_;
};

} // namespace odysseus::game
