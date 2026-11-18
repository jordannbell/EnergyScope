// EnergyScope server: listens on localhost, speaks the line protocol of
// energy/Protocol.hpp, stores data in a SQLite file.
//
// Usage: energy_server [port] [database-file]

#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include "energy/MeasurementRepository.hpp"
#include "energy/Protocol.hpp"

namespace {

constexpr unsigned short kDefaultPort = 5555;
constexpr std::size_t kMaxLineBytes = 4096;  // refuse longer requests
constexpr int kClientTimeoutMs = 5000;       // a silent client cannot block us

// Closes a socket automatically when it goes out of scope.
class Socket {
public:
    explicit Socket(SOCKET s = INVALID_SOCKET) : s_(s) {}
    ~Socket() {
        if (s_ != INVALID_SOCKET) closesocket(s_);
    }
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&& o) noexcept : s_(o.s_) { o.s_ = INVALID_SOCKET; }
    SOCKET get() const { return s_; }
    bool valid() const { return s_ != INVALID_SOCKET; }

private:
    SOCKET s_;
};

bool sendAll(SOCKET s, const std::string& data) {
    std::size_t sent = 0;
    while (sent < data.size()) {
        const int n = send(s, data.data() + sent,
                           static_cast<int>(data.size() - sent), 0);
        if (n == SOCKET_ERROR) return false;
        sent += static_cast<std::size_t>(n);
    }
    return true;
}

// Reads one '\n'-terminated line. Returns false on error, timeout,
// disconnect before a full line, or a line that is too long.
bool readLine(SOCKET s, std::string& pending, std::string& line) {
    for (;;) {
        const auto pos = pending.find('\n');
        if (pos != std::string::npos) {
            line = pending.substr(0, pos);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            pending.erase(0, pos + 1);
            return true;
        }
        if (pending.size() > kMaxLineBytes) return false;
        char buf[512];
        const int n = recv(s, buf, sizeof buf, 0);
        if (n <= 0) return false;
        pending.append(buf, static_cast<std::size_t>(n));
    }
}

void serveClient(SOCKET client, energy::MeasurementRepository& repo) {
    const DWORD timeout = kClientTimeoutMs;
    setsockopt(client, SOL_SOCKET, SO_RCVTIMEO,
               reinterpret_cast<const char*>(&timeout), sizeof timeout);

    std::string pending, line;
    while (readLine(client, pending, line)) {
        if (!sendAll(client, energy::handleRequest(repo, line))) return;
    }
}

Socket makeListener(unsigned short port) {
    Socket listener(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    if (!listener.valid()) throw std::runtime_error("socket() failed");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // this PC only

    if (bind(listener.get(), reinterpret_cast<sockaddr*>(&addr),
             sizeof addr) == SOCKET_ERROR) {
        throw std::runtime_error("bind() failed (port already used?)");
    }
    if (listen(listener.get(), SOMAXCONN) == SOCKET_ERROR) {
        throw std::runtime_error("listen() failed");
    }
    return listener;
}

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

        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }

        energy::MeasurementRepository repo(dbPath);
        Socket listener = makeListener(port);
        std::cout << "EnergyScope server on 127.0.0.1:" << port
                  << ", database: " << dbPath << std::endl;

        for (;;) {  // one client at a time (simple on purpose)
            Socket client(accept(listener.get(), nullptr, nullptr));
            if (!client.valid()) continue;
            serveClient(client.get(), repo);
        }
    } catch (const std::exception& e) {
        std::cerr << "fatal: " << e.what() << std::endl;
        return 1;
    }
}
