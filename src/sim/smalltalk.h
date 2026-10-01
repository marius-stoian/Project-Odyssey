#pragma once

#include "boundary.h"

#include "core/random.h"
#include "sim/dialogue_script.h"
#include "sim/world.h"

#include <deque>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace odysseus::sim::rules {

// Generated small talk (US-163, SDC-03, design docs/plans/M8-dialogue-design.md section 6): what a clan member says when no script was written for them,
// or when a script writes `{smalltalk.hunt}`. Plain and short (D-38). The templates are data, `assets/data/dialogue/smalltalk.json`, documented in
// docs/guides/dialogue-format.md; the facts that fill them come from the simulation: what the person remembers, what they heard, what they need.

struct SmalltalkTemplate {
    std::string text;                // with {tokens}
    std::vector<std::string> moods;  // the mood words it is for; empty = any mood
    int line = 0;
};

class SmalltalkData {
public:
    // Reads the file. Mistakes are added to `report` as "dialogue/smalltalk.json:line: message" and nothing is returned for them.
    static std::optional<SmalltalkData> load(const std::filesystem::path& file, const std::string& shownName, LoadReport& report);
    // Reads the text of a file (tests, the Editor).
    static std::optional<SmalltalkData> parse(std::string_view text, const std::string& shownName, LoadReport& report);

    const std::vector<SmalltalkTemplate>* topic(const std::string& name) const;
    const std::map<std::string, std::vector<SmalltalkTemplate>>& topics() const { return topics_; }

private:
    std::map<std::string, std::vector<SmalltalkTemplate>> topics_;
};

// The topics every file must have, and the tokens each may use (the checker names a token used where its facts do not exist).
const std::vector<std::string>& requiredSmalltalkTopics();
const std::vector<std::string>& smalltalkTokens();

struct SaidLine {
    std::string topic;
    std::string text;
};

class SmallTalk {
public:
    SmallTalk() = default;
    explicit SmallTalk(SmalltalkData data) : data_(std::move(data)) {}
    bool ready() const { return data_.has_value(); }
    bool hasTopic(const std::string& name) const { return data_ && data_->topic(name) != nullptr; }

    // What `npc` says to `hero` now. With no `topic` it is chosen by weights from the person's state: a pressing need, a fresh memory, gossip
    // they have not repeated, the season. A named topic whose facts do not exist (no memory to speak of) falls back to the season. Three numbers are
    // drawn from `random` every call whatever happens, so the stream does not depend on what a person happens to remember. A line this person said
    // in their last three, or that anybody said twice in the last fifty, is avoided while another is possible. Empty when there are no templates.
    std::optional<SaidLine> say(const World& world, int npc, int hero, core::Pcg32& random, const std::string& topic = {});

    // Forgets what was said (a new run).
    void clear();

private:
    std::optional<SmalltalkData> data_;
    std::map<int, std::deque<std::string>> lastByPerson_;
    std::deque<std::string> window_; // the last fifty lines said, whoever said them
};

} // namespace odysseus::sim::rules
