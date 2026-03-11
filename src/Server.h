#pragma once
/**
 * REST API Server for Smart Parking System
 * =========================================
 * Uses cpp-httplib (single header, no install needed)
 * Download: https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h
 * Place httplib.h in src/ folder alongside this file.
 *
 * API Endpoints:
 *   GET  /api/status       -> full lot state as JSON
 *   GET  /api/logs         -> recent 50 event logs as JSON
 *   POST /api/entry        -> simulate a car entering  { "gate": 0 }
 *   POST /api/exit         -> simulate a car exiting   { "plate": "KA-01-AB-1234" }
 *   GET  /                 -> serves index.html
 *   GET  /css/style.css    -> serves stylesheet
 *   GET  /js/app.js        -> serves frontend JS
 */

#include "httplib.h"
#include "ParkingSlotManager.h"
#include "Database.h"
#include "ALPR.h"
#include <string>
#include <deque>
#include <mutex>
#include <functional>
#include <ctime>
#include <sstream>
#include <fstream>
#include <filesystem>

// Simple JSON helpers (no external lib needed)
inline std::string jsonStr(const std::string& s) {
    return "\"" + s + "\"";
}
inline std::string jsonKV(const std::string& k, const std::string& v) {
    return jsonStr(k) + ":" + v;
}
inline std::string jsonKVs(const std::string& k, const std::string& v) {
    return jsonStr(k) + ":" + jsonStr(v);
}

class ParkingServer {
public:
    ParkingServer(ParkingSlotManager& mgr, Database& db, int port = 8080)
        : mgr_(mgr), db_(db), port_(port)
    {
        setupRoutes();
    }

    void start() {
        std::cout << "[Server] Running at http://localhost:" << port_ << "\n";
        std::cout << "[Server] Open your browser to http://localhost:" << port_ << "\n";
        svr_.listen("0.0.0.0", port_);
    }

    void addLog(const std::string& type, const std::string& msg) {
        std::lock_guard<std::mutex> lk(logMtx_);
        // type: "entry" | "exit" | "full" | "info"
        std::time_t now = std::time(nullptr);
        char buf[10];
        std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&now));
        logs_.push_back({std::string(buf), type, msg});
        if (logs_.size() > 200) logs_.pop_front();
    }

