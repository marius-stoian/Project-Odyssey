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

// ---- clan members talking to each other (US-165)

namespace odysseus::game {

namespace {

constexpr std::size_t kMostLines = 6; // a written exchange is cut here: a bubble show, not a play

// The words of a kind of exchange, from the kind of event the simulation recorded; empty for the events that are not shown.
std::string kindOf(sim::EventKind kind) {
    switch (kind) {
    case sim::EventKind::Quarrel: return "quarrel";
    case sim::EventKind::Courtship:
    case sim::EventKind::Pairing: return "courtship";
    case sim::EventKind::Sharing: return "sharing";
    case sim::EventKind::Gift: return "gift";
    default: return {};
    }
}

struct SocialEvent {
    std::string kind;
    int first = -1;
    int second = -1;
};

bool nearHero(const OdysseyGame& game, int person) {
    const auto& figures = game.clanView().figures();
    if (person < 0 || static_cast<std::size_t>(person) >= figures.size() || !figures[static_cast<std::size_t>(person)].present) return false;
    const Figure& figure = figures[static_cast<std::size_t>(person)];
    return std::hypot(figure.x - game.hero().feetX(), figure.y - game.hero().feetY()) <= kExchangeRangeMetres * static_cast<double>(kTileSize);
}

std::string lowerCase(std::string text) {
    for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

std::string replaceAll(std::string text, const std::string& from, const std::string& to) {
    for (std::size_t at = text.find(from); at != std::string::npos; at = text.find(from, at + to.size())) text.replace(at, from.size(), to);
    return text;
}

sim::rules::WhoFacts whoIs(const OdysseyGame& game, const Subject& subject) {
    sim::rules::WhoFacts who;
    who.name = subject.name;
    who.roles = sim::rules::rolesOf(*game.clan(), subject.index);
    return who;
}

// What the two say, in order. A script that fits them first; else two short lines from `social.<kind>` of the small-talk file; else nothing.
std::vector<ExchangeLine> makeExchange(OdysseyGame& game, const SocialEvent& event) {
    std::vector<ExchangeLine> lines;
    const auto first = subjectFor(game, {static_cast<int>(Subject::Kind::Person), event.first});
    const auto second = subjectFor(game, {static_cast<int>(Subject::Kind::Person), event.second});
    if (!first || !second) return lines;
    const GameRuleContext firstContext(game, *first);
    const GameRuleContext secondContext(game, *second);
    const sim::rules::DlgScript* script = sim::rules::selectPair(game.dialogues(), whoIs(game, *first), whoIs(game, *second), event.kind, firstContext, game.dialogueRandom());
    if (script != nullptr) {
        const sim::rules::DlgNode* node = script->startNode();
        if (node == nullptr) return lines;
        for (const sim::rules::DlgLine& line : node->lines) {
            const bool byFirst = lowerCase(line.speaker) == lowerCase(script->pair[0]);
            const bool bySecond = lowerCase(line.speaker) == lowerCase(script->pair[1]);
            if (!byFirst && !bySecond) continue;
            const sim::rules::RuleContext& context = byFirst ? static_cast<const sim::rules::RuleContext&>(firstContext) : secondContext;
            if (line.condition != nullptr && !sim::rules::isTrue(*line.condition, context)) continue;
            // {partner} is the one spoken to, {npc} the speaker.
            const std::string text = sim::rules::fillDialogueTokens(replaceAll(line.text, "{partner}", byFirst ? second->name : first->name), context);
            lines.push_back({byFirst ? event.first : event.second, text});
            if (lines.size() >= kMostLines) break;
        }
        return lines;
    }
    const std::string topic = "social." + event.kind;
    if (!game.smalltalk().hasTopic(topic)) return lines;
    const auto said = game.smalltalk().say(*game.clan(), event.first, event.second, game.dialogueRandom(), topic);
    const auto answer = game.smalltalk().say(*game.clan(), event.second, event.first, game.dialogueRandom(), topic);
    if (said) lines.push_back({event.first, said->text});
    if (answer) lines.push_back({event.second, answer->text});
    return lines;
}

} // namespace

void Exchanges::reset(std::size_t chronicleEntries) {
    seenEntries_ = chronicleEntries;
    waiting_.clear();
    current_.clear();
    index_ = 0;
    ticksLeft_ = 0;
}

void Exchanges::startLine(OdysseyGame& game) {
    if (index_ > 0) game.bubbles().remove(current_[index_ - 1].person); // the next line begins: the last one goes
    if (index_ >= current_.size()) {
        current_.clear();
        index_ = 0;
        return;
    }
    game.bubbles().say(current_[index_].person, current_[index_].text, kExchangeLineSeconds * sim::rules::ActionRunner::kTicksPerSecond);
    ticksLeft_ = kExchangeLineSeconds * sim::rules::ActionRunner::kTicksPerSecond;
    ++index_;
}

void Exchanges::update(OdysseyGame& game) {
    sim::World* world = game.clanMutable();
    const sim::HeroLife* hero = game.life();
    if (world == nullptr || hero == nullptr) return;
    // What the simulation did since last tick: who talked, and the new chronicle entries of the kinds that are shown.
    std::vector<SocialEvent> events;
    for (const sim::World::TalkEvent& talk : world->takeTalks()) events.push_back({"talk", talk.speaker, talk.listener});
    const auto& entries = world->chronicle().entries();
    if (seenEntries_ > entries.size()) seenEntries_ = entries.size();
    for (; seenEntries_ < entries.size(); ++seenEntries_) {
        const sim::ChronicleEntry& entry = entries[seenEntries_];
        const std::string kind = kindOf(entry.kind);
        if (!kind.empty() && entry.who >= 0 && entry.other >= 0) events.push_back({kind, entry.who, entry.other});
    }
    for (const SocialEvent& event : events) {
        if (event.first == hero->personId() || event.second == hero->personId() || event.first == event.second) continue; // the hero's own talk has a panel
        if (!nearHero(game, event.first) || !nearHero(game, event.second)) continue;
        // The same two people are not queued twice, and busy people are not interrupted.
        const auto involves = [&](const std::vector<ExchangeLine>& lines) {
            return std::any_of(lines.begin(), lines.end(), [&](const ExchangeLine& l) { return l.person == event.first || l.person == event.second; });
        };
        if (involves(current_) || std::any_of(waiting_.begin(), waiting_.end(), involves)) continue;
        if (waiting_.size() >= kExchangesWaiting) continue;
        std::vector<ExchangeLine> lines = makeExchange(game, event);
        if (!lines.empty()) waiting_.push_back(std::move(lines));
    }
    // The exchange under way: a line for 3 seconds, then the next; then the next exchange.
    if (current_.empty() && !waiting_.empty()) {
        current_ = std::move(waiting_.front());
        waiting_.erase(waiting_.begin());
        index_ = 0;
        startLine(game);
    } else if (!current_.empty() && --ticksLeft_ <= 0) {
        startLine(game);
    }
}

} // namespace odysseus::game
