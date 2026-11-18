// Characterization tests for the legacy report module.
//
// They pin down what the code does TODAY, including behaviour that looks
// like a bug. Their job is to make refactoring safe: if a test fails after a
// change, the behaviour changed. Quirks are marked "QUIRK".

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "legacy_report.h"

namespace {

std::string report(const std::string& csv, int bufferSize = 4096) {
    std::vector<char> out(static_cast<std::size_t>(bufferSize), 'X');
    const int length = lr_compute_report(csv.c_str(), out.data(), bufferSize);
    EXPECT_EQ(length, static_cast<int>(std::string(out.data()).size()));
    return std::string(out.data());
}

struct LegacyReport : testing::Test {
    void SetUp() override { lr_reset(); }
};

}  // namespace

TEST_F(LegacyReport, ComputesCountMinMaxAverageAndEnergyForOneSite) {
    EXPECT_EQ(report("a,0,10\na,3600,20\n"),
              "a: n=2 min=10.00 max=20.00 avg=15.00 kwh=10.00\n");
}

TEST_F(LegacyReport, ListsSitesInOrderOfFirstAppearance) {
    EXPECT_EQ(report("b,0,1\na,0,2\nb,3600,1\n"),
              "b: n=2 min=1.00 max=1.00 avg=1.00 kwh=1.00\n"
              "a: n=1 min=2.00 max=2.00 avg=2.00 kwh=0.00\n");
}

TEST_F(LegacyReport, EnergyUsesTheLeftRectangleRuleNotTheTrapezoid) {
    // Each interval counts the power of its FIRST sample: 0*1h + 100*1h.
    // A trapezoid rule would give 150.
    EXPECT_EQ(report("a,0,0\na,3600,100\na,7200,100\n"),
              "a: n=3 min=0.00 max=100.00 avg=66.67 kwh=100.00\n");
}

TEST_F(LegacyReport, EmptyInputGivesEmptyReport) {
    EXPECT_EQ(report(""), "");
}

TEST_F(LegacyReport, ReturnsMinusOneWhenOutputBufferHasNoRoom) {
    char out[1] = {'X'};
    EXPECT_EQ(lr_compute_report("a,0,1\n", out, 0), -1);
}

TEST_F(LegacyReport, SkipsLinesThatDoNotHaveTwoCommas) {
    EXPECT_EQ(report("garbage\nx,1\nx,0,5\n\n"),
              "x: n=1 min=5.00 max=5.00 avg=5.00 kwh=0.00\n");
}

TEST_F(LegacyReport, HandlesWindowsLineEndings) {
    EXPECT_EQ(report("a,0,10\r\na,3600,20\r\n"),
              "a: n=2 min=10.00 max=20.00 avg=15.00 kwh=10.00\n");
}

// QUIRK: a CSV header is not recognised; "ts" and "kw" parse as 0.
TEST_F(LegacyReport, QuirkHeaderLineIsCountedAsAMeasurement) {
    EXPECT_EQ(report("site,ts,kw\nx,0,5\n"),
              "site: n=1 min=0.00 max=0.00 avg=0.00 kwh=0.00\n"
              "x: n=1 min=5.00 max=5.00 avg=5.00 kwh=0.00\n");
}

// QUIRK: site names longer than 31 characters are cut, so different long
// names that share their first 31 characters are merged into one site.
TEST_F(LegacyReport, QuirkSiteNamesAreCutAt31Characters) {
    const std::string prefix(31, 'n');
    EXPECT_EQ(report(prefix + "A,0,1\n" + prefix + "B,3600,3\n"),
              prefix + ": n=2 min=1.00 max=3.00 avg=2.00 kwh=1.00\n");
}

// QUIRK: only the first 16 sites are kept, the others vanish silently.
TEST_F(LegacyReport, QuirkSitesBeyondTheSixteenthAreIgnored) {
    std::string csv;
    for (int i = 0; i < 17; ++i) csv += "s" + std::to_string(i) + ",0,1\n";

    const std::string result = report(csv);

    EXPECT_NE(result.find("s15:"), std::string::npos);
    EXPECT_EQ(result.find("s16:"), std::string::npos);
}

// QUIRK: samples out of time order produce negative energy.
TEST_F(LegacyReport, QuirkUnsortedTimestampsGiveNegativeEnergy) {
    EXPECT_EQ(report("a,3600,10\na,0,10\n"),
              "a: n=2 min=10.00 max=10.00 avg=10.00 kwh=-10.00\n");
}

// QUIRK: input is cut at 65535 characters, even in the middle of a line.
TEST_F(LegacyReport, QuirkInputBeyond65535CharactersIsTruncated) {
    std::string csv;
    for (int i = 0; i < 20000; ++i) csv += "a,1,1\n";  // 6 characters per line

    // 65535 / 6 = 10922 whole lines, then "a,1" which has no second comma.
    EXPECT_EQ(report(csv), "a: n=10922 min=1.00 max=1.00 avg=1.00 kwh=0.00\n");
}

// QUIRK: a site line that does not fit is dropped, but later shorter lines
// may still fit.
TEST_F(LegacyReport, QuirkLinesThatDoNotFitTheBufferAreSilentlyDropped) {
    const std::string csv = "a,0,1\nlonger-name,0,1\nb,0,1\n";

    // "a: ..." is 43 characters; the long line does not fit in 60, "b" does
    // not either (43 + 43 > 60), so only the first line survives.
    EXPECT_EQ(report(csv, 60), "a: n=1 min=1.00 max=1.00 avg=1.00 kwh=0.00\n");
}

// FIXED (was a QUIRK): totals used to accumulate across calls because the
// state was global. This test was changed on purpose in the commit that
// fixed it; it used to expect "n=2 ... kwh=10.00".
TEST_F(LegacyReport, EachCallStartsFromScratch) {
    report("a,0,10\n");

    EXPECT_EQ(report("a,3600,20\n"),
              "a: n=1 min=20.00 max=20.00 avg=20.00 kwh=0.00\n");
}

TEST_F(LegacyReport, ResetIsHarmless) {
    report("a,0,10\n");
    lr_reset();

    EXPECT_EQ(report("a,3600,20\n"),
              "a: n=1 min=20.00 max=20.00 avg=20.00 kwh=0.00\n");
}
