/**
 * Smart Parking Management System — SFML GUI Edition
 * ====================================================
 * Opens a real graphical window showing:
 *   - Live color-coded parking grid (green=free, red=occupied, yellow=flash on change)
 *   - Gate markers at corners
 *   - Live occupancy bar at the bottom
 *   - Scrolling event log panel on the right
 *   - Statistics panel (total/used/free/%)
 *
 * Thread model:
 *   - Main thread:    SFML window render loop (SFML requires this)
 *   - Entry threads:  simulate car arrivals, may block if lot is full
 *   - Exit threads:   simulate departures, wake blocked entry threads
 *
 * Run: ./SmartParking [rows] [cols] [numCars]
 */

#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <random>
#include <mutex>
#include <string>

#include "ParkingSlotManager.h"
#include "Database.h"
#include "ALPR.h"
#include "GUI.h"

int main(int argc, char* argv[]) {
    int ROWS     = (argc > 1) ? std::stoi(argv[1]) : 6;
    int COLS     = (argc > 2) ? std::stoi(argv[2]) : 8;
    int NUM_CARS = (argc > 3) ? std::stoi(argv[3]) : 40;

    ParkingSlotManager mgr(ROWS, COLS);
    Database           db("parking.db");
    GUI                gui(1100, 700, mgr);

    gui.addLog("System started | " + std::to_string(ROWS*COLS) + " slots");
    gui.addLog("Simulating " + std::to_string(NUM_CARS) + " vehicles...");

    std::vector<std::string> parkedPlates;
    std::mutex               platesMtx;
    std::atomic<bool>        entryDone{false};

    // ---- Entry simulation (background) ----
    std::thread entryThread([&]() {
        std::vector<std::thread> workers;

        for (int i = 0; i < NUM_CARS; ++i) {
            int gateIdx = i % 4;
            workers.emplace_back([&, gateIdx]() {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(200 + rand() % 400));

                std::string plate = ALPR::generateMockPlate();

                if (mgr.isFull())
                    gui.addLog("G" + std::to_string(gateIdx) + " FULL waiting: " + plate);

                int slotId = mgr.assignSlot(gateIdx, plate);
                if (slotId >= 0) {
                    db.logEntry(plate, slotId, gateIdx, std::time(nullptr));
                    gui.flashSlot(slotId);
                    gui.addLog("Entry G" + std::to_string(gateIdx)
                               + " Slot#" + std::to_string(slotId)
                               + " " + plate);

                    std::lock_guard<std::mutex> lk(platesMtx);
                    parkedPlates.push_back(plate);
                }
            });
            std::this_thread::sleep_for(std::chrono::milliseconds(80));
        }

        for (auto& w : workers) w.join();
        entryDone.store(true);
        gui.addLog("--- All vehicles entered ---");
    });

    // ---- Exit simulation (starts after all entries done) ----
    std::thread exitThread([&]() {
        while (!entryDone.load())
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::this_thread::sleep_for(std::chrono::seconds(2));
        gui.addLog("--- Vehicles leaving ---");

        std::vector<std::string> plates;
        {
            std::lock_guard<std::mutex> lk(platesMtx);
            plates = parkedPlates;
        }

        std::vector<std::thread> workers;
        for (size_t i = 0; i < plates.size(); ++i) {
            std::string plate = plates[i];
            int xgate = (int)(i % 4);
            workers.emplace_back([&, plate, xgate]() {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(300 + rand() % 700));

                int slotId = -1;
                if (mgr.releaseSlot(plate, slotId)) {
                    db.logExit(plate, slotId, std::time(nullptr));
                    gui.flashSlot(slotId);
                    gui.addLog("Exit  G" + std::to_string(xgate)
                               + " Slot#" + std::to_string(slotId)
                               + " " + plate);
                }
            });
            std::this_thread::sleep_for(std::chrono::milliseconds(60));
        }

        for (auto& w : workers) w.join();
        gui.addLog("--- Simulation complete ---");
    });

    // ---- SFML render loop on main thread ----
    while (gui.update()) { /* renders every frame */ }

    entryThread.detach();
    exitThread.detach();

    std::cout << "Window closed.\n";
    return 0;
}
