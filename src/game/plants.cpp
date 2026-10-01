#include "game/plants.h"

#include <algorithm>

namespace odysseus::game {

double plantObstacleHeight(const PlantDef& plant) {
    if (!plant.blocks) {
        return 0.0;
    }
    return plant.size == "tree" ? 3.0 : 1.5;
}

luna::engine::Rect plantExtent(const PlantArt& art, const std::string& plant) {
    if (const auto found = art.sources.find(plant); found != art.sources.end()) {
        return {0, 0, found->second.rect.width, found->second.rect.height};
    }
    return {0, 0, 32, 32};
}

void drawPlant(luna::engine::Renderer& renderer, const PlantArt& art, const std::string& plant, int feetX, int feetY) {
    const auto found = art.sources.find(plant);
    if (found == art.sources.end()) {
        return;
    }
    const auto page = art.pages.find(found->second.page);
    if (page == art.pages.end()) {
        return;
    }
    renderer.draw(page->second, found->second.rect, {feetX - found->second.rect.width / 2, feetY - found->second.rect.height});
}

void drawPlantIcon(luna::engine::Renderer& renderer, const PlantArt& art, const std::string& plant, const luna::engine::Rect& area) {
    const auto found = art.sources.find(plant);
    if (found == art.sources.end()) {
        return;
    }
    const auto page = art.pages.find(found->second.page);
    if (page == art.pages.end()) {
        return;
    }
    const luna::engine::Rect& source = found->second.rect;
    // Whole-number shrink is not needed: the renderer scales; keep the proportions and centre it.
    const double scale = std::min(static_cast<double>(area.width) / source.width, static_cast<double>(area.height) / source.height);
    const int width = std::max(1, static_cast<int>(source.width * scale));
    const int height = std::max(1, static_cast<int>(source.height * scale));
    renderer.drawStyled(page->second, source, {area.x + (area.width - width) / 2, area.y + (area.height - height) / 2, width, height}, {});
}

} // namespace odysseus::game
