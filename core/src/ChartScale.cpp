#include "energy/ChartScale.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace energy {

double niceCeil(double v) {
    if (!(v > 0.0) || !std::isfinite(v)) return 1.0;

    const double base = std::pow(10.0, std::floor(std::log10(v)));
    const double fraction = v / base;  // in [1, 10)
    double nice = 10.0;
    if (fraction <= 1.0) {
        nice = 1.0;
    } else if (fraction <= 2.0) {
        nice = 2.0;
    } else if (fraction <= 5.0) {
        nice = 5.0;
    }
    return nice * base;
}

ChartBounds computeBounds(const std::vector<Measurement>& rows) {
    if (rows.empty()) return {0, 1, 0.0, 1.0};

    std::int64_t tMin = rows.front().timestamp;
    std::int64_t tMax = tMin;
    double lowest = 0.0;   // axis always includes 0
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

    const double yMin = lowest < 0.0 ? -niceCeil(-lowest) : 0.0;
    return {tMin, tMax, yMin, niceCeil(highest)};
}

}  // namespace energy
