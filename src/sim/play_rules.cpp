#include "sim/play_rules.h"

#include "sim/data.h"
#include "sim/json_data.h"

#include <algorithm>
#include <format>

namespace odysseus::sim {

namespace {

using nlohmann::json;
namespace fs = std::filesystem;

int whole(const json& object, const fs::path& file, const std::string& where, const std::string& field, int low, int high) {
    if (!object.is_object() || !object.contains(field) || !object.at(field).is_number_integer() || object.at(field).get<int>() < low || object.at(field).get<int>() > high) {
        throw DataError(file, where + "." + field, std::format("must be a whole number from {} to {}", low, high));
    }
    return object.at(field).get<int>();
}

std::string words(const json& object, const fs::path& file, const std::string& where, const std::string& field) {
    if (!object.is_object() || !object.contains(field) || !object.at(field).is_string() || object.at(field).get<std::string>().empty()) {
        throw DataError(file, where + "." + field, "must be text in quotes");
    }
    return object.at(field).get<std::string>();
}

void yesNo(const json& systems, const fs::path& file, const std::string& field, bool& value) {
    if (!systems.contains(field)) return;
    if (!systems.at(field).is_boolean()) throw DataError(file, "systems." + field, "must be true or false");
    value = systems.at(field).get<bool>();
}

} // namespace

PlayRules loadPlayRules(const fs::path& dataDirectory, const std::string& name) {
    const fs::path file = dataDirectory / "rules" / (name + ".json");
    if (!fs::exists(file)) throw DataError(file, "(file)", "no rules file of this name in assets/data/rules/");
    const json data = readJsonFile(file);
    PlayRules rules;
    rules.name = name;
    if (data.contains("title") && data.at("title").is_string()) rules.title = data.at("title").get<std::string>();
    if (data.contains("systems")) {
        const json& systems = data.at("systems");
        if (!systems.is_object()) throw DataError(file, "systems", "must be a group of true or false switches");
        yesNo(systems, file, "weather", rules.systems.weather);
        yesNo(systems, file, "combat", rules.systems.combat);
        yesNo(systems, file, "rivals", rules.systems.rivals);
        yesNo(systems, file, "tutorial", rules.systems.tutorial);
        yesNo(systems, file, "markers", rules.systems.markers);
        yesNo(systems, file, "chronicle", rules.systems.chronicle);
        yesNo(systems, file, "politics", rules.systems.politics);
    }
    if (data.contains("victory")) {
        const json& victory = data.at("victory");
        rules.hasVictory = true;
        rules.winPercent = whole(victory, file, "victory", "winPercent", 1, 100);
        rules.combinedWinPercent = whole(victory, file, "victory", "combinedWinPercent", 1, 100);
        rules.loseBelowPeople = whole(victory, file, "victory", "loseBelowPeople", 0, 100);
        rules.rivalFollowerPercent = whole(victory, file, "victory", "rivalFollowerPercent", 0, 100);
    }
    if (data.contains("newGame")) {
        const json& newGame = data.at("newGame");
        if (newGame.contains("presets")) {
            const json& presets = newGame.at("presets");
            if (!presets.is_array() || presets.empty()) throw DataError(file, "newGame.presets", "must be a list with at least one entry");
            for (std::size_t i = 0; i < presets.size(); ++i) {
                const std::string where = std::format("newGame.presets[{}]", i);
                PresetConfig preset;
                preset.name = words(presets.at(i), file, where, "name");
                preset.startAge = whole(presets.at(i), file, where, "startAge", 1, 60);
                preset.mantleAge = whole(presets.at(i), file, where, "mantleAge", preset.startAge, 80);
                rules.presets.push_back(preset);
            }
        }
        if (newGame.contains("comforts")) {
            const json& comforts = newGame.at("comforts");
            if (!comforts.is_array() || comforts.empty()) throw DataError(file, "newGame.comforts", "must be a list with at least one entry");
            for (std::size_t i = 0; i < comforts.size(); ++i) {
                const std::string where = std::format("newGame.comforts[{}]", i);
                rules.comforts.push_back({words(comforts.at(i), file, where, "name"), whole(comforts.at(i), file, where, "needsPercent", 10, 400), whole(comforts.at(i), file, where, "foodPercent", 10, 400)});
            }
        }
    }
    return rules;
}

std::vector<std::string> playRuleNames(const fs::path& dataDirectory) {
    std::vector<std::string> names;
    std::error_code error;
    for (const fs::directory_entry& entry : fs::directory_iterator(dataDirectory / "rules", error)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") names.push_back(entry.path().stem().string());
    }
    std::sort(names.begin(), names.end(), [](const std::string& a, const std::string& b) { return a == "standard" ? b != "standard" : (b != "standard" && a < b); });
    return names;
}

void applyRules(HeroConfig& config, const PlayRules& rules) {
    if (!rules.presets.empty()) config.presets = rules.presets;
    if (!rules.comforts.empty()) config.comforts = rules.comforts;
    if (rules.hasVictory) {
        config.dominion.winPercent = rules.winPercent;
        config.dominion.combinedWinPercent = rules.combinedWinPercent;
        config.dominion.loseBelowPeople = rules.loseBelowPeople;
        config.dominion.rivalFollowerPercent = rules.rivalFollowerPercent;
    }
}

std::string describeRules(const PlayRules& rules) {
    const auto state = [](bool on) { return on ? "on" : "off"; };
    const PlaySystems& s = rules.systems;
    std::string line = std::format("weather {}, combat {}, rivals {}, tutorial {}, markers {}, chronicle {}, politics {}", state(s.weather), state(s.combat), state(s.rivals),
                                   state(s.tutorial), state(s.markers), state(s.chronicle), state(s.politics));
    if (rules.hasVictory) line += std::format("; won at {}% of Trade or Religion or {}% of both, lost below {} people", rules.winPercent, rules.combinedWinPercent, rules.loseBelowPeople);
    return line;
}

} // namespace odysseus::sim
