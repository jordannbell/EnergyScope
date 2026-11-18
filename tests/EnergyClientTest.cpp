// End-to-end tests: a real server loop (on a free local port) and a real
// EnergyClient talking over TCP.

#include <gtest/gtest.h>

#include <limits>
#include <thread>

#include "energy/EnergyClient.hpp"
#include "energy/net/Net.hpp"

using energy::EnergyClient;
using energy::Measurement;

namespace {

// Runs the server side for one client connection in a background thread.
class TestServer {
public:
    TestServer()
        : listener_(energy::net::listenOnLoopback(0)),
          repo_(":memory:"),
          thread_([this] {
              const auto client = energy::net::acceptClient(listener_);
              if (client.valid()) energy::net::serveConnection(client, repo_);
          }) {}

    ~TestServer() { thread_.join(); }  // ends when the client disconnects

    unsigned short port() const { return energy::net::localPort(listener_); }

private:
    energy::net::WinsockSession winsock_;
    energy::net::Socket listener_;
    energy::MeasurementRepository repo_;
    std::thread thread_;
};

}  // namespace

TEST(EnergyClient, AddThenGetRoundTripsValues) {
    TestServer server;
    {
        EnergyClient client("127.0.0.1", server.port());
        client.add({"solar-1", 200, 14.0});
        client.add({"solar-1", 100, 12.3456789});

        const auto rows = client.get("solar-1");

        ASSERT_EQ(rows.size(), 2u);
        EXPECT_EQ(rows[0].timestamp, 100);
        EXPECT_DOUBLE_EQ(rows[0].powerKw, 12.3456789);
        EXPECT_EQ(rows[1].timestamp, 200);
    }
}

TEST(EnergyClient, GetUnknownSiteReturnsEmpty) {
    TestServer server;
    {
        EnergyClient client("127.0.0.1", server.port());
        EXPECT_TRUE(client.get("nobody").empty());
    }
}

TEST(EnergyClient, RejectsSiteNamesThatCouldInjectCommands) {
    TestServer server;
    {
        EnergyClient client("127.0.0.1", server.port());
        EXPECT_THROW(client.get("a\nADD x 1 1"), std::invalid_argument);
        EXPECT_THROW(client.get("two words"), std::invalid_argument);
        EXPECT_THROW(client.get(""), std::invalid_argument);
        EXPECT_THROW(client.add({"bad site", 1, 1.0}), std::invalid_argument);
    }
}

TEST(EnergyClient, ThrowsWhenNobodyListens) {
    energy::net::WinsockSession winsock;
    unsigned short freePort = 0;
    {
        auto s = energy::net::listenOnLoopback(0);
        freePort = energy::net::localPort(s);
    }  // listener closed: nothing listens on freePort any more

    EXPECT_THROW(EnergyClient("127.0.0.1", freePort), std::runtime_error);
}

TEST(EnergyClient, ThrowsOnInvalidAddress) {
    EXPECT_THROW(EnergyClient("not-an-ip", 5555), std::runtime_error);
}

TEST(EnergyClient, ReportsServerSideErrors) {
    TestServer server;
    {
        EnergyClient client("127.0.0.1", server.port());
        EXPECT_THROW(client.add({"s", 1, std::numeric_limits<double>::infinity()}), std::runtime_error);
    }
}
