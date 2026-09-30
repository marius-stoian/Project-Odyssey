#pragma once

#include "boundary.h"

#include "game/level.h"

#include <memory>
#include <string>
#include <vector>

namespace odysseus::game {

// One edit that can be undone and done again (the command pattern). Every change the editor
// makes to a level is a Command, so Ctrl+Z and Ctrl+Y can walk back and forth through them.
class Command {
public:
    virtual ~Command() = default;
    virtual void apply(Level& level) const = 0;
    virtual void undo(Level& level) const = 0;
    virtual std::string name() const = 0; // "paint 12 cells", for the status line
};

// Ground cells changed from one kind to another: a brush stroke, a fill, an eraser stroke.
struct CellChange {
    int x = 0;
    int y = 0;
    int before = 0;
    int after = 0;
};

class PaintCommand final : public Command {
public:
    PaintCommand(std::string what, std::vector<CellChange> changes) : what_(std::move(what)), changes_(std::move(changes)) {}
    void apply(Level& level) const override;
    void undo(Level& level) const override;
    std::string name() const override;
    bool empty() const { return changes_.empty(); }

private:
    std::string what_;
    std::vector<CellChange> changes_;
};

// Characters placed, moved, turned, renamed or removed: the whole list before and after, and the
// next free id (ids are never reused, even after an undo).
class CharactersCommand final : public Command {
public:
    CharactersCommand(std::string what, std::vector<PlacedCharacter> before, std::vector<PlacedCharacter> after, int nextIdBefore, int nextIdAfter)
        : what_(std::move(what)), before_(std::move(before)), after_(std::move(after)), nextIdBefore_(nextIdBefore), nextIdAfter_(nextIdAfter) {}
    void apply(Level& level) const override;
    void undo(Level& level) const override;
    std::string name() const override { return what_; }

private:
    std::string what_;
    std::vector<PlacedCharacter> before_;
    std::vector<PlacedCharacter> after_;
    int nextIdBefore_;
    int nextIdAfter_;
};

// Weapon pickups placed, moved or removed (US-134): like characters, the whole list before and after.
class PickupsCommand final : public Command {
public:
    PickupsCommand(std::string what, std::vector<PlacedPickup> before, std::vector<PlacedPickup> after, int nextIdBefore, int nextIdAfter)
        : what_(std::move(what)), before_(std::move(before)), after_(std::move(after)), nextIdBefore_(nextIdBefore), nextIdAfter_(nextIdAfter) {}
    void apply(Level& level) const override;
    void undo(Level& level) const override;
    std::string name() const override { return what_; }

private:
    std::string what_;
    std::vector<PlacedPickup> before_;
    std::vector<PlacedPickup> after_;
    int nextIdBefore_;
    int nextIdAfter_;
};

// Plants placed, moved or removed (US-136): like pickups, the whole list before and after.
class PlantsCommand final : public Command {
public:
    PlantsCommand(std::string what, std::vector<PlacedPlant> before, std::vector<PlacedPlant> after, int nextIdBefore, int nextIdAfter)
        : what_(std::move(what)), before_(std::move(before)), after_(std::move(after)), nextIdBefore_(nextIdBefore), nextIdAfter_(nextIdAfter) {}
    void apply(Level& level) const override;
    void undo(Level& level) const override;
    std::string name() const override { return what_; }

private:
    std::string what_;
    std::vector<PlacedPlant> before_;
    std::vector<PlacedPlant> after_;
    int nextIdBefore_;
    int nextIdAfter_;
};

// A whole-level change: a resize, a new name, another default ground, a moved hero start. It
// keeps the level before and after (a resize changes everything, and is rare).
class LevelCommand final : public Command {
public:
    LevelCommand(std::string what, Level before, Level after) : what_(std::move(what)), before_(std::move(before)), after_(std::move(after)) {}
    void apply(Level& level) const override { level = after_; }
    void undo(Level& level) const override { level = before_; }
    std::string name() const override { return what_; }

private:
    std::string what_;
    Level before_;
    Level after_;
};

// The edits done, and the ones undone that can be done again. At most `limit` are kept.
class History {
public:
    explicit History(std::size_t limit = 100) : limit_(limit) {}

    // Applies the command to the level and remembers it; anything undone is forgotten.
    void run(std::unique_ptr<Command> command, Level& level);
    // Remembers a command that is already applied (a brush stroke, painted while dragging).
    void record(std::unique_ptr<Command> command);
    bool undo(Level& level);
    bool redo(Level& level);
    bool canUndo() const { return !done_.empty(); }
    bool canRedo() const { return !undone_.empty(); }
    std::size_t size() const { return done_.size(); }
    void clear();

private:
    std::size_t limit_;
    std::vector<std::unique_ptr<Command>> done_;
    std::vector<std::unique_ptr<Command>> undone_;
};

// Tools as pure functions over a level: what they would change (nothing is applied).
// Cells on the line from (x0, y0) to (x1, y1) inclusive (a drag between two ticks leaves no gaps).
std::vector<std::pair<int, int>> lineCells(int x0, int y0, int x1, int y1);
// Every cell of the rectangle between two corners, inside the level.
std::vector<CellChange> rectangleFill(const Level& level, int x0, int y0, int x1, int y1, int tile);
// The area of equal ground connected to (x, y) by its four sides, painted `tile`.
std::vector<CellChange> floodFill(const Level& level, int x, int y, int tile);

} // namespace odysseus::game
