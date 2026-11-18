#include <gtest/gtest.h>
#include <zlib.h>

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "energy/PdfChart.hpp"

using energy::Measurement;
using energy::PdfOptions;
using energy::renderChartPdf;

namespace {

// A day of "solar" data sampled every `stepSeconds`, with a little
// deterministic noise (same numbers on every run).
std::vector<Measurement> makeSeries(int count, int stepSeconds) {
    std::vector<Measurement> rows;
    unsigned seed = 12345;
    for (int i = 0; i < count; ++i) {
        seed = seed * 1664525u + 1013904223u;
        const double noise = (static_cast<double>(seed >> 16) / 65536.0 - 0.5) * 0.4;
        const double hour = std::fmod(i * stepSeconds / 3600.0, 24.0);
        const double sun = std::sin(3.14159265358979 * (hour - 6.0) / 12.0);
        rows.push_back({"s", 1700000000LL + static_cast<long long>(i) * stepSeconds,
                        120.0 * std::max(0.0, sun) + noise});
    }
    return rows;
}

std::string inflateBytes(const std::string& packed) {
    uLongf size = static_cast<uLongf>(packed.size()) * 200 + 1024;
    std::string out(size, '\0');
    const int rc = uncompress(reinterpret_cast<Bytef*>(out.data()), &size,
                              reinterpret_cast<const Bytef*>(packed.data()),
                              static_cast<uLong>(packed.size()));
    EXPECT_EQ(rc, Z_OK);
    out.resize(size);
    return out;
}

// Returns the raw bytes between "stream\n" and "\nendstream", using the
// declared /Length (so a wrong /Length makes the test fail).
std::string extractStream(const std::string& pdf) {
    const auto lengthPos = pdf.find("/Length ");
    EXPECT_NE(lengthPos, std::string::npos);
    const std::size_t length = std::stoul(pdf.substr(lengthPos + 8));
    const auto start = pdf.find("stream\n", lengthPos) + 7;
    EXPECT_EQ(pdf.compare(start + length, 10, "\nendstream"), 0)
        << "/Length does not match the stream size";
    return pdf.substr(start, length);
}

}  // namespace

TEST(PdfChart, HasPdfHeaderAndEndMarker) {
    const auto pdf = renderChartPdf(makeSeries(48, 3600), "site", PdfOptions::optimized());

    EXPECT_EQ(pdf.rfind("%PDF-1.4\n", 0), 0u);
    EXPECT_EQ(pdf.substr(pdf.size() - 6), "%%EOF\n");
}

TEST(PdfChart, CrossReferenceOffsetsPointAtTheObjects) {
    const auto pdf = renderChartPdf(makeSeries(48, 3600), "site", PdfOptions::optimized());

    const auto startxref = pdf.rfind("startxref\n");
    ASSERT_NE(startxref, std::string::npos);
    const std::size_t xrefOffset = std::stoul(pdf.substr(startxref + 10));
    ASSERT_EQ(pdf.compare(xrefOffset, 5, "xref\n"), 0);

    // Entries start after "xref\n0 6\n" and the free entry, 20 bytes each.
    const std::size_t firstEntry = xrefOffset + std::string("xref\n0 6\n").size() + 20;
    for (int object = 1; object <= 5; ++object) {
        const std::size_t offset =
            std::stoul(pdf.substr(firstEntry + (object - 1) * 20, 10));
        const std::string expected = std::to_string(object) + " 0 obj";
        EXPECT_EQ(pdf.compare(offset, expected.size(), expected), 0)
            << "object " << object;
    }
}

TEST(PdfChart, CompressedContentInflatesToDrawingOperators) {
    const auto pdf = renderChartPdf(makeSeries(48, 3600), "site", PdfOptions::optimized());

    const std::string content = inflateBytes(extractStream(pdf));

    EXPECT_NE(pdf.find("/FlateDecode"), std::string::npos);
    EXPECT_NE(content.find(" m\n"), std::string::npos);
    EXPECT_NE(content.find(" l\n"), std::string::npos);
    EXPECT_NE(content.find("Puissance"), std::string::npos);
}