private:
    ParkingSlotManager& mgr_;
    Database&           db_;
    httplib::Server     svr_;
    int                 port_;

    struct LogEntry { std::string time, type, msg; };
    std::deque<LogEntry> logs_;
    std::mutex           logMtx_;

    // Read a static file relative to the web/ folder
    std::string readFile(const std::string& path) {
        std::ifstream f(path);
        if (!f) return "";
        return std::string(std::istreambuf_iterator<char>(f),
                           std::istreambuf_iterator<char>());
    }

    // Build full lot-state JSON
    std::string buildStatusJSON() {
        const auto& slots = mgr_.getSlots();
        int total = mgr_.totalSlots();
        int avail = mgr_.availableCount();
        int used  = total - avail;

        std::ostringstream j;
        j << "{"
          << jsonKV("total",  std::to_string(total))  << ","
          << jsonKV("used",   std::to_string(used))   << ","
          << jsonKV("avail",  std::to_string(avail))  << ","
          << jsonKV("rows",   std::to_string(mgr_.getRows())) << ","
          << jsonKV("cols",   std::to_string(mgr_.getCols()))  << ","
          << "\"slots\":[";

        for (size_t i = 0; i < slots.size(); ++i) {
            const auto& s = slots[i];
            if (i > 0) j << ",";
            j << "{"
              << jsonKV("id",       std::to_string(s.id))  << ","
              << jsonKV("row",      std::to_string(s.row)) << ","
              << jsonKV("col",      std::to_string(s.col)) << ","
              << jsonKV("occupied", s.occupied ? "true" : "false") << ","
              << jsonKVs("plate",   s.plate)
              << "}";
        }
        j << "]}";
        return j.str();
    }

    std::string buildLogsJSON(int n = 50) {
        std::lock_guard<std::mutex> lk(logMtx_);
        std::ostringstream j;
        j << "[";
        int start = std::max(0, (int)logs_.size() - n);
        for (int i = start; i < (int)logs_.size(); ++i) {
            if (i > start) j << ",";
            j << "{"
              << jsonKVs("time", logs_[i].time) << ","
              << jsonKVs("type", logs_[i].type) << ","
              << jsonKVs("msg",  logs_[i].msg)
              << "}";
        }
        j << "]";
        return j.str();
    }

    void setupRoutes() {
        // ---- Serve static files ----
        svr_.Get("/", [this](const httplib::Request&, httplib::Response& res) {
            std::string html = readFile("../web/index.html");
            if (html.empty()) {
                res.set_content("<h1>Error: web/index.html not found</h1>", "text/html");
            } else {
                res.set_content(html, "text/html");
            }
        });

        svr_.Get("/css/style.css", [this](const httplib::Request&, httplib::Response& res) {
            res.set_content(readFile("../web/css/style.css"), "text/css");
        });

        svr_.Get("/js/app.js", [this](const httplib::Request&, httplib::Response& res) {
            res.set_content(readFile("../web/js/app.js"), "application/javascript");
        });

        // ---- API: lot status ----
        svr_.Get("/api/status", [this](const httplib::Request&, httplib::Response& res) {
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_content(buildStatusJSON(), "application/json");
        });

        // ---- API: event logs ----
        svr_.Get("/api/logs", [this](const httplib::Request&, httplib::Response& res) {
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_content(buildLogsJSON(), "application/json");
        });

        // ---- API: simulate car entry ----
        svr_.Post("/api/entry", [this](const httplib::Request& req, httplib::Response& res) {
            res.set_header("Access-Control-Allow-Origin", "*");

            // Parse gate from body: {"gate":0}
            int gateId = 0;
            auto pos = req.body.find("\"gate\"");
            if (pos != std::string::npos) {
                auto colon = req.body.find(":", pos);
                if (colon != std::string::npos)
                    gateId = std::stoi(req.body.substr(colon+1, 2)) % 4;
            }

            std::string plate = ALPR::generateMockPlate();

            if (mgr_.isFull()) {
                addLog("full", "Lot full! G" + std::to_string(gateId) + " waiting...");
                res.set_content("{\"error\":\"lot_full\",\"plate\":" + jsonStr(plate) + "}", "application/json");
                return;
            }

            int slotId = mgr_.assignSlot(gateId, plate);
            if (slotId >= 0) {
                db_.logEntry(plate, slotId, gateId, std::time(nullptr));
                addLog("entry", "G" + std::to_string(gateId) + " → Slot#"
                       + std::to_string(slotId) + " [" + plate + "]");
                res.set_content("{"
                    + jsonKVs("plate", plate) + ","
                    + jsonKV("slot",   std::to_string(slotId)) + ","
                    + jsonKV("gate",   std::to_string(gateId))
                    + "}", "application/json");
            } else {
                res.set_content("{\"error\":\"assign_failed\"}", "application/json");
            }
        });

        // ---- API: simulate car exit ----
        svr_.Post("/api/exit", [this](const httplib::Request& req, httplib::Response& res) {
            res.set_header("Access-Control-Allow-Origin", "*");

            // Parse plate from body: {"plate":"KA-01-AB-1234"}
            std::string plate;
            auto pos = req.body.find("\"plate\"");
            if (pos != std::string::npos) {
                auto q1 = req.body.find("\"", pos + 8);
                auto q2 = req.body.find("\"", q1 + 1);
                if (q1 != std::string::npos && q2 != std::string::npos)
                    plate = req.body.substr(q1+1, q2-q1-1);
            }

            if (plate.empty()) {
                // Pick a random occupied slot if no plate given
                const auto& slots = mgr_.getSlots();
                for (const auto& s : slots) {
                    if (s.occupied) { plate = s.plate; break; }
                }
            }

            if (plate.empty()) {
                res.set_content("{\"error\":\"no_plate\"}", "application/json");
                return;
            }

            int slotId = -1;
            if (mgr_.releaseSlot(plate, slotId)) {
                db_.logExit(plate, slotId, std::time(nullptr));
                addLog("exit", "G? ← Slot#" + std::to_string(slotId) + " [" + plate + "]");
                res.set_content("{"
                    + jsonKVs("plate", plate) + ","
                    + jsonKV("slot",   std::to_string(slotId))
                    + "}", "application/json");
            } else {
                res.set_content("{\"error\":\"not_found\",\"plate\":"
                                + jsonStr(plate) + "}", "application/json");
            }
        });
    }
};
