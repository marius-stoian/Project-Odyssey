#pragma once

#include "boundary.h"

#include "game/art.h"
#include "game/level.h"
#include "luna/engine/camera.h"
#include "luna/engine/input.h"
#include "luna/engine/renderer.h"
#include "luna/engine/tile_map.h"

namespace odysseus::game {

// The pictures the editor draws with (created by the game at start).
struct EditorTextures {
    luna::engine::Texture tiles;
    luna::engine::Texture heroSheet;
    luna::engine::Texture characters;
    luna::engine::Texture props;
    luna::engine::Texture ui;
    const ArtSet* art = nullptr;
};

// Editor mode (M2c): the world stands still while the owner looks at and changes the level.
// It edits the game's Level directly; the game rebuilds its play state from it on return.
class Editor {
public:
    Editor(Level& level, const Definitions& definitions, int viewWidth, int viewHeight);

    // Opens the editor looking at (x, y) in world pixels (where the game camera was).
    void enter(double centreX, double centreY);
    // One tick: pan with the move keys or by dragging with the right button.
    void update(const luna::engine::Intents& intents);
    void render(luna::engine::Renderer& renderer, const EditorTextures& textures, double alpha) const;

    Level& level() { return level_; }
    // Call after changing the level from outside (the map shown is rebuilt).
    void levelChanged();
    const luna::engine::Camera& camera() const { return camera_; }
    double centreX() const { return centreX_; }
    double centreY() const { return centreY_; }

    static constexpr int kPanPerTick = 8; // pixels per tick with the keys (160 per second)

private:
    void panTo(double x, double y);

    Level& level_;
    const Definitions& definitions_;
    int viewWidth_;
    int viewHeight_;
    luna::engine::TileMap map_;
    luna::engine::Camera camera_;
    double centreX_ = 0.0;
    double centreY_ = 0.0;
    bool dragging_ = false;
    int dragX_ = 0;
    int dragY_ = 0;
};

} // namespace odysseus::game
