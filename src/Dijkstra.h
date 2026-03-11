#pragma once
#include <vector>

/**
 * Grid-based Dijkstra implementation.
 * Used to route each incoming vehicle from a gate position
 * to the nearest available parking slot on the grid.
 */
class Dijkstra {
public:
    /**
     * Finds the nearest unoccupied slot in the parking grid.
     *
     * @param rows     Number of rows in the grid
     * @param cols     Number of columns in the grid
     * @param srcRow   Starting row (gate's row position)
     * @param srcCol   Starting col (gate's col position)
     * @param occupied Flat vector [rows*cols] - true means slot is taken
     * @return         Index of nearest free slot (row*cols + col), or -1
     */
    static int nearestAvailable(
        int rows, int cols,
        int srcRow, int srcCol,
        const std::vector<bool>& occupied
    );
};
