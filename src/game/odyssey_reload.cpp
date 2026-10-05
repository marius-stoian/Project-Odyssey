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
    reloads_.add({"catalog", {data / "plants.json", data / "objects.json", data / "characters.json"}, [this] { return reloadCatalog(); }});
    reloads_.add({"help", {data / "editor" / "help.json"}, [this] { return reloadHelp(); }});
    // What cannot be swapped while a run is loaded (a weapon, an animal, a tile or an effect is held by pointers and numbers all over the play state): the
    // change is reported, never half-applied, and takes effect at the next start. F5 leaves these alone.
    DataSet later{"next-start",
                  {data / "weapons.json", data / "animals.json", data / "effects.json", data / "weather.json", data / "tiles.json", data / "materials.json", data / "buildings",
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

// Takes a clean copy of the plant and object catalogs and the character kinds into use. The weapons, animals, effects and weather of that copy are
// not taken: the play state holds pointers into them (they apply at the next start). The plants of the play state are pointed at the new catalog by
// their kind's name, never by an address that is about to go (CI-007).
void OdysseyGame::applyCatalog(Definitions fresh, Catalogs catalogs) {
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
    // The names the Editor offers; the animals are not reloaded, so their kinds stay as they were.
    std::vector<CharacterKindDef> kinds;
    for (const CharacterKindDef& kind : fresh.characters) {
        if (!kind.animal) kinds.push_back(kind);
    }
    for (const CharacterKindDef& kind : definitions_.characters) {
        if (kind.animal) kinds.push_back(kind);
    }
    definitions_.characters = std::move(kinds);
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

} // namespace odysseus::game
