#include "ParkingSlotManager.h"
#include "Dijkstra.h"
#include <iostream>

ParkingSlotManager::ParkingSlotManager(int rows, int cols)
    : rows_(rows), cols_(cols)
{
    slots_.reserve(rows * cols);
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
            slots_.emplace_back(r * cols + c, r, c);
}

std::pair<int,int> ParkingSlotManager::gatePosition(int gateId) const {
    // Place 4 entry gates at the four corners of the grid.
    // Dijkstra will route from these positions inward.
    switch (gateId % 4) {
        case 0: return {0,          0         }; // top-left
        case 1: return {0,          cols_ - 1 }; // top-right
        case 2: return {rows_ - 1,  0         }; // bottom-left
        case 3: return {rows_ - 1,  cols_ - 1 }; // bottom-right
        default: return {0, 0};
    }
}

int ParkingSlotManager::assignSlot(int gateId, const std::string& plate) {
    // unique_lock is needed (not lock_guard) because cv_.wait unlocks it
    std::unique_lock<std::mutex> lock(mtx_);

    // Block this entry gate thread until at least one slot is free.
    // cv_.wait atomically releases the lock and suspends the thread.
    // When notified (by releaseSlot), it re-acquires the lock and
    // checks the predicate again (guards against spurious wakeups).
    cv_.wait(lock, [this] {
        return occupiedCount_ < static_cast<int>(slots_.size());
    });

    // Build flat occupied vector for Dijkstra (lock already held)
    std::vector<bool> occ(slots_.size());
    for (size_t i = 0; i < slots_.size(); ++i)
        occ[i] = slots_[i].occupied;

    auto [gRow, gCol] = gatePosition(gateId);
    int slotId = Dijkstra::nearestAvailable(rows_, cols_, gRow, gCol, occ);

    if (slotId < 0) return -1;

    // Mark slot as occupied
    slots_[slotId].occupied  = true;
    slots_[slotId].plate     = plate;
    slots_[slotId].entryTime = std::time(nullptr);
    plateIndex_[plate]       = slotId;
    ++occupiedCount_;

    return slotId;
}

bool ParkingSlotManager::releaseSlot(const std::string& plate, int& slotId) {
    std::unique_lock<std::mutex> lock(mtx_);

    auto it = plateIndex_.find(plate);
    if (it == plateIndex_.end()) return false;

    slotId = it->second;
    slots_[slotId].occupied  = false;
    slots_[slotId].plate     = "";
    slots_[slotId].entryTime = 0;
    plateIndex_.erase(it);
    --occupiedCount_;

    // Wake up ALL waiting entry gate threads.
    // Each will re-check the predicate; exactly one will proceed.
    cv_.notify_all();

    return true;
}

bool ParkingSlotManager::isFull() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return occupiedCount_ >= static_cast<int>(slots_.size());
}

int ParkingSlotManager::availableCount() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return static_cast<int>(slots_.size()) - occupiedCount_;
}
