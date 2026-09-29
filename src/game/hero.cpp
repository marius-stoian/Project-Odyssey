#include "game/hero.h"

#include <cmath>

namespace odysseus::game {

const char* facingName(Facing facing) {
    switch (facing) {
    case Facing::South: return "South";
    case Facing::SouthWest: return "SouthWest";
    case Facing::West: return "West";
    case Facing::NorthWest: return "NorthWest";
    case Facing::North: return "North";
    case Facing::NorthEast: return "NorthEast";
    case Facing::East: return "East";
    case Facing::SouthEast: return "SouthEast";
    default: return "?";
    }
}

Facing facingFor(int moveX, int moveY, Facing current) {
    if (moveX == 0 && moveY == 0) {
        return current;
    }
    if (moveX == 0) {
        return moveY > 0 ? Facing::South : Facing::North;
    }
    if (moveY == 0) {
        return moveX > 0 ? Facing::East : Facing::West;
    }
    if (moveX > 0) {
        return moveY > 0 ? Facing::SouthEast : Facing::NorthEast;
    }
    return moveY > 0 ? Facing::SouthWest : Facing::NorthWest;
}

Hero::Hero(double feetX, double feetY, HeroConfig config)
    : config_(config), x_(feetX), y_(feetY), previousX_(feetX), previousY_(feetY) {}

void Hero::update(const luna::engine::Intents& intents, const luna::engine::TileMap& map) {
    previousX_ = x_;
    previousY_ = y_;
    const int moveX = intents.moveX();
    const int moveY = intents.moveY();
    walking_ = moveX != 0 || moveY != 0;
    if (!walking_) {
        walkTicks_ = 0; // back to the idle pose, facing where we last went
        return;
    }
    facing_ = facingFor(moveX, moveY, facing_);

    // Same speed in every direction (D-17): a diagonal step is shortened by sqrt(2).
    const double step = config_.speedPixelsPerSecond / config_.ticksPerSecond;
    const double length = (moveX != 0 && moveY != 0) ? std::sqrt(2.0) : 1.0;
    const luna::engine::Box moved = luna::engine::moveAndCollide(map, feetBox(), moveX * step / length, moveY * step / length);
    x_ = moved.x + config_.feetWidth / 2.0;
    y_ = moved.y + config_.feetHeight;
    ++walkTicks_;
}

double Hero::feetX(double alpha) const {
    return previousX_ + (x_ - previousX_) * alpha;
}

double Hero::feetY(double alpha) const {
    return previousY_ + (y_ - previousY_) * alpha;
}

int Hero::animationFrame() const {
    if (!walking_) {
        return 0;
    }
    return (walkTicks_ * config_.animationFramesPerSecond / config_.ticksPerSecond) % kWalkFrames;
}

luna::engine::Box Hero::feetBox() const {
    return {x_ - config_.feetWidth / 2.0, y_ - config_.feetHeight, config_.feetWidth, config_.feetHeight};
}

luna::engine::Rect Hero::spriteFrame() const {
    return {animationFrame() * kCharacterWidth, static_cast<int>(facing_) * kCharacterHeight, kCharacterWidth, kCharacterHeight};
}

} // namespace odysseus::game
