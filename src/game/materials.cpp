#include "game/materials.h"

#include "sim/data.h"
#include "sim/json_data.h"

#include <cmath>
#include <cstdint>
#include <format>

namespace odysseus::game {

namespace {

using luna::physics::Fixed;
using sim::DataError;

// Decimal numbers in the file become Fixed through millionths, so "0.9" always gives the
// same Fixed on every computer.
Fixed requireDecimal(const nlohmann::json& object, const std::filesystem::path& file, const std::string& path,
                     const std::string& field, double minimum, double maximum) {
    const std::string name = path + "." + field;
    if (!object.contains(field)) {
        throw DataError(file, name, "is missing");
    }
    const nlohmann::json& value = object.at(field);
    if (!value.is_number()) {
        throw DataError(file, name, "must be a number");
    }
    const double number = value.get<double>();
    if (number < minimum || number > maximum) {
        throw DataError(file, name, std::format("must be between {} and {} (is {})", minimum, maximum, number));
    }
    return Fixed::fromRatio(static_cast<std::int64_t>(std::llround(number * 1'000'000.0)), 1'000'000);
}

const nlohmann::json& requireObject(const nlohmann::json& object, const std::filesystem::path& file,
                                    const std::string& field) {
    if (!object.contains(field) || !object.at(field).is_object()) {
        throw DataError(file, field, "must be an object");
    }
    return object.at(field);
}

std::string requireMaterialName(const nlohmann::json& object, const std::filesystem::path& file, const std::string& path,
                                const std::string& field, const MaterialsConfig& config) {
    const std::string name = path + "." + field;
    if (!object.contains(field) || !object.at(field).is_string()) {
        throw DataError(file, name, "must be the name of a material");
    }
    const std::string material = object.at(field).get<std::string>();
    if (!config.materials.contains(material)) {
        throw DataError(file, name, std::format("names an unknown material '{}'", material));
    }
    return material;
}

} // namespace

const luna::physics::Material& MaterialsConfig::material(const std::string& name) const {
    return materials.at(name);
}

const SpearKind& MaterialsConfig::spear(const std::string& name) const {
    return spears.at(name);
}

MaterialsConfig loadMaterials(const std::filesystem::path& dataDirectory) {
    const std::filesystem::path file = dataDirectory / "materials.json";
    const nlohmann::json root = sim::readJsonFile(file);
    MaterialsConfig config;

    for (const auto& [name, entry] : requireObject(root, file, "materials").items()) {
        const std::string path = "materials." + name;
        luna::physics::Material material;
        material.density = requireDecimal(entry, file, path, "density", 1, 30'000);
        material.hardness = requireDecimal(entry, file, path, "hardness", 1, 10);
        material.sharpness = requireDecimal(entry, file, path, "sharpness", 0, 1);
        material.surface.restitution = requireDecimal(entry, file, path, "restitution", 0, 1);
        material.surface.friction = requireDecimal(entry, file, path, "friction", 0, 2);
        config.materials.emplace(name, material);
    }

    config.hardnessScale = requireDecimal(requireObject(root, file, "damage"), file, "damage", "hardnessScale", 1, 100);

    for (const auto& [name, entry] : requireObject(root, file, "spears").items()) {
        const std::string path = "spears." + name;
        requireMaterialName(entry, file, path, "shaft", config);
        SpearKind spear;
        spear.name = name;
        spear.tip = requireMaterialName(entry, file, path, "tip", config);
        const Fixed shaftMass = requireDecimal(entry, file, path, "shaftMass", 0.1, 20);
        const Fixed tipVolume = requireDecimal(entry, file, path, "tipVolume", 0.000001, 0.01);
        spear.mass = shaftMass + luna::physics::massOf(config.material(spear.tip), tipVolume);
        spear.throwSpeed = requireDecimal(entry, file, path, "throwSpeed", 1, 60);
        config.spears.emplace(name, spear);
    }
    return config;
}

luna::physics::Fixed impactDamage(const MaterialsConfig& config, const SpearKind& spear, luna::physics::Vec3 velocity) {
    const luna::physics::Material& tip = config.material(spear.tip);
    return luna::physics::kineticEnergy(spear.mass, velocity) * tip.hardness / config.hardnessScale * tip.sharpness;
}

} // namespace odysseus::game
