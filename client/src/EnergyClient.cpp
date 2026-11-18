#include "energy/EnergyClient.hpp"

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <stdexcept>

#include "energy/Parse.hpp"
#include "energy/net/Net.hpp"

namespace energy {

namespace {

constexpr std::size_t kMaxSiteLength = 64;
constexpr std::size_t kMaxRows = 1'000'000;   // refuse absurd server answers
constexpr std::size_t kReserveCap = 100'000;  // never pre-allocate more

}  // namespace

struct EnergyClient::Impl {
    net::WinsockSession winsock;
    net::Socket socket;
    std::string pending;
};

EnergyClient::EnergyClient(const std::string& ipv4, unsigned short port)
    : impl_(std::make_unique<Impl>()) {
    impl_->socket = net::connectTo(ipv4, port);
    net::setReceiveTimeout(impl_->socket, net::kDefaultTimeoutMs);
}

EnergyClient::~EnergyClient() = default;

bool EnergyClient::isValidSite(const std::string& site) {
    if (site.empty() || site.size() > kMaxSiteLength) return false;
    for (const unsigned char c : site) {
        if (c <= ' ' || c == 0x7F) return false;
    }
    return true;
}

std::string EnergyClient::readResponseLine() {
    std::string line;
    if (!net::readLine(impl_->socket, impl_->pending, line)) {
        throw std::runtime_error("connection lost or timed out");
    }
    return line;
}

std::string EnergyClient::request(const std::string& line) {
    if (!net::sendAll(impl_->socket, line + "\n")) {
        throw std::runtime_error("connection lost while sending");
    }
    return readResponseLine();
}

void EnergyClient::add(const Measurement& m) {
    if (!isValidSite(m.site)) throw std::invalid_argument("invalid site name");

    std::ostringstream req;
    req << "ADD " << m.site << " " << m.timestamp << " "
        << formatDouble(m.powerKw);
    const std::string answer = request(req.str());
    if (answer != "OK") throw std::runtime_error("server: " + answer);
}

std::vector<Measurement> EnergyClient::get(const std::string& site) {
    if (!isValidSite(site)) throw std::invalid_argument("invalid site name");

    const std::string header = request("GET " + site);
    if (header.rfind("OK ", 0) != 0) throw std::runtime_error("server: " + header);

    std::int64_t count = 0;
    if (!parseInt64(header.substr(3), count) || count < 0 ||
        static_cast<std::size_t>(count) > kMaxRows) {
        throw std::runtime_error("bad answer from server: " + header);
    }

    std::vector<Measurement> rows;
    rows.reserve(std::min<std::size_t>(static_cast<std::size_t>(count),
                                       kReserveCap));
    for (std::int64_t i = 0; i < count; ++i) {
        const std::string line = readResponseLine();
        const auto space = line.find(' ');
        Measurement m;
        m.site = site;
        if (space == std::string::npos ||
            !parseInt64(line.substr(0, space), m.timestamp) ||
            !parseDouble(line.substr(space + 1), m.powerKw)) {
            throw std::runtime_error("bad row from server: " + line);
        }
        rows.push_back(std::move(m));
    }
    return rows;
}

}  // namespace energy
