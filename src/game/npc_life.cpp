#include "game/npc_life.h"

#include "game/game_rules.h"
#include "game/odyssey_game.h"
#include "game/weapons.h"
#include "luna/engine/collision.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace odysseus::game {

namespace {

constexpr double kPixelsPerMetre = static_cast<double>(kTileSize);

bool hasTag(const std::vector<std::string>& tags, const std::string& tag) { return std::find(tags.begin(), tags.end(), tag) != tags.end(); }

std::uint64_t mix(std::uint64_t h, std::uint64_t value) { return h ^ (value + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2)); }

} // namespace

void NpcLife::reset() {
    minds_.clear();
    animalFeet_.clear();
    cooldowns_.clear();
    random_ = core::Pcg32(0x4E50ULL, 7ULL);
}

const NpcLife::Mind* NpcLife::mind(int runnerId) const {
    const auto it = minds_.find(runnerId);
    return it == minds_.end() ? nullptr : &it->second;
}

std::uint64_t NpcLife::hash() const {
    std::uint64_t h = cooldowns_.hash();
    h = mix(h, random_.state());
    for (const auto& [id, mind] : minds_) {
        h = mix(h, static_cast<std::uint64_t>(id));
        h = mix(h, static_cast<std::uint64_t>(mind.state));
        for (const char c : mind.interaction) h = mix(h, static_cast<unsigned char>(c));
        h = mix(h, static_cast<std::uint64_t>(mind.target.kind) * 1000003ULL + static_cast<std::uint64_t>(mind.target.id));
        h = mix(h, static_cast<std::uint64_t>(std::llround(mind.goalX)));
        h = mix(h, static_cast<std::uint64_t>(std::llround(mind.goalY)));
        h = mix(h, static_cast<std::uint64_t>(mind.thinkAt));
    }
    return h;
}

void NpcLife::tick(OdysseyGame& game) {
    if (game.clan() != nullptr) {
        const auto& figures = game.clanView().figures();
        for (std::size_t i = 0; i < figures.size(); ++i) {
            if (figures[i].present) actPerson(game, static_cast<int>(i));
        }
    }
    // The animals that act are the harmless ones placed in the level (deer, rabbits): the prey.
    for (std::size_t b = 0; b < game.bystanders().size(); ++b) {
        const CharacterKindDef* kind = game.definitions().character(game.bystanders()[b].kind);
        if (kind != nullptr && hasTag(kind->tags, "prey")) actAnimal(game, b);
    }
}

void NpcLife::actPerson(OdysseyGame& game, int person) {
    const Figure& figure = game.clanView().figures()[static_cast<std::size_t>(person)];
    runMind(game, kPersonActorBase + person, figure.x, figure.y);
}

void NpcLife::actAnimal(OdysseyGame& game, std::size_t bystanderIndex) {
    const PlacedCharacter& placed = game.bystanders()[bystanderIndex];
    const int runnerId = kAnimalActorBase + placed.id;
    // Exact positions are kept here (the level keeps whole pixels, enough to draw with).
    auto& feet = animalFeet_[placed.id];
    if (feet.first == 0.0 && feet.second == 0.0) feet = {static_cast<double>(placed.feet.x), static_cast<double>(placed.feet.y)};
    runMind(game, runnerId, feet.first, feet.second);
    // The animal walks (the clan view walks the people): one step toward where it is going.
    const auto it = minds_.find(runnerId);
    if (it == minds_.end() || (it->second.state != State::Walking && it->second.state != State::Fleeing)) return;
    const double toX = it->second.goalX - feet.first;
    const double toY = it->second.goalY - feet.second;
    const double distance = std::hypot(toX, toY);
    if (distance < 2.0) return;
    const double step = std::min(it->second.state == State::Fleeing ? kFleePixelsPerTick : kWalkPixelsPerTick, distance);
    const luna::engine::Box box{feet.first - 10.0, feet.second - 10.0, 20.0, 10.0};
    const luna::engine::Box moved = luna::engine::moveAndCollide(game.tileMap(), box, toX / distance * step, toY / distance * step);
    feet = {moved.x + 10.0, moved.y + 10.0};
    PlacedCharacter& shown = game.bystandersMutable()[bystanderIndex];
    shown.feet = {static_cast<int>(std::lround(feet.first)), static_cast<int>(std::lround(feet.second))};
    shown.facing = facingToward(toX, toY);
}
bool NpcLife::dangerNear(const OdysseyGame& game, int runnerId, double x, double y) const {
    const ActorRef self = actorOfRunnerId(runnerId);
    for (const Enemy& enemy : game.enemies()) {
        if (!enemy.isAlive() || (self.kind == ActorRef::Kind::Animal && enemy.id == self.index)) continue;
        const CharacterKindDef* kind = game.definitions().character(enemy.kindName);
        if (kind == nullptr || !hasTag(kind->tags, "hostile")) continue;
        if (std::hypot(enemy.feetX() - x, enemy.feetY() - y) <= kDangerMetres * kPixelsPerMetre) return true;
    }
    return false;
}

