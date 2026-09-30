#include "game/enemy.h"

#include "core/log.h"

#include <algorithm>
#include <format>

namespace odysseus::game {

namespace {
constexpr int kFlashTicks = 5; // a quarter of a second at 20 ticks per second
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

void Enemy::update() {
    if (damageFlashTicks_ > 0) {
        --damageFlashTicks_;
    }
}

} // namespace odysseus::game
