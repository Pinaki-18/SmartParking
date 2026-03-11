#include "Gate.h"
#include "ALPR.h"
#include <iostream>

EntryGate::EntryGate(int id, ParkingSlotManager& mgr, Database& db)
    : id_(id), mgr_(mgr), db_(db) {}

void EntryGate::processVehicle(const std::string& imagePath) {
    std::string plate = imagePath.empty()
        ? ALPR::generateMockPlate()
        : ALPR::readPlate(imagePath);

    std::cout << "[Entry Gate " << id_ << "] Arrived → plate: " << plate;

    if (mgr_.isFull())
        std::cout << "  [LOT FULL — waiting...]";
    std::cout << "\n";

    // This call may block until a slot becomes free
    int slotId = mgr_.assignSlot(id_, plate);
    if (slotId < 0) {
        std::cout << "[Entry Gate " << id_ << "] ERROR: could not assign slot.\n";
        return;
    }

    db_.logEntry(plate, slotId, id_, std::time(nullptr));
    std::cout << "[Entry Gate " << id_ << "] " << plate
              << " → Slot #" << slotId
              << "  (available: " << mgr_.availableCount() << ")\n";
}

ExitGate::ExitGate(int id, ParkingSlotManager& mgr, Database& db)
    : id_(id), mgr_(mgr), db_(db) {}

void ExitGate::processVehicle(const std::string& plate) {
    std::cout << "[Exit  Gate " << id_ << "] Exiting → plate: " << plate << "\n";

    int slotId = -1;
    if (mgr_.releaseSlot(plate, slotId)) {
        db_.logExit(plate, slotId, std::time(nullptr));
        std::cout << "[Exit  Gate " << id_ << "] " << plate
                  << " freed Slot #" << slotId
                  << "  (available: " << mgr_.availableCount() << ")\n";
    } else {
        std::cout << "[Exit  Gate " << id_ << "] Plate " << plate << " NOT FOUND.\n";
    }
}
