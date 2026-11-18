#pragma once

#include <cstdint>
#include <string>

namespace energy {

// One reading from a power site (e.g. a solar farm) at a given time.
struct Measurement {
    std::string site;          // site name, e.g. "Bordeaux-Solar-1"
    std::int64_t timestamp;    // seconds since 1970-01-01 (Unix time)
    double powerKw;            // produced power in kilowatts
};

}  // namespace energy
