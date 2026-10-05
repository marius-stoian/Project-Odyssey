#pragma once

#include "boundary.h"

#include "game/level.h"
#include "luna/engine/image.h"
#include "luna/engine/input.h"
#include "luna/engine/renderer.h"
#include "sim/building_life.h"
#include "sim/building_store.h"

#include <filesystem>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace odysseus::game {

class OdysseyGame;

// What the Game layer does with buildings (M8d, M8e): reads the data, owns the store of the level being played, shows the Build menu and the
// ghost, draws the buildings with placeholder art, keeps the walking obstacles of the standing walls, and carries out the interactions
// (deliver materials, work, cancel, repair, douse). The rules themselves are in the Simulation (sim::buildings::BuildingStore).
class BuildingLayer {
public:
    // ---- data (US-250)
    // Reads assets/data/buildings; the mistakes of a file that did not load are kept as "file:line: message" (shown by the game like the other data mistakes).
    void loadData(const std::filesystem::path& dataDirectory, const std::set<std::string>& knownItems);
    const sim::buildings::BuildingData& data() const { return data_; }
    sim::buildings::BuildingData& dataMutable() { return data_; }
    const std::vector<std::string>& notes() const { return notes_; }
    sim::buildings::BuildingStore& store() { return store_; }
    const sim::buildings::BuildingStore& store() const { return store_; }

    // A level starts (or restarts): the store is made for its map, the buildings of the level stand, the walls block walking.
    void start(OdysseyGame& game);
    // One play tick (after the clan's tick): the Build menu and placing, drops picked up, fire, wear, obstacles.
    void tick(OdysseyGame& game, const luna::engine::Intents& world, const luna::engine::Intents& ui);

    // ---- knowing blueprints (D-55 Q2)
    bool knows(const std::string& kind) const;
    void learn(const std::string& kind);
    const std::set<std::string>& known() const { return known_; }
    void forgetAll(); // a new run: only the blueprints with `known` in their file

    // ---- the Build menu and placing (US-251, US-252)
    bool menuOpen() const { return menuOpen_; }
    bool placing() const { return !selected_.empty(); }
    void openMenu() { menuOpen_ = true; }
    void stopPlacing() { selected_.clear(); }
    void closeMenu() {
        menuOpen_ = false;
        selected_.clear();
    }
    // The Escape key: leaves placing, then the menu; true when it did something (so the run menu does not open).
    bool escape();
    // True while a click belongs to the Build menu (on its panel) or to placing: the attack does not fire.
    bool capturesPointer(const luna::engine::Pointer& uiPointer) const;
    const std::string& selected() const { return selected_; }
    int turns() const { return turns_; }
    void select(const std::string& kind) { selected_ = kind; menuOpen_ = true; }
    void setTurns(int turns) { turns_ = ((turns % 4) + 4) % 4; }
    // Places the selected kind with its top-left cell at (x, y): a blueprint (a finished building when `finished`); the reason when it cannot.
    // With a run the cost comes out of the bag as it is delivered; with no run the materials are free.
    std::string placeSelected(OdysseyGame& game, int cellX, int cellY);
    // Why the selected kind cannot stand with its top-left cell at (x, y): "" when it can.
    std::string whyNot(const OdysseyGame& game, const std::string& kind, int cellX, int cellY, int turns) const;
    // The footprint's top-left cell when the pointer is on cell (x, y): the middle of the footprint is under the pointer.
    std::pair<int, int> anchorFor(const std::string& kind, int cellX, int cellY, int turns) const;
    std::vector<std::string> menuKinds(bool pieces) const; // what the list shows: known buildings, or every piece
    bool piecesTab() const { return piecesTab_; }
    void setPiecesTab(bool pieces) { piecesTab_ = pieces; scroll_ = 0; }

    // ---- what the hero does with a building (US-251, US-255; the interactions of assets/data/interactions call these through built-in actions)
    std::string deliver(OdysseyGame& game, int id);
    std::string work(OdysseyGame& game, int id, int seconds);
    std::string cancel(OdysseyGame& game, int id);
    std::string repair(OdysseyGame& game, int id, int hp);
    std::string douse(OdysseyGame& game, int id);
    // The clan builds too (US-253): a clan member brings what the site still needs from the surroundings (up to 2 of each item) and does the work.
    std::string clanDeliver(OdysseyGame& game, int id);
    std::string clanWork(OdysseyGame& game, int id, int seconds);
    // Fire and raids (US-255): a fire weapon that stops against a standing piece sets it alight; rivals at war raid at the start of a season.
    bool fireHit(OdysseyGame& game, int cellX, int cellY);
    std::string clanDouse(OdysseyGame& game, int id);
    int raidsAtSeasonStart(OdysseyGame& game, int season); // how many rivals raided
    bool raidFrom(OdysseyGame& game, int rival, int season);   // one raid: false when there was nothing to hit
    const sim::buildings::RivalBuilders& rivalBuilders() const { return rivalBuilders_; }
    std::uint64_t hash() const { return store_.hash() ^ (rivalBuilders_.hash() * 31); }
    // Walking obstacles of the finished walls, posts and fences are put on the map again when the store says they changed.
    void syncObstacles(OdysseyGame& game);

