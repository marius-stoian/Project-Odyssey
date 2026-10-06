// US-303 Live reload of every data file: the data sets of the game, what follows each of them, and what the owner sees (a toast, the mistakes panel,
// a red "?" where a placed thing lost its kind). The registry itself is data_reload.*; this file is the game's part.
#include "game/odyssey_game.h"

#include "core/log.h"
#include "game/object_art.h"
#include "luna/engine/image_ops.h"
#include "sim/data.h"

#include <algorithm>
#include <format>
#include <set>

namespace odysseus::game {

namespace {


std::string fileNameOf(const std::filesystem::path& file) { return file.filename().string(); }

} // namespace

// ---- The rules the constructor and the reload share

// Every `light` of the catalogs must name a kind of light of lights.json (US-243).
void OdysseyGame::checkCatalogLights(const Catalogs& catalogs, const LightingData& lighting, const std::filesystem::path& dataDirectory) {
    const auto check = [&](const std::string& light, const std::string& file, const std::string& where) {
        if (!light.empty() && lighting.kind(light) == nullptr) {
            throw sim::DataError(dataDirectory / file, where + ".light", "\"" + light + "\" is not a kind of light in light/lights.json");
        }
    };
    for (const EffectDef& def : catalogs.effects) check(def.light, "effects.json", "effect \"" + def.name + "\"");
    for (const WeaponDef& def : catalogs.weapons) check(def.light, "weapons.json", "weapon \"" + def.name + "\"");
    for (const PlantDef& def : catalogs.plants) check(def.light, def.object ? "objects.json" : "plants.json", "object \"" + def.name + "\"");
}

// The sun and moon (US-248): each names a kind of light of lights.json. A mistake in one of them, or in the eclipse file, does not stop the game: the bad
// entry is left out, the level keeps its default pair, and the problem (file and field) is shown as a message. Returns those messages.
std::vector<std::string> OdysseyGame::addCelestialDefaults(Catalogs& catalogs, const LightingData& lighting, const std::filesystem::path& dataDirectory) {
    std::vector<std::string> notes = catalogs.notes;
    std::erase_if(catalogs.plants, [&](const PlantDef& def) {
        if (!def.celestial || lighting.kind(def.sky.lightKind) != nullptr) return false;
        notes.push_back(sim::DataError(dataDirectory / "objects.json", "object \"" + def.name + "\".celestial.light",
                                       "\"" + def.sky.lightKind + "\" is not a kind of light in light/lights.json").what());
        return true;
    });
    for (const char* body : {"sun", "moon"}) {
        const bool has = std::any_of(catalogs.plants.begin(), catalogs.plants.end(), [&](const PlantDef& def) { return def.celestial && def.sky.followsClock && def.sky.body == body; });
        if (has) continue;
        PlantDef fallback; // the built-in default: a plain sun or moon on the usual orbit
        fallback.name = body;
        fallback.frame = body;
        fallback.object = true;
        fallback.celestial = true;
        fallback.tags = {"object", "celestial"};
        fallback.sky.body = body;
        catalogs.plants.push_back(fallback);
    }
    return notes;
}

// ---- The data sets

void OdysseyGame::registerDataSets() {
    const std::filesystem::path& data = dataDirectory_;
    reloads_.add({"interactions", {data / "interactions", data / "dialogue", data / "quests"}, [this] { return reloadInteractionSet(); }});
    reloads_.add({"npc-classes", {data / "npc-classes", data / "npcs", data / "sim" / "partner-types.json"}, [this] { return reloadNpcClassSet(); }});
    reloads_.add({"lights", {data / "light" / "lights.json"}, [this] { return reloadLights(); }});
    reloads_.add({"catalog",
                  {data / "plants.json", data / "objects.json", data / "characters.json", data / "weapons.json", data / "animals.json", data / "effects.json", data / "weather.json"},
                  [this] { return reloadCatalog(); }});
    reloads_.add({"help", {data / "editor" / "help.json"}, [this] { return reloadHelp(); }});
    // What cannot be swapped while a run is loaded (a tile is a number in the map, the hero and sim data and the buildings are held by the play state): the
    // change is reported, never half-applied, and takes effect at the next start. F5 leaves these alone. The catalogs of weapons, animals, effects and weather
    // moved out of this set in US-191: what holds them keeps a name and is pointed at the new definition by it (CI-007, CI-021).
    DataSet later{"next-start",
                  {data / "tiles.json", data / "materials.json", data / "buildings",
                   data / "hero", data / "sim", data / "light" / "sky.json", data / "light" / "celestial-events.json", data / "story"},
                  [] {
                      ReloadResult result;
                      result.atNextStart = true;
                      return result;
                  },
                  true};
    reloads_.add(std::move(later));
}

ReloadResult OdysseyGame::reloadInteractionSet() {
    ReloadResult result;
    result.ok = reloadInteractions(); // interactions, dialogues, small talk and quests together, all or nothing (US-156)
    for (const sim::rules::Diagnostic& d : interactionReport_.errors) result.errors.push_back(d.text());
    if (result.ok) reloadPartnerDefaults(); // defaults-<type>.json lives in the same folder (US-293)
    return result;
}

ReloadResult OdysseyGame::reloadNpcClassSet() {
    ReloadResult result;
    result.ok = npcClasses_.reload(); // all or nothing: a class file with a mistake keeps the last good classes in use (US-260)
    if (result.ok) {
        editor_.classesChanged(); // the Editor's class lists
        refreshTraders();         // the trade profiles that came with them (US-281)
        reloadPartnerDefaults();  // the default actions of the partner types (US-293)
        refreshLife();            // the schedules (US-290)
    } else {
        for (const sim::rules::Diagnostic& d : npcClasses_.report().errors) result.errors.push_back(d.text());
    }
    return result;
}

ReloadResult OdysseyGame::reloadLights() {
    ReloadResult result;
    try {
        LightingData fresh = loadLighting(dataDirectory_ / "light" / "lights.json");
        // The catalogs and the sun and moon name kinds of light: a file that drops one of them is a mistake, the old kinds stay.
        checkCatalogLights(catalogs_, fresh, dataDirectory_);
        for (const PlantDef& def : catalogs_.plants) {
            if (def.celestial && fresh.kind(def.sky.lightKind) == nullptr) {
                throw sim::DataError(dataDirectory_ / "light" / "lights.json", "kind \"" + def.sky.lightKind + "\"", "is gone, and object \"" + def.name + "\" shines with it");
            }
        }
        lighting_ = std::move(fresh); // placed lights and objects look their kind up each frame: they shine in the new colour at once
        definitions_.lightKinds.clear();
        for (const LightKindDef& kind : lighting_.kinds) definitions_.lightKinds.push_back(kind.name);
        editor_.dataChanged(); // the Light palette
        for (const std::string& note : findMissingKinds()) result.warnings.push_back(note);
    } catch (const std::exception& problem) {
        result.ok = false;
        result.errors.push_back(problem.what());
    }
    return result;
}

ReloadResult OdysseyGame::reloadCatalog() {
    ReloadResult result;
    try {
        Definitions fresh = loadDefinitions(dataDirectory_);                // the names of the plants, objects and characters
        Catalogs catalogs = loadCatalogs(dataDirectory_);                   // read whole, so a mistake anywhere keeps all the old data
        checkCatalogLights(catalogs, lighting_, dataDirectory_);
        const std::vector<std::string> notes = addCelestialDefaults(catalogs, lighting_, dataDirectory_);
        applyCatalog(std::move(fresh), std::move(catalogs));
        result.warnings.insert(result.warnings.end(), notes.begin(), notes.end());
        for (const std::string& note : findMissingKinds()) result.warnings.push_back(note);
    } catch (const std::exception& problem) {
        result.ok = false;
        result.errors.push_back(problem.what());
    }
    return result;
}

// Takes a clean copy of the catalogs into use: plants and objects, weapons, animals, effects and weather, and the character kinds. What the play state holds
// of a definition is held by the kind's name and pointed at the new definition by it, never by an address that is about to go (CI-007, CI-021): the plants
// of the level, the shots in the air, the starter weapons, the weather that is under way. A thing whose kind is gone is dropped (a shot) or skipped and marked
// with a red "?" (a placed plant). The hotbar, the effects, the lights and the animals of the level already hold names and look the definition up each time.
void OdysseyGame::applyCatalog(Definitions fresh, Catalogs catalogs) {
    // What is in the air names its weapon now, before the old definitions go.
    std::vector<std::string> arcNames;
    for (const ArcShot& shot : arcShots_) arcNames.push_back(shot.weapon != nullptr ? shot.weapon->name : std::string());
    std::vector<std::string> projectileNames;
    for (const Projectile& shot : projectiles_) projectileNames.push_back(shot.weapon != nullptr ? shot.weapon->name : std::string());
    const int weatherNow = weather_.current();
    const std::string weatherName = weatherNow >= 0 && weatherNow < static_cast<int>(catalogs_.weather.size()) ? catalogs_.weather[static_cast<std::size_t>(weatherNow)].name : std::string();

    catalogs_.weapons = std::move(catalogs.weapons);
    catalogs_.animals = std::move(catalogs.animals);
    catalogs_.effects = std::move(catalogs.effects);
    catalogs_.weather = std::move(catalogs.weather);
    catalogs_.classes = catalogs.classes;
    catalogs_.elements = catalogs.elements;
    for (std::size_t i = 0; i < arcShots_.size(); ++i) arcShots_[i].weapon = arcNames[i].empty() ? nullptr : catalogs_.weapon(arcNames[i]);
    std::erase_if(arcShots_, [](const ArcShot& shot) { return shot.weapon == nullptr; }); // a weapon that is gone takes its shots with it
    for (std::size_t i = 0; i < projectiles_.size(); ++i) projectiles_[i].weapon = projectileNames[i].empty() ? nullptr : catalogs_.weapon(projectileNames[i]);
    std::erase_if(projectiles_, [](const Projectile& shot) { return shot.weapon == nullptr; });
    rebuildWeaponLists();
    rebuildCatalogArt();
    if (!weatherName.empty()) { // the cycle goes on under the weather it was in, when that weather is still there
        weather_ = WeatherCycle(catalogs_.weather, weatherSeed_);
        for (std::size_t i = 0; i < catalogs_.weather.size(); ++i) {
            if (catalogs_.weather[i].name == weatherName) weather_.force(static_cast<int>(i));
        }
    }

    std::vector<double> heightBefore(plants_.size(), 0.0);
    for (std::size_t i = 0; i < plants_.size(); ++i) {
        if (plants_[i].alive && plants_[i].def != nullptr) heightBefore[i] = plantObstacleHeight(*plants_[i].def);
    }
    catalogs_.plants = std::move(catalogs.plants);
    defaultBodies_.clear();
    for (const PlantDef& def : catalogs_.plants) {
        if (def.celestial && def.sky.followsClock) defaultBodies_.push_back(&def);
    }
    for (std::size_t i = 0; i < plants_.size(); ++i) {
        WorldPlant& plant = plants_[i];
        plant.def = catalogs_.plant(plant.kind); // nullptr when the kind is gone: skipped by play, a red "?" marks it
        if (plant.def == nullptr) {
            if (heightBefore[i] > 0.0) {
                const PixelPoint cell = plantCell(plant.feet);
                map_.setObstacle(cell.x, cell.y, 0.0);
            }
            continue;
        }
        const std::vector<std::string>& states = plant.def->states;
        if (states.empty()) plant.state.clear();
        else if (std::find(states.begin(), states.end(), plant.state) == states.end()) plant.state = states.front();
        const double height = plant.alive ? plantObstacleHeight(*plant.def) : 0.0;
        if (height != heightBefore[i]) {
            const PixelPoint cell = plantCell(plant.feet);
            map_.setObstacle(cell.x, cell.y, height);
        }
    }
    // A plant the level places whose kind has come (back) into the catalog stands in the world now.
    for (const PlacedPlant& placed : level_.plants) {
        if (plantIndexById(placed.id) >= 0) continue;
        const PlantDef* def = catalogs_.plant(placed.kind);
        if (def == nullptr) continue;
        plants_.push_back({placed.id, placed.kind, def, placed.feet, true, 0, def->states.empty() ? std::string() : def->states.front()});
        plants_.back().overrides = placed.overrides;
        if (const double height = plantObstacleHeight(*def); height > 0.0) {
            const PixelPoint cell = plantCell(placed.feet);
            map_.setObstacle(cell.x, cell.y, height);
        }
    }
    // The names the Editor offers: the people and the animals.
    definitions_.characters = std::move(fresh.characters);
    definitions_.weapons = std::move(fresh.weapons);
    definitions_.loopingEffects = std::move(fresh.loopingEffects);
    definitions_.plants = std::move(fresh.plants);
    definitions_.objects = std::move(fresh.objects);
    rebuildPlantArt();
    syncGraphCatalog(); // the tags the catalog carries, for the checks of the graph editor
    editor_.dataChanged(); // the palettes of plants, objects and characters
}

// The pictures of the plants and objects: the atlas frame of each plant, and the page of programmer art the objects are drawn on.
void OdysseyGame::rebuildPlantArt() {
    if (renderer_ == nullptr || !contentLoaded_) return;
    plantArt_.sources.clear();
    for (const PlantDef& plant : catalogs_.plants) {
        const auto frame = content_.frames.find(plant.frame);
        const auto rect = content_.rect(plant.frame);
        if (frame != content_.frames.end() && rect) plantArt_.sources[plant.name] = {frame->second.page, *rect};
    }
    std::vector<const PlantDef*> objects;
    for (const PlantDef& plant : catalogs_.plants) {
        if (plant.object) objects.push_back(&plant);
    }
    if (objects.empty()) return;
    std::map<std::string, core::Rect> rects;
    const luna::engine::Image objectPage = makeObjectPage(objects, rects);
    plantArt_.pages["objects"] = renderer_->createTexture(objectPage);
    silhouettes_[plantArt_.pages["objects"].id] = renderer_->createTexture(luna::engine::silhouette(objectPage));
    for (const auto& [name, rect] : rects) plantArt_.sources[name] = {"objects", rect};
}

// The starter weapons of the catalog, and the weapons the Editor's palette offers: the starters, then the two demo weapons (D-23).
void OdysseyGame::rebuildWeaponLists() {
    starters_.clear();
    std::vector<std::string> palette;
    for (const WeaponDef& weapon : catalogs_.weapons) {
        if (weapon.starter) {
            starters_.push_back(&weapon);
            palette.push_back(weapon.name);
        }
    }
    palette.push_back(kSpearThrowName);
    palette.push_back(kSwordSlashName);
    editor_.setWeaponPalette(std::move(palette));
}

// The place of each weapon, animal, effect and weather in the atlas. The pictures themselves stay; a new entry that names a frame the atlas has is drawn.
void OdysseyGame::rebuildCatalogArt() {
    if (!contentLoaded_) return;
    weatherFrames_.clear();
    if (content_.pictures.find("weather") != content_.pictures.end()) {
        for (const WeatherDef& def : catalogs_.weather) {
            for (int i = 0; i < def.frames; ++i) {
                if (const auto rect = content_.rect(content_.frameName(def.name, i))) weatherFrames_[def.name].push_back(*rect);
            }
        }
    }
    effectArt_.firstFrame.clear();
    for (const EffectDef& def : catalogs_.effects) {
        if (const auto rect = content_.rect(content_.frameName(def.name, 0))) effectArt_.firstFrame[def.name] = *rect;
    }
    animalArt_.sources.clear();
    for (const AnimalDef& animal : catalogs_.animals) {
        if (const auto rect = content_.rect(animal.frame)) animalArt_.sources[animal.name] = *rect;
    }
    weaponArt_.sources.clear();
    for (const WeaponDef& weapon : catalogs_.weapons) {
        if (const auto icon = content_.rect(weapon.frame)) weaponArt_.sources[weapon.name] = *icon;
    }
}

ReloadResult OdysseyGame::reloadHelp() {
    ReloadResult result;
    result.errors = editorHelp_.reload(dataDirectory_ / "editor" / "help.json");
    result.ok = result.errors.empty();
    return result;
}

// ---- What the owner sees

// The things the level places whose kind the data does not have: each is skipped by play and marked with a red "?". Returns the warnings that are new.
std::vector<std::string> OdysseyGame::findMissingKinds() {
    const std::vector<MissingKind> before = missing_;
    missing_.clear();
    const auto note = [&](std::string what, double x, double y) { missing_.push_back({std::move(what), x, y}); };
    for (const PlacedPlant& placed : level_.plants) {
        if (catalogs_.plant(placed.kind) == nullptr) {
            note(std::format("level \"{}\": plant #{} \"{}\" has no kind in plants.json or objects.json", level_.name, placed.id, placed.kind), placed.feet.x, placed.feet.y);
        }
    }
    for (const PlacedLight& placed : level_.lights) {
        if (lighting_.kind(placed.kind) == nullptr) {
            note(std::format("level \"{}\": light #{} \"{}\" has no kind in light/lights.json", level_.name, placed.id, placed.kind), placed.at.x, placed.at.y);
        }
    }
    for (const PlacedCharacter& placed : level_.characters) {
        if (definitions_.character(placed.kind) == nullptr) {
            note(std::format("level \"{}\": character #{} \"{}\" has no kind \"{}\" in characters.json", level_.name, placed.id, placed.name, placed.kind), placed.feet.x, placed.feet.y);
        }
    }
    editor_.setMissing(missing_);
    std::vector<std::string> fresh;
    for (const MissingKind& entry : missing_) {
        const bool known = std::any_of(before.begin(), before.end(), [&](const MissingKind& old) { return old.what == entry.what; });
        if (!known) fresh.push_back(entry.what);
    }
    return fresh;
}

// One line for the owner after a reload (D-59 Q3): a two-second toast, red when something failed. The mistakes stay in the panel until the files are fixed.
void OdysseyGame::reported(const std::vector<ReloadOutcome>& outcomes) {
    if (outcomes.empty()) return;
    std::string reloaded;
    std::string failed;
    std::string later;
    for (const ReloadOutcome& outcome : outcomes) {
        if (outcome.set != "interactions") { // the interactions keep their own report (interactionReport_)
            if (outcome.result.errors.empty()) reloadErrors_.erase(outcome.set);
            else reloadErrors_[outcome.set] = outcome.result.errors;
        }
        const auto add = [](std::string& list, const std::string& name) { list += (list.empty() ? "" : ", ") + name; };
        for (const std::string& warning : outcome.result.warnings) core::logWarning("Data: " + warning);
        if (outcome.result.atNextStart) {
            add(later, outcome.file.empty() ? outcome.set : fileNameOf(outcome.file));
        } else if (!outcome.result.ok) {
            add(failed, outcome.set);
            for (const std::string& error : outcome.result.errors) core::logWarning("Data: " + outcome.set + ": " + error);
        } else {
            add(reloaded, outcome.set);
            core::logInfo(std::format("Data: {} reloaded in {:.1f} ms", outcome.set, outcome.result.milliseconds));
        }
    }
    toastFailed_ = !failed.empty();
    if (!failed.empty()) toast_ = "Not reloaded: " + failed + " (mistakes listed; the last good data stays in use)";
    else if (!later.empty() && reloaded.empty()) toast_ = later + " applies at the next start";
    else if (!reloaded.empty()) toast_ = "Reloaded " + reloaded;
    else return;
    toastTicks_ = kToastTicks;
}

void OdysseyGame::drawToast(luna::engine::Renderer& renderer) const {
    if (toastTicks_ <= 0 || toast_.empty()) return;
    luna::engine::UiPainter painter(renderer, uiSheet_);
    const int width = luna::engine::UiPainter::textWidth(toast_) + 10;
    const luna::engine::Rect box{(uiWidth() - width) / 2, uiHeight() - 34, width, luna::engine::kGlyphHeight + 8}; // above the status line, clear of the Editor's toolbar
    painter.fill(box, luna::engine::UiColor::Dark);
    painter.outline(box, toastFailed_ ? luna::engine::UiColor::Red : luna::engine::UiColor::Border);
    painter.text(box.x + 5, box.y + 4, toast_, toastFailed_ ? luna::engine::UiColor::Red : luna::engine::UiColor::Text);
}

// ---- Watching the files for changes made outside the game (US-304)

void OdysseyGame::setWatching(bool on) {
    watching_ = on;
    if (!on) return;
    for (const DataSet& set : reloads_.sets()) {
        for (const std::filesystem::path& root : set.watch) watcher_.watch(root);
    }
    watcher_.watch(levelFile_); // the open level: read again when the Editor has nothing unsaved
    watcher_.snapshot();        // what is there now is not a change
}

// One tick of the watcher. The files that changed outside and have settled are read again by the sets that watch them; the open level is handled apart.
std::vector<ReloadOutcome> OdysseyGame::pollFiles(double nowSeconds) {
    std::vector<ReloadOutcome> outcomes;
    const std::vector<std::filesystem::path> files = watcher_.poll(nowSeconds);
    if (files.empty()) return outcomes;
    std::vector<std::filesystem::path> data;
    bool level = false;
    for (const std::filesystem::path& file : files) {
        std::error_code error;
        if (std::filesystem::equivalent(file, levelFile_, error) && !error) level = true;
        else data.push_back(file);
    }
    if (!data.empty()) {
        outcomes = reloads_.changed(data);
        reported(outcomes);
        for (const std::filesystem::path& file : data) editor_.data().changedOnDisk(file); // the Data tab shows the file it has open as it is on disk now (US-191)
    }
    if (level) levelChangedOutside();
    return outcomes;
}

// An Editor save read this set itself: the watcher takes the times of its files as they are, so it does not read the set a second time.
void OdysseyGame::ownReload(const std::string& set) {
    reported(reloads_.reload(set));
    for (const DataSet& entry : reloads_.sets()) {
        if (entry.name != set) continue;
        for (const std::filesystem::path& root : entry.watch) watcher_.resync(root);
    }
}

// The Data tab wrote this file: the sets that watch it read it again now (each once), and the watcher does not report the same write a second time.
void OdysseyGame::dataFileSaved(const std::filesystem::path& file) {
    watcher_.noteOwnWrite(file);
    for (const std::string& name : reloads_.setsFor(file)) ownReload(name);
}

void OdysseyGame::levelChangedOutside() {
    if (mode_ == Mode::Editor) applyLevelFromDisk();
    else levelReloadPending_ = true; // the run goes on; the Editor reads the new file when it opens
}

void OdysseyGame::applyLevelFromDisk() {
    levelReloadPending_ = false;
    const std::string name = levelFile_.filename().string();
    if (editor_.levelChangedOnDisk()) {
        findMissingKinds();
        toast_ = "Reloaded " + name;
        toastFailed_ = false;
    } else {
        toast_ = name + " changed on disk (your unsaved changes are kept)";
        toastFailed_ = true;
    }
    toastTicks_ = kToastTicks;
}

} // namespace odysseus::game
