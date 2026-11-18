// Tests for the new object API and for improvements that the refactoring
// brings (these cases could not be tested safely on the old code).

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "legacy_report.h"
#include "report_builder.hpp"

using energy::legacy::ReportBuilder;

TEST(ReportBuilder, AccumulatesAcrossAddCsvCalls) {
    ReportBuilder builder;
    builder.addCsv("a,0,10\n");
    builder.addCsv("a,3600,20\n");

    EXPECT_EQ(builder.format(), "a: n=2 min=10.00 max=20.00 avg=15.00 kwh=10.00\n");
}

TEST(ReportBuilder, TwoBuildersDoNotShareState) {
    ReportBuilder first;
    ReportBuilder second;
    first.addCsv("a,0,10\n");

    EXPECT_EQ(second.format(), "");
}

TEST(ReportBuilder, SiteLinesAreOneStringPerSite) {
    ReportBuilder builder;
    builder.addCsv("b,0,1\na,0,2\n");

    const auto lines = builder.siteLines();

    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[0].rfind("b:", 0), 0u);
    EXPECT_EQ(lines[1].rfind("a:", 0), 0u);
}

TEST(ReportBuilder, ResetForgetsEverything) {
    ReportBuilder builder;
    builder.addCsv("a,0,10\n");
    builder.reset();

    EXPECT_EQ(builder.format(), "");
}

// The old code formatted into a 512-byte buffer with sprintf: values this
// large wrote past its end. Now the line is sized to fit.
TEST(ReportBuilder, VeryLargeValuesDoNotOverflowAnyBuffer) {
    ReportBuilder builder;
    builder.addCsv("a,0,1e300\na,3600,-1e300\n");

    const std::string text = builder.format();

    EXPECT_EQ(text.rfind("a: n=2 min=-1000", 0), 0u);
    EXPECT_GT(text.size(), 512u);  // longer than the old fixed buffer
    EXPECT_EQ(text.back(), '\n');
}

// A site name is data, never a format string.
TEST(ReportBuilder, PercentSignsInSiteNamesArePrintedAsIs) {
    ReportBuilder builder;
    builder.addCsv("%s%d%n,0,1\n");

    EXPECT_EQ(builder.format().rfind("%s%d%n: n=1", 0), 0u);
}

// The old code stored times in a `long`, which is 32 bits on Windows and
// overflows in 2038.
TEST(ReportBuilder, TimestampsBeyondYear2038AreHandled) {
    ReportBuilder builder;
    builder.addCsv("a,0,10\na,4000000000,10\n");

    EXPECT_NE(builder.format().find("kwh=11111111.11"), std::string::npos);
}

TEST(LegacyReportCApi, NullInputIsTreatedAsEmpty) {
    lr_reset();
    char out[16] = {'X'};

    EXPECT_EQ(lr_compute_report(nullptr, out, sizeof out), 0);
    EXPECT_STREQ(out, "");
}

TEST(LegacyReportCApi, NullOutputIsRefused) {
    EXPECT_EQ(lr_compute_report("a,0,1\n", nullptr, 100), -1);
}
