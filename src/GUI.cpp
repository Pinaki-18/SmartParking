#include "GUI.h"
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iostream>

GUI::GUI(int windowW, int windowH, ParkingSlotManager& mgr)
    : window_(sf::VideoMode(windowW, windowH), "Smart Parking System",
              sf::Style::Titlebar | sf::Style::Close),
      mgr_(mgr),
      flashTimer_(mgr.totalSlots(), 0.0f)
{
    window_.setFramerateLimit(60);

    // Try to load a font — fall back to a default if not found
    if (!font_.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf") &&
        !font_.loadFromFile("/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf") &&
        !font_.loadFromFile("/System/Library/Fonts/Courier.ttc") &&
        !font_.loadFromFile("C:/Windows/Fonts/consola.ttf"))
    {
        std::cerr << "[GUI] Warning: no monospace font found. Text may not render.\n";
    }

    // Compute slot cell size to fill the left 65% of the window
    int gridAreaW = (int)(windowW * 0.65f) - 40;
    int gridAreaH = windowH - 120;
    slotW_ = gridAreaW  / mgr_.getCols();
    slotH_ = gridAreaH  / mgr_.getRows();
    slotW_ = slotH_ = std::min(slotW_, slotH_); // keep square

    // Center the grid in the left area
    gridOffX_ = 20 + (gridAreaW  - slotW_ * mgr_.getCols()) / 2;
    gridOffY_ = 80 + (gridAreaH  - slotH_ * mgr_.getRows()) / 2;
}

bool GUI::update() {
    sf::Event event;
    while (window_.pollEvent(event)) {
        if (event.type == sf::Event::Closed)
            window_.close();
    }
    if (!window_.isOpen()) return false;

    // Compute delta time for flash animations
    static sf::Clock clock;
    float dt = clock.restart().asSeconds();

    // Decay flash timers
    for (auto& t : flashTimer_)
        if (t > 0.0f) t = std::max(0.0f, t - dt);

    window_.clear(sf::Color(18, 18, 28));

    drawGrid(dt);
    drawStats();
    drawOccupancyBar();
    drawLog();

    window_.display();
    return true;
}

sf::Color GUI::slotColor(int slotId) const {
    const auto& slots = mgr_.getSlots();
    bool occ = slots[slotId].occupied;

    if (flashTimer_[slotId] > 0.0f) {
        // Flash yellow on change
        float t = flashTimer_[slotId];
        uint8_t y = (uint8_t)(255 * (t / 0.8f));
        return sf::Color(y, y, 0);
    }
    if (occ)
        return sf::Color(200, 50, 50);   // red = taken
    else
        return sf::Color(40, 180, 80);   // green = free
}

void GUI::drawGrid(float /*dt*/) {
    int rows = mgr_.getRows();
    int cols = mgr_.getCols();
    const auto& slots = mgr_.getSlots();

    // Section title
    sf::Text title("PARKING LOT", font_, 16);
    title.setFillColor(sf::Color(150, 200, 255));
    title.setPosition(gridOffX_, 20);
    window_.draw(title);

    // Subtitle: gate legend
    sf::Text legend("  G0=TopLeft  G1=TopRight  G2=BotLeft  G3=BotRight", font_, 11);
    legend.setFillColor(sf::Color(120, 120, 160));
    legend.setPosition(gridOffX_, 42);
    window_.draw(legend);

    int pad = 3; // gap between cells

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int id = r * cols + c;
            float x = gridOffX_ + c * slotW_;
            float y = gridOffY_ + r * slotH_;

            // Slot rectangle
            sf::RectangleShape cell(sf::Vector2f(slotW_ - pad, slotH_ - pad));
            cell.setPosition(x + pad/2.f, y + pad/2.f);
            cell.setFillColor(slotColor(id));
            cell.setOutlineThickness(1);
            cell.setOutlineColor(sf::Color(40, 40, 60));
            window_.draw(cell);

            // Slot number label (small)
            if (slotW_ >= 28) {
                sf::Text num(std::to_string(id), font_, 9);
                num.setFillColor(sf::Color(0, 0, 0, 160));
                num.setPosition(x + pad/2.f + 2, y + pad/2.f + 1);
                window_.draw(num);
            }

            // Show plate on occupied slot (if cell is big enough)
            if (slots[id].occupied && slotW_ >= 60) {
                std::string plate = slots[id].plate;
                if (plate.size() > 8) plate = plate.substr(0, 8);
                sf::Text pt(plate, font_, 9);
                pt.setFillColor(sf::Color(255, 230, 230));
                pt.setPosition(x + 4, y + slotH_/2.f - 6);
                window_.draw(pt);
            }
        }
    }

    // Gate markers at corners
    auto drawGate = [&](const std::string& label, int row, int col) {
        float x = gridOffX_ + col * slotW_;
        float y = gridOffY_ + row * slotH_;
        sf::CircleShape dot(8);
        dot.setFillColor(sf::Color(255, 200, 0));
        dot.setPosition(x - 8, y - 8);
        window_.draw(dot);
        sf::Text gt(label, font_, 12);
        gt.setFillColor(sf::Color(255, 200, 0));
        gt.setPosition(x - 6, y - 26);
        window_.draw(gt);
    };
    drawGate("G0", 0,        0);
    drawGate("G1", 0,        cols - 1);
    drawGate("G2", rows - 1, 0);
    drawGate("G3", rows - 1, cols - 1);
}

