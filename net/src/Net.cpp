#include "energy/net/Net.hpp"

#include <stdexcept>
#include <utility>

#include "energy/Protocol.hpp"

namespace energy::net {

WinsockSession::WinsockSession() {
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
}

WinsockSession::~WinsockSession() { WSACleanup(); }

Socket::~Socket() {
    if (valid()) closesocket(s_);
}

Socket::Socket(Socket&& other) noexcept : s_(other.s_) {
    other.s_ = INVALID_SOCKET;
}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        if (valid()) closesocket(s_);
        s_ = other.s_;
        other.s_ = INVALID_SOCKET;
    }
    return *this;
}

namespace {

sockaddr_in loopbackAddress(unsigned short port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    return addr;
}

}  // namespace

Socket listenOnLoopback(unsigned short port) {
    Socket listener(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    if (!listener.valid()) throw std::runtime_error("socket() failed");

    const sockaddr_in addr = loopbackAddress(port);
    if (bind(listener.get(), reinterpret_cast<const sockaddr*>(&addr),
             sizeof addr) == SOCKET_ERROR) {
        throw std::runtime_error("bind() failed (port already used?)");
    }
    if (listen(listener.get(), SOMAXCONN) == SOCKET_ERROR) {
        throw std::runtime_error("listen() failed");
    }
    return listener;
}

unsigned short localPort(const Socket& s) {
    sockaddr_in addr{};
    int len = sizeof addr;
    if (getsockname(s.get(), reinterpret_cast<sockaddr*>(&addr), &len) ==
        SOCKET_ERROR) {
        throw std::runtime_error("getsockname() failed");
    }
    return ntohs(addr.sin_port);
}

Socket acceptClient(const Socket& listener) {
    return Socket(accept(listener.get(), nullptr, nullptr));
}

Socket connectTo(const std::string& ipv4, unsigned short port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ipv4.c_str(), &addr.sin_addr) != 1) {
        throw std::runtime_error("invalid IPv4 address: " + ipv4);
    }
    Socket s(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    if (!s.valid()) throw std::runtime_error("socket() failed");
    if (connect(s.get(), reinterpret_cast<const sockaddr*>(&addr),
                sizeof addr) == SOCKET_ERROR) {
        throw std::runtime_error("cannot connect to " + ipv4 + ":" +
                                 std::to_string(port));
    }
    return s;
}

void setReceiveTimeout(const Socket& s, int milliseconds) {
    const DWORD timeout = static_cast<DWORD>(milliseconds);
    setsockopt(s.get(), SOL_SOCKET, SO_RCVTIMEO,
               reinterpret_cast<const char*>(&timeout), sizeof timeout);
}

bool sendAll(const Socket& s, const std::string& data) {
    std::size_t sent = 0;
    while (sent < data.size()) {
        const int n = send(s.get(), data.data() + sent,
                           static_cast<int>(data.size() - sent), 0);
        if (n == SOCKET_ERROR) return false;
        sent += static_cast<std::size_t>(n);
    }
    return true;
}

bool readLine(const Socket& s, std::string& pending, std::string& line) {
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
        const int n = recv(s.get(), buf, sizeof buf, 0);
        if (n <= 0) return false;
        pending.append(buf, static_cast<std::size_t>(n));
    }
}

void serveConnection(const Socket& client, MeasurementRepository& repo) {
    setReceiveTimeout(client, kDefaultTimeoutMs);
    std::string pending, line;
    while (readLine(client, pending, line)) {
        if (!sendAll(client, handleRequest(repo, line))) return;
    }
}

}  // namespace energy::net
