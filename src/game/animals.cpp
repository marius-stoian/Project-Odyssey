#include "game/animals.h"

#include <algorithm>

namespace odysseus::game {

bool facesWest(Facing facing) { return facing == Facing::West || facing == Facing::SouthWest || facing == Facing::NorthWest; }

void drawAnimal(luna::engine::Renderer& renderer, const AnimalArt& art, const std::string& animal, int feetX, int feetY, Facing facing, bool flash) {
    const auto found = art.sources.find(animal);
    if (found == art.sources.end()) {
        return;
    }
    luna::engine::Rect source = found->second;
    const bool west = facesWest(facing);
    if (west) {
        source.x = art.pageWidth - source.x - source.width; // the same cell in the mirrored page
    }
    const luna::engine::Texture& texture = west ? art.mirrored : art.page;
    const luna::engine::Rect destination{feetX - source.width / 2, feetY - source.height, source.width, source.height};
    renderer.drawStyled(texture, source, destination, {});
    if (flash) {
        renderer.drawStyled(texture, source, destination, {200, luna::engine::Blend::Add}); // the hit flash: the picture glows white
    }
}

void drawAnimalIcon(luna::engine::Renderer& renderer, const AnimalArt& art, const std::string& animal, const luna::engine::Rect& area) {
    const auto found = art.sources.find(animal);
    if (found == art.sources.end()) {
        return;
    }
    const luna::engine::Rect& source = found->second;
    const double scale = std::min(static_cast<double>(area.width) / source.width, static_cast<double>(area.height) / source.height);
    const int width = std::max(1, static_cast<int>(source.width * scale));
    const int height = std::max(1, static_cast<int>(source.height * scale));
    renderer.drawStyled(art.page, source, {area.x + (area.width - width) / 2, area.y + (area.height - height) / 2, width, height}, {});
}

} // namespace odysseus::game
