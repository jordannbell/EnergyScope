#pragma once

#include <cstdint>
#include <vector>

#include "energy/Measurement.hpp"

namespace energy {

// The visible window of a chart: time axis (Unix seconds) and power axis (kW).
// tMax is always greater than tMin and yMax greater than yMin, so a chart can
// divide by the spans without checks.
struct ChartBounds {
    std::int64_t tMin;
    std::int64_t tMax;
    double yMin;
    double yMax;
};

// Smallest "round" number (1, 2 or 5 times a power of ten) that is >= v.
// Returns 1 when v is not positive. Gives axes like 0..200 instead of 0..137.
double niceCeil(double v);

// Axis ranges that fit all rows. Empty input gives the default 0..1 x 0..1.
// The power axis always includes 0.
ChartBounds computeBounds(const std::vector<Measurement>& rows);

}  // namespace energy
