#include "game/npc_marker.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numbers>

namespace odysseus::game {

namespace {

const std::map<std::string, std::vector<std::string>>& icons() {
    static const std::map<std::string, std::vector<std::string>> table = {
        {"person", {"...##...", "...##...", "..####..", ".######.", "...##...", "...##...", "..#..#..", "..#..#.."}},
        {"coin", {"..####..", ".######.", "###..###", "##.##.##", "##.##.##", "###..###", ".######.", "..####.."}},
        {"crown", {"#..##..#", "##.##.##", "########", "########", "########", ".######.", "........", "........"}},
        {"shield", {"########", "########", "########", "########", ".######.", ".######.", "..####..", "...##..."}},
        {"sword", {"......##", ".....###", "....###.", "#..###..", ".####...", "..##....", ".#.#....", "#...#..."}},
        {"bow", {"##......", ".##.....", "..#.#...", "..#..#..", "..#..#..", "..#.#...", ".##.....", "##......"}},
        {"heart", {".##..##.", "########", "########", "########", ".######.", "..####..", "...##...", "........"}},
        {"skull", {".######.", "########", "##.##.##", "##.##.##", "########", ".######.", ".#.##.#.", ".######."}},
        {"star", {"...##...", "...##...", "########", ".######.", "..####..", ".######.", ".##..##.", ".#....#."}},
        {"flame", {"...#....", "..##....", "..###...", ".####.#.", ".######.", "########", ".######.", "..####.."}},
        {"leaf", {".....###", "...#####", "..######", ".######.", ".#####..", "##.##...", "#..#....", "#......."}},
        {"paw", {".#....#.", "###..###", "###..###", ".#....#.", "..####..", ".######.", ".######.", "..#..#.."}},
        {"book", {".######.", "#.#..#.#", "#.#..#.#", "#.#..#.#", "#.#..#.#", "#.####.#", "########", "........"}},
        {"cross", {"...##...", "...##...", "...##...", "########", "########", "...##...", "...##...", "...##..."}},
        {"hammer", {".######.", ".######.", "...##...", "...##...", "...##...", "...##...", "...##...", "...##..."}},
        {"pick", {".#####..", "#...#.#.", "....#..#", "...#....", "..#.....", ".#......", "#.......", "........"}},
        {"flask", {"..####..", "...##...", "...##...", "..####..", ".######.", "########", "########", ".######."}},
        {"key", {".####...", "##..##..", "##..##..", ".####...", "..##....", "..###...", "..##....", "..###..."}},
        {"eye", {"........", "..####..", ".######.", "##.##.##", "##.##.##", ".######.", "..####..", "........"}},
        {"moon", {"..####..", ".###....", "###.....", "###.....", "###.....", ".###....", "..####..", "........"}},
        {"sun", {"#..##..#", ".#.##.#.", "..####..", "########", "########", "..####..", ".#.##.#.", "#..##..#"}},
        {"drop", {"...#....", "...##...", "..####..", "..####..", ".######.", ".######.", ".######.", "..####.."}},
        {"tooth", {".######.", "########", "########", "########", ".##..##.", ".##..##.", ".##..##.", ".#....#."}},
        {"wing", {".....###", "...#####", ".#######", "########", ".######.", "..####..", "...##...", "....#..."}},
    };
    return table;
}

luna::engine::Color rgb(int colour, std::uint8_t alpha = 255) {
    return {static_cast<std::uint8_t>((colour >> 16) & 0xFF), static_cast<std::uint8_t>((colour >> 8) & 0xFF), static_cast<std::uint8_t>(colour & 0xFF), alpha};
}

} // namespace

const std::vector<std::string>& iconBitmap(const std::string& name) {
    const auto found = icons().find(name);
    return found != icons().end() ? found->second : icons().at("person");
}

std::optional<NpcMarker> markerFor(const sim::rules::ResolvedNpc& resolved, const sim::rules::NpcClassCatalog& catalog) {
    NpcMarker marker;
    for (const std::string& id : resolved.classes) {
        const sim::rules::NpcClass* found = catalog.find(id);
        if (found == nullptr) continue; // a class without a file has no colour
        if (marker.colours.empty()) marker.icon = found->icon;
        marker.colours.push_back(found->colour);
    }
    if (marker.colours.empty()) return std::nullopt;
    return marker;
}

luna::engine::Image markerImage(const NpcMarker& marker) {
    luna::engine::Image image(kMarkerSize, kMarkerSize);
    const double centre = (kMarkerSize - 1) / 2.0;
    const int arcs = std::max<int>(1, static_cast<int>(marker.colours.size()));
    for (int y = 0; y < kMarkerSize; ++y) {
        for (int x = 0; x < kMarkerSize; ++x) {
            const double dx = x - centre;
            const double dy = y - centre;
            const double distance = std::sqrt(dx * dx + dy * dy);
            if (distance > 8.6) continue;
            if (distance >= 6.6) {
                // The angle from the top, clockwise, as a fraction of a full turn.
                double turn = std::atan2(dx, -dy) / (2.0 * std::numbers::pi);
                if (turn < 0.0) turn += 1.0;
                const int arc = std::min(arcs - 1, static_cast<int>(turn * arcs));
                image.set(x, y, rgb(marker.colours.empty() ? 0xFFFFFF : marker.colours[static_cast<std::size_t>(arc)]));
            } else {
                image.set(x, y, {24, 24, 32, 200}); // the dark disc the icon sits on
            }
        }
    }
    const std::vector<std::string>& bitmap = iconBitmap(marker.icon);
    const int offset = (kMarkerSize - 8) / 2;
    for (int row = 0; row < 8; ++row) {
        for (int column = 0; column < 8; ++column) {
            if (bitmap[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)] == '#') image.set(offset + column, offset + row, {245, 245, 235, 255});
        }
    }
    return image;
}

} // namespace odysseus::game
