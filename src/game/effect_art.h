#pragma once

#include "boundary.h"

#include "luna/engine/renderer.h"

#include <map>
#include <string>

namespace odysseus::game {

// The first picture of each effect of effects.json (US-138), for the Editor's effect palette and for showing placed
// effects there (the game plays them as animations through the Luna effect player).
struct EffectArt {
    luna::engine::Texture page;
    std::map<std::string, luna::engine::Rect> firstFrame; // effect name -> frame 0 in the page
};

inline void drawEffectPicture(luna::engine::Renderer& renderer, const EffectArt& art, const std::string& effect, const luna::engine::Rect& area) {
    const auto found = art.firstFrame.find(effect);
    if (found == art.firstFrame.end()) return;
    renderer.drawStyled(art.page, found->second, area, {220, luna::engine::Blend::Add});
}

} // namespace odysseus::game
