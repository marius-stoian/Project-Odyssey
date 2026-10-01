#pragma once

#include "boundary.h"

#include "game/placeholder_art.h"
#include "luna/engine/tile_map.h"

#include <filesystem>
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

    int tileNumber(const std::string& name) const;                     // -1 when unknown
    const CharacterKindDef* character(const std::string& name) const;  // nullptr when unknown
    bool hasWeapon(const std::string& name) const;
    bool hasPlant(const std::string& name) const;
    bool hasLoopingEffect(const std::string& name) const;
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
    friend bool operator==(const PlacedCharacter&, const PlacedCharacter&) = default;
};

// Version 2 (US-134, US-136) adds weapon pickups and plants; version 1 files still load, without any.
inline constexpr int kLevelVersion = 2;
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

// A plant growing in the level (US-136). Ids come from the same counter as the characters and pickups.
// The feet are the middle of its bottom edge: the cell they are in is the one a big plant blocks.
struct PlacedPlant {
    int id = 0;
    std::string kind;     // a name from plants.json
    PixelPoint feet;      // world pixels
    friend bool operator==(const PlacedPlant&, const PlacedPlant&) = default;
};

// A looping effect placed in the level (US-138): fireflies, a campfire, a portal. Same ids as the rest.
struct PlacedEffect {
    int id = 0;
    std::string name;     // a looping effect of effects.json
    PixelPoint at;        // world pixels: the middle of the effect
    friend bool operator==(const PlacedEffect&, const PlacedEffect&) = default;
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
    bool clan = false;                   // the simulated clan lives here (US-032); written only when true
    PixelPoint heroStart;
    std::vector<PixelPoint> targets;     // straw targets of the spear demo (US-029)
    int nextId = 1;

    int at(int x, int y) const { return ground[static_cast<std::size_t>(y * width + x)]; }
    void set(int x, int y, int tile) { ground[static_cast<std::size_t>(y * width + x)] = tile; }
    bool inside(int x, int y) const { return x >= 0 && y >= 0 && x < width && y < height; }
    friend bool operator==(const Level&, const Level&) = default;
};

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
