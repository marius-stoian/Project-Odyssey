#include "game/odyssey_game.h"

#include "game/game_rules.h"

#include "luna/engine/ui.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace odysseus::game {

// A sign over people who have a quest to give (gold !) or who a quest wants the hero to talk to (gold ?), when the hero is near (US-182).
void OdysseyGame::drawQuestSigns(luna::engine::Renderer& renderer, const luna::engine::Rect& view, double alpha) const {
    if (quests_.quests().empty()) return;
    luna::engine::UiPainter painter(renderer, uiSheet_);
    const double heroX = hero_.feetX(alpha);
    const double heroY = hero_.feetY(alpha);
    constexpr double kNear = 8.0 * kTileSize;
    const auto sign = [&](const Subject& subject) {
        const double dx = subject.x - heroX;
        const double dy = subject.y - heroY;
        if (dx * dx + dy * dy > kNear * kNear) return;
        const QuestSign kind = questSignFor(*this, subject);
        if (kind == QuestSign::None) return;
        const int x = static_cast<int>(std::lround(subject.x)) - view.x;
        const int y = static_cast<int>(std::lround(subject.y)) - view.y - kCharacterHeight - 16;
        painter.text(x - 2, y, kind == QuestSign::Offer ? "!" : "?", luna::engine::UiColor::Gold);
    };
    for (const PlacedCharacter& placed : level_.characters) {
        if (const std::optional<Subject> subject = npcSubject(*this, placed.id)) sign(*subject);
    }
    if (clan_) {
        for (std::size_t i = 0; i < clanView_.figures().size(); ++i) {
            if (!clanView_.figures()[i].present) continue;
            if (const std::optional<Subject> subject = subjectFor(*this, {static_cast<int>(Subject::Kind::Person), static_cast<int>(i)})) sign(*subject);
        }
    }
}


std::string trackedQuest(const sim::rules::QuestBook& book) {
    std::string best;
    std::int64_t bestTick = -1;
    for (const sim::rules::Quest& quest : book.quests()) {
        const sim::rules::QuestState* state = book.state(quest.id);
        if (state == nullptr || state->status != sim::rules::QuestStatus::Active) continue;
        if (state->lastProgressTick > bestTick) {
            bestTick = state->lastProgressTick;
            best = quest.id;
        }
    }
    return best;
}

std::optional<std::pair<double, double>> questMarkerPosition(const OdysseyGame& game, const std::string& spec) {
    const std::size_t colon = spec.find(':');
    if (colon == std::string::npos) return std::nullopt;
    const std::string kind = spec.substr(0, colon);
    const std::string word = questWord(spec.substr(colon + 1));
    const double heroX = game.hero().feetX(1.0);
    const double heroY = game.hero().feetY(1.0);
    std::optional<std::pair<double, double>> best;
    double bestDistance = 0.0;
    const auto consider = [&](double x, double y) {
        const double d = (x - heroX) * (x - heroX) + (y - heroY) * (y - heroY);
        if (!best || d < bestDistance) {
            best = std::pair<double, double>{x, y};
            bestDistance = d;
        }
    };
    const auto matches = [&](const Subject& subject) {
        if (kind == "tag") {
            for (const std::string& tag : subject.info.tags) {
                if (questWord(tag) == word) return true;
            }
            return false;
        }
        const std::vector<std::string> aliases = subjectAliases(game, subject);
        return std::find(aliases.begin(), aliases.end(), word) != aliases.end();
    };
    if (kind == "place") {
        for (const PlacedPlace& place : game.level().places) {
            if (questWord(place.name) == word) consider(place.at.x, place.at.y);
        }
        return best;
    }
    if (kind != "tag" && kind != "object" && kind != "npc") return std::nullopt;
    for (const PlacedCharacter& placed : game.level().characters) {
        if (const std::optional<Subject> subject = npcSubject(game, placed.id); subject && matches(*subject)) consider(subject->x, subject->y);
    }
    if (game.clan() != nullptr) {
        for (std::size_t i = 0; i < game.clanView().figures().size(); ++i) {
            if (!game.clanView().figures()[i].present) continue;
            if (const std::optional<Subject> subject = subjectFor(game, {static_cast<int>(Subject::Kind::Person), static_cast<int>(i)}); subject && matches(*subject)) consider(subject->x, subject->y);
        }
    }
    if (kind != "npc") {
        for (const WorldPlant& plant : game.plants()) {
            if (!plant.present()) continue;
            if (kind == "object" && questWord(plant.kind) != word) continue;
            if (kind == "tag" && (plant.def == nullptr || std::find_if(plant.def->tags.begin(), plant.def->tags.end(), [&](const std::string& t) { return questWord(t) == word; }) == plant.def->tags.end())) continue;
            consider(plant.feet.x, plant.feet.y);
        }
    }
    return best;
}

