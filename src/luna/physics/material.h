#pragma once

#include "boundary.h"

#include "luna/physics/rigid_body.h"

namespace luna::physics {

// What a thing is made of (PHY-05). Games load these from data (materials.json); Luna only
// knows the numbers. Heat and chemistry join in later Ages.
struct Material {
    Fixed density;   // kg/m^3: flint 2600, wood 700
    Fixed hardness;  // Mohs scale, 1 (talc) to 10 (diamond): flint 7
    Fixed sharpness; // 0..1: how well an edge of it cuts
    SurfaceMaterial surface;
};

// The mass of `volume` cubic metres of the material.
Fixed massOf(const Material& material, Fixed volume);

// Kinetic energy, E = 1/2 m v^2 (joules): what a moving thing brings to an impact.
Fixed kineticEnergy(Fixed mass, Vec3 velocity);

} // namespace luna::physics
