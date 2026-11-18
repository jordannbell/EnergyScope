// Command-line PDF export: fetches a site from the server and writes a chart.
//
// Usage: energy_export <site> <output.pdf> [--naive]
//   --naive  produce the heavy "careless" version, to compare file sizes.

#include <cstring>
#include <iostream>
#include <string>

#include "energy/EnergyClient.hpp"
#include "energy/FileIo.hpp"
#include "energy/PdfChart.hpp"

namespace {

constexpr const char* kServerHost = "127.0.0.1";
constexpr unsigned short kServerPort = 5555;

int usage() {
    std::cerr << "usage: energy_export <site> <output.pdf> [--naive]\n";
    return 2;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3 || argc > 4) return usage();
    const std::string site = argv[1];
    const std::string outputPath = argv[2];
    bool naive = false;
    if (argc == 4) {
        if (std::strcmp(argv[3], "--naive") != 0) return usage();
        naive = true;
    }

    try {
        energy::EnergyClient client(kServerHost, kServerPort);
        const auto rows = client.get(site);

        const auto options =
            naive ? energy::PdfOptions::naive() : energy::PdfOptions::optimized();
        const std::string pdf = energy::renderChartPdf(rows, site, options);
        energy::writeBinaryFile(outputPath, pdf);

        std::cout << rows.size() << " points -> " << outputPath << " ("
                  << pdf.size() << " bytes, "
                  << (naive ? "naive" : "optimized") << ")\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
