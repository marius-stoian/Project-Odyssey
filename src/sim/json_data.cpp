#include "sim/json_data.h"

#include <format>
#include <fstream>

namespace odysseus::sim {

DataError::DataError(const std::filesystem::path& file, const std::string& field, const std::string& problem)
    : std::runtime_error(std::format("{}: {}: {}", file.generic_string(), field, problem)) {}

nlohmann::json readJsonFile(const std::filesystem::path& file) {
    std::ifstream in(file);
    if (!in) {
        throw DataError(file, "(file)", "cannot be opened");
    }
    try {
        return nlohmann::json::parse(in);
    } catch (const nlohmann::json::parse_error& error) {
        throw DataError(file, "(syntax)", error.what());
    }
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

} // namespace odysseus::sim
