#pragma once

#include "boundary.h"

#include "game/placeholder_art.h"
#include "sim/economy.h"
#include "sim/npc_extras.h"
#include "luna/engine/tile_map.h"

#include <filesystem>
#include <optional>
#include <string_view>
#include <utility>
#include <string>
#include <vector>

namespace odysseus::game {

// What kinds of ground and characters exist (US-122), from assets/data/tiles.json and
// characters.json. Levels name them; the order here is the tile number used in memory.
struct TileKindDef {
    std::string name;       // "grass"
    std::string atlas;      // its frame in the art atlas
    bool solid = false;     // blocks walking and spears
};

struct CharacterKindDef {
    std::string name;       // "goblin"
    std::string frames;     // atlas frame, or the frame set of an 8-direction figure
    int directions = 1;     // 8: frames "<frames>.S.0" ...; 1: a single front view "<frames>"
    int hp = 100;
    int swordDamage = 5;
    bool enemy = true;      // the hero's sword can hit it
    double reach = 1.5;     // metres: how far its strike back reaches (US-131)
    double height = 1.7;    // US-244: metres, for its shadow (1.0 for an animal)
    bool shadow = true;     // false: casts no shadow
    bool animal = false;    // a creature of animals.json (US-137): one side-view picture from the content atlas; rames is its name there
    std::vector<std::string> tags; // US-151: "hero", "person" or "hostile"...; "tags" in characters.json replaces them
};

struct Definitions {
    std::vector<TileKindDef> tiles;
    std::vector<CharacterKindDef> characters;
    std::vector<std::string> loopingEffects; // names of the effects of effects.json that loop (US-138): what an Editor may place
    std::vector<std::string> plants;    // names of plants.json (US-136): what a placed plant may be
    std::vector<std::string> objects;   // names of objects.json (US-155): world objects, placed in the Editor like plants
    std::vector<std::string> weapons;   // names a pickup may carry (US-134): the weapons of weapons.json, then the built-in demo weapons
    std::vector<std::string> lightKinds; // names of the kinds of light of lights.json (US-247): what the Editor's Light tool may place
    std::vector<std::string> buildingKinds;  // ids of kinds.json and the prefabs (US-250): what a level may hold finished or as a blueprint
    std::vector<std::string> buildingPieces; // ids of pieces.json: a lone piece may be placed on its own (US-252)

