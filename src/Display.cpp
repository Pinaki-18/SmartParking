#include "Display.h"
#include <iostream>
#include <iomanip>
#include <string>

void Display::printBanner() {
    std::cout << R"(
 ___                _     ___           _    _
/ __|_ __  __ _ _ _| |_  | _ \__ _ _ _| |__(_)_ _  __ _
\__ \ '  \/ _` | '_|  _| |  _/ _` | '_| / /| | ' \/ _` |
|___/_|_|_\__,_|_|  \__| |_| \__,_|_| |_\_\|_|_||_\__, |
                                                    |___/
  Multi-Threaded Parking System  |  Dijkstra Routing  |  SQLite Logs
)" << "\n";
}

void Display::printGrid(const ParkingSlotManager& mgr) {
    const auto& slots = mgr.getSlots();
    int rows = mgr.getRows();
    int cols = mgr.getCols();

    std::cout << "\n  Gate0                       Gate1\n";
    std::cout << "  G0 ";
    for (int c = 0; c < cols; ++c)
        std::cout << std::setw(3) << c;
    std::cout << " G1\n";
    std::cout << "     " << std::string(cols * 3, '-') << "\n";

    for (int r = 0; r < rows; ++r) {
        std::cout << std::setw(2) << r << " | ";
        for (int c = 0; c < cols; ++c) {
            int id = r * cols + c;
            std::cout << (slots[id].occupied ? "[X]" : "[ ]");
        }
        std::cout << " |\n";
    }

    std::cout << "     " << std::string(cols * 3, '-') << "\n";
    std::cout << "  G2 ";
    for (int c = 0; c < cols; ++c)
        std::cout << std::setw(3) << c;
    std::cout << " G3\n";
    std::cout << "  Gate2                       Gate3\n\n";
}

void Display::printStats(const ParkingSlotManager& mgr) {
    int avail = mgr.availableCount();
    int total = mgr.totalSlots();
    int used  = total - avail;

    // ASCII occupancy bar
    int barWidth = 40;
    int filled   = (total > 0) ? (used * barWidth / total) : 0;
    std::string bar(filled, '#');
    bar += std::string(barWidth - filled, '-');

    std::cout << "Occupancy: [" << bar << "] "
              << used << "/" << total
              << "  (" << avail << " free)\n";
}