void NpcLife::runMind(OdysseyGame& game, int runnerId, double x, double y) {
    const std::int64_t now = game.actionClock();
    auto found = minds_.find(runnerId);
    if (found == minds_.end()) {
        Mind fresh;
        fresh.thinkAt = now + runnerId % kThinkTicks; // not everyone looks around on the same tick
        found = minds_.emplace(runnerId, fresh).first;
    }
    Mind& mind = found->second;
    const ActorRef actor = actorOfRunnerId(runnerId);
    const auto stop = [&] {
        game.actions().cancel(runnerId);
        if (actor.kind == ActorRef::Kind::Person) game.clanViewMutable().clearErrand(actor.index);
        mind.state = State::Idle;
        mind.interaction.clear();
        mind.thinkAt = now;
    };

    // Danger drops whatever they were doing (D-36); the next look decides what to do about it.
    if (mind.state != State::Idle && mind.state != State::Fleeing && dangerNear(game, runnerId, x, y)) {
        stop();
        return;
    }

    switch (mind.state) {
    case State::Idle:
        if (now >= mind.thinkAt) {
            mind.thinkAt = now + kThinkTicks;
            // A clan member with a hostile close by does not set off on an errand; an animal looks anyway, because it may want to flee.
            if (actor.kind == ActorRef::Kind::Person && dangerNear(game, runnerId, x, y)) break;
            think(game, runnerId, x, y);
        }
        break;
    case State::Walking: {
        const sim::rules::Interaction* interaction = game.interactions().find(mind.interaction);
        const std::optional<Subject> target = subjectFor(game, mind.target);
        if (interaction == nullptr || !target) {
            stop();
            break;
        }
        if (std::hypot(target->x - x, target->y - y) <= 0.8 * interaction->rangeMilli * kPixelsPerMetre / 1000.0) {
            startWork(game, runnerId, mind);
        } else if (now > mind.thinkAt + 600) { // wedged somewhere: give up after half a minute and look again
            cooldowns_.start(runnerId, mind.interaction, now + 100);
            stop();
        }
        break;
    }
    case State::Working:
        if (game.actions().running(runnerId) == nullptr) {
            if (const sim::rules::Interaction* interaction = game.interactions().find(mind.interaction); interaction != nullptr && interaction->npc) {
                cooldowns_.start(runnerId, mind.interaction, now + static_cast<std::int64_t>(interaction->npc->cooldownSeconds) * sim::rules::ActionRunner::kTicksPerSecond);
            }
            if (actor.kind == ActorRef::Kind::Person) game.clanViewMutable().clearErrand(actor.index);
            mind.state = State::Idle;
            mind.interaction.clear();
            mind.thinkAt = now + kThinkTicks;
        }
        break;
    case State::Fleeing:
        if (std::hypot(mind.goalX - x, mind.goalY - y) < 2.0 || now > mind.thinkAt + 120) {
            if (actor.kind == ActorRef::Kind::Person) game.clanViewMutable().clearErrand(actor.index);
            mind.state = State::Idle;
            mind.thinkAt = now + kThinkTicks / 2;
        }
        break;
    }
}

