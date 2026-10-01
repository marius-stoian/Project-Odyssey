#pragma once

#include "boundary.h"

#include <array>
#include <filesystem>
#include <set>
#include <string>
#include <vector>

namespace odysseus::game {

struct ContentAtlas;

// The M2d content catalogs (US-130), read from assets/data/. Each entry names its frame in the
// content atlas (content_art.h); the numbers are first guesses the owner can tune in the JSON.

enum class WeaponClass { Sword, Axe, Spear, Bow, Thrown, Whip, Staff, Gun };
enum class Element { None, Fire, Ice, Lightning, Poison, Void };

struct WeaponDef {
    std::string name;
    std::string frame;
    WeaponClass weaponClass = WeaponClass::Sword;
    Element element = Element::None;
    bool future = false;   // era "future": guns and energy weapons, for later Ages
    bool starter = false;  // playable in M2d (D-21)
    int damage = 5;
    double speed = 1.0;    // attacks per second
    double range = 1.5;    // metres
    std::vector<std::string> tags; // US-151: "item", "weapon", its class, its element, "starter"; "tags" in weapons.json replaces them
};

struct PlantDef {
    std::string name;
    std::string frame;
    std::string size;      // "small", "tall" or "tree"
    bool blocks = false;   // blocks walking like a rock
    bool edible = false;   // heals the hero when destroyed
    std::string inspect;   // shown on Interact
    std::vector<std::string> tags;   // US-151: "plant" always, then "edible", "solid", "tree"...; "tags" in plants.json replaces them
    std::vector<std::string> states; // US-151: the first is where it starts ("ripe", "picked"); none for a plant that never changes
};

struct AnimalDef {
    std::string name;
    std::string frame;
    int hp = 80;
    bool enemy = false;    // can be hit and strikes back (D-21: predators and boars)
    int strikeDamage = 0;
    double reach = 1.5;    // metres
    std::vector<std::string> tags; // US-151: "animal" and "hostile" or "prey"; "tags" in animals.json replaces them
};

struct EffectDef {
    std::string name;
    int frames = 1;
    int ticksPerFrame = 3;
    bool loop = false;
};

struct WeatherDef {
    std::string name;
    int frames = 4;        // 0: nothing is drawn ("clear")
    int ticksPerFrame = 4;
    int weight = 1;        // how often it is picked, against the others
    bool additive = true;  // light weather adds light; fog and clouds are drawn see-through
};

// What an element does (US-135), from weapons.json under "elements". Only the fields an element uses are
// set: fire and poison drip damage, ice slows, lightning chains, void drains. Effect names are effects.json entries.
struct ElementDef {
    double perSecond = 0.0;     // fire, poison: HP lost per second while it lasts
    double seconds = 0.0;       // fire, poison, ice: how long it lasts
    double slowTo = 1.0;        // ice: speed left (0.5 = half speed)
    double chainMetres = 0.0;   // lightning: how far it jumps
    double chainFraction = 0.0; // lightning: the jump does this part of the damage
    double drainFraction = 0.0; // void: this part of the damage heals the hero
    std::string effect;         // shown on the target while the status lasts
    std::string hitEffect;      // shown at the moment of the hit (lightning: at the target it jumps to)
    std::string healEffect;     // void: shown on the hero when healed
};

// What a weapon class shoots with (US-141), from weapons.json under "classes": how fast its shots leave, metres per second.
struct ClassDef {
    double launchSpeed = 0.0; // 0: the class shoots nothing (melee)
};

struct Catalogs {
    std::vector<WeaponDef> weapons;
    std::vector<PlantDef> plants;
    std::vector<AnimalDef> animals;
    std::vector<EffectDef> effects;
    std::vector<WeatherDef> weather;
    std::array<ClassDef, 8> classes{};    // by WeaponClass
    std::array<ElementDef, 6> elements{}; // by Element; "none" does nothing

    const WeaponDef* weapon(const std::string& name) const;
    const PlantDef* plant(const std::string& name) const;
    const AnimalDef* animal(const std::string& name) const;
    const EffectDef* effect(const std::string& name) const;
    // Every tag any catalog entry carries: what an interaction may target (a tag outside this set earns a warning).
    std::set<std::string> knownTags() const;
    const ClassDef& weaponClass(WeaponClass weaponClass) const { return classes.at(static_cast<std::size_t>(weaponClass)); }
    const ElementDef& element(Element element) const { return elements.at(static_cast<std::size_t>(element)); }
};

// Reads weapons.json, plants.json, animals.json, effects.json and weather.json. Every problem
// is a DataError naming the file and the field. With an atlas, every frame must exist in it.
Catalogs loadCatalogs(const std::filesystem::path& dataDirectory, const ContentAtlas* atlas = nullptr);

const char* weaponClassName(WeaponClass weaponClass);
const char* elementName(Element element);

} // namespace odysseus::game
