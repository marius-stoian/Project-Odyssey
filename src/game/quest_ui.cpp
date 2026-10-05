#include "game/odyssey_game.h"

#include "game/game_rules.h"

#include "luna/engine/ui.h"

#include <cmath>

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

} // namespace odysseus::game
