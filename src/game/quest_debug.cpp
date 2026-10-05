#include "game/odyssey_game.h"

#include "game/game_rules.h"

#include "luna/engine/ui.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <set>

namespace odysseus::game {

namespace {

enum DebugAction {
    kSelectBase = 1000, kStart = 1, kComplete, kFail, kReset, kPrevStep, kNextStep, kOpinionUp, kOpinionDown, kHour, kDay, kSeason, kTeleport,
    kFlagBase = 100, kItemBase = 200
};

// The names a quest looks at: flags in its conditions and objectives, items in its objectives.
void collectNames(const sim::rules::Quest& quest, std::vector<std::string>& flags, std::vector<std::string>& items) {
    std::set<std::string> flagSet;
    std::set<std::string> itemSet;
    const auto scan = [&](const std::string& source) {
        std::size_t at = 0;
        while ((at = source.find("flag(", at)) != std::string::npos) {
            const std::size_t close = source.find(')', at);
            if (close == std::string::npos) break;
            flagSet.insert(source.substr(at + 5, close - at - 5));
            at = close;
        }
    };
    for (const sim::rules::QuestCondition& c : quest.requires_) scan(c.source);
    for (const sim::rules::QuestCondition& c : quest.fail) scan(c.source);
    for (const sim::rules::QuestStep& step : quest.steps) {
        for (const sim::rules::QuestBranch& b : step.branches) scan(b.condition.source);
        using Kind = sim::rules::QuestObjective::Kind;
        if (step.objective.kind == Kind::Flag) flagSet.insert(step.objective.subject);
        if (step.objective.kind == Kind::Gather || step.objective.kind == Kind::Give || step.objective.kind == Kind::Craft) itemSet.insert(step.objective.subject);
    }
    flags.assign(flagSet.begin(), flagSet.end());
    items.assign(itemSet.begin(), itemSet.end());
}

} // namespace

// The quest debugger (US-186, EDT-06): F10 in Debug builds. It changes the running game at once; Play here (P in the Editor) runs on a copy of the level, so
// nothing of it reaches the files unless the owner saves.
void OdysseyGame::layoutQuestDebug(std::vector<DebugButton>& buttons, std::vector<DebugText>& texts) const {
    const int panelWidth = 250;
    const int x0 = uiWidth() - panelWidth - 4;
    int y = 70;
    const auto text = [&](const std::string& line, luna::engine::UiColor colour = luna::engine::UiColor::Text) {
        texts.push_back({x0 + 6, y, line, colour});
        y += 11;
    };
    const auto row = [&](const std::vector<std::pair<std::string, int>>& items) {
        int x = x0 + 6;
        for (const auto& [label, action] : items) {
            const int w = luna::engine::UiPainter::textWidth(label) + 8;
            buttons.push_back({{x, y, w, 12}, label, action, false});
            x += w + 3;
        }
        y += 15;
    };
    text("QUEST DEBUGGER  F10 closes", luna::engine::UiColor::Gold);
    const auto& quests = quests_.quests();
    if (quests.empty()) {
        text("No quests are loaded.", luna::engine::UiColor::Dim);
        return;
    }
    std::vector<std::pair<std::string, int>> picks;
    for (std::size_t i = 0; i < quests.size() && i < 6; ++i) picks.push_back({quests[i].id, kSelectBase + static_cast<int>(i)});
    row(picks);
    const std::size_t selected = std::min<std::size_t>(static_cast<std::size_t>(std::max(0, questDebugSelected_)), quests.size() - 1);
    const sim::rules::Quest& quest = quests[selected];
    const sim::rules::QuestState* state = quests_.state(quest.id);
    const sim::rules::QuestStatus status = quests_.status(quest.id);
    text(std::format("{}: {}  step {}  progress {}", quest.id, sim::rules::statusWord(status), state != nullptr && !state->step.empty() ? state->step : "-", state != nullptr ? state->progress : 0));
    row({{"Start", kStart}, {"Complete", kComplete}, {"Fail", kFail}, {"Reset", kReset}, {"< step", kPrevStep}, {"step >", kNextStep}});

    std::vector<std::string> flags;
    std::vector<std::string> items;
    collectNames(quest, flags, items);
    for (std::size_t i = 0; i < flags.size() && i < 4; ++i) {
        text(std::format("flag {} = {}", flags[i], flags_.get(flags[i])), luna::engine::UiColor::Dim);
        row({{flags_.get(flags[i]) > 0 ? "clear" : "set", kFlagBase + static_cast<int>(i)}});
    }
    std::vector<std::pair<std::string, int>> giveRow;
    for (std::size_t i = 0; i < items.size() && i < 4; ++i) giveRow.push_back({"+1 " + items[i], kItemBase + static_cast<int>(i)});
    if (!giveRow.empty() && life_) row(giveRow);
    row({{"Opinion +10", kOpinionUp}, {"Opinion -10", kOpinionDown}});
    row({{"+1 hour", kHour}, {"+1 day", kDay}, {"+1 season", kSeason}, {"To marker", kTeleport}});

    // Why not: every condition that stands between the quest and its next move, with the answer now.
    text("Why not?", luna::engine::UiColor::Gold);
    const GameRuleContext context(*this, heroSubject(*this));
    const auto cond = [&](const std::string& label, const sim::rules::QuestCondition& c) {
        const bool holds = quests_.holds(context, c);
        text(std::format("{} {}: {}", holds ? "[yes]" : "[no] ", label, c.source), holds ? luna::engine::UiColor::Text : luna::engine::UiColor::Gold);
    };
    if (status == sim::rules::QuestStatus::Locked || status == sim::rules::QuestStatus::Available) {
        if (quest.requires_.empty()) text("no prerequisites", luna::engine::UiColor::Dim);
        for (const sim::rules::QuestCondition& c : quest.requires_) cond("requires", c);
        if (status == sim::rules::QuestStatus::Available) text(quest.giver == "none" ? "starts by itself" : "waits for its giver: " + quest.giver, luna::engine::UiColor::Dim);
    } else if (status == sim::rules::QuestStatus::Active && state != nullptr) {
        if (const sim::rules::QuestStep* step = quest.find(state->step)) {
            text("objective: " + step->objective.source + std::format("  ({} / {})", state->progress, step->objective.amount));
            for (const sim::rules::QuestBranch& b : step->branches) cond("branch to " + b.to, b.condition);
        }
        for (const sim::rules::QuestCondition& c : quest.fail) cond("fails when", c);
    } else {
        text("nothing: the quest is finished", luna::engine::UiColor::Dim);
    }
}

void OdysseyGame::updateQuestDebug(const luna::engine::Intents& intents) {
#ifndef NDEBUG
    if (intents.pressed(luna::engine::Intent::QuestDebug)) questDebugOpen_ = !questDebugOpen_;
#endif
    if (!questDebugOpen_ || quests_.quests().empty() || !intents.pointer().pressed[0]) return;
    std::vector<DebugButton> buttons;
    std::vector<DebugText> texts;
    layoutQuestDebug(buttons, texts);
    const luna::engine::Pointer& pointer = intents.pointer();
    for (const DebugButton& button : buttons) {
        if (pointer.x < button.rect.x || pointer.y < button.rect.y || pointer.x >= button.rect.x + button.rect.width || pointer.y >= button.rect.y + button.rect.height) continue;
        runQuestDebugAction(button.action);
        return;
    }
}

void OdysseyGame::runQuestDebugAction(int action) {
    const auto& quests = quests_.quests();
    if (quests.empty()) return;
    if (action >= kSelectBase) {
        questDebugSelected_ = std::clamp(action - kSelectBase, 0, static_cast<int>(quests.size()) - 1);
        return;
    }
    const sim::rules::Quest& quest = quests[std::min<std::size_t>(static_cast<std::size_t>(std::max(0, questDebugSelected_)), quests.size() - 1)];
    const std::int64_t now = actionClock_;
    std::vector<std::string> flags;
    std::vector<std::string> items;
    collectNames(quest, flags, items);
    if (action >= kItemBase && action < kItemBase + 50) {
        if (life_ && static_cast<std::size_t>(action - kItemBase) < items.size()) life_->give(items[static_cast<std::size_t>(action - kItemBase)], 1);
    } else if (action >= kFlagBase && action < kFlagBase + 50) {
        if (static_cast<std::size_t>(action - kFlagBase) < flags.size()) {
            const std::string& name = flags[static_cast<std::size_t>(action - kFlagBase)];
            flags_.set(name, flags_.get(name) > 0 ? 0 : 1);
        }
    } else if (action == kStart) {
        if (quests_.status(quest.id) == sim::rules::QuestStatus::Locked || quests_.status(quest.id) == sim::rules::QuestStatus::Failed || quests_.status(quest.id) == sim::rules::QuestStatus::Done) quests_.reset(quest.id);
        quests_.jumpTo(quest.id, quest.start, now);
    } else if (action == kComplete) {
        completeQuest(*this, quest.id);
    } else if (action == kFail) {
        quests_.fail(quest.id);
    } else if (action == kReset) {
        quests_.reset(quest.id);
    } else if (action == kPrevStep || action == kNextStep) {
        const sim::rules::QuestState* state = quests_.state(quest.id);
        std::size_t index = 0;
        for (std::size_t i = 0; i < quest.steps.size(); ++i) {
            if (state != nullptr && quest.steps[i].id == state->step) index = i;
        }
        if (action == kNextStep && index + 1 < quest.steps.size()) ++index;
        if (action == kPrevStep && index > 0) --index;
        quests_.jumpTo(quest.id, quest.steps[index].id, now);
    } else if (action == kOpinionUp || action == kOpinionDown) {
        int nearest = -1;
        double best = 0.0;
        for (const PlacedCharacter& placed : level_.characters) {
            if (npcPopulation_.indexOf(placed.id) < 0) continue;
            const PixelPoint at = npcPosition(placed.id);
            const double d = (at.x - hero_.feetX()) * (at.x - hero_.feetX()) + (at.y - hero_.feetY()) * (at.y - hero_.feetY());
            if (nearest < 0 || d < best) {
                nearest = placed.id;
                best = d;
            }
        }
        if (nearest >= 0) npcPopulation_.adjust(nearest, sim::NpcPopulation::kHero, action == kOpinionUp ? 10 : -10);
    } else if (action == kHour || action == kDay || action == kSeason) {
        if (clan_) {
            const auto perDay = static_cast<std::uint64_t>(clan_->calendar().ticksPerDay());
            const std::uint64_t ticks = action == kHour ? perDay / 24 : action == kDay ? perDay : perDay * static_cast<std::uint64_t>(clan_->calendar().daysPerYear() / 4);
            clan_->runTicks(ticks);
        }
    } else if (action == kTeleport) {
        if (questMarker_.valid) {
            hero_ = Hero(questMarker_.x, questMarker_.y + 16.0);
            camera_.centreOn(hero_.feetX(), hero_.feetY());
        }
    }
    tickQuests(*this); // the quests look at the change at once
}

void OdysseyGame::drawQuestDebug(luna::engine::Renderer& renderer) const {
    if (!questDebugOpen_) return;
    std::vector<DebugButton> buttons;
    std::vector<DebugText> texts;
    layoutQuestDebug(buttons, texts);
    luna::engine::UiPainter painter(renderer, uiSheet_);
    int bottom = 70;
    for (const DebugText& t : texts) bottom = std::max(bottom, t.y + 12);
    for (const DebugButton& b : buttons) bottom = std::max(bottom, b.rect.y + 16);
    const luna::engine::Rect panel{uiWidth() - 254, 66, 250, bottom - 62};
    painter.fill(panel, luna::engine::UiColor::Shade);
    painter.outline(panel, luna::engine::UiColor::Gold);
    for (const DebugText& t : texts) painter.text(t.x, t.y, t.text, t.colour);
    for (const DebugButton& b : buttons) {
        painter.fill(b.rect, luna::engine::UiColor::Panel);
        painter.outline(b.rect, luna::engine::UiColor::Border);
        painter.text(b.rect.x + 4, b.rect.y + 2, b.label, luna::engine::UiColor::Text);
    }
}

} // namespace odysseus::game
