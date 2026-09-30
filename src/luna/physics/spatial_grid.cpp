#include "luna/physics/spatial_grid.h"

#include <algorithm>

namespace luna::physics {

SpatialGrid::SpatialGrid(Fixed originX, Fixed originY, Fixed width, Fixed depth, Fixed cellSize)
    : originX_(originX), originY_(originY), cellSize_(cellSize) {
    ODYSSEUS_ASSERT(cellSize > kFixedZero && width > kFixedZero && depth > kFixedZero, "grid needs a positive size");
    // Round up, so the last partial column and row still get cells.
    columns_ = static_cast<int>((width / cellSize).floorToInt()) + 1;
    rows_ = static_cast<int>((depth / cellSize).floorToInt()) + 1;
    cells_.resize(static_cast<std::size_t>(columns_) * static_cast<std::size_t>(rows_));
}

void SpatialGrid::clear() {
    for (auto& cell : cells_) {
        cell.clear(); // keeps each cell's memory for the next step
    }
}

int SpatialGrid::columnOf(Fixed x) const {
    const std::int64_t column = ((x - originX_) / cellSize_).floorToInt();
    return static_cast<int>(std::clamp<std::int64_t>(column, 0, columns_ - 1));
}

int SpatialGrid::rowOf(Fixed y) const {
    const std::int64_t row = ((y - originY_) / cellSize_).floorToInt();
    return static_cast<int>(std::clamp<std::int64_t>(row, 0, rows_ - 1));
}

void SpatialGrid::insert(std::uint32_t id, const Box& shapeBounds) {
    const int firstColumn = columnOf(shapeBounds.min.x);
    const int lastColumn = columnOf(shapeBounds.max.x);
    const int firstRow = rowOf(shapeBounds.min.y);
    const int lastRow = rowOf(shapeBounds.max.y);
    for (int row = firstRow; row <= lastRow; ++row) {
        for (int column = firstColumn; column <= lastColumn; ++column) {
            cells_[static_cast<std::size_t>(row) * static_cast<std::size_t>(columns_) + static_cast<std::size_t>(column)]
                .push_back(id);
        }
    }
}

std::vector<std::pair<std::uint32_t, std::uint32_t>> SpatialGrid::candidatePairs() const {
    std::vector<std::pair<std::uint32_t, std::uint32_t>> pairs;
    for (const auto& cell : cells_) {
        for (std::size_t i = 0; i < cell.size(); ++i) {
            for (std::size_t j = i + 1; j < cell.size(); ++j) {
                pairs.emplace_back(std::min(cell[i], cell[j]), std::max(cell[i], cell[j]));
            }
        }
    }
    // A pair that shares several cells was added several times: sort, then drop repeats.
    std::sort(pairs.begin(), pairs.end());
    pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());
    return pairs;
}

ContactReport findContacts(const std::vector<Shape>& shapes, SpatialGrid& grid) {
    grid.clear();
    for (std::size_t i = 0; i < shapes.size(); ++i) {
        grid.insert(static_cast<std::uint32_t>(i), bounds(shapes[i]));
    }
    ContactReport report;
    for (const auto& [first, second] : grid.candidatePairs()) {
        ++report.pairsTested;
        if (const auto contact = overlap(shapes[first], shapes[second])) {
            report.contacts.push_back({first, second, *contact});
        }
    }
    return report;
}

} // namespace luna::physics
