#pragma once

#include <vector>

namespace energy {

struct Point {
    double x;
    double y;
};

// Ramer-Douglas-Peucker line simplification.
//
// Removes points that lie within `tolerance` of the straight segment between
// their kept neighbours. Guarantee: every original point is at most
// `tolerance` away from the simplified polyline. First and last points are
// always kept. Worst case O(n^2), typically O(n log n) on smooth curves.
// tolerance <= 0 (or NaN) returns the input unchanged.
std::vector<Point> simplifyPolyline(const std::vector<Point>& points,
                                    double tolerance);

}  // namespace energy
