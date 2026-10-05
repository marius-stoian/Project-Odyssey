#pragma once

#include "boundary.h"

#include "sim/economy.h"
#include "sim/interaction.h"

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace odysseus::sim::buildings {

// Buildings as data (US-250, D-42, D-55): pieces and kinds, read from assets/data/buildings/. Everything is whole numbers: a cell is one tile
// (one metre), time is in thousandths of a second, hp is whole points. docs/guides/building-data.md describes every field.

// Which of the three things a cell can hold at once a piece is: a floor under, a wall (or door, window, post, fence) standing, a roof over.
enum class Layer { Floor, Wall, Roof };
inline constexpr int kLayerCount = 3;

enum class PieceType { Wall, Floor, Roof, Door, Window, Post, Fence };

const char* pieceTypeName(PieceType type);
std::optional<PieceType> pieceTypeFromName(std::string_view name);
Layer layerOf(PieceType type);
// A finished piece of these types shuts its cell to walking: a wall, a post, a fence (a door is a way through, a window is a gap, floors and roofs lie flat).
bool blocksWalking(PieceType type);
// The types that close a room's boundary in the flood fill: wall, door, window and fence (a post only holds a roof up).
bool closesRoom(PieceType type);

// How a material burns (the "materials" table of pieces.json).
struct MaterialFire {
    int flammability = 0; // percent chance per second that a burning neighbour sets it alight
    int burnSeconds = 0;  // how long a piece of it burns before it falls to rubble (when it has hp left)
};

struct PieceDef {
    std::string id;       // "wall-wood"
    std::string label;    // "Wooden wall"
    PieceType type = PieceType::Wall;
    std::string material; // a key of the materials table
    int hp = 100;
    int buildMilli = 3000;
    ItemCounts cost;      // item id -> count
    int colour = 0x8B6B3E; // 0xRRGGBB: the placeholder art
    int heightMilli = 2000; // metres in thousandths: shadows and shots
    friend bool operator==(const PieceDef&, const PieceDef&) = default;
};

// A piece of a kind's layout, at a cell offset from the footprint's top-left corner (before any turn).
struct LayoutPiece {
    std::string piece;
    int x = 0;
    int y = 0;
    friend bool operator==(const LayoutPiece&, const LayoutPiece&) = default;
};

enum class InteriorMode { Fade, Map };
const char* interiorModeName(InteriorMode mode);
std::optional<InteriorMode> interiorModeFromName(std::string_view name);

// What the building does for the clan: shelter and warmth, a place to sleep, storage, work.
inline constexpr const char* kUses[] = {"shelter", "sleep", "store", "work"};
bool isUse(std::string_view word);

struct KindDef {
    std::string id;        // "hut"
    std::string label;     // "Hut"
    int width = 1;         // footprint in cells
    int height = 1;
    std::vector<LayoutPiece> layout;
    ItemCounts cost;       // what is brought to the site; the sum of the pieces unless the file says
    int buildMilli = 0;    // the work needed; the sum of the pieces unless the file says
    InteriorMode interior = InteriorMode::Fade;
    std::string interiorLevel; // the level an interior map is read from (mode Map)
    std::vector<std::string> uses;
    std::string light;     // a kind of lights.json that shines from a finished building at night ("" = none)
    std::string owner = "clan"; // none, clan or person
    std::map<std::string, int> wear; // season name (lower case) -> hp each piece loses when that season ends
    bool buildable = true; // offered in the Build menu once known
    bool known = false;    // the hero knows the blueprint at the start of a run
    int capacity = 0;      // storage: meals of the clan's store kept at half the spoilage (US-257)
    int colour = 0x8B6B3E; // the Build menu's icon
    std::string note;
    bool prefab = false;   // read from prefabs/<id>.json (the Editor's Prefab tab writes these)
    std::string file;      // "buildings/kinds.json" or "buildings/prefabs/lodge.json"
    friend bool operator==(const KindDef&, const KindDef&) = default;
};

// The text name of the seasons used in `wear`.
inline constexpr const char* kSeasonWords[] = {"spring", "summer", "autumn", "winter"};

class BuildingData {
public:
    // Reads pieces.json, kinds.json and prefabs/*.json from `folder` (assets/data/buildings). A file with mistakes is left out and its mistakes are
    // listed as "file:line: message" in the report; a missing folder is no error (no buildings). `knownItems`: when not empty, a cost naming any other item is an error.
    static BuildingData load(const std::filesystem::path& folder, rules::LoadReport& report, const std::set<std::string>& knownItems = {});
    // From text, for tests and the Editor; `name` is how errors name the file.
    static BuildingData parse(std::string_view piecesText, std::string_view kindsText, rules::LoadReport& report);
    // One kind or prefab from text, checked against the pieces already loaded.
    std::optional<KindDef> parseKind(std::string_view text, const std::string& name, rules::LoadReport& report, bool prefab, const std::string& expectedId = {}) const;

    const std::vector<PieceDef>& pieces() const { return pieces_; }
    const std::vector<KindDef>& kinds() const { return kinds_; }
    const PieceDef* piece(std::string_view id) const;
    const KindDef* kind(std::string_view id) const;
    const MaterialFire* material(std::string_view name) const;
    int maxRoomCells() const { return maxRoomCells_; }
    // The kinds a new run's hero already knows, and the ones offered in the Build menu (known or not).
    std::vector<const KindDef*> buildableKinds() const;
    // Adds a kind (a prefab the Editor just saved); replaces one of the same id.
    void addKind(KindDef kind);

private:
    std::vector<PieceDef> pieces_;   // sorted by id
    std::vector<KindDef> kinds_;     // kinds.json in file order, then the prefabs by id
    std::map<std::string, MaterialFire> materials_;
    int maxRoomCells_ = 64;
};

// Canonical text of a prefab (a kind) as the Editor saves it; parseKind gives the same kind back.
std::string toJson(const KindDef& kind);

// The cell of an offset in a footprint after `turns` quarter turns clockwise (0 to 3). The footprint is width x height before the turn; after an odd number of
// turns it is height x width.
struct Cell {
    int x = 0;
    int y = 0;
    friend bool operator==(const Cell&, const Cell&) = default;
};
Cell turnedOffset(int x, int y, int width, int height, int turns);
// The footprint's size after the turns.
Cell turnedSize(int width, int height, int turns);
// The sum of the costs of the layout's pieces, and of their build times.
ItemCounts layoutCost(const BuildingData& data, const std::vector<LayoutPiece>& layout);
int layoutBuildMilli(const BuildingData& data, const std::vector<LayoutPiece>& layout);

} // namespace odysseus::sim::buildings
