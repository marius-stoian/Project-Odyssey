#include "game/catalogs.h"

#include "game/content_art.h"
#include "game/tags.h"
#include "sim/data.h"
#include "sim/json_data.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <format>
#include <set>

namespace odysseus::game {

using nlohmann::json;

namespace {

constexpr std::array<const char*, 8> kClassNames{"sword", "axe", "spear", "bow", "thrown", "whip", "staff", "gun"};
constexpr std::array<const char*, 6> kElementNames{"none", "fire", "ice", "lightning", "poison", "void"};

// Reads one catalog file: the list under `key`, each entry checked by `read`, names unique.
template <typename Def, typename Read>
std::vector<Def> readList(const std::filesystem::path& file, const std::string& key, Read read) {
    const json data = sim::readJsonFile(file);
    if (!data.contains(key) || !data.at(key).is_array()) {
        throw sim::DataError(file, key, "must be a list");
    }
    std::vector<Def> list;
    std::set<std::string> names;
    for (std::size_t i = 0; i < data.at(key).size(); ++i) {
        const json& entry = data.at(key).at(i);
        const std::string where = std::format("{}[{}]", key, i);
        if (!entry.is_object() || !entry.contains("name") || !entry.at("name").is_string() || entry.at("name").get<std::string>().empty()) {
            throw sim::DataError(file, where + ".name", "must be a name in quotes");
        }
        Def def = read(entry, where);
        def.name = entry.at("name").get<std::string>();
        if (!names.insert(def.name).second) {
            throw sim::DataError(file, where + ".name", "\"" + def.name + "\" is listed twice");
        }
        list.push_back(std::move(def));
    }
    return list;
}

struct Fields {
    const std::filesystem::path& file;
    const json& entry;
    const std::string& where;

