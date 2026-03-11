#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include "ParkingSlotManager.h"

/**
 * SFML-based real-time parking lot visualization.
 *
 * Renders:
 *   - Color-coded parking grid (green=free, red=occupied, yellow=just changed)
 *   - Gate labels at corners
 *   - Live occupancy bar
 *   - Scrolling event log panel
 *   - Stats: total/used/free counts
 */
class GUI {
public:
    GUI(int windowW, int windowH, ParkingSlotManager& mgr);

    /**
     * Call from main thread. Returns false when window is closed.
     */
    bool update();

    /**
     * Thread-safe: log a message into the side panel.
     */
    void addLog(const std::string& msg);

    /**
     * Mark a slot as "just changed" so it flashes yellow briefly.
     */
    void flashSlot(int slotId);

    bool isOpen() const { return window_.isOpen(); }

private:
    sf::RenderWindow        window_;
    sf::Font                font_;
    ParkingSlotManager&     mgr_;

    // Flash animation: slotId -> remaining flash time (seconds)
    std::vector<float>      flashTimer_;

    // Event log (scrolling)
    std::deque<std::string> logs_;
    std::mutex              logMtx_;

    int slotW_, slotH_;   // pixel size of each slot cell
    int gridOffX_, gridOffY_;

    void drawGrid(float dt);
    void drawStats();
    void drawLog();
    void drawOccupancyBar();

    sf::Color slotColor(int slotId) const;
};
