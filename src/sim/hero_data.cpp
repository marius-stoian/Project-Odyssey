#include "hero_data.h"

#include "json_data.h"
#include "sim/rule_expr.h"

#include <algorithm>
#include <format>

namespace odysseus::sim {

namespace {

using nlohmann::json;
namespace fs = std::filesystem;

constexpr std::array<const char*, kAffinityCount> kNames{"Hunter", "Gatherer", "Flint-knapper", "Fire-keeper", "Shaman-healer", "Trade", "Religion"};
constexpr std::array<const char*, kAffinityCount> kKeys{"hunter", "gatherer", "knapper", "fireKeeper", "shaman", "trade", "religion"};

std::string text(const json& object, const fs::path& file, const std::string& where, const std::string& field) {
    if (!object.is_object() || !object.contains(field) || !object.at(field).is_string() || object.at(field).get<std::string>().empty()) {
        throw DataError(file, where + "." + field, "must be text in quotes");
    }
    return object.at(field).get<std::string>();
}

std::string optionalText(const json& object, const std::string& field) {
    return object.contains(field) && object.at(field).is_string() ? object.at(field).get<std::string>() : std::string();
}

int number(const json& object, const fs::path& file, const std::string& where, const std::string& field, int low, int high) {
    if (!object.is_object() || !object.contains(field) || !object.at(field).is_number_integer() || object.at(field).get<int>() < low || object.at(field).get<int>() > high) {
        throw DataError(file, where + "." + field, std::format("must be a whole number from {} to {}", low, high));
    }
    return object.at(field).get<int>();
}

const json& list(const json& root, const fs::path& file, const std::string& key) {
    if (!root.is_object() || !root.contains(key) || !root.at(key).is_array() || root.at(key).empty()) {
        throw DataError(file, key, "must be a list with at least one entry");
    }
    return root.at(key);
}

// {"hunter": 6, "trade": 1} -> values by affinity; an unknown key is an error.
AffinityValues affinities(const json& object, const fs::path& file, const std::string& where) {
    AffinityValues values{};
    if (object.is_null()) return values;
    if (!object.is_object()) throw DataError(file, where, "must be an object of affinity names and numbers");
    for (const auto& [key, value] : object.items()) {
        const auto affinity = affinityFromKey(key);
        if (!affinity || !value.is_number_integer()) throw DataError(file, where + "." + key, "must be one of hunter, gatherer, knapper, fireKeeper, shaman, trade, religion with a whole number");
        values[static_cast<std::size_t>(*affinity)] = value.get<int>();
    }
    return values;
}

} // namespace

const char* affinityName(Affinity affinity) { return kNames.at(static_cast<std::size_t>(affinity)); }

std::optional<Affinity> affinityFromKey(const std::string& key) {
    for (std::size_t i = 0; i < kKeys.size(); ++i) {
        if (key == kKeys[i]) return static_cast<Affinity>(i);
    }
    return std::nullopt;
}

const Item* HeroData::item(const std::string& id) const {
    const auto found = std::find_if(items.begin(), items.end(), [&](const Item& item) { return item.id == id; });
    return found == items.end() ? nullptr : &*found;
}

const Profession* HeroData::profession(const std::string& id) const {
    const auto found = std::find_if(professions.begin(), professions.end(), [&](const Profession& p) { return p.id == id; });
    return found == professions.end() ? nullptr : &*found;
}

int HeroData::professionIndex(const std::string& id) const {
    for (std::size_t i = 0; i < professions.size(); ++i) {
        if (professions[i].id == id) return static_cast<int>(i);
    }
    return -1;
}

const Recipe* HeroData::recipe(const std::string& id) const {
    const auto found = std::find_if(recipes.begin(), recipes.end(), [&](const Recipe& r) { return r.id == id; });
    return found == recipes.end() ? nullptr : &*found;
}

// One story event from its JSON (the same shape in story/events/<id>.json and in the older crossroads.json).
static CrossroadsEvent readEvent(const json& entry, const fs::path& eventsFile, const std::string& where) {
    CrossroadsEvent event;
    event.id = text(entry, eventsFile, where, "id");
    event.title = text(entry, eventsFile, where, "title");
    event.text = text(entry, eventsFile, where, "text");
    event.minAge = number(entry, eventsFile, where, "minAge", 1, 80);
    event.maxAge = number(entry, eventsFile, where, "maxAge", event.minAge, 80);
    event.role = optionalText(entry, "role");
    event.order = entry.value("order", 0);
    event.trigger = optionalText(entry, "trigger");
    if (!event.trigger.empty()) {
        const rules::ParsedExpr parsed = rules::parseExpression(event.trigger);
        if (parsed.problem) throw DataError(eventsFile, where + ".trigger", parsed.problem->message);
    }
    if (entry.contains("requires")) {
        const auto affinity = affinityFromKey(text(entry.at("requires"), eventsFile, where + ".requires", "affinity"));
        if (!affinity) throw DataError(eventsFile, where + ".requires.affinity", "is not an affinity");
        event.needsAffinity = affinity;
        event.needsMinimum = number(entry.at("requires"), eventsFile, where + ".requires", "min", 0, 100);
    }
    const json& options = list(entry, eventsFile, "options");
    if (options.size() < 2 || options.size() > 3) throw DataError(eventsFile, where + ".options", "must have 2 or 3 options");
    for (std::size_t o = 0; o < options.size(); ++o) {
        const std::string optionWhere = std::format("{}.options[{}]", where, o);
        CrossroadsOption option;
        option.text = text(options.at(o), eventsFile, optionWhere, "text");
        option.affinity = affinities(options.at(o).contains("affinity") ? options.at(o).at("affinity") : json(), eventsFile, optionWhere + ".affinity");
        option.opinion = options.at(o).value("opinion", 0);
        option.trait = optionalText(options.at(o), "trait");
        option.note = text(options.at(o), eventsFile, optionWhere, "note");
        event.options.push_back(option);
    }
    return event;
}

HeroData loadHeroData(const fs::path& dataDirectory) {
    const fs::path folder = dataDirectory / "hero";
    HeroData data;

    // hero.json: the curve, the presets, the origins and the numbers of trade, dominion and the sacred fire.
    const fs::path heroFile = folder / "hero.json";
    const json hero = readJsonFile(heroFile);
    const json& imprint = hero.contains("imprint") ? hero.at("imprint") : json();
    data.config.imprint.startAge = number(imprint, heroFile, "imprint", "startAge", 1, 60);
    data.config.imprint.peakPercent = number(imprint, heroFile, "imprint", "peakPercent", 0, 1000);
    data.config.imprint.midAge = number(imprint, heroFile, "imprint", "midAge", data.config.imprint.startAge, 80);
    data.config.imprint.midPercent = number(imprint, heroFile, "imprint", "midPercent", 0, 1000);
    data.config.imprint.endAge = number(imprint, heroFile, "imprint", "endAge", data.config.imprint.midAge, 80);
    data.config.imprint.endPercent = number(imprint, heroFile, "imprint", "endPercent", 0, 1000);
    data.config.imprint.settledPercent = number(imprint, heroFile, "imprint", "settledPercent", 0, 1000);
    for (std::size_t i = 0; i < list(hero, heroFile, "presets").size(); ++i) {
        const json& entry = hero.at("presets").at(i);
        const std::string where = std::format("presets[{}]", i);
        PresetConfig preset;
        preset.name = text(entry, heroFile, where, "name");
        preset.startAge = number(entry, heroFile, where, "startAge", 1, 60);
        preset.mantleAge = number(entry, heroFile, where, "mantleAge", preset.startAge, 80);
        data.config.presets.push_back(preset);
    }
    for (std::size_t i = 0; i < list(hero, heroFile, "comforts").size(); ++i) {
        const json& entry = hero.at("comforts").at(i);
        const std::string where = std::format("comforts[{}]", i);
        data.config.comforts.push_back({text(entry, heroFile, where, "name"), number(entry, heroFile, where, "needsPercent", 10, 400), number(entry, heroFile, where, "foodPercent", 10, 400)});
    }
    const json& aging = hero.contains("aging") ? hero.at("aging") : json();
    data.config.aging = {number(aging, heroFile, "aging", "fromYears", 20, 100), number(aging, heroFile, "aging", "startPercent", 0, 500), number(aging, heroFile, "aging", "perYearPercent", 0, 100)};
    for (std::size_t i = 0; i < list(hero, heroFile, "origins").size(); ++i) {
        const json& entry = hero.at("origins").at(i);
        OriginConfig origin;
        origin.name = text(entry, heroFile, std::format("origins[{}]", i), "name");
        origin.affinity = affinities(entry.contains("affinity") ? entry.at("affinity") : json(), heroFile, std::format("origins[{}].affinity", i));
        data.config.origins.push_back(origin);
    }
    data.config.skillLevelPoints = number(hero, heroFile, "hero", "skillLevelPoints", 1, 100);
    data.config.masterBonusPercent = number(hero, heroFile, "hero", "masterBonusPercent", 0, 1000);
    data.config.trustOpinion = number(hero, heroFile, "hero", "trustOpinion", -100, 100);
    const json& dominion = hero.contains("dominion") ? hero.at("dominion") : json();
    DominionConfig& dc = data.config.dominion;
    dc.tradePerPerson = number(dominion, heroFile, "dominion", "tradePerPerson", 0, 1000);
    dc.rivalTradePerPerson = number(dominion, heroFile, "dominion", "rivalTradePerPerson", 0, 1000);
    dc.rivalTradeGrowthPerDay = number(dominion, heroFile, "dominion", "rivalTradeGrowthPerDay", 0, 1000);
    dc.winPercent = number(dominion, heroFile, "dominion", "winPercent", 1, 100);
    dc.combinedWinPercent = number(dominion, heroFile, "dominion", "combinedWinPercent", 1, 100);
    dc.loseBelowPeople = number(dominion, heroFile, "dominion", "loseBelowPeople", 0, 100);
    dc.rivalFollowerPercent = number(dominion, heroFile, "dominion", "rivalFollowerPercent", 0, 100);
    const json& trade = hero.contains("trade") ? hero.at("trade") : json();
    TradeConfig& tc = data.config.trade;
    tc.dueDays = number(trade, heroFile, "trade", "dueDays", 1, 365);
    tc.maxDebts = number(trade, heroFile, "trade", "maxDebts", 0, 50);
    tc.defaultRelationshipPenalty = number(trade, heroFile, "trade", "defaultRelationshipPenalty", 0, 200);
    tc.defaultTradePenaltyPercent = number(trade, heroFile, "trade", "defaultTradePenaltyPercent", 0, 100);
    tc.relationshipPerDeal = number(trade, heroFile, "trade", "relationshipPerDeal", 0, 100);
    tc.needPercent = number(trade, heroFile, "trade", "needPercent", 100, 500);
    const json& fire = hero.contains("fire") ? hero.at("fire") : json();
    FireConfig& fc = data.config.fire;
    fc.skillLevelToFound = number(fire, heroFile, "fire", "skillLevelToFound", 0, 100);
    fc.untendedDaysBeforeOut = number(fire, heroFile, "fire", "untendedDaysBeforeOut", 1, 365);
    fc.ritualMinimumAttendees = number(fire, heroFile, "fire", "ritualMinimumAttendees", 1, 100);
    fc.ritualFaith = number(fire, heroFile, "fire", "ritualFaith", 0, 100);
    fc.followerFaith = number(fire, heroFile, "fire", "followerFaith", 0, 100);
    fc.neglectFaithLoss = number(fire, heroFile, "fire", "neglectFaithLoss", 0, 100);
    fc.conversionPerMille = number(fire, heroFile, "fire", "conversionPerMille", 0, 1000);
    fc.ritualRadiusTiles = number(fire, heroFile, "fire", "ritualRadiusTiles", 1, 50);

    // items.json
    const fs::path itemsFile = folder / "items.json";
    const json items = readJsonFile(itemsFile);
    for (std::size_t i = 0; i < list(items, itemsFile, "items").size(); ++i) {
        const json& entry = items.at("items").at(i);
        const std::string where = std::format("items[{}]", i);
        Item item{text(entry, itemsFile, where, "id"), text(entry, itemsFile, where, "name"), text(entry, itemsFile, where, "kind"), number(entry, itemsFile, where, "value", 1, 10000)};
        if (data.item(item.id) != nullptr) throw DataError(itemsFile, where + ".id", "\"" + item.id + "\" is listed twice");
        data.items.push_back(item);
    }

    // professions.json: every tool a profession needs must be an item.
    const fs::path professionsFile = folder / "professions.json";
    const json professions = readJsonFile(professionsFile);
    for (std::size_t i = 0; i < list(professions, professionsFile, "professions").size(); ++i) {
        const json& entry = professions.at("professions").at(i);
        const std::string where = std::format("professions[{}]", i);
        Profession profession;
        profession.id = text(entry, professionsFile, where, "id");
        profession.name = text(entry, professionsFile, where, "name");
        profession.skill = text(entry, professionsFile, where, "skill");
        profession.pillar = text(entry, professionsFile, where, "pillar");
        for (const json& tool : entry.contains("tools") ? entry.at("tools") : json::array()) {
            const std::string id = tool.is_string() ? tool.get<std::string>() : std::string();
            if (data.item(id) == nullptr) {
                throw DataError(professionsFile, where + ".tools", std::format("the profession {} needs the tool \"{}\", which items.json does not describe", profession.name, id));
            }
            profession.tools.push_back(id);
        }
        for (const json& action : entry.contains("actions") ? entry.at("actions") : json::array()) profession.actions.push_back(action.get<std::string>());
        if (data.profession(profession.id) != nullptr) throw DataError(professionsFile, where + ".id", "\"" + profession.id + "\" is listed twice");
        data.professions.push_back(profession);
    }
    if (data.professions.size() != kProfessionCount) throw DataError(professionsFile, "professions", std::format("must list exactly {} professions (the Age 1 set)", kProfessionCount));

    // recipes.json
    const fs::path recipesFile = folder / "recipes.json";
    const json recipes = readJsonFile(recipesFile);
    for (std::size_t i = 0; i < list(recipes, recipesFile, "recipes").size(); ++i) {
        const json& entry = recipes.at("recipes").at(i);
        const std::string where = std::format("recipes[{}]", i);
        Recipe recipe;
        recipe.id = text(entry, recipesFile, where, "id");
        recipe.name = text(entry, recipesFile, where, "name");
        recipe.station = text(entry, recipesFile, where, "station");
        recipe.profession = data.professionIndex(text(entry, recipesFile, where, "skill"));
        if (recipe.profession < 0) throw DataError(recipesFile, where + ".skill", "is not a profession in professions.json");
        if (entry.contains("inputs")) {
            for (const auto& [id, count] : entry.at("inputs").items()) {
                if (data.item(id) == nullptr) throw DataError(recipesFile, where + ".inputs", "\"" + id + "\" is not an item");
                recipe.inputs.push_back({id, count.get<int>()});
            }
        }
        for (const json& tool : entry.contains("tools") ? entry.at("tools") : json::array()) {
            if (data.item(tool.get<std::string>()) == nullptr) throw DataError(recipesFile, where + ".tools", "\"" + tool.get<std::string>() + "\" is not an item");
            recipe.tools.push_back(tool.get<std::string>());
        }
        recipe.output = text(entry, recipesFile, where, "output");
        if (data.item(recipe.output) == nullptr) throw DataError(recipesFile, where + ".output", "\"" + recipe.output + "\" is not an item");
        recipe.count = number(entry, recipesFile, where, "count", 1, 100);
        data.recipes.push_back(recipe);
    }

    // activities.json
    const fs::path activitiesFile = folder / "activities.json";
    const json activities = readJsonFile(activitiesFile);
    for (std::size_t i = 0; i < list(activities, activitiesFile, "activities").size(); ++i) {
        const json& entry = activities.at("activities").at(i);
        const std::string where = std::format("activities[{}]", i);
        Activity activity;
        activity.id = text(entry, activitiesFile, where, "id");
        activity.name = text(entry, activitiesFile, where, "name");
        activity.text = text(entry, activitiesFile, where, "text");
        activity.affinity = affinities(entry.contains("affinity") ? entry.at("affinity") : json(), activitiesFile, where + ".affinity");
        const AffinityValues skills = affinities(entry.contains("skill") ? entry.at("skill") : json(), activitiesFile, where + ".skill");
        for (std::size_t s = 0; s < kProfessionCount; ++s) activity.skill[s] = skills[s];
        data.activities.push_back(activity);
    }

    // The story events (US-185): one file each in story/events/, in the order of their "order" field; the older single crossroads.json is still read
    // when that folder is missing.
    const fs::path eventsFolder = folder.parent_path() / "story" / "events";
    std::error_code eventsError;
    if (fs::is_directory(eventsFolder, eventsError)) {
        std::vector<fs::path> files;
        for (const auto& entry : fs::directory_iterator(eventsFolder, eventsError)) {
            const std::string name = entry.path().filename().string();
            const bool layout = name.size() > 12 && name.compare(name.size() - 12, 12, ".layout.json") == 0;
            if (entry.is_regular_file() && entry.path().extension() == ".json" && !layout) files.push_back(entry.path());
        }
        std::sort(files.begin(), files.end());
        std::vector<CrossroadsEvent> loaded;
        for (const fs::path& file : files) {
            const json entry = readJsonFile(file);
            CrossroadsEvent event = readEvent(entry, file, "event");
            if (event.id != file.stem().string()) throw DataError(file, "id", std::format("\"{}\" must match the file name \"{}\"", event.id, file.stem().string()));
            loaded.push_back(std::move(event));
        }
        std::stable_sort(loaded.begin(), loaded.end(), [](const CrossroadsEvent& a, const CrossroadsEvent& b) { return a.order != b.order ? a.order < b.order : a.id < b.id; });
        data.events = std::move(loaded);
    } else {
        const fs::path eventsFile = folder / "crossroads.json";
        const json events = readJsonFile(eventsFile);
        for (std::size_t i = 0; i < list(events, eventsFile, "events").size(); ++i) {
            CrossroadsEvent event = readEvent(events.at("events").at(i), eventsFile, std::format("events[{}]", i));
            event.order = static_cast<int>(i);
            data.events.push_back(std::move(event));
        }
    }
    return data;
}

} // namespace odysseus::sim

