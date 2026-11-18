#pragma once

// Talks to energy_server. Keeps Winsock out of this header so MFC code can
// include it safely. All errors are reported as std::runtime_error
// (std::invalid_argument for bad site names).

#include <memory>
#include <string>
#include <vector>

#include "energy/Measurement.hpp"

namespace energy {

class EnergyClient {
public:
    // Connects immediately. `ipv4` is a numeric address like "127.0.0.1".
    EnergyClient(const std::string& ipv4, unsigned short port);
    ~EnergyClient();

    EnergyClient(const EnergyClient&) = delete;
    EnergyClient& operator=(const EnergyClient&) = delete;

    void add(const Measurement& measurement);
    std::vector<Measurement> get(const std::string& site);

    // A site name is valid if it is 1..64 chars with no whitespace or
    // control characters (they would break the line protocol).
    static bool isValidSite(const std::string& site);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    std::string request(const std::string& line);
    std::string readResponseLine();
};

}  // namespace energy
