#pragma once

#include "boundary.h"

#include "game/art.h"
#include "game/editor_history.h"
#include "game/level.h"
#include "luna/engine/camera.h"
#include "luna/engine/input.h"
#include "luna/engine/renderer.h"
#include "luna/engine/tile_map.h"
#include "luna/engine/ui.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

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

// What a left click on the map does.
enum class EditorTool { Brush, Rectangle, Fill, Eraser };

const char* toolName(EditorTool tool);

// Editor mode (M2c): the world stands still while the owner looks at and changes the level.
// It edits the game's Level directly; the game rebuilds its play state from it on return.
class Editor {
public:
    Editor(Level& level, const Definitions& definitions, std::filesystem::path levelFile, int viewWidth, int viewHeight);

    // Gives the editor its pictures and builds its panels (the tile palette shows the tiles).
    void setTextures(const EditorTextures& textures);

    // Opens the editor looking at (x, y) in world pixels (where the game camera was).
    void enter(double centreX, double centreY);
    // One tick: panels, shortcuts, panning, and the chosen tool on the map.
    void update(const luna::engine::Intents& intents);
    void render(luna::engine::Renderer& renderer, double alpha) const;

    Level& level() { return level_; }
    const Level& level() const { return level_; }
    // Call after changing the level from outside the editor (the map shown is rebuilt).
    void levelChanged();
    const luna::engine::Camera& camera() const { return camera_; }
    double centreX() const { return centreX_; }
    double centreY() const { return centreY_; }

    EditorTool tool() const { return tool_; }
    void setTool(EditorTool tool) { tool_ = tool; }
    int tile() const { return tile_; }
    void setTile(int tile) { tile_ = tile; }
    bool gridShown() const { return grid_; }

    // Every edit goes through here: applied, and remembered for Undo.
    void run(std::unique_ptr<Command> command);
    bool undo();
    bool redo();
    const History& history() const { return history_; }
    // Saves to the level file (safely, with backups). Returns false and says why when it cannot.
    bool save();
    bool unsaved() const { return unsaved_; }
    const std::string& status() const { return status_; }

    // The map cell under a point of the screen (virtual pixels), if it is on the level.
    std::optional<std::pair<int, int>> cellAt(int screenX, int screenY) const;

    static constexpr int kPanPerTick = 8; // pixels per tick with the keys (160 per second)

private:
    void panTo(double x, double y);
    void say(std::string message);
    void buildPanels();
    bool handlePanels(const luna::engine::UiInput& input);
    void useTool(const luna::engine::Pointer& pointer, bool overPanel);
    void paintAt(int x, int y);
    void finishStroke();

    Level& level_;
    const Definitions& definitions_;
    std::filesystem::path levelFile_;
    int viewWidth_;
    int viewHeight_;
    luna::engine::TileMap map_;
    luna::engine::Camera camera_;
    EditorTextures textures_;
    double centreX_ = 0.0;
    double centreY_ = 0.0;
    bool dragging_ = false;
    int dragX_ = 0;
    int dragY_ = 0;

    EditorTool tool_ = EditorTool::Brush;
    int tile_ = 0;
    bool grid_ = true;
    History history_;
    bool unsaved_ = false;
    std::string status_;
    int statusTicks_ = 0;

    // A brush or eraser stroke in progress: painted as the pointer moves, one Command at the end.
    bool stroking_ = false;
    std::pair<int, int> lastCell_{0, 0};
    std::vector<CellChange> stroke_;
    std::set<std::pair<int, int>> strokeCells_;
    // A rectangle being dragged out: its first corner and the cell under the pointer now.
    std::optional<std::pair<int, int>> rectangleStart_;
    std::pair<int, int> rectangleEnd_{0, 0};
    std::optional<std::pair<int, int>> hover_;

    std::unique_ptr<luna::engine::Panel> toolbar_;
    std::unique_ptr<luna::engine::Panel> palette_;
};

} // namespace odysseus::game
