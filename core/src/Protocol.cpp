#include "energy/Protocol.hpp"

#include <charconv>
#include <cmath>
#include <cstdint>
#include <exception>
#include <sstream>
#include <vector>

namespace energy {

namespace {

constexpr std::size_t kMaxSiteLength = 64;

std::string err(const std::string& reason) { return "ERR " + reason + "\n"; }

std::vector<std::string> splitWords(const std::string& line) {
    std::istringstream in(line);
    std::vector<std::string> words;
    std::string w;
    while (in >> w) {
        words.push_back(w);
    }
    return words;
}

bool parseInt64(const std::string& s, std::int64_t& out) {
    const char* end = s.data() + s.size();
    const auto r = std::from_chars(s.data(), end, out);
    return r.ec == std::errc() && r.ptr == end;
}

bool parseDouble(const std::string& s, double& out) {
    const char* end = s.data() + s.size();
    const auto r = std::from_chars(s.data(), end, out);
    return r.ec == std::errc() && r.ptr == end && std::isfinite(out);
}

bool validSite(const std::string& s) {
    return !s.empty() && s.size() <= kMaxSiteLength;
}

std::string handleAdd(MeasurementRepository& repo,
                      const std::vector<std::string>& words) {
    if (words.size() != 4) {
        return err("usage: ADD <site> <timestamp> <powerKw>");
    }
    Measurement m;
    m.site = words[1];
    if (!validSite(m.site)) {
        return err("invalid site");
    }
    if (!parseInt64(words[2], m.timestamp)) {
        return err("invalid timestamp");
    }
    if (!parseDouble(words[3], m.powerKw)) {
        return err("invalid power");
    }
    repo.insert(m);
    return "OK\n";
}

std::string handleGet(MeasurementRepository& repo,
                      const std::vector<std::string>& words) {
    if (words.size() != 2) {
        return err("usage: GET <site>");
    }
    if (!validSite(words[1])) {
        return err("invalid site");
    }
    const auto rows = repo.findBySite(words[1]);
    std::ostringstream out;
    out << "OK " << rows.size() << "\n";
    for (const auto& m : rows) {
        out << m.timestamp << " " << m.powerKw << "\n";
    }
    return out.str();
}

}  // namespace

std::string handleRequest(MeasurementRepository& repo,
                          const std::string& line) {
    try {
        const auto words = splitWords(line);
        if (words.empty()) {
            return err("empty request");
        }
        if (words[0] == "ADD") {
            return handleAdd(repo, words);
        }
        if (words[0] == "GET") {
            return handleGet(repo, words);
        }
        return err("unknown command");
    } catch (const std::exception&) {
        return err("internal error");
    }
}

}  // namespace energy
