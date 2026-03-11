/**
 * Smart Parking — Web Server Entry Point
 * =======================================
 * Starts the HTTP REST API server on port 8080.
 * Open browser at http://localhost:8080
 *
 * Build:  see CMakeLists.txt
 * Run:    ./SmartParkingServer [rows] [cols]
 */

#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <csignal>

#include "ParkingSlotManager.h"
#include "Database.h"
#include "Server.h"

static std::atomic<bool> running{true};

void sigHandler(int) { running.store(false); std::exit(0); }

int main(int argc, char* argv[]) {
    std::signal(SIGINT,  sigHandler);
    std::signal(SIGTERM, sigHandler);

    int ROWS = (argc > 1) ? std::stoi(argv[1]) : 6;
    int COLS = (argc > 2) ? std::stoi(argv[2]) : 8;
    int PORT = (argc > 3) ? std::stoi(argv[3]) : 8080;

    std::cout << R"(
  ___ ___ _  _ ___ ___  _ ___ _  _  ___ ___ ___
 | _ \ _ | \| / __| __|| | _ | \| |/ __| __| _ \
 |  _/  _| .` \__ \ _| | |  _| .` | (_ | _||   /
 |_| |_| |_|\_|___/_|  |_|_| |_|\_|\___|___|_|_\

  Smart Parking Web Server | C++17 + httplib
)" << "\n";

    std::cout << "Grid: " << ROWS << "x" << COLS
              << " (" << ROWS*COLS << " slots)\n"
              << "Port: " << PORT << "\n\n";

    ParkingSlotManager mgr(ROWS, COLS);
    Database           db("parking.db");
    ParkingServer      server(mgr, db, PORT);

    server.addLog("info", "Server started | " + std::to_string(ROWS*COLS) + " slots ready");

    // Optional: auto-simulate a few cars on startup for demo
    std::thread demo([&]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        for (int i = 0; i < std::min(ROWS*COLS/3, 10); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(400));
            std::string plate = ALPR::generateMockPlate();
            int gate   = i % 4;
            int slotId = mgr.assignSlot(gate, plate);
            if (slotId >= 0) {
                db.logEntry(plate, slotId, gate, std::time(nullptr));
                server.addLog("entry", "Demo G" + std::to_string(gate)
                              + " → Slot#" + std::to_string(slotId)
                              + " [" + plate + "]");
            }
        }
        server.addLog("info", "Demo vehicles loaded. Use buttons to add/remove.");
    });
    demo.detach();

    // Blocks until Ctrl+C
    server.start();
    return 0;
}
