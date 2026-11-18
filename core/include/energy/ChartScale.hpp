#pragma once

#include <cstdint>
#include <vector>

#include "energy/Measurement.hpp"

namespace energy {

// Number of equal divisions drawn on the power axis (grid lines + labels).
constexpr int kPowerDivisions = 4;

// The visible window of a chart: time axis (Unix seconds) and power axis (kW).
// tMax is always greater than tMin and yMax greater than yMin, so a chart can
// divide by the spans without checks.
//
// The power axis is cut into kPowerDivisions equal steps; the step is a
// "round" number (1, 2, 2.5 or 5 times a power of ten) and 0 always falls on
// a grid line. Example: data from -1 to 120 gives -50..150 with ticks
// -50, 0, 50, 100, 150, never ugly ticks such as 49.25.
struct ChartBounds {
    std::int64_t tMin;
    std::int64_t tMax;
    double yMin;
    double yMax;
};

// Axis ranges that fit all rows. Empty input gives the default 0..1 x 0..1.
// The power axis always includes 0.
ChartBounds computeBounds(const std::vector<Measurement>& rows);

}  // namespace energy
