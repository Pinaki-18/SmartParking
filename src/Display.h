#pragma once
#include "ParkingSlotManager.h"

/**
 * Console-based visual display of parking lot state.
 *
 * Slot symbols:
 *   [ ] = free
 *   [X] = occupied
 *
 * Gate markers at corners:
 *   G0 top-left, G1 top-right, G2 bottom-left, G3 bottom-right
 */
class Display {
public:
    static void printGrid (const ParkingSlotManager& mgr);
    static void printStats(const ParkingSlotManager& mgr);
    static void printBanner();
};
