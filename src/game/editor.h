#pragma once

#include "boundary.h"

#include "game/animals.h"
#include "game/art.h"
#include "game/editor_history.h"
#include "game/level.h"
#include "game/pickups.h"
#include "game/plants.h"
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
    const WeaponArt* weapons = nullptr; // the weapon icons, for pickups and the weapon palette (US-134)
    const PlantArt* plants = nullptr;   // the plant pictures, for the plant palette and placed plants (US-136)
    const AnimalArt* animals = nullptr; // the animal pictures, for the character palette and placed animals (US-137)
};

// What a left click on the map does.
enum class EditorTool { Brush, Rectangle, Fill, Eraser, Place, Select, Weapon, Plant };

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
    // The character kind the Place tool puts down (an index into Definitions::characters).
    int kind() const { return kind_; }
    // The character palette shows 12 kinds to a page (the first page is the twelve characters, then the animals).
    static constexpr int kKindsPerPage = 12;
    int kindPage() const { return kindPage_; }
    void setKind(int kind) { kind_ = kind; }
    // Weapon pickups (US-134): the weapons the palette offers, in order (names that are in weapons.json
    // or built in); the Weapon tool places the chosen one.
    void setWeaponPalette(std::vector<std::string> names);
    const std::vector<std::string>& weaponPalette() const { return weaponNames_; }
    int weapon() const { return weapon_; }
    void setWeapon(int weapon) { weapon_ = weapon; }
    // The pickup whose icon covers a screen point, if any.
    std::optional<int> pickupAt(int screenX, int screenY) const;
    // Plants (US-136): the Plant tool places the chosen plant of Definitions::plants at the clicked cell (its feet
    // in the middle of the cell's bottom edge); the palette shows them by picture, 36 to a page.
    int plant() const { return plant_; }
    void setPlant(int plant) { plant_ = plant; }
    int plantPage() const { return plantPage_; }
    static constexpr int kPlantsPerPage = 36;
    // The placed plant whose picture covers a screen point, if any.
    std::optional<int> plantAt(int screenX, int screenY) const;
    // The placed character selected with the Select tool (its id), if any.
    std::optional<int> selected() const { return selected_; }
    void select(std::optional<int> id);
    // Changes one property of the selected character, as one step of Undo.
    void setSelectedName(const std::string& name);
    void setSelectedHp(int hp);
    void setSelectedSwordDamage(int damage);
    // The character whose picture covers a screen point (the one drawn last, on top), if any.
    std::optional<int> characterAt(int screenX, int screenY) const;

    // Level settings (US-126), each one step of Undo.
    void setLevelName(const std::string& name);
    void setLevelSize(int width, int height);  // keeps painted cells; drops characters outside
    void setDefaultGround(int tile);
    void moveHeroStart(PixelPoint feet);
    bool settingsShown() const { return settingsShown_; }
    void showSettings(bool shown);

    // Levels on disk: the file being edited, the other levels next to it, and switching.
    const std::filesystem::path& levelFile() const { return levelFile_; }
    std::vector<std::filesystem::path> levelFiles() const;
    // Opens another level, or starts a new one. With unsaved changes they only ask first
    // (`asking()`); `answer` then saves, discards or cancels.
    void requestOpen(const std::filesystem::path& file);
    void requestNew();
    enum class Answer { Save, Discard, Cancel };
    bool asking() const { return pending_.has_value(); }
    void answer(Answer answer);

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
    void usePlaceOrSelect(const luna::engine::Pointer& pointer, bool pressed, bool held, bool released);
    void changeCharacters(const std::string& what, std::vector<PlacedCharacter> after, int nextIdAfter);
    PlacedCharacter* find(int id);
    const PlacedPickup* findPickup(int id) const;
    const PlacedPlant* findPlant(int id) const;
    void changePlants(const std::string& what, std::vector<PlacedPlant> after, int nextIdAfter);
    void buildPlantPalette();
    void buildCharacterPalette();
    void changePickups(const std::string& what, std::vector<PlacedPickup> after, int nextIdAfter);
    void removeSelected();
    void buildProperties();
    std::pair<int, int> toWorld(int screenX, int screenY) const;
    void changeLevel(const std::string& what, Level after);
    void buildSettings();
    void buildOpenList();
    void buildQuestion();
    void replaceLevel(Level level, std::filesystem::path file, const std::string& what);
    void doPending();

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
    std::unique_ptr<luna::engine::Panel> characterPalette_;
    std::unique_ptr<luna::engine::Panel> weaponPalette_;
    std::vector<std::string> weaponNames_;
    std::unique_ptr<luna::engine::Panel> plantPalette_;
    int plant_ = 0;
    int kindPage_ = 0;
    int kindPageWanted_ = 0;
    int plantPage_ = 0;
    int plantPageWanted_ = 0; // a page button sets this; the palette is rebuilt at the start of the next tick
    int weapon_ = 0;
    std::unique_ptr<luna::engine::Panel> properties_;
    int propertiesFor_ = -1;       // the character the properties panel shows (-1: none)
    bool propertiesStale_ = false; // the character changed (undo, redo): show its values again

    int kind_ = 0;
    std::optional<int> selected_;
    // A character being dragged with the Select tool: the list before, and where it was grabbed.
    bool moving_ = false;
    std::vector<PlacedCharacter> movingBefore_;
    // A pickup being dragged with the Select tool: the list before.
    bool movingPickup_ = false;
    std::vector<PlacedPickup> movingPickupsBefore_;
    // A plant being dragged with the Select tool: the list before.
    bool movingPlant_ = false;
    std::vector<PlacedPlant> movingPlantsBefore_;
    bool movingStart_ = false; // the hero start marker is being dragged
    PixelPoint startBefore_;

    bool settingsShown_ = false;
    bool settingsStale_ = false;
    std::unique_ptr<luna::engine::Panel> settings_;
    std::unique_ptr<luna::engine::Panel> openList_;
    std::unique_ptr<luna::engine::Panel> question_;
    // What waits for an answer about unsaved changes: open this file (or, when empty, a new level).
    std::optional<std::filesystem::path> pending_;
    int grabX_ = 0;
    int grabY_ = 0;
};

} // namespace odysseus::game
