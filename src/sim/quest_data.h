#pragma once

#include "boundary.h"

#include "sim/interaction.h"
#include "sim/rule_effect.h"
#include "sim/rule_expr.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace odysseus::sim::rules {

// A quest written as a JSON file (US-180, STO-04): `assets/data/quests/<id>.json`, documented in docs/guides/quests.md. Conditions and effects are the
// language of the interaction files (ADR-019). Nothing here knows about the world; the QuestBook (quest_book.h) runs a quest and the Game feeds it.

// One condition as written and as parsed, kept together so the writer prints it back unchanged.
struct QuestCondition {
    std::string source; // "not quest(first-day) == done"
    ExprPtr expr;
    int line = 0;
};

// What the player must do to finish a step: `gather berries 3`, `talk elder`, `wait 30s` ...
struct QuestObjective {
    enum class Kind { Talk, Goto, Gather, Give, Craft, Interact, Defeat, Wait, Flag };
    Kind kind = Kind::Talk;
    std::string subject; // who, place, item, interaction, kind of enemy or flag; empty for wait
    int amount = 1;      // how many (gather, give, craft, defeat); for wait, the length in `waitUnit`
    DelayUnit waitUnit = DelayUnit::Seconds;
    std::string source;  // as written
    int line = 0;
};

struct QuestHint {
    int afterSeconds = 0; // shown after this long without progress
    std::string text;
};

struct QuestBranch {
    QuestCondition condition;
    std::string to; // a step id, or "END"
    int toLine = 0;
};

struct QuestStep {
    std::string id;
    std::string text;
    QuestObjective objective;
    std::string marker; // "tag:edible", "object:clan-fire", "npc:elder", "place:stream" or empty
    std::optional<QuestHint> hint;
    std::string next; // a step id, or "END"
    int nextLine = 0;
    std::vector<QuestBranch> branches; // the first whose condition holds wins; otherwise `next`
    int line = 0;
};

struct Quest {
    std::string id;
    std::string title;
    std::string giver = "none"; // a person id, "role:<role>", or "none" (the quest starts by itself)
    std::string note;           // the owner's own words; survives saves
    std::vector<QuestCondition> requires_;
    std::string start;
    std::vector<QuestStep> steps; // in file order
    std::vector<QuestCondition> fail;
    std::vector<Effect> rewards;
    std::string journal;
    std::string file; // "quests/first-day.json"

    const QuestStep* find(std::string_view stepId) const;
};

constexpr const char* kQuestEnd = "END";

// One quest from text. `name` is how errors name the file; `expectedId` (the file name without .json) must match the id when not empty. Mistakes are
// added to `report` as "file:line: message"; the result is empty when there are any.
std::optional<Quest> parseQuest(std::string_view text, const std::string& name, LoadReport& report, const std::string& expectedId = {});

// One objective line: `gather berries 3`. `problem` carries the message when it cannot be read.
struct ParsedObjective {
    QuestObjective objective;
    std::string problem;
};
ParsedObjective parseObjective(std::string_view source);

// The canonical text of a quest: what the Editor saves. Reading it again gives the same quest; comments are dropped.
std::string writeQuest(const Quest& quest);

// Every `*.json` of a folder (not the `.layout.json` ones), each on its own: one with mistakes is left out and listed. Sorted by id.
std::vector<Quest> loadQuests(const std::filesystem::path& folder, LoadReport& report);

} // namespace odysseus::sim::rules
