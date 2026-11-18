#include "energy/Geometry.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

namespace energy {

namespace {

// Distance from point p to the SEGMENT a-b (not to the infinite line).
// Using the segment matters: a point can be close to the line yet far from
// the segment, and dropping it would then break the tolerance guarantee.
double distanceToSegment(const Point& p, const Point& a, const Point& b) {
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    const double lengthSquared = dx * dx + dy * dy;
    if (lengthSquared == 0.0) return std::hypot(p.x - a.x, p.y - a.y);

    // Position of p's projection along a-b: 0 at a, 1 at b.
    const double t =
        std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / lengthSquared, 0.0, 1.0);
    return std::hypot(p.x - (a.x + t * dx), p.y - (a.y + t * dy));
}

}  // namespace

std::vector<Point> simplifyPolyline(const std::vector<Point>& points,
                                    double tolerance) {
    const std::size_t n = points.size();
    if (n < 3 || !(tolerance > 0.0)) return points;

    std::vector<bool> keep(n, false);
    keep.front() = keep.back() = true;

    // Explicit stack instead of recursion: a long, smooth series would
    // otherwise recurse as deep as the number of points.
    std::vector<std::pair<std::size_t, std::size_t>> pending{{0, n - 1}};
    while (!pending.empty()) {
        const auto [first, last] = pending.back();
        pending.pop_back();

        double farthest = -1.0;
        std::size_t farthestIndex = first;
        for (std::size_t i = first + 1; i < last; ++i) {
            const double d = distanceToSegment(points[i], points[first], points[last]);
            if (d > farthest) {
                farthest = d;
                farthestIndex = i;
            }
        }
        if (farthest > tolerance) {
            keep[farthestIndex] = true;
            pending.emplace_back(first, farthestIndex);
            pending.emplace_back(farthestIndex, last);
        }
    }

    std::vector<Point> result;
    for (std::size_t i = 0; i < n; ++i) {
        if (keep[i]) result.push_back(points[i]);
    }
    return result;
}

}  // namespace energy
