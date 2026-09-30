#include "game/sword.h"

#include <cmath>

namespace odysseus::game {

Sword::Sword(SwordConfig config) : config_(config) {}

void Sword::slash(Facing facing) {
    // Only slash if not on cooldown
    if (state_.cooldownTicks <= 0) {
        state_.state = SlashState::Slashing;
        state_.slashTicks = 0;
        state_.cooldownTicks = 0;
        lastSlashFacing_ = facing;
    }
}

void Sword::update() {
    switch (state_.state) {
    case SlashState::Idle:
        state_.cooldownTicks = std::max(0, state_.cooldownTicks - 1);
        break;
    case SlashState::Slashing:
        ++state_.slashTicks;
        if (state_.slashTicks >= static_cast<int>(config_.slashDurationTicks)) {
            // Slash complete: enter cooldown
            state_.state = SlashState::Cooldown;
            state_.cooldownTicks = config_.slashCooldownTicks;
            state_.slashTicks = 0;
        }
        break;
    case SlashState::Cooldown:
        --state_.cooldownTicks;
        if (state_.cooldownTicks <= 0) {
            state_.state = SlashState::Idle;
            state_.cooldownTicks = 0;
        }
        break;
    }
}

int Sword::animationFrame() const {
    if (state_.state != SlashState::Slashing) {
        return 0; // no slash, show idle frame
    }
    // 4 frames: 0-1, 1-2, 2-3, 3+ (normalize to 0-3)
    const double frameProgress = state_.slashTicks / config_.slashDurationTicks;
    return static_cast<int>(std::floor(frameProgress * 4.0)) % 4;
}

} // namespace odysseus::game
