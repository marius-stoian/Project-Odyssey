#pragma once

#include "boundary.h"

#include "game/building_layer.h"
#include "game/editor_help.h"
#include "game/editor_history.h"
#include "game/level.h"
#include "luna/engine/input.h"
#include "luna/engine/renderer.h"
#include "luna/engine/tile_map.h"
#include "luna/engine/ui.h"
#include "sim/building_data.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace odysseus::game {

// The Editor's part for buildings (US-256): the Build tool that puts finished buildings or blueprints in the level, the panel of a selected building
// (turn, interior mode, owner), and the Prefab tab where the owner composes a building from pieces on a grid and saves it as a prefab file. The Editor
// owns one of these and forwards to it; it is kept apart because the Editor is already the largest class of the game.
class BuildingEditor {
public:
    struct Host {
        sim::buildings::BuildingData* data = nullptr;               // the game's data: a saved prefab joins it
        std::filesystem::path prefabFolder;                         // assets/data/buildings/prefabs
        std::function<void(const sim::buildings::KindDef&)> saved;  // the game is told (its Definitions, its Build menu)
    };
    // `run` puts a command through the Editor's history; `say` writes the Editor's status line.
    BuildingEditor(Level& level, int viewWidth, int viewHeight, std::function<void(std::unique_ptr<Command>)> run, std::function<void(const std::string&)> say);

    void setHost(Host host);
    void setLayer(const BuildingLayer* layer) { layer_ = layer; }
    bool ready() const { return host_.data != nullptr; }

    // ---- the Build tool
    std::vector<std::string> kinds() const;       // kinds.json, then the prefabs: the palette's rows
    int kindIndex() const { return kind_; }
    void setKind(int index);
    std::string currentKind() const;
    // Places the current kind with its middle on cell (x, y), finished (or as a blueprint when `blueprint`); false and a message when it cannot stand there.
    bool placeAt(const luna::engine::TileMap& ground, int cellX, int cellY, bool blueprint = false);
    // Why the current kind cannot stand with its middle on cell (x, y): "" when it can.
    std::string whyNot(const luna::engine::TileMap& ground, int cellX, int cellY) const;
    void turnPlaced() { turns_ = (turns_ + 1) % 4; }
    // ---- a selected building
    bool selectAt(int worldX, int worldY);  // true when a building of the level is there (the topmost); it becomes the selection
    void clearSelection();
    bool hasSelection() const { return selected_ != 0; }
    const PlacedBuildingSpec* selectedSpec() const;
    bool turnSelected();
    bool removeSelected();
    bool setInterior(const std::string& mode, const std::string& levelName); // "", "fade" or "map"; "" follows the kind
    bool setOwner(int person);
    bool setFinished(bool finished);
    // The footprint of a spec in cells (width, height after its turns).
    std::pair<int, int> sizeOf(const PlacedBuildingSpec& spec) const;

    // ---- the Prefab tab
    bool prefabsShown() const { return prefabsShown_; }
    void showPrefabs(bool shown);
    sim::buildings::KindDef& draft() { return draft_; }
    const sim::buildings::KindDef& draft() const { return draft_; }
    void newPrefab();
    bool loadPrefab(const std::string& id);         // the draft is a copy of the kind or prefab (a kind is saved as a new prefab with another id)
    bool setPiece(int cellX, int cellY, const std::string& pieceId);    // "" takes the piece of that layer away; a cell outside the footprint is refused
    bool setSize(int width, int height);
    void setPrefabId(const std::string& id) { draft_.id = id; draftChanged_ = true; }
    void setPrefabLabel(const std::string& label) { draft_.label = label; draftChanged_ = true; }
    void setPrefabInterior(const std::string& mode, const std::string& levelName);
    void togglePrefabUse(const std::string& use);
    void setPrefabFlags(bool buildable, bool known);
    bool setPrefabCost(const std::string& text);    // "wood=10 fur=2"; empty: the sum of the pieces
    void setPrefabSeconds(int seconds);             // 0: the sum of the pieces
    bool savePrefab();                              // checks the draft, writes assets/data/buildings/prefabs/<id>.json, adds it to the data and tells the game

    // ---- the panels
    // Every tick: true when the pointer is over a panel of this class (so the map does not get the click).
    bool handle(const luna::engine::UiInput& input, bool buildTool);
    bool typing() const;
    // Gives the fields of both panels their help (US-300); call once a tick.
    void applyHelp(EditorHelp& help);
    void drawPanels(luna::engine::UiPainter& painter, luna::engine::Renderer& renderer, bool buildTool) const;
    void drawOverlay(luna::engine::UiPainter& painter, bool buildTool) const;
    // The placed buildings, the ghost of the Build tool and the selection, in the world (under the characters).
    void drawWorld(luna::engine::Renderer& renderer, luna::engine::UiPainter& painter, const luna::engine::Rect& view, const luna::engine::TileMap& ground, bool buildTool, std::optional<std::pair<int, int>> hover) const;

    static constexpr int kCanvasCells = 12;

private:
    void buildPalette();
    void buildPanel();
    void buildPrefabPanel();
    void change(const std::string& what, std::vector<PlacedBuildingSpec> after, int nextIdAfter);
    const sim::buildings::BuildingData* data() const { return host_.data; }
    luna::engine::Rect canvasRect() const;

    Level& level_;
    int viewWidth_;
    int viewHeight_;
    std::function<void(std::unique_ptr<Command>)> run_;
    std::function<void(const std::string&)> say_;
    Host host_;
    const BuildingLayer* layer_ = nullptr;

    int kind_ = 0;
    int turns_ = 0;
    int selected_ = 0;
    bool stale_ = true;
    std::unique_ptr<luna::engine::Panel> palette_;
    std::unique_ptr<luna::engine::Panel> panel_;     // the selected building

    bool prefabsShown_ = false;
    sim::buildings::KindDef draft_;
    bool draftChanged_ = true;
    bool draftIsNew_ = true;
    std::string piece_;           // the piece the canvas places ("" erases)
    std::string costText_;
    int secondsOverride_ = 0;
    std::unique_ptr<luna::engine::Panel> prefab_;
    std::string prefabStatus_;
};

} // namespace odysseus::game
