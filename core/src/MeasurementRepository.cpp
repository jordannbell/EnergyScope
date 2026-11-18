#include "energy/MeasurementRepository.hpp"

#include <sqlite3.h>

#include <memory>
#include <stdexcept>

namespace energy {

namespace {

struct StmtDeleter {
    void operator()(sqlite3_stmt* s) const { sqlite3_finalize(s); }
};
using Stmt = std::unique_ptr<sqlite3_stmt, StmtDeleter>;

[[noreturn]] void fail(sqlite3* db, const char* what) {
    throw std::runtime_error(std::string(what) + ": " + sqlite3_errmsg(db));
}

Stmt prepare(sqlite3* db, const char* sql) {
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &raw, nullptr) != SQLITE_OK) {
        fail(db, "prepare");
    }
    return Stmt(raw);
}

}  // namespace

MeasurementRepository::MeasurementRepository(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
        std::string msg = db_ ? sqlite3_errmsg(db_) : "out of memory";
        sqlite3_close(db_);
        db_ = nullptr;
        throw std::runtime_error("open: " + msg);
    }
    const char* schema =
        "CREATE TABLE IF NOT EXISTS measurement ("
        " id INTEGER PRIMARY KEY,"
        " site TEXT NOT NULL,"
        " ts INTEGER NOT NULL,"
        " power_kw REAL NOT NULL);"
        "CREATE INDEX IF NOT EXISTS idx_measurement_site_ts"
        " ON measurement(site, ts);";
    char* err = nullptr;
    if (sqlite3_exec(db_, schema, nullptr, nullptr, &err) != SQLITE_OK) {
        std::string msg = err ? err : "unknown";
        sqlite3_free(err);
        sqlite3_close(db_);
        db_ = nullptr;
        throw std::runtime_error("schema: " + msg);
    }
}

MeasurementRepository::~MeasurementRepository() { sqlite3_close(db_); }

void MeasurementRepository::insert(const Measurement& m) {
    auto stmt = prepare(
        db_, "INSERT INTO measurement(site, ts, power_kw) VALUES(?1, ?2, ?3)");
    sqlite3_bind_text(stmt.get(), 1, m.site.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt.get(), 2, m.timestamp);
    sqlite3_bind_double(stmt.get(), 3, m.powerKw);
    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        fail(db_, "insert");
    }
}

std::vector<Measurement> MeasurementRepository::findBySite(
    const std::string& site) const {
    auto stmt = prepare(db_,
                        "SELECT site, ts, power_kw FROM measurement"
                        " WHERE site = ?1 ORDER BY ts");
    sqlite3_bind_text(stmt.get(), 1, site.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<Measurement> out;
    int rc;
    while ((rc = sqlite3_step(stmt.get())) == SQLITE_ROW) {
        const auto* text = sqlite3_column_text(stmt.get(), 0);
        out.push_back({reinterpret_cast<const char*>(text),
                       sqlite3_column_int64(stmt.get(), 1),
                       sqlite3_column_double(stmt.get(), 2)});
    }
    if (rc != SQLITE_DONE) {
        fail(db_, "select");
    }
    return out;
}

}  // namespace energy