    std::string text(const std::string& field) const {
        if (!entry.contains(field) || !entry.at(field).is_string()) throw sim::DataError(file, where + "." + field, "must be text in quotes");
        return entry.at(field).get<std::string>();
    }
    bool flag(const std::string& field) const {
        if (!entry.contains(field) || !entry.at(field).is_boolean()) throw sim::DataError(file, where + "." + field, "must be true or false");
        return entry.at(field).get<bool>();
    }
    int whole(const std::string& field, int low, int high) const {
        if (!entry.contains(field) || !entry.at(field).is_number_integer() || entry.at(field).get<int>() < low || entry.at(field).get<int>() > high) {
            throw sim::DataError(file, where + "." + field, std::format("must be a whole number from {} to {}", low, high));
        }
        return entry.at(field).get<int>();
    }
    double number(const std::string& field, double low, double high) const {
        if (!entry.contains(field) || !entry.at(field).is_number() || entry.at(field).get<double>() < low || entry.at(field).get<double>() > high) {
            throw sim::DataError(file, where + "." + field, std::format("must be a number from {} to {}", low, high));
        }
        return entry.at(field).get<double>();
    }
    template <std::size_t N>
    int choice(const std::string& field, const std::array<const char*, N>& options) const {
        const std::string value = text(field);
        for (std::size_t i = 0; i < N; ++i) {
            if (value == options[i]) return static_cast<int>(i);
        }
        std::string list;
        for (const char* option : options) list += std::string(list.empty() ? "" : ", ") + "\"" + option + "\"";
        throw sim::DataError(file, where + "." + field, "must be one of " + list);
    }
};

template <typename Def>
const Def* byName(const std::vector<Def>& list, const std::string& name) {
    const auto it = std::find_if(list.begin(), list.end(), [&](const Def& def) { return def.name == name; });
    return it == list.end() ? nullptr : &*it;
}

} // namespace

const char* weaponClassName(WeaponClass weaponClass) { return kClassNames.at(static_cast<std::size_t>(weaponClass)); }
const char* elementName(Element element) { return kElementNames.at(static_cast<std::size_t>(element)); }

const WeaponDef* Catalogs::weapon(const std::string& name) const { return byName(weapons, name); }
const PlantDef* Catalogs::plant(const std::string& name) const { return byName(plants, name); }
const AnimalDef* Catalogs::animal(const std::string& name) const { return byName(animals, name); }
const EffectDef* Catalogs::effect(const std::string& name) const { return byName(effects, name); }

std::set<std::string> Catalogs::knownTags() const {
    std::set<std::string> tags;
    for (const WeaponDef& weapon : weapons) tags.insert(weapon.tags.begin(), weapon.tags.end());
    for (const PlantDef& plant : plants) tags.insert(plant.tags.begin(), plant.tags.end());
    for (const AnimalDef& animal : animals) tags.insert(animal.tags.begin(), animal.tags.end());
    return tags;
}

Catalogs loadCatalogs(const std::filesystem::path& dataDirectory, const ContentAtlas* atlas) {
    Catalogs catalogs;
    // A frame named in a catalog must exist in the atlas (when one is given to check against).
    auto checkFrame = [&](const std::filesystem::path& file, const std::string& where, const std::string& frame) {
        if (atlas != nullptr && !atlas->frameCounts.contains(frame)) {
            throw sim::DataError(file, where + ".frame", "\"" + frame + "\" is not in the content atlas");
        }
    };

    const auto weaponsFile = dataDirectory / "weapons.json";
    catalogs.weapons = readList<WeaponDef>(weaponsFile, "weapons", [&](const json& entry, const std::string& where) {
        const Fields f{weaponsFile, entry, where};
        WeaponDef def;
        def.frame = f.text("frame");
        checkFrame(weaponsFile, where, def.frame);
        def.weaponClass = static_cast<WeaponClass>(f.choice("class", kClassNames));
        def.element = static_cast<Element>(f.choice("element", kElementNames));
        def.light = entry.value("light", std::string());
        def.future = f.choice("era", std::array<const char*, 2>{"fantasy", "future"}) == 1;
        def.starter = f.flag("starter");
        def.damage = f.whole("damage", 0, 1000);
        def.speed = f.number("speed", 0.1, 20.0);
        def.range = f.number("range", 0.5, 50.0);
        std::vector<std::string> derived{"item", "weapon", kClassNames.at(static_cast<std::size_t>(def.weaponClass))};
        if (def.element != Element::None) derived.push_back(kElementNames.at(static_cast<std::size_t>(def.element)));
        if (def.starter) derived.push_back("starter");
        def.tags = readTags(entry, weaponsFile, where, std::move(derived));
        return def;
    });

    const auto plantsFile = dataDirectory / "plants.json";
    catalogs.plants = readList<PlantDef>(plantsFile, "plants", [&](const json& entry, const std::string& where) {
        const Fields f{plantsFile, entry, where};
        PlantDef def;
        def.frame = f.text("frame");
        checkFrame(plantsFile, where, def.frame);
        constexpr std::array<const char*, 3> kSizes{"small", "tall", "tree"};
        def.size = kSizes[static_cast<std::size_t>(f.choice("size", kSizes))];
        def.blocks = f.flag("blocks");
        def.edible = f.flag("edible");
        def.inspect = f.text("inspect");
        // Before tags existed: an edible plant that can be walked through is food to gather; a solid one that bears fruit is not
        // gathered (it is chopped), so it does not get the "edible" tag by default.
        std::vector<std::string> derived{"plant"};
        if (def.edible) derived.push_back(def.blocks ? "fruit-bearing" : "edible");
        if (def.blocks) derived.push_back("solid");
        if (def.size == "tree") derived.push_back("tree");
        def.tags = readTags(entry, plantsFile, where, std::move(derived));
        def.states = readStates(entry, plantsFile, where, (def.edible && !def.blocks) ? std::vector<std::string>{"ripe", "picked"} : std::vector<std::string>{});
        return def;
    });

    // World objects (US-155): placed like plants and kept in the same list, flagged `object`; the game draws them itself, so no atlas frame is
    // needed ("frame" names the picture the game draws). Tags and states are written in the file (an object with none has only "object").
    const auto objectsFile = dataDirectory / "objects.json";
    if (std::filesystem::exists(objectsFile)) {
        const std::vector<PlantDef> objects = readList<PlantDef>(objectsFile, "objects", [&](const json& entry, const std::string& where) {
            const Fields f{objectsFile, entry, where};
            PlantDef def;
            def.frame = f.text("frame");
            def.size = "small";
            def.blocks = f.flag("blocks");
            def.edible = false;
            def.object = true;
            def.inspect = f.text("inspect");
            def.tags = readTags(entry, objectsFile, where, {"object"});
            def.states = readStates(entry, objectsFile, where, {});
            def.light = entry.value("light", std::string());
            def.lightState = entry.value("lightState", std::string());
            return def;
        });
        for (const PlantDef& object : objects) {
            if (catalogs.plant(object.name) != nullptr) throw sim::DataError(objectsFile, "objects", "\"" + object.name + "\" is already a plant in plants.json");
            catalogs.plants.push_back(object);
        }
    }

    const auto animalsFile = dataDirectory / "animals.json";
    catalogs.animals = readList<AnimalDef>(animalsFile, "animals", [&](const json& entry, const std::string& where) {
        const Fields f{animalsFile, entry, where};
        AnimalDef def;
        def.frame = f.text("frame");
        checkFrame(animalsFile, where, def.frame);
        def.hp = f.whole("hp", 1, 10000);
        def.enemy = f.flag("enemy");
        def.strikeDamage = f.whole("strikeDamage", 0, 1000);
        def.reach = f.number("reach", 0.5, 10.0);
        def.tags = readTags(entry, animalsFile, where, {"animal", def.enemy ? "hostile" : "prey"});
        return def;
    });

    const auto effectsFile = dataDirectory / "effects.json";
    catalogs.effects = readList<EffectDef>(effectsFile, "effects", [&](const json& entry, const std::string& where) {
        const Fields f{effectsFile, entry, where};
        EffectDef def;
        def.frames = f.whole("frames", 1, 16);
        def.ticksPerFrame = f.whole("ticksPerFrame", 1, 60);
        def.loop = f.flag("loop");
        def.light = entry.value("light", std::string());
        return def;
    });
    if (atlas != nullptr) {
        for (std::size_t i = 0; i < catalogs.effects.size(); ++i) {
            const auto count = atlas->frameCounts.find(catalogs.effects[i].name);
            if (count == atlas->frameCounts.end() || count->second != catalogs.effects[i].frames) {
                throw sim::DataError(effectsFile, std::format("effects[{}].frames", i), "must match the frames cut for \"" + catalogs.effects[i].name + "\"");
            }
        }
    }


    // Launch speeds of the classes that shoot (US-141): bow (also crossbows), thrown, staff and gun.
    {
        const json data = sim::readJsonFile(weaponsFile);
        if (!data.contains("classes") || !data.at("classes").is_object()) {
            throw sim::DataError(weaponsFile, "classes", "must list bow, thrown, staff and gun with their launchSpeed");
        }
        for (const char* name : {"bow", "thrown", "staff", "gun"}) {
            const std::string where = std::string("classes.") + name;
            if (!data.at("classes").contains(name) || !data.at("classes").at(name).is_object()) {
                throw sim::DataError(weaponsFile, where, "is missing");
            }
            const Fields f{weaponsFile, data.at("classes").at(name), where};
            for (std::size_t i = 0; i < kClassNames.size(); ++i) {
                if (std::string(kClassNames[i]) == name) catalogs.classes[i].launchSpeed = f.number("launchSpeed", 1.0, 100.0);
            }
        }
    }

    // Element numbers (US-135): weapons.json "elements" has an entry per element; unused fields stay off.
    {
        const json data = sim::readJsonFile(weaponsFile);
        if (!data.contains("elements") || !data.at("elements").is_object()) {
            throw sim::DataError(weaponsFile, "elements", "must list fire, ice, lightning, poison and void");
        }
        for (std::size_t i = 1; i < kElementNames.size(); ++i) {
            const std::string where = std::string("elements.") + kElementNames[i];
            if (!data.at("elements").contains(kElementNames[i]) || !data.at("elements").at(kElementNames[i]).is_object()) {
                throw sim::DataError(weaponsFile, where, "is missing");
            }
            const json& entry = data.at("elements").at(kElementNames[i]);
            const Fields f{weaponsFile, entry, where};
            ElementDef& def = catalogs.elements[i];
            const auto maybeNumber = [&](const char* field, double low, double high, double fallback) {
                return entry.contains(field) ? f.number(field, low, high) : fallback;
            };
            const auto maybeEffect = [&](const char* field) {
                if (!entry.contains(field)) return std::string();
                const std::string name = f.text(field);
                if (catalogs.effect(name) == nullptr) throw sim::DataError(weaponsFile, where + "." + field, "\"" + name + "\" is not an effect in effects.json");
                return name;
            };
            def.perSecond = maybeNumber("perSecond", 0.0, 1000.0, 0.0);
            def.seconds = maybeNumber("seconds", 0.0, 600.0, 0.0);
            def.slowTo = maybeNumber("slowTo", 0.05, 1.0, 1.0);
            def.chainMetres = maybeNumber("chainMetres", 0.0, 50.0, 0.0);
            def.chainFraction = maybeNumber("chainFraction", 0.0, 10.0, 0.0);
            def.drainFraction = maybeNumber("drainFraction", 0.0, 1.0, 0.0);
            def.effect = maybeEffect("effect");
            def.hitEffect = maybeEffect("hitEffect");
            def.healEffect = maybeEffect("healEffect");
        }
    }
    const auto weatherFile = dataDirectory / "weather.json";
    catalogs.weather = readList<WeatherDef>(weatherFile, "weather", [&](const json& entry, const std::string& where) {
        const Fields f{weatherFile, entry, where};
        WeatherDef def;
        def.frames = f.whole("frames", 0, 16);
        def.ticksPerFrame = f.whole("ticksPerFrame", 1, 60);
        def.weight = f.whole("weight", 0, 1000);
        def.additive = f.choice("blend", std::array<const char*, 2>{"alpha", "add"}) == 1;
        return def;
    });
    return catalogs;
}

} // namespace odysseus::game
