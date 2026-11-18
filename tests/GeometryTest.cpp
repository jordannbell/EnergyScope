#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <limits>

#include "energy/Geometry.hpp"

using energy::Point;
using energy::simplifyPolyline;

namespace {

double distanceToSegment(const Point& p, const Point& a, const Point& b) {
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    const double lengthSquared = dx * dx + dy * dy;
    double t = lengthSquared == 0.0
                   ? 0.0
                   : ((p.x - a.x) * dx + (p.y - a.y) * dy) / lengthSquared;
    t = std::clamp(t, 0.0, 1.0);
    return std::hypot(p.x - (a.x + t * dx), p.y - (a.y + t * dy));
}

double distanceToPolyline(const Point& p, const std::vector<Point>& line) {
    double best = std::numeric_limits<double>::max();
    for (std::size_t i = 1; i < line.size(); ++i) {
        best = std::min(best, distanceToSegment(p, line[i - 1], line[i]));
    }
    return best;
}

}  // namespace

TEST(SimplifyPolyline, CollinearPointsCollapseToTheTwoEnds) {
    const std::vector<Point> line{{0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4}};

    const auto out = simplifyPolyline(line, 0.1);

    ASSERT_EQ(out.size(), 2u);
    EXPECT_DOUBLE_EQ(out.front().x, 0);
    EXPECT_DOUBLE_EQ(out.back().x, 4);
}

TEST(SimplifyPolyline, KeepsAPeakThatExceedsTheTolerance) {
    const std::vector<Point> line{{0, 0}, {1, 0}, {2, 5}, {3, 0}, {4, 0}};

    // (1,0) and (3,0) are 0.93 away from the chords to the peak: dropped.
    const auto out = simplifyPolyline(line, 1.0);

    ASSERT_EQ(out.size(), 3u);
    EXPECT_DOUBLE_EQ(out[1].y, 5);
}

TEST(SimplifyPolyline, KeepsPointsFartherThanTheToleranceFromTheChord) {
    const std::vector<Point> line{{0, 0}, {1, 0}, {2, 5}, {3, 0}, {4, 0}};

    EXPECT_EQ(simplifyPolyline(line, 0.5).size(), 5u);
}

TEST(SimplifyPolyline, DropsABumpSmallerThanTheTolerance) {
    const std::vector<Point> line{{0, 0}, {1, 0.05}, {2, 0}};

    EXPECT_EQ(simplifyPolyline(line, 0.1).size(), 2u);
}

TEST(SimplifyPolyline, ZeroOrInvalidToleranceKeepsEverything) {
    const std::vector<Point> line{{0, 0}, {1, 0.05}, {2, 0}};

    EXPECT_EQ(simplifyPolyline(line, 0.0).size(), 3u);
    EXPECT_EQ(simplifyPolyline(line, -1.0).size(), 3u);
    EXPECT_EQ(simplifyPolyline(line, std::nan("")).size(), 3u);
}

TEST(SimplifyPolyline, ShortInputsAreReturnedAsIs) {
    EXPECT_TRUE(simplifyPolyline({}, 1.0).empty());
    EXPECT_EQ(simplifyPolyline({{1, 1}}, 1.0).size(), 1u);
    EXPECT_EQ(simplifyPolyline({{1, 1}, {2, 2}}, 1.0).size(), 2u);
}

TEST(SimplifyPolyline, NeverStraysFurtherThanTheToleranceFromTheOriginal) {
    std::vector<Point> curve;
    for (int i = 0; i < 2000; ++i) {
        const double x = i * 0.5;
        curve.push_back({x, 100.0 * std::sin(x / 40.0) + 3.0 * std::sin(x * 1.7)});
    }
    const double tolerance = 0.8;

    const auto out = simplifyPolyline(curve, tolerance);

    EXPECT_LT(out.size(), curve.size());
    for (const auto& p : curve) {
        EXPECT_LE(distanceToPolyline(p, out), tolerance + 1e-9);
    }
}

TEST(SimplifyPolyline, HandlesHugeSmoothSeriesWithoutDeepRecursion) {
    std::vector<Point> curve;
    for (int i = 0; i < 200000; ++i) {
        curve.push_back({static_cast<double>(i), std::sqrt(static_cast<double>(i))});
    }

    const auto out = simplifyPolyline(curve, 0.01);

    EXPECT_GE(out.size(), 2u);
    EXPECT_LT(out.size(), curve.size());
}
