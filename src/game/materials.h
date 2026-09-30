#pragma once

#include "boundary.h"

#include "luna/physics/material.h"

#include <filesystem>
#include <map>
#include <string>

namespace odysseus::game {

// A spear as described in materials.json: a wooden shaft with a tip of some material.
struct SpearKind {
    std::string name;       // "flint", "wooden"
    std::string tip;        // the tip's material
    luna::physics::Fixed mass;       // kg: shaft + tip volume x tip density
    luna::physics::Fixed throwSpeed; // m/s
};

// Everything in assets/data/materials.json (Charter rule 7: content is data).
struct MaterialsConfig {
    std::map<std::string, luna::physics::Material> materials;
    std::map<std::string, SpearKind> spears;
    luna::physics::Fixed hardnessScale;

    const luna::physics::Material& material(const std::string& name) const;
    const SpearKind& spear(const std::string& name) const;
};

// Reads and checks the file. Every problem throws a DataError that names the file and the
// field, for example "materials.json: materials.flint.hardness: must be between 1 and 10".
MaterialsConfig loadMaterials(const std::filesystem::path& dataDirectory);

// How much an impact hurts (US-029): the kinetic energy it brings, scaled by how hard and how
// sharp the tip is. Heavier (denser) tips bring more energy; harder, sharper tips waste less.
luna::physics::Fixed impactDamage(const MaterialsConfig& config, const SpearKind& spear, luna::physics::Vec3 velocity);

} // namespace odysseus::game
