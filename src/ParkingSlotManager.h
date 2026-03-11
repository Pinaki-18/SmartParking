#pragma once
#include <vector>
#include <mutex>
#include <condition_variable>
#include <string>
#include <ctime>
#include <unordered_map>

/**
 * Represents one physical parking slot on the grid.
 */
struct ParkingSlot {
    int id;               // Unique index (row * cols + col)
    int row, col;         // Grid coordinates
    bool occupied;
    std::string plate;    // License plate of parked vehicle
    std::time_t entryTime;

    ParkingSlot(int id, int row, int col)
        : id(id), row(row), col(col), occupied(false), entryTime(0) {}
};

/**
 * Thread-safe manager for all parking slots.
 *
 * Synchronization:
 *   - std::mutex protects all slot state reads/writes
 *   - std::condition_variable blocks entry gates when lot is full
 *     and wakes them when an exit gate releases a slot
 */
class ParkingSlotManager {
public:
    ParkingSlotManager(int rows, int cols);

    /**
     * Assign the nearest slot (via Dijkstra) to an incoming vehicle.
     * Blocks until a slot is available if lot is currently full.
     *
     * @param gateId  Which entry gate (0-3) the vehicle came from
     * @param plate   License plate string
     * @return        Slot id assigned, or -1 on error
     */
    int assignSlot(int gateId, const std::string& plate);

    /**
     * Release the slot held by the vehicle with the given plate.
     * Notifies all blocked entry gate threads after release.
     *
     * @param plate   License plate of departing vehicle
     * @param slotId  [out] Slot that was released
     * @return        true if vehicle was found and released
     */
    bool releaseSlot(const std::string& plate, int& slotId);

    bool isFull() const;
    int  availableCount() const;
    int  totalSlots()     const { return rows_ * cols_; }

    int getRows() const { return rows_; }
    int getCols() const { return cols_; }
    const std::vector<ParkingSlot>& getSlots() const { return slots_; }

    // Returns the grid position of each gate (for Dijkstra start node)
    std::pair<int,int> gatePosition(int gateId) const;

private:
    int rows_, cols_;
    std::vector<ParkingSlot> slots_;

    // Maps license plate -> slot id for O(1) exit lookup
    std::unordered_map<std::string, int> plateIndex_;

    mutable std::mutex mtx_;
    std::condition_variable cv_;   // signaled on every slot release
    int occupiedCount_ = 0;
};
