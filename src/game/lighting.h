#pragma once

#include "boundary.h"

#include "luna/engine/renderer.h"

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
    friend bool operator==(const LightKindDef&, const LightKindDef&) = default;
};

// The light data of the game: the ambient colour (255 255 255 at strength 1 leaves every sprite as drawn) and the kinds of point light.
struct LightingData {
    int ambientRed = 255, ambientGreen = 255, ambientBlue = 255;
    double ambientStrength = 1.0;
    std::vector<LightKindDef> kinds;

    const LightKindDef* kind(const std::string& name) const;
    // The frame the renderer is given for the world: the ambient colour times its strength, no point lights yet.
    luna::engine::LightFrame ambientFrame() const;
    friend bool operator==(const LightingData&, const LightingData&) = default;
};

// Reads lights.json; a mistake becomes a sim::DataError naming the file and the field. A missing file is not an error: the game is then unlit.
LightingData loadLighting(const std::filesystem::path& file);
// The file's text for `data` (loadLighting reads it back to the same data).
std::string lightingToText(const LightingData& data);

} // namespace odysseus::game