    // ---- drawing (placeholder art until the owner adds sheets)
    void loadArt(luna::engine::Renderer& renderer);
    bool hasArt() const { return art_.id >= 0; }
    bool artStale() const { return art_.id >= 0 && artKinds_ != data_.kinds().size(); }
    // Floors and the ground part of blueprints; under everything.
    void drawGround(luna::engine::Renderer& renderer, const luna::engine::Rect& view) const;
    // Standing pieces (walls, doors, windows, posts, fences) of finished buildings: the ones behind the hero's feet, or the ones in front.
    void drawStanding(luna::engine::Renderer& renderer, const luna::engine::Rect& view, double heroFeetY, bool behind) const;
    // Roofs (faded over the room the hero is in), then the ghosts of blueprints under way and their labels.
    void drawRoofs(const OdysseyGame& game, luna::engine::Renderer& renderer, const luna::engine::Rect& view) const;
    void drawBlueprints(const OdysseyGame& game, luna::engine::Renderer& renderer, const luna::engine::Texture& uiSheet, const luna::engine::Rect& view) const;
    // A whole kind drawn at a cell, floors then walls then roofs (the Editor shows placed buildings and its ghost this way; roofs may be more see-through).
    void drawLayout(luna::engine::Renderer& renderer, const std::string& kind, int cellX, int cellY, int turns, const luna::engine::Rect& view, int alpha, int roofAlpha) const;
    // A piece as a flat square of its colour (the Prefab tab's canvas).
    void drawFlat(luna::engine::Renderer& renderer, const std::string& pieceId, const luna::engine::Rect& destination, int alpha) const;
    // The ghost under the pointer while placing.
    void drawGhost(const OdysseyGame& game, luna::engine::Renderer& renderer, const luna::engine::Rect& view) const;
    // The panel of the Build menu and its hints, in interface pixels.
    void drawMenu(const OdysseyGame& game, luna::engine::Renderer& ui, const luna::engine::Texture& uiSheet) const;

    // ---- saving
    std::string saveText() const;
    std::vector<std::string> loadText(const std::string& text);

    // The cell under a world point.
    static std::pair<int, int> cellOf(double worldX, double worldY);

    // Screenshots and tests: what the ghost is, and the rectangle of the panel.
    luna::engine::Rect panelRect(const OdysseyGame& game) const;
    int ghostCellX() const { return ghostX_; }
    int ghostCellY() const { return ghostY_; }
    bool ghostValid() const { return ghostProblem_.empty(); }
    const std::string& ghostProblem() const { return ghostProblem_; }
    int itemIndexAt(const OdysseyGame& game, int uiX, int uiY) const; // the list row under an interface point, or -1
    int tabAt(const OdysseyGame& game, int uiX, int uiY) const;       // 0 buildings, 1 pieces, -1 neither

private:
    int artIndex(const std::string& pieceId) const;
    void drawPiece(luna::engine::Renderer& renderer, int artIndex, int cellX, int cellY, const luna::engine::Rect& view, int alpha) const;
    void drawSwatch(luna::engine::Renderer& renderer, int swatch, const luna::engine::Rect& area, const luna::engine::Rect& view, int alpha) const;
    std::string costText(const OdysseyGame& game, const sim::ItemCounts& cost) const;
    bool blockedForBuilding(const OdysseyGame& game, int x, int y) const;
    bool heroInTheWay(const OdysseyGame& game, const sim::buildings::PlacedBuilding& building) const;
    void updatePointer(OdysseyGame& game, const luna::engine::Intents& world, const luna::engine::Intents& ui);

    sim::buildings::BuildingData data_;
    sim::buildings::BuildingStore store_{nullptr};
    sim::buildings::RivalBuilders rivalBuilders_;
    std::vector<std::string> notes_;
    std::set<std::string> known_;
    std::vector<std::pair<int, int>> obstacleCells_; // the cells this layer put on the map, to take them off again
    std::uint64_t obstacleVersion_ = 0;
    bool menuOpen_ = false;
    bool piecesTab_ = false;
    std::string selected_;
    int turns_ = 0;
    int scroll_ = 0;
    int hoverRow_ = -1;
    int ghostX_ = 0;
    int ghostY_ = 0;
    std::string ghostProblem_ = "none";
    luna::engine::Texture art_;
    int artPieces_ = 0;
    std::size_t artKinds_ = 0;
    std::uint64_t dayAtLastTick_ = ~0ULL;
    int seasonAtLastTick_ = -1;
};

} // namespace odysseus::game
