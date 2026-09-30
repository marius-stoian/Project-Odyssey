#pragma once

#include "boundary.h"

#include "game/clan_art.h"
#include "game/level.h"
#include "luna/engine/tile_map.h"
#include "sim/world.h"

#include <cstdint>
#include <vector>

namespace odysseus::game {

// The clan on screen (US-032, US-031, D-30). The simulation has no places, only what people do each hour; the game
// gives that a place. Each action has somewhere to happen around the camp (the fire, the gathering ground to the
// west, the hunting ground to the east, the store), each person has a spot of their own there, and walks between
// them at walking pace on the game's 20 ticks a second, so the movement is smooth. Nothing here changes the
// simulation: the view only reads it.

enum class Emote { None, Cold, Hungry, Sick, Tired, Lonely };

const char* emoteName(Emote emote);

// What a person's face or posture says now. The most urgent need wins: cold, hunger, sickness, tiredness,
// loneliness. Critical means 20 or less (15 for tiredness and loneliness).
Emote emoteOf(const sim::Person& person);

struct Figure {
    bool present = false;       // alive and still in the clan
    bool placed = false;        // has been given a starting place
    double x = 0.0;             // feet, world pixels
    double y = 0.0;
    double previousX = 0.0;
    double previousY = 0.0;
    Facing facing = Facing::South;
    bool walking = false;
    int walkTicks = 0;
    int stuckTicks = 0;
    LookSpec look;
    bool child = false;
    Emote emote = Emote::None;

    double feetX(double alpha) const { return previousX + (x - previousX) * alpha; }
    double feetY(double alpha) const { return previousY + (y - previousY) * alpha; }
    int animationFrame() const { return walking ? (walkTicks * 8 / 20) % kWalkFrames : 0; }
};

// Children (under 12) are drawn three quarters the size.
inline constexpr int kChildYears = 12;
inline constexpr double kClanWalkPixelsPerTick = 3.0; // 60 pixels a second, slower than the hero

class ClanView {
public:
    // `camp`: the middle of the camp, world pixels (the fire).
    explicit ClanView(PixelPoint camp = {}) : camp_(camp) {}

    // One game tick: everyone walks a step toward where their action takes them.
    void update(const sim::World& world, const luna::engine::TileMap& map);

    // Indexed by person id (the dead and the exiled are not present).
    const std::vector<Figure>& figures() const { return figures_; }
    PixelPoint camp() const { return camp_; }
    // The person the player plays is drawn as the hero avatar, not as a clan member: no figure for them.
    void setHidden(int personId) { hidden_ = personId; }

    // Where a person doing `action` goes, in world pixels (before nudging it onto free ground): the place of the action
    // plus the person's own spot there.
    static PixelPoint targetOf(int personId, sim::Action action, PixelPoint camp, std::int64_t hourNumber);

private:
    PixelPoint camp_;
    int hidden_ = -1;
    std::vector<Figure> figures_;
};

} // namespace odysseus::game
