#include "energy/PdfWriter.hpp"

#include <zlib.h>

#include <cstdio>
#include <stdexcept>
#include <vector>

#include "energy/Parse.hpp"

namespace energy {

namespace {

constexpr int kCompressionLevel = 9;  // smallest output; fine for one page

std::string deflateBytes(const std::string& input) {
    uLongf size = compressBound(static_cast<uLong>(input.size()));
    std::string out(size, '\0');
    const int rc = compress2(reinterpret_cast<Bytef*>(out.data()), &size,
                             reinterpret_cast<const Bytef*>(input.data()),
                             static_cast<uLong>(input.size()), kCompressionLevel);
    if (rc != Z_OK) throw std::runtime_error("PDF compression failed");
    out.resize(size);
    return out;
}

std::string streamObject(const std::string& data, bool compressed) {
    std::string body = "<< /Length " + std::to_string(data.size());
    if (compressed) body += " /Filter /FlateDecode";
    body += " >>\nstream\n" + data + "\nendstream";
    return body;
}

}  // namespace

std::string buildSinglePagePdf(const std::string& contentStream, double pageWidth,
                               double pageHeight, bool compress) {
    const std::string mediaBox = "[0 0 " + formatFixed(pageWidth, 2) + " " +
                                 formatFixed(pageHeight, 2) + "]";

    // Object numbers are 1-based and match the order of this list.
    const std::vector<std::string> objects = {
        "<< /Type /Catalog /Pages 2 0 R >>",
        "<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        "<< /Type /Page /Parent 2 0 R /MediaBox " + mediaBox +
            " /Contents 4 0 R /Resources << /Font << /F1 5 0 R >> >> >>",
        streamObject(compress ? deflateBytes(contentStream) : contentStream,
                     compress),
        "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica"
        " /Encoding /WinAnsiEncoding >>",
    };

    // The second line holds high bytes so tools treat the file as binary.
    std::string pdf = "%PDF-1.4\n%\xE2\xE3\xCF\xD3\n";
    std::vector<std::size_t> offsets;
    for (std::size_t i = 0; i < objects.size(); ++i) {
        offsets.push_back(pdf.size());  // where "N 0 obj" starts
        pdf += std::to_string(i + 1) + " 0 obj\n" + objects[i] + "\nendobj\n";
    }

    const std::size_t xrefOffset = pdf.size();
    pdf += "xref\n0 " + std::to_string(objects.size() + 1) + "\n";
    pdf += "0000000000 65535 f \n";  // each entry is exactly 20 bytes
    for (const std::size_t offset : offsets) {
        char entry[32];
        std::snprintf(entry, sizeof entry, "%010zu 00000 n \n", offset);
        pdf += entry;
    }
    pdf += "trailer\n<< /Size " + std::to_string(objects.size() + 1) +
           " /Root 1 0 R >>\nstartxref\n" + std::to_string(xrefOffset) +
           "\n%%EOF\n";
    return pdf;
}

}  // namespace energy