// Looks at what is near, scores every interaction this actor may do to it, and starts the best one.
void NpcLife::think(OdysseyGame& game, int runnerId, double x, double y) {
    const ActorRef actor = actorOfRunnerId(runnerId);
    const sim::rules::ThingInfo info = actorInfo(game, actor);
    const std::int64_t now = game.actionClock();
    const double reach = kLookMetres * kPixelsPerMetre;

    // What is near: plants for everyone; for animals also the hostile and the hero.
    std::vector<Subject> subjects;
    if (actor.kind == ActorRef::Kind::Person) { // the clan builds (US-253) and fights fires (US-255): blueprints and burning buildings close by are jobs
        for (const sim::buildings::PlacedBuilding& building : game.buildings().store().all()) {
            if (building.state != sim::buildings::State::Blueprint && !game.buildings().store().burning(building)) continue;
            const std::optional<Subject> job = buildingSubject(game, building.id);
            if (job && std::hypot(job->x - x, job->y - y) <= reach) subjects.push_back(*job);
        }
    }
    for (std::size_t i = 0; i < game.plants().size(); ++i) {
        const WorldPlant& plant = game.plants()[i];
        if (plant.present() && std::hypot(plant.feet.x - x, plant.feet.y - y) <= reach) subjects.push_back(plantSubject(game, i));
    }
    if (actor.kind == ActorRef::Kind::Animal) {
        for (std::size_t e = 0; e < game.enemies().size(); ++e) {
            const Enemy& other = game.enemies()[e];
            if (!other.isAlive() || other.id == actor.index || std::hypot(other.feetX() - x, other.feetY() - y) > reach) continue;
            subjects.push_back(animalSubject(game, e));
        }
        const Subject hero = heroSubject(game);
        if (std::hypot(hero.x - x, hero.y - y) <= reach) subjects.push_back(hero);
    }

    struct Candidate {
        const sim::rules::Interaction* interaction;
        std::size_t subject;
    };
    std::vector<Candidate> candidates;
    std::vector<int> scores;
    for (std::size_t s = 0; s < subjects.size(); ++s) {
        const GameRuleContext context(game, subjects[s], actor);
        // Distance 0: the range is for the doing, not for the wanting; a walker closes the distance.
        for (const sim::rules::Offer& offer : game.interactions().offered(info, subjects[s].info, 0, context)) {
            if (!offer.enabled || !offer.interaction->npc || !cooldowns_.ready(runnerId, offer.interaction->id, now)) continue;
            const long long score = sim::rules::evaluate(*offer.interaction->npc->score, context).number;
            candidates.push_back({offer.interaction, s});
            scores.push_back(static_cast<int>(std::clamp<long long>(score, 0, 100000)));
        }
    }
    const std::optional<std::size_t> pick = sim::rules::pickBest(scores, kMinScore, random_);
    if (!pick) return;

    const Candidate& chosen = candidates[*pick];
    const Subject& subject = subjects[chosen.subject];
    Mind& mind = minds_[runnerId];
    mind.interaction = chosen.interaction->id;
    mind.target = refOf(game, subject);
    mind.goalX = subject.x;
    mind.goalY = subject.y;
    mind.state = State::Walking;
    mind.thinkAt = now; // the walk is timed from here
    if (actor.kind == ActorRef::Kind::Person) game.clanViewMutable().setErrand(actor.index, {static_cast<int>(std::lround(subject.x)), static_cast<int>(std::lround(subject.y))});
    if (std::hypot(subject.x - x, subject.y - y) <= 0.8 * chosen.interaction->rangeMilli * kPixelsPerMetre / 1000.0) startWork(game, runnerId, mind);
}

// They are close enough: the same runner the hero uses starts the interaction.
void NpcLife::startWork(OdysseyGame& game, int runnerId, Mind& mind) {
    const std::int64_t now = game.actionClock();
    mind.state = State::Idle; // an instant interaction may send them running (do flee), which sets it again
    startInteractionFor(game, runnerId, mind.interaction, mind.target);
    if (game.actions().running(runnerId) != nullptr) {
        mind.state = State::Working;
        return;
    }
    if (mind.state == State::Fleeing) return;
    if (const sim::rules::Interaction* interaction = game.interactions().find(mind.interaction); interaction != nullptr && interaction->npc) {
        cooldowns_.start(runnerId, mind.interaction, now + static_cast<std::int64_t>(interaction->npc->cooldownSeconds) * sim::rules::ActionRunner::kTicksPerSecond);
    }
    const ActorRef actor = actorOfRunnerId(runnerId);
    if (actor.kind == ActorRef::Kind::Person) game.clanViewMutable().clearErrand(actor.index);
    mind.interaction.clear();
    mind.thinkAt = now + kThinkTicks;
}

void NpcLife::fleeFrom(OdysseyGame& game, int runnerId, const Subject& threat) {
    const ActorRef actor = actorOfRunnerId(runnerId);
    double x = 0.0;
    double y = 0.0;
    if (!actorPosition(game, actor, x, y)) return;
    double dx = x - threat.x;
    double dy = y - threat.y;
    double length = std::hypot(dx, dy);
    if (length < 1.0) {
        dx = 1.0;
        dy = 0.0;
        length = 1.0;
    }
    const double far = kFleeMetres * kPixelsPerMetre;
    const double maxX = game.tileMap().pixelWidth() - kPixelsPerMetre;
    const double maxY = game.tileMap().pixelHeight() - kPixelsPerMetre;
    Mind& mind = minds_[runnerId];
    mind.goalX = std::clamp(x + dx / length * far, kPixelsPerMetre, maxX);
    mind.goalY = std::clamp(y + dy / length * far, kPixelsPerMetre, maxY);
    mind.state = State::Fleeing;
    mind.interaction.clear();
    mind.thinkAt = game.actionClock(); // timed from here
    if (actor.kind == ActorRef::Kind::Person) game.clanViewMutable().setErrand(actor.index, {static_cast<int>(std::lround(mind.goalX)), static_cast<int>(std::lround(mind.goalY))});
}

} // namespace odysseus::game
