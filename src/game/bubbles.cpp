#include "game/bubbles.h"

#include "game/game_rules.h"
#include "game/odyssey_game.h"

#include "sim/dialogue_select.h"

#include <algorithm>
#include <cmath>

namespace odysseus::game {

void Bubbles::say(int person, std::string text, int ticks) {
    std::erase_if(bubbles_, [person](const Bubble& b) { return b.person == person; });
    bubbles_.push_back({person, std::move(text), ticks});
}

void Bubbles::tick() {
    for (Bubble& bubble : bubbles_) --bubble.ticksLeft;
    std::erase_if(bubbles_, [](const Bubble& b) { return b.ticksLeft <= 0; });
}

const Bubble* Bubbles::of(int person) const {
    const auto found = std::find_if(bubbles_.begin(), bubbles_.end(), [person](const Bubble& b) { return b.person == person; });
    return found == bubbles_.end() ? nullptr : &*found;
}

void updateGreetings(OdysseyGame& game) {
    const sim::World* world = game.clan();
    const sim::HeroLife* hero = game.life();
    if (world == nullptr || hero == nullptr || hero->phase() != sim::Phase::Free) return;
    const auto& figures = game.clanView().figures();
    const double reach = kGreetingRangeMetres * static_cast<double>(kTileSize); // a tile is a metre
    const std::int64_t now = game.actionClock();
    // Who may greet now: friendly, within 3 m, their minute over. The nearest first (then the lower id), so the order never depends on luck.
    struct Near {
        double distance = 0.0;
        int person = 0;
    };
    std::vector<Near> waiting;
    for (std::size_t i = 0; i < figures.size(); ++i) {
        const Figure& figure = figures[i];
        const int person = static_cast<int>(i);
        if (!figure.present || person == hero->personId()) continue;
        const double distance = std::hypot(figure.x - game.hero().feetX(), figure.y - game.hero().feetY());
        if (distance > reach) continue;
        if (world->opinion(person, hero->personId()) < 0) continue; // only the friendly greet
        if (!game.greetingCooldowns().ready(kPersonActorBase + person, "greeting", now)) continue;
        if (game.bubbles().of(person) != nullptr) continue;
        waiting.push_back({distance, person});
    }
    std::sort(waiting.begin(), waiting.end(), [](const Near& a, const Near& b) { return a.distance != b.distance ? a.distance < b.distance : a.person < b.person; });
    for (const Near& next : waiting) {
        if (static_cast<int>(game.bubbles().all().size()) >= kGreetingBubblesAtOnce) break;
        const auto subject = subjectFor(game, {static_cast<int>(Subject::Kind::Person), next.person});
        if (!subject) continue;
        const GameRuleContext context(game, *subject);
        sim::rules::WhoFacts who;
        who.name = subject->name;
        who.roles = sim::rules::rolesOf(*world, next.person);
        const sim::rules::DlgScript* script = sim::rules::selectBark(game.dialogues(), who, context, game.dialogueRandom());
        if (script == nullptr) continue;
        const std::string text = sim::rules::barkText(*script, context);
        if (text.empty()) continue;
        game.bubbles().say(next.person, text, kGreetingSeconds * sim::rules::ActionRunner::kTicksPerSecond);
        // The minute counts from now, so a person who stays beside the hero does not greet them over and over.
        game.greetingCooldowns().start(kPersonActorBase + next.person, "greeting", now + static_cast<std::int64_t>(kGreetingCooldownSeconds) * sim::rules::ActionRunner::kTicksPerSecond);
    }
}
} // namespace odysseus::game
