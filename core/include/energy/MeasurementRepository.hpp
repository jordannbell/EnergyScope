#pragma once

#include <string>
#include <vector>

#include "energy/Measurement.hpp"

struct sqlite3;

namespace energy {

// Stores and reads measurements in a SQLite database file.
// Use ":memory:" as path for a throw-away database (handy for tests).
// Throws std::runtime_error on any database error.
class MeasurementRepository {
public:
    explicit MeasurementRepository(const std::string& path);
    ~MeasurementRepository();

    MeasurementRepository(const MeasurementRepository&) = delete;
    MeasurementRepository& operator=(const MeasurementRepository&) = delete;

    void insert(const Measurement& measurement);

    // All measurements of one site, oldest first.
    std::vector<Measurement> findBySite(const std::string& site) const;

private:
    sqlite3* db_ = nullptr;
};

}  // namespace energy
