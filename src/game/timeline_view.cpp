#include "game/timeline_view.h"

#include "sim/npc_schedule.h"

#include <algorithm>
#include <format>

namespace odysseus::game {

using luna::engine::PointerButton;
using luna::engine::Rect;
using luna::engine::UiColor;
using luna::engine::UiInput;
using luna::engine::UiPainter;

TimelineView::TimelineView(Rect bounds, std::string title, std::vector<Block> blocks, std::function<void(std::size_t, int)> onMove)
    : Widget(bounds), title_(std::move(title)), blocks_(std::move(blocks)), onMove_(std::move(onMove)) {
    this->bounds.height = kHeight;
}

int TimelineView::clampedStart(const std::vector<Block>& blocks, std::size_t index, int minute) {
    const int low = index == 0 ? 0 : blocks[index - 1].minute + kStep;
    const int high = index + 1 < blocks.size() ? blocks[index + 1].minute - kStep : 24 * 60 - kStep;
    const int snapped = (minute + kStep / 2) / kStep * kStep;
    return std::clamp(snapped, low, std::max(low, high));
}

int TimelineView::blockAt(int x, int y) const {
    const Rect area = bar();
    if (y < area.y || y >= area.y + area.height) return -1;
    for (std::size_t i = 0; i < blocks_.size(); ++i) {
        if (std::abs(x - (area.x + pixelOf(blocks_[i].minute, area.width))) <= kGrab) return static_cast<int>(i);
    }
    return -1;
}

int TimelineView::blockUnder(int x) const {
    if (blocks_.empty()) return -1;
    const int minute = minuteAt(x - bar().x, bar().width);
    int found = static_cast<int>(blocks_.size()) - 1; // before the first block: the one that came over midnight
    for (std::size_t i = 0; i < blocks_.size(); ++i) {
        if (blocks_[i].minute <= minute) found = static_cast<int>(i);
    }
    return found;
}

bool TimelineView::handle(const UiInput& input) {
    const luna::engine::Pointer& pointer = input.pointer;
    hoverX_ = contains(pointer.x, pointer.y) ? pointer.x : -1;
    hoverY_ = pointer.y;
    if (drag_ >= 0) {
        const std::size_t index = static_cast<std::size_t>(drag_);
        if (pointer.isHeld(PointerButton::Left)) {
            blocks_[index].minute = clampedStart(blocks_, index, minuteAt(pointer.x - bar().x, bar().width));
            return true;
        }
        const int minute = blocks_[index].minute; // the button went up: the drag is over
        drag_ = -1;
        if (minute != dragFrom_ && onMove_) onMove_(index, minute);
        return true;
    }
    if (pointer.wasPressed(PointerButton::Left) && contains(pointer.x, pointer.y)) {
        drag_ = blockAt(pointer.x, pointer.y);
        if (drag_ >= 0) dragFrom_ = blocks_[static_cast<std::size_t>(drag_)].minute;
        return true; // a click on the bar is used, so what is behind it does not get it
    }
    return false;
}

void TimelineView::draw(UiPainter& painter) const {
    painter.text(bounds.x, bounds.y + 1, title_, UiColor::Dim);
    const Rect area = bar();
    painter.fill(area, UiColor::Dark);
    for (int hour = 0; hour < 24; hour += 3) { // a mark every three hours, the hour above it
        const int x = area.x + pixelOf(hour * 60, area.width);
        painter.fill({x, area.y - 2, 1, 2}, UiColor::Border);
        painter.text(x + 2, bounds.y + 1, std::format("{}", hour), UiColor::Dim);
    }
    for (std::size_t i = 0; i < blocks_.size(); ++i) {
        const int from = pixelOf(blocks_[i].minute, area.width);
        const bool last = i + 1 == blocks_.size();
        const int to = last ? area.width : pixelOf(blocks_[i + 1].minute, area.width);
        UiColor colour = UiColor::PanelLight;
        if (blocks_[i].activity == "sleep") colour = UiColor::Panel;
        else if (blocks_[i].activity == "work") colour = UiColor::Selected;
        else if (blocks_[i].activity == "eat") colour = UiColor::Hover;
        painter.fill({area.x + from, area.y, std::max(1, to - from), area.height}, colour);
        painter.fill({area.x + from, area.y, 1, area.height}, static_cast<int>(i) == drag_ ? UiColor::Gold : UiColor::Border);
        const int room = (to - from - 3) / luna::engine::kTextAdvance;
        if (room > 0) painter.text(area.x + from + 3, area.y + 2, blocks_[i].activity.substr(0, static_cast<std::size_t>(room)), UiColor::Text);
    }
    if (!blocks_.empty() && blocks_.front().minute > 0) { // the last block holds over midnight into the first hours
        const int to = pixelOf(blocks_.front().minute, area.width);
        painter.fill({area.x, area.y, std::max(1, to), 1}, UiColor::Dim);
    }
    painter.outline(area, UiColor::Border);
}

void TimelineView::drawOverlay(UiPainter& painter) const {
    if (hoverX_ < 0 || blocks_.empty()) return;
    const int under = drag_ >= 0 ? drag_ : blockUnder(hoverX_);
    if (under < 0) return;
    const Block& block = blocks_[static_cast<std::size_t>(under)];
    const std::string text = std::format("{} {} at {}{}", sim::rules::formatClock(block.minute), block.activity, block.place.empty() ? "home" : block.place,
                                         blockAt(hoverX_, hoverY_) >= 0 || drag_ >= 0 ? ": drag to move" : "");
    const int width = UiPainter::textWidth(text) + 6;
    const int x = std::min(hoverX_ + 8, bounds.x + bounds.width - width);
    const Rect box{std::max(bounds.x, x), bounds.y + kHeight + 2, width, luna::engine::kLineHeight + 4};
    painter.fill(box, UiColor::Shade);
    painter.outline(box, UiColor::Gold);
    painter.text(box.x + 3, box.y + 3, text, UiColor::Gold);
}

} // namespace odysseus::game
