#pragma once

#include "boundary.h"

#include "luna/physics/shapes.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace luna::physics {

// The "broad phase": a grid of square cells laid over the ground (x and y; height does not
// matter for a top-down world). Each shape is filed in every cell its bounds touch. Only
// shapes that share a cell can touch, so 1,000 shapes need a few hundred exact tests
// instead of 499,500 (every pair).
class SpatialGrid {
public:
    // Covers `width` x `depth` metres starting at (originX, originY); cells are `cellSize`
    // metres wide (2 m in the M1b design). Shapes outside are filed in the edge cells.
    SpatialGrid(Fixed originX, Fixed originY, Fixed width, Fixed depth, Fixed cellSize);

    void clear();
    void insert(std::uint32_t id, const Box& shapeBounds);

    // Every pair of ids that share at least one cell, each pair once, smaller id first,
    // in sorted order (so the result never depends on memory layout: determinism).
    std::vector<std::pair<std::uint32_t, std::uint32_t>> candidatePairs() const;

    int columns() const { return columns_; }
    int rows() const { return rows_; }

private:
    int columnOf(Fixed x) const;
    int rowOf(Fixed y) const;

    Fixed originX_;
    Fixed originY_;
    Fixed cellSize_;
    int columns_ = 0;
    int rows_ = 0;
    std::vector<std::vector<std::uint32_t>> cells_; // row after row, like the tile map
};

// One contact found in a step: which two shapes (indices), and where.
struct ContactPair {
    std::uint32_t first = 0;
    std::uint32_t second = 0;
    Contact contact;
};

struct ContactReport {
    std::vector<ContactPair> contacts;
    int pairsTested = 0; // exact overlap tests run (the narrow phase)
};

// One collision step: files every shape in the grid, then tests only the candidate pairs.
ContactReport findContacts(const std::vector<Shape>& shapes, SpatialGrid& grid);

} // namespace luna::physics
