#include "game/editor.h"

#include "luna/engine/ui.h"

#include <algorithm>
#include <cmath>

namespace odysseus::game {

using luna::engine::Intents;
using luna::engine::PointerButton;

Editor::Editor(Level& level, const Definitions& definitions, int viewWidth, int viewHeight)
    : level_(level), definitions_(definitions), viewWidth_(viewWidth), viewHeight_(viewHeight), map_(buildTileMap(level, definitions)),
      camera_(viewWidth, viewHeight, map_.pixelWidth(), map_.pixelHeight()) {}

void Editor::levelChanged() {
    map_ = buildTileMap(level_, definitions_);
    const double x = centreX_;
    const double y = centreY_;
    camera_ = luna::engine::Camera(viewWidth_, viewHeight_, map_.pixelWidth(), map_.pixelHeight());
    centreX_ = x;
    centreY_ = y;
    camera_.centreOn(x, y);
}

void Editor::enter(double centreX, double centreY) {
    levelChanged();
    centreX_ = centreX;
    centreY_ = centreY;
    camera_.centreOn(centreX_, centreY_);
}

void Editor::panTo(double x, double y) {
    // The centre stays where a camera can look, so panning back responds at once.
    const double halfW = viewWidth_ / 2.0;
    const double halfH = viewHeight_ / 2.0;
    const double worldW = map_.pixelWidth();
    const double worldH = map_.pixelHeight();
    centreX_ = worldW <= viewWidth_ ? worldW / 2.0 : std::clamp(x, halfW, worldW - halfW);
    centreY_ = worldH <= viewHeight_ ? worldH / 2.0 : std::clamp(y, halfH, worldH - halfH);
}

void Editor::update(const Intents& intents) {
    double x = centreX_ + intents.moveX() * kPanPerTick;
    double y = centreY_ + intents.moveY() * kPanPerTick;
    // Right-button drag: the world moves with the pointer, like sliding a map on a table.
    const auto& pointer = intents.pointer();
    if (pointer.isHeld(PointerButton::Right) && pointer.inside()) {
        if (dragging_) {
            x -= pointer.x - dragX_;
            y -= pointer.y - dragY_;
        }
        dragging_ = true;
        dragX_ = pointer.x;
        dragY_ = pointer.y;
    } else {
        dragging_ = false;
    }
    panTo(x, y);
    camera_.follow(centreX_, centreY_, 1.0);
}

void Editor::render(luna::engine::Renderer& renderer, const EditorTextures& textures, double alpha) const {
    map_.draw(renderer, textures.tiles, camera_, alpha);
    const luna::engine::Rect view = camera_.view(alpha);
    auto screen = [&view](int worldX, int worldY) { return luna::engine::Point{worldX - view.x, worldY - view.y}; };
    luna::engine::UiPainter painter(renderer, textures.ui);

    for (const PixelPoint& target : level_.targets) {
        renderer.draw(textures.props, kTargetFrame, screen(target.x - kTargetFrame.width / 2, target.y - kTargetFrame.height));
    }
    for (const PlacedCharacter& placed : level_.characters) {
        const CharacterKindDef* kind = definitions_.character(placed.kind);
        if (kind == nullptr || textures.art == nullptr) continue;
        renderer.draw(textures.characters, textures.art->frame(kind->frames, kind->directions, placed.facing, 0),
                      screen(placed.feet.x - kCharacterWidth / 2, placed.feet.y - kCharacterHeight));
    }
    // Where the hero will begin: the hero, framed in gold, with a label.
    const auto start = screen(level_.heroStart.x - kCharacterWidth / 2, level_.heroStart.y - kCharacterHeight);
    renderer.draw(textures.heroSheet, {0, 0, kCharacterWidth, kCharacterHeight}, start);
    painter.outline({start.x - 1, start.y - 1, kCharacterWidth + 2, kCharacterHeight + 2}, luna::engine::UiColor::Gold);
    painter.text(start.x + (kCharacterWidth - luna::engine::UiPainter::textWidth("START")) / 2, start.y - 9, "START",
                 luna::engine::UiColor::Gold);
}

} // namespace odysseus::game
