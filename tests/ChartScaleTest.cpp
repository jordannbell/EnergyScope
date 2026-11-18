#include <gtest/gtest.h>

#include <limits>

#include "energy/ChartScale.hpp"

using energy::computeBounds;
using energy::niceCeil;

TEST(NiceCeil, RoundsUpToOneTwoFiveTen) {
    EXPECT_DOUBLE_EQ(niceCeil(7), 10);
    EXPECT_DOUBLE_EQ(niceCeil(12.5), 20);
    EXPECT_DOUBLE_EQ(niceCeil(120), 200);
    EXPECT_DOUBLE_EQ(niceCeil(300), 500);
    EXPECT_DOUBLE_EQ(niceCeil(0.3), 0.5);
}

TEST(NiceCeil, KeepsValuesThatAreAlreadyRound) {
    EXPECT_DOUBLE_EQ(niceCeil(1), 1);
    EXPECT_DOUBLE_EQ(niceCeil(100), 100);
    EXPECT_DOUBLE_EQ(niceCeil(5), 5);
}

TEST(NiceCeil, ReturnsOneForNonPositiveOrInvalidInput) {
    EXPECT_DOUBLE_EQ(niceCeil(0), 1);
    EXPECT_DOUBLE_EQ(niceCeil(-4), 1);
    EXPECT_DOUBLE_EQ(niceCeil(std::numeric_limits<double>::quiet_NaN()), 1);
    EXPECT_DOUBLE_EQ(niceCeil(std::numeric_limits<double>::infinity()), 1);
}

TEST(ComputeBounds, EmptyInputGivesDefaultRange) {
    const auto b = computeBounds({});

    EXPECT_EQ(b.tMin, 0);
    EXPECT_EQ(b.tMax, 1);
    EXPECT_DOUBLE_EQ(b.yMin, 0);
    EXPECT_DOUBLE_EQ(b.yMax, 1);
}

TEST(ComputeBounds, FitsTimeRangeAndNicePowerCeiling) {
    const auto b = computeBounds({{"s", 100, 10}, {"s", 400, 137}, {"s", 250, 50}});

    EXPECT_EQ(b.tMin, 100);
    EXPECT_EQ(b.tMax, 400);
    EXPECT_DOUBLE_EQ(b.yMin, 0);
    EXPECT_DOUBLE_EQ(b.yMax, 200);
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

    EXPECT_LT(b.yMin, b.yMax);
}

TEST(ComputeBounds, NegativePowerExtendsAxisBelowZero) {
    const auto b = computeBounds({{"s", 1, -30}, {"s", 2, 80}});

    EXPECT_DOUBLE_EQ(b.yMin, -50);
    EXPECT_DOUBLE_EQ(b.yMax, 100);
}
