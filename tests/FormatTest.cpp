#include <gtest/gtest.h>

#include "energy/Parse.hpp"
#include "energy/UtcTime.hpp"

TEST(FormatFixed, TrimsTrailingZerosAndDot) {
    EXPECT_EQ(energy::formatFixed(12.5, 4), "12.5");
    EXPECT_EQ(energy::formatFixed(3.0, 2), "3");
    EXPECT_EQ(energy::formatFixed(100.0, 2), "100");
}

TEST(FormatFixed, RoundsToRequestedDecimals) {
    EXPECT_EQ(energy::formatFixed(1234.5678, 2), "1234.57");
    EXPECT_EQ(energy::formatFixed(0.123456789, 6), "0.123457");
}

TEST(FormatFixed, NegativeValuesThatRoundToZeroPrintAsZero) {
    EXPECT_EQ(energy::formatFixed(-0.0001, 2), "0");
    EXPECT_EQ(energy::formatFixed(-2.5, 2), "-2.5");
}

TEST(FormatUtcDayMinute, EpochStart) {
    EXPECT_EQ(energy::formatUtcDayMinute(0), "01/01 00:00");
}

TEST(FormatUtcDayMinute, ArbitraryInstant) {
    EXPECT_EQ(energy::formatUtcDayMinute(1700000000), "14/11 22:13");
}

TEST(FormatUtcDayMinute, LeapDay) {
    EXPECT_EQ(energy::formatUtcDayMinute(1709208000), "29/02 12:00");
}

TEST(FormatUtcDayMinute, BeforeEpochCountsBackwards) {
    EXPECT_EQ(energy::formatUtcDayMinute(-1), "31/12 23:59");
}
