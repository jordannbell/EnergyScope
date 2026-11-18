#include "energy/PdfChart.hpp"

#include <algorithm>
#include <cstddef>

#include "energy/ChartScale.hpp"
#include "energy/Geometry.hpp"
#include "energy/Parse.hpp"
#include "energy/PdfWriter.hpp"
#include "energy/UtcTime.hpp"

namespace energy {

namespace {

// A4 landscape, in PDF points (1 pt = 1/72 inch).
constexpr double kPageWidth = 842.0;
constexpr double kPageHeight = 595.0;
constexpr double kMarginLeft = 70.0;
constexpr double kMarginRight = 30.0;
constexpr double kMarginTop = 50.0;
constexpr double kMarginBottom = 60.0;

constexpr int kGridDivisions = kPowerDivisions;  // same grid on both axes
constexpr double kFontSize = 9.0;
constexpr double kGlyphWidthEm = 0.556;  // Helvetica: most digits/letters
constexpr double kLabelGap = 6.0;
constexpr int kLabelDecimals = 2;

constexpr const char* kGridColor = "0.8 0.8 0.8 RG";
constexpr const char* kCurveColor = "1 0.62 0.15 RG";
constexpr const char* kTextColor = "0.2 0.2 0.2 rg";
constexpr const char* kCurveWidth = "1.2 w";

struct PlotRect {
    double left;
    double bottom;
    double width;
    double height;
};

std::string asciiOnly(const std::string& text) {
    std::string out;
    for (const unsigned char c : text) {
        out += (c >= 32 && c < 127) ? static_cast<char>(c) : '?';
    }
    return out;
}

std::string escapePdfText(const std::string& text) {
    std::string out;
    for (const char c : text) {
        if (c == '(' || c == ')' || c == '\\') out += '\\';
        out += c;
    }
    return out;
}

// Rough width of a text in Helvetica; good enough to right-align / center.
double approxTextWidth(const std::string& text) {
    return static_cast<double>(text.size()) * kGlyphWidthEm * kFontSize;
}

class Content {
public:
    explicit Content(int decimals) : decimals_(decimals) {}

    std::string num(double v) const { return formatFixed(v, decimals_); }
    void raw(const std::string& s) { out_ += s; }
    void line(const std::string& s) { out_ += s + "\n"; }

    void moveTo(double x, double y) { line(num(x) + " " + num(y) + " m"); }
    void lineTo(double x, double y) { line(num(x) + " " + num(y) + " l"); }

    void text(double x, double y, const std::string& s) {
        line("BT /F1 " + formatFixed(kFontSize, 1) + " Tf " + num(x) + " " +
             num(y) + " Td (" + escapePdfText(asciiOnly(s)) + ") Tj ET");
    }

    const std::string& str() const { return out_; }

private:
    int decimals_;
    std::string out_;
};

void drawGrid(Content& c, const PlotRect& r) {
    c.line(kGridColor);
    c.line("0.5 w");
    for (int i = 0; i <= kGridDivisions; ++i) {
        const double f = static_cast<double>(i) / kGridDivisions;
        c.moveTo(r.left, r.bottom + r.height * f);
        c.lineTo(r.left + r.width, r.bottom + r.height * f);
        c.moveTo(r.left + r.width * f, r.bottom);
        c.lineTo(r.left + r.width * f, r.bottom + r.height);
    }
    c.line("S");  // one stroke for the whole grid
}

void drawCurve(Content& c, const std::vector<Point>& pts,
               const PdfOptions& options) {
    if (pts.empty()) return;
    if (options.strokePerSegment) {
        for (std::size_t i = 1; i < pts.size(); ++i) {
            c.line(kCurveColor);
            c.line(kCurveWidth);
            c.moveTo(pts[i - 1].x, pts[i - 1].y);
            c.lineTo(pts[i].x, pts[i].y);
            c.line("S");
        }
        return;
    }
    c.line(kCurveColor);
    c.line(kCurveWidth);
    c.moveTo(pts.front().x, pts.front().y);
    for (std::size_t i = 1; i < pts.size(); ++i) c.lineTo(pts[i].x, pts[i].y);
    c.line("S");
}

void drawLabels(Content& c, const PlotRect& r, const ChartBounds& b,
                const std::string& title) {
    c.line(kTextColor);
    c.text(r.left, r.bottom + r.height + kLabelGap + 4.0,
           title + "  -  Puissance (kW), heures en UTC");

    for (int i = 0; i <= kGridDivisions; ++i) {  // power axis
        const double f = static_cast<double>(i) / kGridDivisions;
        const std::string label =
            formatFixed(b.yMin + (b.yMax - b.yMin) * f, kLabelDecimals);
        c.text(r.left - kLabelGap - approxTextWidth(label),
               r.bottom + r.height * f - kFontSize / 3.0, label);
    }
    const int step = kGridDivisions / 2;
    for (int i = 0; i <= kGridDivisions; i += step) {  // time axis
        const double f = static_cast<double>(i) / kGridDivisions;
        const auto t = b.tMin + static_cast<std::int64_t>(
                                    static_cast<double>(b.tMax - b.tMin) * f);
        const std::string label = formatUtcDayMinute(t);
        c.text(r.left + r.width * f - approxTextWidth(label) / 2.0,
               r.bottom - kFontSize - kLabelGap, label);
    }
}

std::vector<Point> toPagePoints(const std::vector<Measurement>& rows,
                                const ChartBounds& b, const PlotRect& r) {
    const double tSpan = static_cast<double>(b.tMax - b.tMin);
    const double ySpan = b.yMax - b.yMin;

    std::vector<Point> pts;
    pts.reserve(rows.size());
    for (const auto& m : rows) {
        pts.push_back({r.left + static_cast<double>(m.timestamp - b.tMin) / tSpan * r.width,
                       r.bottom + (m.powerKw - b.yMin) / ySpan * r.height});
    }
    return pts;
}

}  // namespace

std::string renderChartPdf(const std::vector<Measurement>& rows,
                           const std::string& title, const PdfOptions& options) {
    const PlotRect plot{kMarginLeft, kMarginBottom,
                        kPageWidth - kMarginLeft - kMarginRight,
                        kPageHeight - kMarginTop - kMarginBottom};
    const ChartBounds bounds = computeBounds(rows);

    Content content(options.decimals);
    drawGrid(content, plot);
    drawCurve(content,
              simplifyPolyline(toPagePoints(rows, bounds, plot),
                               options.simplifyTolerancePt),
              options);
    drawLabels(content, plot, bounds, title);
    if (rows.empty()) {
        content.text(plot.left + plot.width / 2.0 - approxTextWidth("No data") / 2.0,
                     plot.bottom + plot.height / 2.0, "No data");
    }
    return buildSinglePagePdf(content.str(), kPageWidth, kPageHeight,
                              options.compress);
}

}  // namespace energy
