#include "game/enemy.h"

#include "core/log.h"

#include <algorithm>
#include <format>

namespace odysseus::game {

namespace {
constexpr int kFlashTicks = 5;   // a quarter of a second at 20 ticks per second
constexpr int kWindUpTicks = 10; // half a second to see it coming (D-21)
} // namespace

Enemy::Enemy(double feetX, double feetY, int maxHp) : feetX_(feetX), feetY_(feetY), hp_(maxHp), maxHp_(maxHp) {}

bool Enemy::takeDamage(int damage) {
    hp_ = std::max(0, hp_ - damage);
    damageFlashTicks_ = kFlashTicks;
    core::logInfo(std::format("{} took {} damage, HP {} / {}", name, damage, hp_, maxHp_));
    if (hp_ == 0) {
        core::logInfo(name + " was defeated");
        return true;
    }
    return false;
}

void Enemy::provoke() {
    if (isAlive() && state_ == Strike::Idle) {
        state_ = Strike::WindUp;
        windUpTicks_ = kWindUpTicks;
    }
}

bool Enemy::update() {
    if (damageFlashTicks_ > 0) {
        --damageFlashTicks_;
    }
    if (state_ != Strike::WindUp) {
        return false;
    }
    if (!isAlive()) {
        state_ = Strike::Idle; // the dead do not strike
        return false;
    }
    if (--windUpTicks_ > 0) {
        return false;
    }
    state_ = Strike::Idle;
    return true;
}

} // namespace odysseus::game
