#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace energy::legacy {

// Per-site statistics computed from CSV text "site,timestamp,kw".
//
// Replaces the global state and fixed buffers of the old C-style code with an
// ordinary object. The limits of the old code are kept on purpose so that
// behaviour does not change during the refactoring (see the tests).
class ReportBuilder {
public:
    static constexpr std::size_t kMaxSites = 16;           // extra sites ignored
    static constexpr std::size_t kMaxNameLength = 31;      // longer names are cut
    static constexpr std::size_t kMaxInputChars = 65535;   // longer input is cut

    // Adds every well-formed line of `csv`. Lines without two commas are
    // skipped. Can be called several times: statistics accumulate.
    void addCsv(std::string_view csv);

    // One text line per site, in order of first appearance, each ending in
    // '\n': "<site>: n=<count> min=<kw> max=<kw> avg=<kw> kwh=<energy>\n".
    std::vector<std::string> siteLines() const;

    // All site lines joined.
    std::string format() const;

    void reset();

private:
    struct SiteStats {
        std::string name;
        int count;
        double min;
        double max;
        double sum;
        double kwh;      // energy, left-rectangle rule
        long long lastTimestamp;
        double lastKw;
    };

    void addSample(const std::string& site, long long timestamp, double kw);

    std::vector<SiteStats> sites_;
};

}  // namespace energy::legacy
