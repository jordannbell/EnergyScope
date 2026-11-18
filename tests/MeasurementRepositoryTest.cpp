#include <gtest/gtest.h>

#include "energy/MeasurementRepository.hpp"

using energy::Measurement;
using energy::MeasurementRepository;

TEST(MeasurementRepository, ReturnsEmptyWhenSiteUnknown) {
    MeasurementRepository repo(":memory:");
    EXPECT_TRUE(repo.findBySite("nowhere").empty());
}

TEST(MeasurementRepository, ReturnsInsertedMeasurementsOldestFirst) {
    MeasurementRepository repo(":memory:");
    repo.insert({"A", 200, 2.5});
    repo.insert({"A", 100, 1.5});

    const auto rows = repo.findBySite("A");

    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0].timestamp, 100);
    EXPECT_DOUBLE_EQ(rows[0].powerKw, 1.5);
    EXPECT_EQ(rows[1].timestamp, 200);
}

TEST(MeasurementRepository, FiltersBySite) {
    MeasurementRepository repo(":memory:");
    repo.insert({"A", 1, 1.0});
    repo.insert({"B", 1, 9.0});

    const auto rows = repo.findBySite("B");

    ASSERT_EQ(rows.size(), 1u);
    EXPECT_DOUBLE_EQ(rows[0].powerKw, 9.0);
}

TEST(MeasurementRepository, TreatsSqlInSiteNameAsPlainText) {
    MeasurementRepository repo(":memory:");
    repo.insert({"x'; DROP TABLE measurement;--", 1, 1.0});

    EXPECT_EQ(repo.findBySite("x'; DROP TABLE measurement;--").size(), 1u);
}

TEST(MeasurementRepository, ThrowsWhenPathIsInvalid) {
    EXPECT_THROW(MeasurementRepository("Z:/no/such/dir/db.sqlite"),
                 std::runtime_error);
}