TEST(PdfChart, NaiveContentIsStoredAsPlainText) {
    const auto pdf = renderChartPdf(makeSeries(48, 3600), "site", PdfOptions::naive());

    EXPECT_EQ(pdf.find("/FlateDecode"), std::string::npos);
    EXPECT_NE(pdf.find("Puissance"), std::string::npos);
    EXPECT_EQ(extractStream(pdf).rfind("0.8 0.8 0.8 RG", 0), 0u);
}

TEST(PdfChart, EmptyInputStillProducesAPageSayingNoData) {
    const auto pdf = renderChartPdf({}, "site", PdfOptions::naive());

    EXPECT_NE(pdf.find("No data"), std::string::npos);
    EXPECT_EQ(pdf.substr(pdf.size() - 6), "%%EOF\n");
}

TEST(PdfChart, TitleIsEscapedAndReducedToAscii) {
    const auto pdf = renderChartPdf({}, "a(b)\\\xC3\xA9", PdfOptions::naive());

    // "(" ")" "\" are backslash-escaped; the 2 UTF-8 bytes become "??".
    EXPECT_NE(pdf.find("a\\(b\\)\\\\??"), std::string::npos);
}

TEST(PdfChart, SinglePointDoesNotCrashAndDrawsNothingInvalid) {
    const auto pdf = renderChartPdf({{"s", 100, 5.0}}, "site", PdfOptions::naive());

    EXPECT_EQ(pdf.find("nan"), std::string::npos);
    EXPECT_EQ(pdf.find("inf"), std::string::npos);
}

TEST(PdfChart, OptimizedFileIsMuchSmallerThanNaiveOnALargeSeries) {
    // 7 days, one measurement every 30 seconds.
    const auto rows = makeSeries(7 * 24 * 120, 30);

    const auto naive = renderChartPdf(rows, "site", PdfOptions::naive()).size();
    const auto optimized = renderChartPdf(rows, "site", PdfOptions::optimized()).size();

    std::printf("[ size ] %zu points: naive %zu bytes, optimized %zu bytes (%.1f%%)\n",
                rows.size(), naive, optimized, 100.0 * optimized / naive);
    EXPECT_LT(optimized * 10, naive);  // at least 10x lighter
}

TEST(PdfChart, EachLeverMakesTheFileNoHeavier) {
    const auto rows = makeSeries(7 * 24 * 120, 30);

    // Levers are switched on one after the other, cumulatively.
    PdfOptions step = PdfOptions::naive();
    std::vector<std::pair<std::string, PdfOptions>> steps{{"naive", step}};
    step.strokePerSegment = false;
    steps.emplace_back("+ one path for the whole curve", step);
    step.decimals = 2;
    steps.emplace_back("+ 2 decimals instead of 6", step);
    step.simplifyTolerancePt = 0.1;
    steps.emplace_back("+ simplify (0.1 pt)", step);
    step.compress = true;
    steps.emplace_back("+ Flate compression", step);

    std::size_t previous = static_cast<std::size_t>(-1);
    for (const auto& [name, options] : steps) {
        const auto size = renderChartPdf(rows, "site", options).size();
        std::printf("[ lever ] %-34s %9zu bytes\n", name.c_str(), size);
        EXPECT_LE(size, previous) << name;
        previous = size;
    }
}

TEST(PdfChart, SimplificationAloneRemovesMostPointsOfASmoothCurve) {
    const auto rows = makeSeries(7 * 24 * 120, 30);
    PdfOptions onlySimplify = PdfOptions::naive();
    onlySimplify.simplifyTolerancePt = 0.5;

    const auto plain = renderChartPdf(rows, "site", PdfOptions::naive()).size();
    const auto simplified = renderChartPdf(rows, "site", onlySimplify).size();

    EXPECT_LT(simplified, plain);
}