// The tracker, top right (US-183): the tracked quest title, its step, how far the objective is, and the hint once the hero is stuck.
void OdysseyGame::drawQuestTracker(luna::engine::Renderer& renderer) const {
    const std::string id = trackedQuest(quests_);
    if (id.empty() || runFlow_.modal()) return;
    const sim::rules::Quest* quest = quests_.find(id);
    const sim::rules::QuestState* state = quests_.state(id);
    const sim::rules::QuestStep* step = quest != nullptr && state != nullptr ? quest->find(state->step) : nullptr;
    if (step == nullptr) return;
    std::vector<std::string> lines = {quest->title, step->text};
    if (step->objective.kind != sim::rules::QuestObjective::Kind::Wait && step->objective.amount > 1) {
        lines.push_back(std::format("{} / {}", state->progress, step->objective.amount));
    }
    const bool hint = quests_.hintDue(id, actionClock_) && step->hint.has_value();
    if (hint) lines.push_back(step->hint->text);
    int width = 0;
    for (const std::string& l : lines) width = std::max(width, luna::engine::UiPainter::textWidth(l));
    width += 12;
    luna::engine::UiPainter painter(renderer, uiSheet_);
    const luna::engine::Rect box{uiWidth() - width - 8, 8, width, static_cast<int>(lines.size()) * 12 + 8};
    painter.fill(box, luna::engine::UiColor::Shade);
    painter.outline(box, luna::engine::UiColor::Border);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const bool hintLine = hint && i + 1 == lines.size();
        painter.text(box.x + 6, box.y + 6 + static_cast<int>(i) * 12, lines[i], i == 0 ? luna::engine::UiColor::Gold : (hintLine ? luna::engine::UiColor::Dim : luna::engine::UiColor::Text));
    }
}

// A marker over the target of the tracked step, or an arrow at the edge of the screen pointing the way (US-183). Off when the setting says so.
void OdysseyGame::drawQuestMarker(luna::engine::Renderer& renderer, const luna::engine::Rect& view, double alpha) const {
    (void)alpha;
    if (!questMarker_.valid || settings_.markers == 0 || !rules_.systems.markers) return;
    luna::engine::UiPainter painter(renderer, uiSheet_);
    const int x = static_cast<int>(std::lround(questMarker_.x)) - view.x;
    const int y = static_cast<int>(std::lround(questMarker_.y)) - view.y;
    const int w = viewWidth();
    const int h = viewHeight();
    if (x >= 0 && y >= 0 && x < w && y < h) {
        painter.text(x - 2, y - kCharacterHeight - 26, "v", luna::engine::UiColor::Gold);
        return;
    }
    const int ax = std::clamp(x, 8, w - 16);
    const int ay = std::clamp(y, 8, h - 16);
    const bool horizontal = std::abs(x - w / 2) * h > std::abs(y - h / 2) * w;
    const char* arrow = horizontal ? (x < 0 ? "<" : ">") : (y < 0 ? "^" : "v");
    painter.text(ax, ay, arrow, luna::engine::UiColor::Gold);
}

} // namespace odysseus::game
