#pragma once

// Small Winsock helpers shared by the server and the client library.
// Windows only. Do NOT include this header from MFC code (winsock2.h and
// the winsock.h pulled in by <afxwin.h> clash); MFC code uses
// energy/EnergyClient.hpp instead.

#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstddef>
#include <string>

#include "energy/MeasurementRepository.hpp"

namespace energy::net {

constexpr std::size_t kMaxLineBytes = 4096;  // longer lines are refused
constexpr int kDefaultTimeoutMs = 5000;      // silent peer cannot block us

// Calls WSAStartup / WSACleanup. Keep one alive while using sockets.
class WinsockSession {
public:
    WinsockSession();
    ~WinsockSession();
    WinsockSession(const WinsockSession&) = delete;
    WinsockSession& operator=(const WinsockSession&) = delete;
};

// Owns a socket handle and closes it automatically.
class Socket {
public:
    Socket() = default;
    explicit Socket(SOCKET s) : s_(s) {}
    ~Socket();
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    SOCKET get() const { return s_; }
    bool valid() const { return s_ != INVALID_SOCKET; }

private:
    SOCKET s_ = INVALID_SOCKET;
};

// Listens on 127.0.0.1 only. port 0 lets Windows pick a free port.
// Throws std::runtime_error on failure.
Socket listenOnLoopback(unsigned short port);
unsigned short localPort(const Socket& s);

// Returns an invalid Socket if accept failed.
Socket acceptClient(const Socket& listener);

// ipv4 must be a numeric address like "127.0.0.1" (no DNS lookup).
// Throws std::runtime_error on failure.
Socket connectTo(const std::string& ipv4, unsigned short port);

void setReceiveTimeout(const Socket& s, int milliseconds);
bool sendAll(const Socket& s, const std::string& data);

// Reads one '\n'-terminated line (a trailing '\r' is removed). `pending`
// keeps bytes received after the newline for the next call. Returns false
// on error, timeout, disconnect, or a line longer than kMaxLineBytes.
bool readLine(const Socket& s, std::string& pending, std::string& line);

// Answers requests on one connection until the peer leaves.
void serveConnection(const Socket& client, MeasurementRepository& repo);

}  // namespace energy::net
