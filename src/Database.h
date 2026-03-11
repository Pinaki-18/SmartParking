#pragma once
#include <string>
#include <mutex>
#include <ctime>
#include <sqlite3.h>

/**
 * Thread-safe SQLite wrapper for vehicle entry/exit logging.
 *
 * Schema:
 *   vehicle_log(id, plate, slot_id, gate_id, entry_time, exit_time)
 *
 * SQLite3 is NOT thread-safe in its default compile mode unless
 * SQLITE_THREADSAFE=1. We wrap all calls with our own mutex to be safe.
 */
class Database {
public:
    explicit Database(const std::string& dbPath);
    ~Database();

    bool logEntry(const std::string& plate, int slotId, int gateId, std::time_t t);
    bool logExit (const std::string& plate, int slotId, std::time_t t);

    void printRecentLogs(int n = 15);

private:
    sqlite3*   db_  = nullptr;
    std::mutex mtx_;

    bool        exec(const std::string& sql);
    void        createTables();
};
