#include <gtest/gtest.h>

#include "energy/Protocol.hpp"

using energy::handleRequest;
using energy::MeasurementRepository;

namespace {
struct ProtocolTest : testing::Test {
    MeasurementRepository repo{":memory:"};
};
}  // namespace

TEST_F(ProtocolTest, AddStoresMeasurementAndReturnsOk) {
    EXPECT_EQ(handleRequest(repo, "ADD solar-1 100 12.5"), "OK\n");
    EXPECT_EQ(repo.findBySite("solar-1").size(), 1u);
}

TEST_F(ProtocolTest, GetReturnsCountThenRows) {
    handleRequest(repo, "ADD s 200 2");
    handleRequest(repo, "ADD s 100 1.5");

    EXPECT_EQ(handleRequest(repo, "GET s"), "OK 2\n100 1.5\n200 2\n");
}

TEST_F(ProtocolTest, GetKeepsFullPrecisionOfPower) {
    handleRequest(repo, "ADD s 1 12.3456789");

    EXPECT_EQ(handleRequest(repo, "GET s"), "OK 1\n1 12.3456789\n");
}

TEST_F(ProtocolTest, GetUnknownSiteReturnsZeroRows) {
    EXPECT_EQ(handleRequest(repo, "GET nope"), "OK 0\n");
}

TEST_F(ProtocolTest, RejectsEmptyRequest) {
    EXPECT_EQ(handleRequest(repo, "   "), "ERR empty request\n");
}

TEST_F(ProtocolTest, RejectsUnknownCommand) {
    EXPECT_EQ(handleRequest(repo, "DROP everything"), "ERR unknown command\n");
}

TEST_F(ProtocolTest, RejectsWrongArgumentCount) {
    EXPECT_EQ(handleRequest(repo, "ADD s 1").rfind("ERR usage", 0), 0u);
    EXPECT_EQ(handleRequest(repo, "GET").rfind("ERR usage", 0), 0u);
}

TEST_F(ProtocolTest, RejectsBadTimestampAndPower) {
    EXPECT_EQ(handleRequest(repo, "ADD s abc 1"), "ERR invalid timestamp\n");
    EXPECT_EQ(handleRequest(repo, "ADD s 1 12x"), "ERR invalid power\n");
    EXPECT_EQ(handleRequest(repo, "ADD s 1 nan"), "ERR invalid power\n");
    EXPECT_EQ(handleRequest(repo, "ADD s 1 inf"), "ERR invalid power\n");
}

TEST_F(ProtocolTest, RejectsTooLongSiteName) {
    const std::string longSite(65, 'a');
    EXPECT_EQ(handleRequest(repo, "ADD " + longSite + " 1 1"),
              "ERR invalid site\n");
}

TEST_F(ProtocolTest, FailedAddStoresNothing) {
    handleRequest(repo, "ADD s 1 nan");
    EXPECT_TRUE(repo.findBySite("s").empty());
}
