// EnergyScope server: listens on localhost, speaks the line protocol of
// energy/Protocol.hpp, stores data in a SQLite file.
//
// Usage: energy_server [port] [database-file]

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include "energy/MeasurementRepository.hpp"
#include "energy/net/Net.hpp"

namespace {

constexpr unsigned short kDefaultPort = 5555;

unsigned short parsePort(const char* text) {
    char* end = nullptr;
    const long v = std::strtol(text, &end, 10);
    if (*text == '\0' || *end != '\0' || v < 1 || v > 65535) {
        throw std::invalid_argument(std::string("invalid port: ") + text);
    }
    return static_cast<unsigned short>(v);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const unsigned short port =
            argc > 1 ? parsePort(argv[1]) : kDefaultPort;
        const std::string dbPath = argc > 2 ? argv[2] : "energy.sqlite";

        energy::net::WinsockSession winsock;
        energy::MeasurementRepository repo(dbPath);
        const auto listener = energy::net::listenOnLoopback(port);
        std::cout << "EnergyScope server on 127.0.0.1:" << port
                  << ", database: " << dbPath << std::endl;

        for (;;) {  // one client at a time (simple on purpose)
            const auto client = energy::net::acceptClient(listener);
            if (!client.valid()) continue;
            energy::net::serveConnection(client, repo);
        }
    } catch (const std::exception& e) {
        std::cerr << "fatal: " << e.what() << std::endl;
        return 1;
    }
}
