#include "sim/json_data.h"

#include "core/text.h"
#include "sim/schema.h"

#include <format>

namespace odysseus::sim {

DataError::DataError(const std::filesystem::path& file, const std::string& field, const std::string& problem)
    : std::runtime_error(std::format("{}: {}: {}", file.generic_string(), field, problem)) {}

nlohmann::json readJsonFile(const std::filesystem::path& file) {
    const std::optional<std::string> text = core::readTextFile(file);
    if (!text) {
        throw DataError(file, "(file)", "cannot be opened");
    }
    nlohmann::json data;
    try {
        data = nlohmann::json::parse(*text, nullptr, true, true); // comments allowed, like every data file
    } catch (const nlohmann::json::parse_error& error) {
        throw DataError(file, "(syntax)", error.what());
    }
    schema::checkLoaded(file, data, *text); // US-190: type, range and choices, with file, line and field
    return data;
}

int requireInt(const nlohmann::json& object, const std::filesystem::path& file, const std::string& field, int minimum,
               int maximum) {
    if (!object.contains(field)) {
        throw DataError(file, field, "is missing");
    }
    const nlohmann::json& value = object.at(field);
    if (!value.is_number_integer()) {
        throw DataError(file, field, "must be a whole number");
    }
    const auto number = value.get<long long>();
    if (number < minimum || number > maximum) {
        throw DataError(file, field, std::format("must be between {} and {} (is {})", minimum, maximum, number));
    }
    return static_cast<int>(number);
}

int requireInt(const nlohmann::json& object, const std::filesystem::path& file, const std::string& section, const std::string& field,
               int minimum, int maximum) {
    if (!object.contains(section) || !object.at(section).is_object()) {
        throw DataError(file, section, "is missing (it must be an object)");
    }
    try {
        return requireInt(object.at(section), file, field, minimum, maximum);
    } catch (const DataError& error) {
        // Say "quarrel.percent" instead of just "percent": which section is part of the address.
        throw DataError(file, section + "." + field, std::string(error.what()).substr(std::string(error.what()).rfind(": ") + 2));
    }
}

} // namespace odysseus::sim
