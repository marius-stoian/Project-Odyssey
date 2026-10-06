#pragma once

#include "boundary.h"

#include "luna/engine/ui.h"

#include <functional>
#include <string>
#include <vector>

namespace odysseus::game {

// The 24 hours of a routine as a bar (US-196): one coloured block for each entry of a schedule, from its start to the start of the next, the last one holding over midnight
// to the first. Dragging the left edge of a block moves its start (in steps of 15 minutes, never past a neighbour); letting go reports it. The bar edits nothing itself: the
// owner of the data does, through the same writer as the form below it, so the form and the timeline always show the same schedule.
class TimelineView final : public luna::engine::Widget {
public:
    struct Block {
        int minute = 0;        // when it begins, 0 to 1439
        std::string activity;  // "work", "sleep", ...
        std::string place;     // "market", "home"
    };
    static constexpr int kStep = 15;      // minutes: the least a block moves
    static constexpr int kHeight = 22;    // pixels: the hour marks and the bar
    static constexpr int kGrab = 4;       // pixels either side of a block's start that grab it

    // `onMove(index, minute)` is called once, when a drag ends on a new time.
    TimelineView(luna::engine::Rect bounds, std::string title, std::vector<Block> blocks, std::function<void(std::size_t, int)> onMove);

    // Where a block of `blocks` may begin when it is dragged to `minute`: snapped to the step and kept between its neighbours (the first block between 00:00 and the next).
    static int clampedStart(const std::vector<Block>& blocks, std::size_t index, int minute);
    // The pixel of a minute of the day in a bar `width` wide, and back.
    static int pixelOf(int minute, int width) { return minute * width / 1440; }
    static int minuteAt(int pixel, int width) { return width <= 0 ? 0 : pixel * 1440 / width; }

    bool handle(const luna::engine::UiInput& input) override;
    void draw(luna::engine::UiPainter& painter) const override;
    void drawOverlay(luna::engine::UiPainter& painter) const override;

    const std::string& title() const { return title_; }
    bool dragging() const { return drag_ >= 0; }
    const std::vector<Block>& blocks() const { return blocks_; } // as drawn now, a block being dragged at its new place

private:
    luna::engine::Rect bar() const { return {bounds.x, bounds.y + 9, bounds.width, kHeight - 9}; }
    int blockAt(int x, int y) const;       // the block whose start edge is under the pointer, or -1
    int blockUnder(int x) const;           // the block that covers this pixel of the bar

    std::string title_;
    std::vector<Block> blocks_;
    std::function<void(std::size_t, int)> onMove_;
    int drag_ = -1;          // the block being dragged
    int dragFrom_ = 0;       // its start when the drag began
    int hoverX_ = -1;
    int hoverY_ = -1;
};

} // namespace odysseus::game
