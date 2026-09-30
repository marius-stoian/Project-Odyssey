#include "sim/clan.h"

#include "sim/json_data.h"

#include <algorithm>

namespace odysseus::sim {

namespace {

std::vector<std::string> requireNames(const nlohmann::json& json, const std::filesystem::path& file, const std::string& field) {
    if (!json.contains(field) || !json.at(field).is_array() || json.at(field).empty()) {
        throw DataError(file, field, "must be a list of names");
    }
    std::vector<std::string> names;
    for (const auto& name : json.at(field)) {
        if (!name.is_string() || name.get<std::string>().empty()) {
            throw DataError(file, field, "every entry must be a name in quotes");
        }
        names.push_back(name.get<std::string>());
    }
    return names;
}

} // namespace

ClanConfig loadClanConfig(const std::filesystem::path& file) {
    const nlohmann::json json = readJsonFile(file);
    ClanConfig config;
    config.startingPeople = requireInt(json, file, "startingPeople", 1, 200);
    if (!json.contains("startingAgeYears") || !json.at("startingAgeYears").is_object()) {
        throw DataError(file, "startingAgeYears", "must be an object with minimum and maximum");
    }
    const nlohmann::json& ages = json.at("startingAgeYears");
    config.minimumStartingAgeYears = requireInt(ages, file, "minimum", 0, 80);
    config.maximumStartingAgeYears = requireInt(ages, file, "maximum", config.minimumStartingAgeYears, 80);
    config.startingFood = requireInt(json, file, "startingFood", 0, 100'000);
    return config;
}

NameList loadNameList(const std::filesystem::path& file) {
    const nlohmann::json json = readJsonFile(file);
    return {requireNames(json, file, "female"), requireNames(json, file, "male")};
}

std::string pickName(const NameList& names, Sex sex, const std::vector<Person>& living, core::Pcg32& random) {
    const std::vector<std::string>& pool = sex == Sex::Female ? names.female : names.male;
    // Try a few random names for one nobody alive has; a clan can still repeat names later.
    std::string name;
    for (int attempt = 0; attempt < 8; ++attempt) {
        name = pool[random.below(static_cast<std::uint32_t>(pool.size()))];
        const bool taken = std::any_of(living.begin(), living.end(),
                                       [&name](const Person& p) { return p.alive && p.name == name; });
        if (!taken) {
            break;
        }
    }
    return name;
}

std::vector<Person> makeStartingClan(const ClanConfig& config, const NameList& names, int daysPerYear, core::Pcg32& random) {
    std::vector<Person> people;
    const auto ageSpan = static_cast<std::uint32_t>((config.maximumStartingAgeYears - config.minimumStartingAgeYears + 1) * daysPerYear);
    for (int i = 0; i < config.startingPeople; ++i) {
        Person person;
        person.id = i;
        person.sex = random.chance(50) ? Sex::Female : Sex::Male;
        person.ageDays = config.minimumStartingAgeYears * daysPerYear + static_cast<int>(random.below(ageSpan));
        person.name = pickName(names, person.sex, people, random);
        people.push_back(person);
    }
    return people;
}

} // namespace odysseus::sim
