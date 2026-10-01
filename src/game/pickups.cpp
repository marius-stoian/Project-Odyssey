#include "game/pickups.h"

namespace odysseus::game {

std::string badgeLetters(const std::string& weapon) {
    if (weapon == kSpearThrowName) return "Sp";
    if (weapon == kSwordSlashName) return "Sw";
    return weapon.substr(0, 2);
}

void drawWeaponIcon(luna::engine::Renderer& renderer, luna::engine::UiPainter& painter, const WeaponArt& art, const std::string& weapon,
                    const luna::engine::Rect& area) {
    if (const auto found = art.sources.find(weapon); found != art.sources.end()) {
        renderer.drawStyled(art.icons, found->second, area, {});
        return;
    }
    painter.fill(area, luna::engine::UiColor::Shade);
    painter.outline(area, luna::engine::UiColor::Gold);
    const std::string letters = badgeLetters(weapon);
    painter.text(area.x + (area.width - luna::engine::UiPainter::textWidth(letters)) / 2, area.y + (area.height - luna::engine::kGlyphHeight) / 2, letters,
                 luna::engine::UiColor::Gold);
}

} // namespace odysseus::game
