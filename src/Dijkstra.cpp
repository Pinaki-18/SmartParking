#include "Dijkstra.h"
#include <queue>
#include <climits>
#include <array>
#include <algorithm>

int Dijkstra::nearestAvailable(
    int rows, int cols,
    int srcRow, int srcCol,
    const std::vector<bool>& occupied)
{
    // Min-heap: (distance, row, col)
    using T = std::tuple<int,int,int>;
    std::priority_queue<T, std::vector<T>, std::greater<T>> pq;

    // Distance matrix, all infinity initially
    std::vector<std::vector<int>> dist(rows, std::vector<int>(cols, INT_MAX));

    // Clamp start position to grid bounds
    int sr = std::clamp(srcRow, 0, rows - 1);
    int sc = std::clamp(srcCol, 0, cols - 1);

    dist[sr][sc] = 0;
    pq.push({0, sr, sc});

    // 4-directional grid movement (no diagonals)
    const std::array<std::pair<int,int>, 4> dirs = {{{-1,0},{1,0},{0,-1},{0,1}}};

    while (!pq.empty()) {
        auto [d, r, c] = pq.top(); pq.pop();

        // Discard stale entries
        if (d > dist[r][c]) continue;

        int slotId = r * cols + c;

        // First unoccupied slot we reach = nearest (Dijkstra guarantees this)
        if (!occupied[slotId]) {
            return slotId;
        }

        // Expand to neighbors
        for (auto [dr, dc] : dirs) {
            int nr = r + dr, nc = c + dc;
            if (nr >= 0 && nr < rows && nc >= 0 && nc < cols) {
                int nd = d + 1;
                if (nd < dist[nr][nc]) {
                    dist[nr][nc] = nd;
                    pq.push({nd, nr, nc});
                }
            }
        }
    }

    return -1; // No available slot found
}