    int tileNumber(const std::string& name) const;                     // -1 when unknown
    const CharacterKindDef* character(const std::string& name) const;  // nullptr when unknown
    bool hasWeapon(const std::string& name) const;
    bool hasPlant(const std::string& name) const;
    bool hasLoopingEffect(const std::string& name) const;
    bool hasLightKind(const std::string& name) const;
    bool hasBuildingKind(const std::string& name) const; // a kind or prefab, or "piece:<id>" of a known piece
};

// The demo weapons of M1b and US-029 (the physics spear throw, the plain sword slash) are not in
// weapons.json; a pickup may still carry them (D-23).
inline constexpr const char* kSpearThrowName = "Spear throw";
inline constexpr const char* kSwordSlashName = "Sword";

Definitions loadDefinitions(const std::filesystem::path& dataDirectory);

struct PixelPoint {
    int x = 0;
    int y = 0;
    friend bool operator==(const PixelPoint&, const PixelPoint&) = default;
};

// A character someone placed in the level. Ids are never reused within a level.
struct PlacedCharacter {
    int id = 0;
    std::string kind;
    PixelPoint feet;        // world pixels
    Facing facing = Facing::South;
    std::string name;
    int hp = 100;
    int swordDamage = 5;
    // What this NPC sets itself over its kind file (US-261); a field left empty is not set and the kind (then the classes) decides. Level version 4.
    std::vector<std::string> classes;  // NPC Classes (US-260): ids of assets/data/npc-classes
    std::string attitude;              // one of the nine attitude words
    std::vector<std::string> tags;     // added to the tags of the kind and the classes
    std::vector<std::pair<std::string, std::string>> dialogues; // partner type -> .dlg file
    std::vector<std::string> allow;    // interaction ids
    std::vector<std::string> deny;
    int family = 0;                    // a family id (0 = none); persons of the same family start with the same-family opinion of each other (US-264)
    sim::rules::NpcExtras extras;      // level version 5: what this NPC trades (US-281), later its schedule and actions; only what it sets itself
    friend bool operator==(const PlacedCharacter&, const PlacedCharacter&) = default;
};

// Version 6 (US-250) adds the buildings of the level (their own list, not plants: CI-008).
// Version 2 (US-134, US-136, US-138) adds weapon pickups, plants and effects; version 3 (US-247) adds placed lights; version 4 (US-260) adds the
// NPC Classes of placed characters; version 5 (US-280, M9b and M9c) adds the region economy (currencies, prices, resources), the NPC trade, schedule and action fields and the places. Older files still load, without them, and are written as version 5 the next time they are saved.
inline constexpr int kLevelVersion = 6;
inline constexpr int kLevelBackups = 3;
inline constexpr int kLevelMinSize = 8;
inline constexpr int kLevelMaxSize = 256;

// A weapon lying in the level for the hero to find (US-134). Ids come from the same counter as
// the characters' and are never reused within a level.
struct PlacedPickup {
    int id = 0;
    std::string weapon;   // a name from weapons.json, or a built-in demo weapon
    PixelPoint at;        // world pixels: the middle of the icon
    friend bool operator==(const PlacedPickup&, const PlacedPickup&) = default;
};

// One value of an interaction changed for one placed thing (US-173, D-56 Q14): only this bush regrows in 60 s. `field` is "duration" or "delay" (the wait of
// every `after` effect), `valueMilli` is in thousandths of a second. Saved in the thing's own `overrides` list; a thing with none writes none.
struct ThingOverride {
    std::string interaction;
    std::string field;
    int valueMilli = 0;
    friend bool operator==(const ThingOverride&, const ThingOverride&) = default;
};

// A plant growing in the level (US-136). Ids come from the same counter as the characters and pickups.
// The feet are the middle of its bottom edge: the cell they are in is the one a big plant blocks.
struct PlacedPlant {
    int id = 0;
    std::string kind;     // a name from plants.json
    PixelPoint feet;      // world pixels
    std::vector<ThingOverride> overrides; // values of interactions changed for this plant (US-173)
    friend bool operator==(const PlacedPlant&, const PlacedPlant&) = default;
};

// A looping effect placed in the level (US-138): fireflies, a campfire, a portal. Same ids as the rest.
struct PlacedEffect {
    int id = 0;
    std::string name;     // a looping effect of effects.json
    PixelPoint at;        // world pixels: the middle of the effect
    friend bool operator==(const PlacedEffect&, const PlacedEffect&) = default;
};

// A point of light placed in the level with the Editor's Light tool (US-247): a kind of lights.json (colour, reach, strength) that shines in
// the game after dark. Same ids as the rest.
struct PlacedLight {
    int id = 0;
    std::string kind;     // a kind of light of lights.json
    PixelPoint at;        // world pixels: where the light hangs
    friend bool operator==(const PlacedLight&, const PlacedLight&) = default;
};

// A building the owner placed in the level with the Editor (US-250, US-256), or a blueprint left standing. Same ids as the rest. Cells are tiles (one metre).
struct PlacedBuildingSpec {
    int id = 0;
    std::string kind;        // a kind or prefab id, or "piece:<id>"
    int x = 0;               // the footprint's top-left cell
    int y = 0;
    int turns = 0;           // quarter turns clockwise
    bool finished = true;    // false: a blueprint waiting for materials
    int owner = -1;          // a clan member id, or -1
    std::string interior;    // "", "fade" or "map": overrides the kind (US-254)
    std::string interiorLevel;
    friend bool operator==(const PlacedBuildingSpec&, const PlacedBuildingSpec&) = default;
};

// A named spot of the level (US-290): where a schedule sends people ("market", "well"), and what the environment interactions use (tags: forage, shelter, water, shrine).
// The name "home" is built in (where an NPC was placed). Level version 5.
struct PlacedPlace {
    std::string name;
    PixelPoint at;                  // world pixels
    std::vector<std::string> tags;
    friend bool operator==(const PlacedPlace&, const PlacedPlace&) = default;
};

// A level: the ground, who stands where, and where the hero begins. Plain data.
struct Level {
    std::string name = "Untitled";
    int width = 0;
    int height = 0;
    int defaultGround = 0;               // tile number
    std::vector<int> ground;             // tile numbers, row after row
    std::vector<PlacedCharacter> characters;
    std::vector<PlacedPickup> pickups;
    std::vector<PlacedPlant> plants;
    std::vector<PlacedEffect> effects;
    std::vector<PlacedLight> lights;     // level version 3 (US-247)
    std::vector<PlacedBuildingSpec> buildings; // level version 6 (US-250): finished buildings and blueprints, in their own list
    sim::RegionEconomy economy;          // level version 5 (US-280): the currencies, market prices and resources of this region; written only when set
    std::vector<PlacedPlace> places;     // level version 5 (US-290): the named spots schedules refer to; written only when there are some
    bool clan = false;                   // the simulated clan lives here (US-032); written only when true
    PixelPoint heroStart;
    std::vector<PixelPoint> targets;     // straw targets of the spear demo (US-029)
    int nextId = 1;

    int at(int x, int y) const { return ground[static_cast<std::size_t>(y * width + x)]; }
    void set(int x, int y, int tile) { ground[static_cast<std::size_t>(y * width + x)] = tile; }
    bool inside(int x, int y) const { return x >= 0 && y >= 0 && x < width && y < height; }
    friend bool operator==(const Level&, const Level&) = default;
};

// The Editor's text of the places of a level (US-290): "market=20,10 grove=30,12/forage/shelter", tile coordinates (the middle of the tile), tags after slashes; and back.
// A mistake (not name=x,y, a name that is no word or is "home" or is used twice, a spot outside the level) comes back as nothing with the reason.
std::string placesText(const std::vector<PlacedPlace>& places);
std::optional<std::vector<PlacedPlace>> parsePlacesText(std::string_view text, const Level& level, std::string& problem);

// A new level filled with one ground.
Level makeLevel(std::string name, int width, int height, int ground);

// The same level at a new size (US-126): painted cells keep their places, new cells get the
// default ground, characters and targets outside are dropped, the hero start moves inside.
Level resized(const Level& level, int width, int height);

// The tile map the game draws and walks on.
luna::engine::TileMap buildTileMap(const Level& level, const Definitions& definitions);

// Reads one level file; every problem is a DataError naming the file and the field.
Level readLevelFile(const std::filesystem::path& file, const Definitions& definitions);

// Reads a level, falling back to its backups (.bak1 .. .bak3) when the file is damaged.
struct LoadedLevel {
    Level level;
    std::filesystem::path loadedFrom;
    std::vector<std::string> notes; // which files were skipped, and why
};
LoadedLevel loadLevel(const std::filesystem::path& file, const Definitions& definitions);

// Saves safely: a temporary file, then a rename; the last three saves are kept as backups.
void saveLevel(const Level& level, const Definitions& definitions, const std::filesystem::path& file);

// "S", "SW", ... as written in level files.
const char* facingCode(Facing facing);

} // namespace odysseus::game
