#pragma once

#include "boundary.h"

#include "core/random.h"
#include "sim/building_data.h"

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace odysseus::sim::buildings {

// The buildings of a level and what happens to them (US-250..US-257): placing, delivering materials, working, wear, damage, fire, rooms.
// Everything is whole numbers and ordered containers; the only randomness is the stream "buildings" (Charter rule 6), so the same inputs always
// build and burn the same way. The store knows cells (one tile = one metre) and ticks (20 a second), never pixels.

inline constexpr int kTicksPerSecond = 20;

enum class State { Blueprint, Finished, Rubble };
const char* stateName(State state);

// A piece standing (or waiting to be built) in a cell.
struct PieceState {
    std::string piece; // an id of pieces.json
    int x = 0;         // the cell, absolute
    int y = 0;
    int hpMilli = 0;   // thousandths of a hit point; 0 with `built` true means it fell
    bool built = false;
    bool burning = false;
    int burnTicks = 0;
    bool fell() const { return built && hpMilli <= 0; }
    friend bool operator==(const PieceState&, const PieceState&) = default;
};

struct PlacedBuilding {
    int id = 0;
    std::string kind;       // a kind id, or "piece:<id>" for a piece placed on its own (US-252)
    int x = 0;              // the footprint's top-left cell, absolute
    int y = 0;
    int turns = 0;          // quarter turns clockwise, 0 to 3
    int width = 1;          // the footprint after the turns
    int height = 1;
    State state = State::Blueprint;
    int workMilli = 0;      // work done on the site (thousandths of a second)
    ItemCounts delivered;   // materials brought to the site
    std::vector<PieceState> pieces; // in the order they rise: floors, walls, roofs
    int faction = 0;        // 0: the player's clan; n: rival clan n
    int owner = -1;         // the clan member who lives here (kind owner "person"), or -1
    std::string interior;   // "" follows the kind; "fade" or "map" overrides it (the Editor, US-254)
    std::string interiorLevel;
    ItemCounts contents;    // what is stored here (US-257)
    int rubbleDays = 0;     // days since it fell to rubble
    friend bool operator==(const PlacedBuilding&, const PlacedBuilding&) = default;
};

// Materials left on the ground where a blueprint was cancelled (US-251): whoever walks over them picks them up.
struct Drop {
    int x = 0; // the cell
    int y = 0;
    ItemCounts items;
    friend bool operator==(const Drop&, const Drop&) = default;
};

// A room (US-252): enclosed by walls, doors and windows, fully roofed and with a door: warm and sheltered.
struct Room {
    std::vector<std::pair<int, int>> cells; // sorted
    int doors = 0;
    std::vector<int> buildings;             // ids of the buildings whose pieces close it, sorted
};

// What the world around the store says about a moment (weather and time).
struct StepEnv {
    int rainPercent = 0; // 0 to 100: rain slows fire and puts it out
};

class BuildingStore {
public:
    // The map size and a way to ask whether a cell is closed to building (a solid tile, water, a plant or object, a person): the game gives it.
    using Blocked = std::function<bool(int x, int y)>;

    explicit BuildingStore(const BuildingData* data = nullptr, std::uint64_t seed = 1) : data_(data), random_(seed, 11) {}
    void setData(const BuildingData* data) { data_ = data; }
    const BuildingData* data() const { return data_; }
    void reset(int mapWidth, int mapHeight, std::uint64_t seed);
    int mapWidth() const { return width_; }
    int mapHeight() const { return height_; }

    // ---- placing (US-251, US-252)
    // "" when a kind (or a lone piece) could stand there; else why not (shown in the Build menu).
    std::string whyNot(const std::string& kind, int x, int y, int turns, const Blocked& blocked) const;
    struct Placed {
        int id = 0;
        std::string problem;
    };
    // finished: true places it complete (the Editor, rival camps at the start); false places a blueprint.
    Placed place(const std::string& kind, int x, int y, int turns, int faction, bool finished, const Blocked& blocked);
    // The cells a kind would cover from (x, y) after the turns: every layout cell, for the ghost.
    std::vector<std::pair<int, int>> footprintCells(const std::string& kind, int x, int y, int turns) const;
    // The pieces a kind would have from (x, y) after the turns, not built yet (the ghost), and the cells among them that would close walking.
    std::vector<PieceState> layoutPieces(const std::string& kind, int x, int y, int turns) const;
    std::vector<std::pair<int, int>> blockingCells(const std::string& kind, int x, int y, int turns) const;
    // The footprint's size after the turns (1 x 1 for a lone piece, 0 x 0 for an unknown kind).
    std::pair<int, int> sizeOf(const std::string& kind, int turns) const;
    const PlacedBuilding* find(int id) const;
    PlacedBuilding* findMutable(int id);
    const std::vector<PlacedBuilding>& all() const { return buildings_; }
    // The building with a piece in this cell (the wall layer first, then the roof, then the floor), and the index of that piece; -1 when none.
    int buildingAt(int x, int y, int* pieceIndex = nullptr) const;
    // The building whose footprint contains the cell (even where no piece stands in it), or 0.
    int footprintAt(int x, int y) const;
    bool remove(int id);

    // ---- what a building is (from its kind, or the piece)
    std::string label(const PlacedBuilding& building) const;
    ItemCounts cost(const PlacedBuilding& building) const;
    int buildMilli(const PlacedBuilding& building) const;
    std::vector<std::string> uses(const PlacedBuilding& building) const;
    std::string interiorMode(const PlacedBuilding& building) const;      // "fade" or "map", the Editor's override included
    std::string interiorLevelOf(const PlacedBuilding& building) const;
    int condition(const PlacedBuilding& building) const;                 // 0 to 100: the built pieces' hp against their full hp
    bool burning(const PlacedBuilding& building) const;
    // The word the rule language reads as target.state (US-251): waiting (for materials), ready, finished, damaged, burning or rubble.
    std::string ruleState(const PlacedBuilding& building) const;
    // The tags the interactions match: "building", plus "blueprint", "construction", "repairable", "burning", and the uses.
    std::vector<std::string> tags(const PlacedBuilding& building) const;
    std::pair<int, int> centre(const PlacedBuilding& building) const; // the middle cell of the footprint

    // ---- building (US-251)
    // Brings what the site still needs out of `from`; returns what moved.
    ItemCounts deliver(int id, ItemCounts& from);
    void waiveCost(int id); // the site has every material (a hero with no run has no bag)
    bool fullyDelivered(const PlacedBuilding& building) const;
    // Works `milli` thousandths of a second on the site. Work is limited by the share of materials delivered. True when it was finished by this work.
    bool work(int id, int milli);
    // How much work could be done now (the cap from the materials), in thousandths of a second.
    int workAvailable(const PlacedBuilding& building) const;
    // Cancels a blueprint under way: the delivered materials are dropped on the site (Drop) and the building goes. False for a finished building.
    bool cancel(int id);
    const std::vector<Drop>& drops() const { return drops_; }
    ItemCounts takeDropsNear(int x, int y, int radiusCells);

    // ---- wear, damage, fire, repair (US-255)
    // A season ends: each finished building's pieces lose the hp its kind's `wear` says for that season (0 spring .. 3 winter). Never below 1 hp.
    void seasonEnded(int season);
    // A day ends: rubble that lies about long enough is cleared.
    void dayEnded();
    // A hit on the wall-layer piece in a cell (else the roof, else the floor): true when a piece was hit; `fell` is set when it fell to rubble.
    bool damageAt(int x, int y, int amount, bool* fell = nullptr);
    bool damagePiece(int id, std::size_t pieceIndex, int amount, bool* fell = nullptr);
    // Sets a piece alight (false when it is not built, fell, cannot burn or already burns).
    bool igniteAt(int x, int y);
    bool ignitePiece(int id, std::size_t pieceIndex);
    // One tick: burning pieces lose hp and may fall, burn on towards their neighbours once a second, and rain puts them out.
    void advance(const StepEnv& env);
    // Puts out the burning piece nearest (cx, cy) in the building; false when nothing burns there.
    bool douse(int id, int cx, int cy);
    // Heals the most damaged built piece by `hp`; false when nothing is damaged.
    bool repair(int id, int hp);
    bool anyBurning() const;

    // ---- the world the buildings make
    // Bumped whenever a piece is built, falls or is removed: the game redraws its obstacles and the rooms are found again.
    std::uint64_t version() const { return version_; }
    // Cells finished pieces close to walking, with the height of the piece in metres (thousandths).
    struct Obstacle {
        int x = 0;
        int y = 0;
        int heightMilli = 0;
    };
    std::vector<Obstacle> obstacles() const;
    const std::vector<Room>& rooms() const;
    int roomAt(int x, int y) const; // the index into rooms(), or -1
    // A finished piece of this type in the cell.
    bool hasFinishedPiece(int x, int y, PieceType type) const;
    // Warm and sheltered: in a room, or on the footprint of a finished building whose uses include "shelter".
    bool sheltered(int x, int y) const;
    // The finished buildings whose uses include `use`, in id order.
    std::vector<const PlacedBuilding*> withUse(const std::string& use, int faction = -1) const;

    // ---- stores (US-257)
    int deposit(int id, const std::string& item, int count);
    int withdraw(int id, const std::string& item, int count);

    // ---- saving
    std::string toJson() const;
    // Replaces the content with a saved text; false and `problem` when it cannot be read.
    bool fromJson(const std::string& text, std::string& problem);
    std::uint64_t hash() const;
    int nextId() const { return nextId_; }

private:
    struct Occupant {
        int building = 0;
        int piece = -1;
    };
    Occupant& occupant(Layer layer, int x, int y) { return grid_[static_cast<int>(layer)][static_cast<std::size_t>(y * width_ + x)]; }
    const Occupant& occupant(Layer layer, int x, int y) const { return grid_[static_cast<int>(layer)][static_cast<std::size_t>(y * width_ + x)]; }
    bool inside(int x, int y) const { return x >= 0 && y >= 0 && x < width_ && y < height_; }
    void index(PlacedBuilding& building);   // puts a building's pieces on the grid
    void unindex(const PlacedBuilding& building);
    void rebuildGrid();
    void touch() { ++version_; roomsVersion_ = ~0ULL; }
    std::vector<PieceState> makePieces(const std::string& kind, int x, int y, int turns, int& width, int& height, bool finished) const;
    const PieceDef* pieceDef(const PieceState& piece) const { return data_ != nullptr ? data_->piece(piece.piece) : nullptr; }
    void finish(PlacedBuilding& building);
    void restage(PlacedBuilding& building);
    void breakPiece(PlacedBuilding& building, PieceState& piece);
    void updateRubble(PlacedBuilding& building);
    void spreadFire(const StepEnv& env);
    int fullHpMilli(const PieceState& piece) const;

    const BuildingData* data_ = nullptr;
    core::Pcg32 random_;
    int width_ = 0;
    int height_ = 0;
    std::vector<Occupant> grid_[kLayerCount];
    std::vector<PlacedBuilding> buildings_; // sorted by id
    std::vector<Drop> drops_;
    int nextId_ = 1;
    std::uint64_t version_ = 1;
    int fireSecondTicks_ = 0;
    mutable std::vector<Room> rooms_;
    mutable std::uint64_t roomsVersion_ = ~0ULL;
    mutable std::vector<int> roomOfCell_;
    void findRooms() const;
};

} // namespace odysseus::sim::buildings
