#include "energy/ChartScale.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace energy {

namespace {

struct PowerRange {
    double yMin;
    double yMax;
};

constexpr PowerRange kDefaultPowerRange{0.0, 1.0};
constexpr double kStepMantissas[] = {1.0, 2.0, 2.5, 5.0};
constexpr double kRelativeSlack = 1e-9;  // absorbs floating-point rounding

// Finds the smallest round step so that kPowerDivisions steps, starting on a
// multiple of the step at or below `lowest`, reach at least `highest`.
PowerRange fitPowerRange(double lowest, double highest) {
    const double minStep = (highest - lowest) / kPowerDivisions;
    for (int exponent = static_cast<int>(std::floor(std::log10(minStep)));; ++exponent) {
        for (const double mantissa : kStepMantissas) {
            const double step = mantissa * std::pow(10.0, exponent);
            if (step < minStep * (1.0 - kRelativeSlack)) continue;

            const double yMin = std::floor(lowest / step) * step;
            const double yMax = yMin + kPowerDivisions * step;
            if (yMax >= highest - step * kRelativeSlack) return {yMin, yMax};
        }
        if (exponent > std::numeric_limits<double>::max_exponent10) break;
    }
    return kDefaultPowerRange;  // unreachable for finite input
}

}  // namespace

ChartBounds computeBounds(const std::vector<Measurement>& rows) {
    if (rows.empty()) return {0, 1, kDefaultPowerRange.yMin, kDefaultPowerRange.yMax};

    std::int64_t tMin = rows.front().timestamp;
    std::int64_t tMax = tMin;
    double lowest = 0.0;   // the axis always includes 0
    double highest = 0.0;
    for (const auto& m : rows) {
        tMin = std::min(tMin, m.timestamp);
        tMax = std::max(tMax, m.timestamp);
        lowest = std::min(lowest, m.powerKw);
        highest = std::max(highest, m.powerKw);
    }

    if (tMin == tMax) {  // a single instant still needs a non-empty axis
        if (tMax == std::numeric_limits<std::int64_t>::max()) {
            --tMin;
        } else {
            ++tMax;
        }
    }

    if (highest == lowest) highest = lowest + 1.0;  // all zero
    PowerRange power = std::isfinite(lowest) && std::isfinite(highest)
                           ? fitPowerRange(lowest, highest)
                           : kDefaultPowerRange;
    if (!std::isfinite(power.yMin) || !std::isfinite(power.yMax)) {
        power = kDefaultPowerRange;
    }
    return {tMin, tMax, power.yMin, power.yMax};
}

}  // namespace energy
