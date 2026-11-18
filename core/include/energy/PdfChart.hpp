#pragma once

#include <string>
#include <vector>

#include "energy/Measurement.hpp"

namespace energy {

// How the PDF is produced. Each field is one "weight" lever.
struct PdfOptions {
    // Curve points closer than this (in PDF points, 1/72 inch) to the line
    // between their neighbours are dropped. 0 keeps every point.
    double simplifyTolerancePt;
    // Digits kept after the decimal point in coordinates.
    int decimals;
    // Flate-compress the page content.
    bool compress;
    // Legacy style: every segment is its own path with its own colour and
    // width instructions. Much heavier than one path for the whole curve.
    bool strokePerSegment;

    // What a careless generator does: heavy file, same picture.
    static PdfOptions naive() { return {0.0, 6, false, true}; }
    // What EnergyScope uses: visually identical, much lighter.
    static PdfOptions optimized() { return {0.1, 2, true, false}; }
};

// One-page A4 landscape vector chart of power over time (times in UTC).
// `title` is reduced to printable ASCII (the built-in font has no Unicode).
// Never fails on empty input: the page then shows "No data".
std::string renderChartPdf(const std::vector<Measurement>& rows,
                           const std::string& title, const PdfOptions& options);

}  // namespace energy
