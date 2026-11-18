#pragma once

#include <charconv>
#include <cmath>
#include <cstdint>
#include <string>
#include <system_error>

namespace energy {

// Strict number parsing: the whole text must be a number (no trailing junk).

inline bool parseInt64(const std::string& s, std::int64_t& out) {
    const char* end = s.data() + s.size();
    const auto r = std::from_chars(s.data(), end, out);
    return r.ec == std::errc() && r.ptr == end;
}

// Rejects nan and inf.
inline bool parseDouble(const std::string& s, double& out) {
    const char* end = s.data() + s.size();
    const auto r = std::from_chars(s.data(), end, out);
    return r.ec == std::errc() && r.ptr == end && std::isfinite(out);
}

// Fixed notation with at most `decimals` digits, trailing zeros removed:
// (12.5000, 4) -> "12.5", (3.0, 2) -> "3", (-0.0001, 2) -> "0".
inline std::string formatFixed(double v, int decimals) {
    char buf[400];
    const auto r = std::to_chars(buf, buf + sizeof buf, v,
                                 std::chars_format::fixed, decimals);
    std::string s(buf, r.ptr);
    if (s.find('.') != std::string::npos) {
        while (s.back() == '0') s.pop_back();
        if (s.back() == '.') s.pop_back();
    }
    return s == "-0" ? "0" : s;
}

// Shortest text that reads back as exactly the same double.
inline std::string formatDouble(double v) {
    char buf[64];
    const auto r = std::to_chars(buf, buf + sizeof buf, v);
    return std::string(buf, r.ptr);
}

}  // namespace energy
