#pragma once

#include <string>

namespace energy {

// Wraps a page content stream (PDF drawing operators) into a complete,
// valid one-page PDF 1.4 file: header, objects, cross-reference table and
// trailer. Text uses the built-in Helvetica font, so no font is embedded
// (embedding would add tens of kilobytes).
//
// compress = true stores the content stream with Flate (zlib) compression.
// Throws std::runtime_error if compression fails.
std::string buildSinglePagePdf(const std::string& contentStream, double pageWidth,
                               double pageHeight, bool compress);

}  // namespace energy
