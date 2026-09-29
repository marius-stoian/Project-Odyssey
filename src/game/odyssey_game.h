#pragma once

#include "boundary.h"

#include "luna/engine/application.h"
#include "luna/engine/game.h"
#include "luna/engine/renderer.h"

#include <cstdint>

namespace odysseus::game {

// Project Odyssey as Luna sees it. It grows story by story: a window now, a walking
// character by the end of M1, the living clan in M3.
class OdysseyGame final : public luna::engine::Game {
public:
    void start(luna::engine::Renderer& renderer) override;
    void update(const luna::engine::Intents& intents) override;
    void render(luna::engine::Renderer& renderer, double alpha) override;

    std::uint64_t ticks() const;

private:
    std::uint64_t ticks_ = 0;
    luna::engine::Texture characters_;
    luna::engine::Texture tiles_;
};

// Window title, sizes and colours for Luna.
luna::engine::AppConfig odysseyAppConfig();

} // namespace odysseus::game