namespace odysseus::sim {

const char* affinityKey(Affinity affinity) { return kKeys.at(static_cast<std::size_t>(affinity)); }

std::optional<CrossroadsEvent> parseStoryEvent(const std::string& jsonText, const std::string& name, std::string& problem) {
    try {
        const json entry = json::parse(jsonText, nullptr, true, true);
        return readEvent(entry, fs::path(name), "event");
    } catch (const std::exception& error) {
        problem = error.what();
        return std::nullopt;
    }
}

std::string writeStoryEvent(const CrossroadsEvent& event) {
    nlohmann::ordered_json out = nlohmann::ordered_json::object();
    out["id"] = event.id;
    out["order"] = event.order;
    out["title"] = event.title;
    out["text"] = event.text;
    out["minAge"] = event.minAge;
    out["maxAge"] = event.maxAge;
    if (!event.role.empty()) out["role"] = event.role;
    if (!event.trigger.empty()) out["trigger"] = event.trigger;
    if (event.needsAffinity) out["requires"] = {{"affinity", affinityKey(*event.needsAffinity)}, {"min", event.needsMinimum}};
    nlohmann::ordered_json options = nlohmann::ordered_json::array();
    for (const CrossroadsOption& option : event.options) {
        nlohmann::ordered_json o = nlohmann::ordered_json::object();
        o["text"] = option.text;
        nlohmann::ordered_json affinity = nlohmann::ordered_json::object();
        for (std::size_t a = 0; a < kAffinityCount; ++a) {
            if (option.affinity[a] != 0) affinity[affinityKey(static_cast<Affinity>(a))] = option.affinity[a];
        }
        o["affinity"] = affinity;
        o["opinion"] = option.opinion;
        if (!option.trait.empty()) o["trait"] = option.trait;
        o["note"] = option.note;
        options.push_back(o);
    }
    out["options"] = options;
    return out.dump(2) + "\n";
}

} // namespace odysseus::sim
