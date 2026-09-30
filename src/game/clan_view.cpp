#include "game/clan_view.h"

#include "game/weapons.h"
#include "luna/engine/collision.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace odysseus::game {

namespace {

constexpr int kCriticalNeed = 20;   // Hunger and Warmth this low show on the face
constexpr int kTiredNeed = 15;      // Energy and Social this low

// Where on the map each action happens, as tiles from the camp (x east, y south) and how wide the place is.
struct Place {
    double tilesX;
    double tilesY;
    double radiusTiles;
};

Place placeOf(sim::Action action) {
    switch (action) {
    case sim::Action::Gather: return {-9.0, 2.0, 3.0};      // the gathering ground, west
    case sim::Action::Hunt: return {12.0, -3.0, 3.0};       // the hunting ground, east
    case sim::Action::Sleep: return {0.0, 0.0, 2.6};        // beds in a ring around the fire
    case sim::Action::WarmByFire: return {0.0, 0.0, 1.3};   // close to the fire
    case sim::Action::Talk: return {0.0, 1.6, 2.0};         // the talking place, south of the fire
    case sim::Action::GiveGift: return {0.0, 1.6, 1.4};
    case sim::Action::Steal: return {3.0, -3.0, 0.8};       // the food store
    case sim::Action::Rest: return {0.0, 0.0, 3.2};
    case sim::Action::Wander: return {0.0, 0.0, 6.0};
    default: return {0.0, 0.0, 3.0};
    }
}

// A fixed, well-spread spot inside a place for each person: the golden angle walks round the circle without repeating.
void spotIn(int personId, double radiusTiles, double& dx, double& dy) {
    const double angle = personId * 2.399963229728653;
    const double share = std::fmod(personId * 0.6180339887, 1.0);
    const double radius = radiusTiles * (0.35 + 0.65 * share);
    dx = std::cos(angle) * radius;
    dy = std::sin(angle) * radius;
}

// The nearest walkable cell to a point, looking outward in rings up to 4 tiles.
void nudgeOntoFreeGround(const luna::engine::TileMap& map, double& x, double& y) {
    const int cellX = static_cast<int>(std::floor(x / kTileSize));
    const int cellY = static_cast<int>(std::floor(y / kTileSize));
    if (!map.isSolid(cellX, cellY)) return;
    for (int ring = 1; ring <= 4; ++ring) {
        for (int dy = -ring; dy <= ring; ++dy) {
            for (int dx = -ring; dx <= ring; ++dx) {
                if (std::max(std::abs(dx), std::abs(dy)) != ring) continue;
                if (!map.isSolid(cellX + dx, cellY + dy)) {
                    x = (cellX + dx) * kTileSize + kTileSize / 2.0;
                    y = (cellY + dy) * kTileSize + kTileSize / 2.0;
                    return;
                }
            }
        }
    }
}

} // namespace

const char* emoteName(Emote emote) {
    switch (emote) {
    case Emote::Cold: return "cold";
    case Emote::Hungry: return "hungry";
    case Emote::Sick: return "unwell";
    case Emote::Tired: return "tired";
    case Emote::Lonely: return "lonely";
    default: return "";
    }
}

Emote emoteOf(const sim::Person& person) {
    if (!person.alive || person.exiled) return Emote::None;
    if (person.needs[sim::Need::Warmth] <= kCriticalNeed) return Emote::Cold;
    if (person.needs[sim::Need::Hunger] <= kCriticalNeed) return Emote::Hungry;
    if (person.health != sim::Health::Well) return Emote::Sick;
    if (person.needs[sim::Need::Energy] <= kTiredNeed) return Emote::Tired;
    if (person.needs[sim::Need::Social] <= kTiredNeed) return Emote::Lonely;
    return Emote::None;
}

PixelPoint ClanView::targetOf(int personId, sim::Action action, PixelPoint camp, std::int64_t hourNumber) {
    const Place place = placeOf(action);
    double dx = 0.0;
    double dy = 0.0;
    if (action == sim::Action::Wander) {
        // Wanderers stroll: a new spot each hour.
        const double angle = personId * 1.7 + static_cast<double>(hourNumber) * 2.1;
        dx = std::cos(angle) * place.radiusTiles * 0.8;
        dy = std::sin(angle) * place.radiusTiles * 0.6;
    } else {
        spotIn(personId, place.radiusTiles, dx, dy);
    }
    return {camp.x + static_cast<int>(std::lround((place.tilesX + dx) * kTileSize)), camp.y + static_cast<int>(std::lround((place.tilesY + dy) * kTileSize))};
}

void ClanView::update(const sim::World& world, const luna::engine::TileMap& map) {
    const auto& people = world.people();
    if (figures_.size() < people.size()) figures_.resize(people.size());
    const int daysPerYear = world.calendar().daysPerYear();
    const std::int64_t hourNumber = static_cast<std::int64_t>(world.ticks() / static_cast<std::uint64_t>(world.calendar().ticksPerHour()));
    for (std::size_t i = 0; i < people.size(); ++i) {
        const sim::Person& person = people[i];
        Figure& figure = figures_[i];
        figure.present = person.alive && !person.exiled;
        if (!figure.present) {
            figure.walking = false;
            continue;
        }
        const int ageYears = person.ageYears(daysPerYear);
        figure.look = lookOf(person, ageYears);
        figure.child = ageYears < kChildYears;
        figure.emote = emoteOf(person);

        const PixelPoint target = targetOf(person.id, person.action, camp_, hourNumber);
        double goalX = target.x;
        double goalY = target.y;
        nudgeOntoFreeGround(map, goalX, goalY);
        if (!figure.placed) {
            // A newcomer (a founder, or a baby) appears at the fire, then walks to the day's first place.
            double dx = 0.0;
            double dy = 0.0;
            spotIn(person.id, 1.6, dx, dy);
            figure.x = camp_.x + dx * kTileSize;
            figure.y = camp_.y + dy * kTileSize;
            nudgeOntoFreeGround(map, figure.x, figure.y);
            figure.previousX = figure.x;
            figure.previousY = figure.y;
            figure.placed = true;
        }
        figure.previousX = figure.x;
        figure.previousY = figure.y;
        const double toX = goalX - figure.x;
        const double toY = goalY - figure.y;
        const double distance = std::hypot(toX, toY);
        if (distance < 2.0) {
            figure.walking = false;
            figure.walkTicks = 0;
            figure.stuckTicks = 0;
            continue;
        }
        const double step = std::min(kClanWalkPixelsPerTick, distance);
        const luna::engine::Box feet{figure.x - 10.0, figure.y - 10.0, 20.0, 10.0};
        const luna::engine::Box moved = luna::engine::moveAndCollide(map, feet, toX / distance * step, toY / distance * step);
        const double newX = moved.x + 10.0;
        const double newY = moved.y + 10.0;
        const double gained = std::hypot(newX - figure.x, newY - figure.y);
        figure.x = newX;
        figure.y = newY;
        figure.walking = gained > 0.05;
        figure.stuckTicks = gained < step * 0.25 ? figure.stuckTicks + 1 : 0;
        if (figure.stuckTicks > 60) { // wedged behind something: step to the goal rather than stand forever
            figure.x = goalX;
            figure.y = goalY;
            figure.stuckTicks = 0;
        }
        if (figure.walking) {
            ++figure.walkTicks;
            figure.facing = facingToward(toX, toY);
        } else {
            figure.walkTicks = 0;
        }
    }
}

} // namespace odysseus::game
