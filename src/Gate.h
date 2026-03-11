#pragma once
#include <string>
#include <atomic>
#include "ParkingSlotManager.h"
#include "Database.h"

/**
 * EntryGate simulates one of 4 physical entry gates.
 *
 * Each gate runs on its own std::thread.
 * When called:
 *   1. Reads license plate (ALPR or mock)
 *   2. Calls mgr_.assignSlot() — may block if lot is full
 *   3. Logs entry to the database
 */
class EntryGate {
public:
    EntryGate(int id, ParkingSlotManager& mgr, Database& db);

    void processVehicle(const std::string& imagePath = "");
    int  getId() const { return id_; }

private:
    int id_;
    ParkingSlotManager& mgr_;
    Database& db_;
};

/**
 * ExitGate simulates one of 4 physical exit gates.
 *
 * Each gate runs on its own std::thread.
 * When called:
 *   1. Looks up license plate
 *   2. Calls mgr_.releaseSlot() — wakes waiting entry gates via cv_.notify_all()
 *   3. Logs exit to the database
 */
class ExitGate {
public:
    ExitGate(int id, ParkingSlotManager& mgr, Database& db);

    void processVehicle(const std::string& plate);
    int  getId() const { return id_; }

private:
    int id_;
    ParkingSlotManager& mgr_;
    Database& db_;
};
