#include <gtest/gtest.h>

#include <cmath>
#include <limits>

#include "energy/ChartScale.hpp"

using energy::computeBounds;
using energy::kPowerDivisions;

TEST(ComputeBounds, EmptyInputGivesDefaultRange) {
    const auto b = computeBounds({});

    EXPECT_EQ(b.tMin, 0);
    EXPECT_EQ(b.tMax, 1);
    EXPECT_DOUBLE_EQ(b.yMin, 0);
    EXPECT_DOUBLE_EQ(b.yMax, 1);
}

TEST(ComputeBounds, FitsTimeRangeAndRoundPowerCeiling) {
    const auto b = computeBounds({{"s", 100, 10}, {"s", 400, 137}, {"s", 250, 50}});

    EXPECT_EQ(b.tMin, 100);
    EXPECT_EQ(b.tMax, 400);
    EXPECT_DOUBLE_EQ(b.yMin, 0);
    EXPECT_DOUBLE_EQ(b.yMax, 200);  // 4 steps of 50
}

TEST(ComputeBounds, UsesTheTightestRoundStep) {
    // 4 steps of 25 reach 100, enough for 99.
    const auto b = computeBounds({{"s", 1, 99}});

    EXPECT_DOUBLE_EQ(b.yMax, 100);
}

TEST(ComputeBounds, ValueExactlyOnATickDoesNotGrowTheAxis) {
    EXPECT_DOUBLE_EQ(computeBounds({{"s", 1, 100}}).yMax, 100);
    EXPECT_DOUBLE_EQ(computeBounds({{"s", 1, 200}}).yMax, 200);
}

TEST(ComputeBounds, DoesNotRequireSortedInput) {
    const auto b = computeBounds({{"s", 400, 1}, {"s", 100, 1}});

    EXPECT_EQ(b.tMin, 100);
    EXPECT_EQ(b.tMax, 400);
}

TEST(ComputeBounds, SinglePointStillHasNonEmptyTimeAxis) {
    const auto b = computeBounds({{"s", 500, 5}});

    EXPECT_LT(b.tMin, b.tMax);
}

TEST(ComputeBounds, SinglePointAtLimitDoesNotOverflow) {
    const auto big = std::numeric_limits<std::int64_t>::max();
    const auto b = computeBounds({{"s", big, 5}});

    EXPECT_LT(b.tMin, b.tMax);
    EXPECT_EQ(b.tMax, big);
}

TEST(ComputeBounds, AllZeroPowerKeepsNonEmptyPowerAxis) {
    const auto b = computeBounds({{"s", 1, 0}, {"s", 2, 0}});

    EXPECT_DOUBLE_EQ(b.yMin, 0);
    EXPECT_DOUBLE_EQ(b.yMax, 1);
}

TEST(ComputeBounds, NegativePowerExtendsAxisBelowZeroOnARoundStep) {
    const auto b = computeBounds({{"s", 1, -30}, {"s", 2, 80}});

    EXPECT_DOUBLE_EQ(b.yMin, -50);
    EXPECT_DOUBLE_EQ(b.yMax, 150);
}

TEST(ComputeBounds, TinyNegativeNoiseKeepsRoundTicks) {
    // Regression: -0.99..120 once gave -1..200 with ticks like 49.25.
    const auto b = computeBounds({{"s", 1, -0.99}, {"s", 2, 120}});

    EXPECT_DOUBLE_EQ(b.yMin, -50);
    EXPECT_DOUBLE_EQ(b.yMax, 150);
}

TEST(ComputeBounds, SmallValuesGetSmallRoundSteps) {
    const auto b = computeBounds({{"s", 1, 0.3}});

    EXPECT_DOUBLE_EQ(b.yMin, 0);
    EXPECT_DOUBLE_EQ(b.yMax, 0.4);
}

TEST(ComputeBounds, AxisAlwaysContainsTheDataAndZeroAndHasRoundTicks) {
    const double samples[] = {0.001, 0.07, 1, 3.3, 24.9, 99, 137, 5000, 1.2e6};
    for (const double high : samples) {
        for (const double low : {0.0, -0.5, -high / 3.0, -high}) {
            const auto b = computeBounds({{"s", 1, low}, {"s", 2, high}});

            EXPECT_LE(b.yMin, std::min(low, 0.0));
            EXPECT_GE(b.yMax, std::max(high, 0.0));
            EXPECT_LE(b.yMin, 0.0);
            EXPECT_GE(b.yMax, 0.0);

            // 0 is on a tick: yMin is a whole number of steps below it.
            const double step = (b.yMax - b.yMin) / kPowerDivisions;
            const double stepsBelowZero = -b.yMin / step;
            EXPECT_NEAR(stepsBelowZero, std::round(stepsBelowZero), 1e-6)
                << "low=" << low << " high=" << high;
        }
    }
}
