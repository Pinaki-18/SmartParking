#include "Database.h"
#include <iostream>
#include <sstream>
#include <iomanip>

Database::Database(const std::string& dbPath) {
    int rc = sqlite3_open(dbPath.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "[DB] Cannot open: " << sqlite3_errmsg(db_) << "\n";
        sqlite3_close(db_);
        db_ = nullptr;
        return;
    }
    createTables();
    std::cout << "[DB] Opened " << dbPath << "\n";
}

Database::~Database() {
    if (db_) sqlite3_close(db_);
}

void Database::createTables() {
    exec(R"(
        CREATE TABLE IF NOT EXISTS vehicle_log (
            id         INTEGER PRIMARY KEY AUTOINCREMENT,
            plate      TEXT    NOT NULL,
            slot_id    INTEGER NOT NULL,
            gate_id    INTEGER,
            entry_time INTEGER,
            exit_time  INTEGER
        );
    )");
}

bool Database::exec(const std::string& sql) {
    char* err = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::cerr << "[DB] SQL error: " << err << "\n";
        sqlite3_free(err);
        return false;
    }
    return true;
}

bool Database::logEntry(const std::string& plate, int slotId, int gateId, std::time_t t) {
    std::lock_guard<std::mutex> lk(mtx_);
    std::ostringstream ss;
    ss << "INSERT INTO vehicle_log (plate,slot_id,gate_id,entry_time) VALUES ('"
       << plate << "'," << slotId << "," << gateId << "," << (long long)t << ");";
    return exec(ss.str());
}

bool Database::logExit(const std::string& plate, int slotId, std::time_t t) {
    std::lock_guard<std::mutex> lk(mtx_);
    std::ostringstream ss;
    ss << "UPDATE vehicle_log SET exit_time=" << (long long)t
       << " WHERE plate='" << plate << "' AND exit_time IS NULL;";
    return exec(ss.str());
}

void Database::printRecentLogs(int n) {
    std::lock_guard<std::mutex> lk(mtx_);
    std::ostringstream q;
    q << "SELECT plate,slot_id,gate_id,entry_time,exit_time "
      << "FROM vehicle_log ORDER BY id DESC LIMIT " << n << ";";

    std::cout << "\n====== Recent Vehicle Logs ======\n";
    std::cout << std::left
              << std::setw(16) << "Plate"
              << std::setw(7)  << "Slot"
              << std::setw(7)  << "Gate"
              << std::setw(12) << "Entry"
              << std::setw(12) << "Exit"
              << "\n" << std::string(54,'-') << "\n";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, q.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        return;

    auto fmtTime = [](long long t) -> std::string {
        if (!t) return "Still In";
        char buf[16];
        std::time_t tt = (std::time_t)t;
        std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&tt));
        return buf;
    };

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string pl = (const char*)sqlite3_column_text(stmt, 0);
        int sid  = sqlite3_column_int(stmt, 1);
        int gid  = sqlite3_column_int(stmt, 2);
        long long en = sqlite3_column_int64(stmt, 3);
        long long ex = sqlite3_column_int64(stmt, 4);
        std::cout << std::left
                  << std::setw(16) << pl
                  << std::setw(7)  << sid
                  << std::setw(7)  << gid
                  << std::setw(12) << fmtTime(en)
                  << std::setw(12) << fmtTime(ex)
                  << "\n";
    }
    sqlite3_finalize(stmt);
}