void GUI::drawStats() {
    int total = mgr_.totalSlots();
    int avail = mgr_.availableCount();
    int used  = total - avail;

    int panelX = (int)(window_.getSize().x * 0.67f);
    int y      = 20;

    auto line = [&](const std::string& txt, sf::Color col, int sz = 15) {
        sf::Text t(txt, font_, sz);
        t.setFillColor(col);
        t.setPosition((float)panelX, (float)y);
        window_.draw(t);
        y += sz + 6;
    };

    line("STATISTICS", sf::Color(150, 200, 255), 16);
    y += 4;
    line("Total Slots : " + std::to_string(total), sf::Color::White);
    line("Occupied    : " + std::to_string(used),  sf::Color(220, 80,  80));
    line("Available   : " + std::to_string(avail), sf::Color(80,  200, 80));

    float pct = total > 0 ? (float)used / total * 100.f : 0.f;
    std::ostringstream ss;
    ss << "Occupancy   : " << std::fixed << std::setprecision(1) << pct << "%";
    sf::Color pctColor = pct > 80 ? sf::Color(255,80,80)
                        : pct > 50 ? sf::Color(255,200,60)
                                   : sf::Color(80,220,80);
    line(ss.str(), pctColor);

    // Color legend
    y += 10;
    line("LEGEND", sf::Color(150, 200, 255), 14);

    auto legend = [&](sf::Color col, const std::string& txt) {
        sf::RectangleShape sq({14,14});
        sq.setFillColor(col);
        sq.setPosition((float)panelX, (float)y);
        window_.draw(sq);
        sf::Text lt(txt, font_, 13);
        lt.setFillColor(sf::Color::White);
        lt.setPosition((float)panelX + 20, (float)y - 1);
        window_.draw(lt);
        y += 20;
    };
    legend(sf::Color(40, 180, 80),  " Free slot");
    legend(sf::Color(200, 50, 50),  " Occupied");
    legend(sf::Color(220, 220, 0),  " Just changed");
    legend(sf::Color(255, 200, 0),  " Gate position");
}

void GUI::drawOccupancyBar() {
    int total  = mgr_.totalSlots();
    int used   = total - mgr_.availableCount();
    int winW   = (int)window_.getSize().x;
    int winH   = (int)window_.getSize().y;
    int barW   = (int)(winW * 0.65f) - 40;
    int barX   = 20;
    int barY   = winH - 32;
    int barH   = 18;

    // Background
    sf::RectangleShape bg({(float)barW, (float)barH});
    bg.setPosition((float)barX, (float)barY);
    bg.setFillColor(sf::Color(40, 40, 55));
    bg.setOutlineThickness(1);
    bg.setOutlineColor(sf::Color(80, 80, 100));
    window_.draw(bg);

    // Fill
    if (total > 0) {
        float ratio = (float)used / total;
        sf::RectangleShape fill({barW * ratio, (float)barH});
        fill.setPosition((float)barX, (float)barY);
        sf::Color fillColor = ratio > 0.8f ? sf::Color(220,60,60)
                            : ratio > 0.5f ? sf::Color(220,180,40)
                                           : sf::Color(50,180,80);
        fill.setFillColor(fillColor);
        window_.draw(fill);
    }

    // Label
    sf::Text lbl("Occupancy Bar", font_, 11);
    lbl.setFillColor(sf::Color(180,180,200));
    lbl.setPosition((float)barX + 4, (float)barY + 2);
    window_.draw(lbl);
}

void GUI::drawLog() {
    std::lock_guard<std::mutex> lk(logMtx_);
    int panelX = (int)(window_.getSize().x * 0.67f);
    int winH   = (int)window_.getSize().y;
    int startY = 260;
    int maxLines = (winH - startY - 20) / 16;

    sf::Text hdr("EVENT LOG", font_, 14);
    hdr.setFillColor(sf::Color(150,200,255));
    hdr.setPosition((float)panelX, (float)startY - 22);
    window_.draw(hdr);

    // Draw divider
    sf::RectangleShape div({(float)(window_.getSize().x - panelX - 10), 1});
    div.setPosition((float)panelX, (float)startY - 4);
    div.setFillColor(sf::Color(60,60,90));
    window_.draw(div);

    int show = std::min((int)logs_.size(), maxLines);
    int startIdx = (int)logs_.size() - show;
    for (int i = 0; i < show; ++i) {
        const std::string& line = logs_[startIdx + i];
        sf::Text t(line, font_, 11);

        // Color code by entry/exit
        sf::Color col = sf::Color(200,200,200);
        if (line.find("Entry") != std::string::npos) col = sf::Color(80,220,100);
        if (line.find("Exit")  != std::string::npos) col = sf::Color(220,100,100);
        if (line.find("FULL")  != std::string::npos) col = sf::Color(255,200,50);
        t.setFillColor(col);
        t.setPosition((float)panelX, (float)(startY + i * 16));
        window_.draw(t);
    }
}

void GUI::addLog(const std::string& msg) {
    std::lock_guard<std::mutex> lk(logMtx_);
    // Prefix with timestamp HH:MM:SS
    std::time_t now = std::time(nullptr);
    char buf[10];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&now));
    std::string entry = std::string(buf) + " " + msg;
    logs_.push_back(entry);
    if (logs_.size() > 200) logs_.pop_front();
}

void GUI::flashSlot(int slotId) {
    if (slotId >= 0 && slotId < (int)flashTimer_.size())
        flashTimer_[slotId] = 0.8f; // flash for 0.8 seconds
}
