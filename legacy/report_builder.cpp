#include "report_builder.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace energy::legacy {

namespace {

constexpr double kSecondsPerHour = 3600.0;

struct Sample {
    std::string site;
    long long timestamp;
    double kw;
};

// A line is "site,timestamp,kw". Numbers use the same lenient reading as the
// old atol/atof: unreadable text counts as 0, trailing junk is ignored.
bool parseLine(const std::string& line, Sample& sample) {
    const auto firstComma = line.find(',');
    if (firstComma == std::string::npos) return false;
    const auto secondComma = line.find(',', firstComma + 1);
    if (secondComma == std::string::npos) return false;

    sample.site = line.substr(
        0, std::min(firstComma, ReportBuilder::kMaxNameLength));
    sample.timestamp = std::strtoll(line.c_str() + firstComma + 1, nullptr, 10);
    sample.kw = std::strtod(line.c_str() + secondComma + 1, nullptr);
    return true;
}

}  // namespace

void ReportBuilder::addCsv(std::string_view csv) {
    csv = csv.substr(0, kMaxInputChars);

    std::size_t start = 0;
    while (start < csv.size()) {
        std::size_t end = csv.find('\n', start);
        if (end == std::string_view::npos) end = csv.size();

        if (end > start) {  // empty lines are skipped
            Sample sample;
            if (parseLine(std::string(csv.substr(start, end - start)), sample)) {
                addSample(sample.site, sample.timestamp, sample.kw);
            }
        }
        start = end + 1;
    }
}

void ReportBuilder::addSample(const std::string& site, long long timestamp,
                              double kw) {
    auto it = std::find_if(sites_.begin(), sites_.end(),
                           [&](const SiteStats& s) { return s.name == site; });
    if (it == sites_.end()) {
        if (sites_.size() >= kMaxSites) return;  // old behaviour: ignored
        sites_.push_back({site, 0, kw, kw, 0.0, 0.0, 0, 0.0});
        it = std::prev(sites_.end());
    }

    SiteStats& stats = *it;
    if (stats.count > 0) {
        const double seconds = static_cast<double>(timestamp - stats.lastTimestamp);
        stats.kwh += stats.lastKw * seconds / kSecondsPerHour;
    }
    ++stats.count;
    stats.min = std::min(stats.min, kw);
    stats.max = std::max(stats.max, kw);
    stats.sum += kw;
    stats.lastTimestamp = timestamp;
    stats.lastKw = kw;
}

std::vector<std::string> ReportBuilder::siteLines() const {
    constexpr const char* kFormat = "%s: n=%d min=%.2f max=%.2f avg=%.2f kwh=%.2f\n";

    std::vector<std::string> lines;
    lines.reserve(sites_.size());
    for (const SiteStats& s : sites_) {
        const double average = s.sum / s.count;
        // First call measures the length, second call writes: no fixed buffer.
        const int length = std::snprintf(nullptr, 0, kFormat, s.name.c_str(),
                                         s.count, s.min, s.max, average, s.kwh);
        if (length < 0) continue;
        std::string line(static_cast<std::size_t>(length), '\0');
        std::snprintf(line.data(), line.size() + 1, kFormat, s.name.c_str(),
                      s.count, s.min, s.max, average, s.kwh);
        lines.push_back(std::move(line));
    }
    return lines;
}

std::string ReportBuilder::format() const {
    std::string text;
    for (const std::string& line : siteLines()) text += line;
    return text;
}

void ReportBuilder::reset() { sites_.clear(); }

}  // namespace energy::legacy
