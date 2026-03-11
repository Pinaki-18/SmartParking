# Smart Parking Management System

A high-performance, multi-threaded C++17 parking simulation with:
- **Dijkstra's Algorithm** — routes each car to the nearest free slot
- **Multi-threading** — 4 entry + 4 exit gate threads with `std::mutex` & `std::condition_variable`
- **OpenCV ALPR** — license plate region detection pipeline
- **SQLite3** — persistent entry/exit vehicle logs
- **Console Display** — real-time grid visualization

---

## Project Structure

```
SmartParking/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── main.cpp               # Thread orchestration + simulation
│   ├── Dijkstra.h / .cpp      # Grid-based shortest path to nearest slot
│   ├── ParkingSlotManager.h / .cpp  # Thread-safe slot manager
│   ├── Gate.h / .cpp          # Entry & exit gate logic
│   ├── ALPR.h / .cpp          # OpenCV license plate detection
│   ├── Database.h / .cpp      # SQLite3 vehicle log
│   └── Display.h / .cpp       # Console grid display
└── build/                     # CMake build output
```

---

## Dependencies

| Library  | Purpose              | Install (Ubuntu)                        |
|----------|----------------------|-----------------------------------------|
| OpenCV 4 | ALPR image processing| `sudo apt install libopencv-dev`        |
| SQLite3  | Vehicle log storage  | `sudo apt install libsqlite3-dev`       |
| pthread  | Threading (included) | Part of GCC/Clang on Linux              |

---

## Build Instructions

```bash
# 1. Install dependencies
sudo apt update
sudo apt install -y cmake build-essential libopencv-dev libsqlite3-dev

# 2. Build
cd SmartParking
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# 3. Run (defaults: 6x8 grid, 30 cars)
./SmartParking

# Custom: 10x12 grid, 50 cars
./SmartParking 10 12 50
```

---

## Key Design Decisions

### Thread Synchronization
- `std::mutex` protects all reads/writes to the slot grid
- `std::condition_variable` lets entry gate threads **sleep** when the lot is full
- `cv_.notify_all()` in `releaseSlot()` wakes all sleeping entry gates; each re-checks the predicate

### Dijkstra Routing
- Grid modelled as a graph: each slot = node, adjacent slots = edges (weight=1)
- Entry gates at the 4 grid corners; Dijkstra fans outward from each gate
- First unoccupied node reached = nearest available slot (BFS/Dijkstra property)

### ALPR Pipeline
- Grayscale → Bilateral filter → Canny edges → Contour detection
- Aspect ratio filter (2.0–5.5) isolates plate-shaped rectangles
- Extend with Tesseract OCR for real plate reading (see ALPR.cpp comments)

---

## Example Output

```
 Smart Parking Management System

Grid: 6x8  (48 slots)  |  30 cars to simulate

[Entry G0] KA-03-XY-4521 → Slot #0   (free: 47)
[Entry G1] MH-09-AB-8823 → Slot #7   (free: 46)
[Entry G2] TN-22-PQ-0034 → Slot #40  (free: 45)
...

  Gate0                       Gate1
  G0   0  1  2  3  4  5  6  7 G1
     ------------------------
 0 | [X][X][ ][ ][ ][ ][X][X] |
 1 | [X][ ][ ][ ][ ][ ][ ][X] |
...

Occupancy: [############################--------] 30/48  (18 free)

====== Recent Vehicle Logs ======
Plate           Slot   Gate   Entry       Exit
------------------------------------------------------
KA-03-XY-4521   0      0      14:23:01    14:23:08
...
```
