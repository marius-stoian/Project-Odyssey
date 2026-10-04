#pragma once

#include "boundary.h"

#include "luna/engine/renderer.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace odysseus::game {

// A kind of light (US-240, assets/data/light/lights.json): its colour, how far it reaches in tiles, how strong it is and how high above the ground
// it hangs (a higher light is flatter on a surface). Steady, no flicker (D-49).
struct LightKindDef {
    std::string name;
    int red = 255, green = 255, blue = 255;
    double radiusTiles = 4.0;
    double strength = 1.0;
    double height = 24.0; // pixels
    double flicker = 0.0; // 0 steady (D-49), up to 1: how much the strength wavers, by a seeded noise in the Game (never the simulation)
    friend bool operator==(const LightKindDef&, const LightKindDef&) = default;
};

// The light data of the game: the ambient colour (255 255 255 at strength 1 leaves every sprite as drawn) and the kinds of point light.
struct LightingData {
    int ambientRed = 255, ambientGreen = 255, ambientBlue = 255;
    double ambientStrength = 1.0;
    std::vector<LightKindDef> kinds;
    std::string clanTorch; // the kind of light each clan member carries at night ("" = none)

    const LightKindDef* kind(const std::string& name) const;
    // The frame the renderer is given for the world: the ambient colour times its strength, no point lights yet.
    luna::engine::LightFrame ambientFrame() const;
    friend bool operator==(const LightingData&, const LightingData&) = default;
};

// A smooth natural wobble between 0 and 1 for a flame (US-243): value noise, a new random value every `period` seconds blended smoothly into the next.
// Made from a hash of (seed, step), so the same light flickers the same way every run, and nothing here touches the simulation's random streams.
double flickerNoise(std::uint64_t seed, double seconds, double period = 0.15);

// Reads lights.json; a mistake becomes a sim::DataError naming the file and the field. A missing file is not an error: the game is then unlit.
LightingData loadLighting(const std::filesystem::path& file);
// The file's text for `data` (loadLighting reads it back to the same data).
std::string lightingToText(const LightingData& data);

} // namespace odysseus::game
