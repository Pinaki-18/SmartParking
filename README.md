# 🅿️ Smart Parking Management System

<div align="center">

![C++](https://img.shields.io/badge/C%2B%2B-17-blue?style=for-the-badge&logo=cplusplus)
![SQLite](https://img.shields.io/badge/SQLite-3-lightgrey?style=for-the-badge&logo=sqlite)
![OpenCV](https://img.shields.io/badge/OpenCV-4-green?style=for-the-badge&logo=opencv)
![License](https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge)

A production-grade **C++ parking management system** with real-time slot tracking, intelligent Dijkstra routing, multi-threaded gate simulation, and a live cyberpunk-themed web dashboard.

![Dashboard Preview](assets/dashboard.png)

</div>

---

## ✨ Features

- 🧠 **Dijkstra's Algorithm** — finds the nearest free slot from any gate in O((V+E) log V)
- ⚡ **Multi-threaded Gates** — each entry/exit gate runs on its own `std::thread`
- 🔒 **Thread Safety** — `std::mutex` + `std::condition_variable` for zero busy-waiting
- 📷 **ALPR** — OpenCV license plate recognition pipeline
- 🗄️ **SQLite Database** — persistent vehicle entry/exit logs
- 🌐 **Live Web Dashboard** — cyberpunk-themed UI with real-time stats, polling every second

---

## 🏗️ Architecture

```
Entry Gate (thread) ──┐
Entry Gate (thread) ──┤──► ParkingSlotManager ──► Dijkstra Router ──► Assign Slot
Entry Gate (thread) ──┤         (mutex + cv)
Entry Gate (thread) ──┘
                              │
                         SQLite Log
                              │
                    HTTP REST API (cpp-httplib)
                              │
                    Web Dashboard (localhost:8080)
```

---

## 🛠️ Tech Stack

| Component  | Technology                              |
|------------|-----------------------------------------|
| Language   | C++17                                   |
| Routing    | Dijkstra min-heap O((V+E)logV)          |
| Threading  | std::thread, std::mutex, std::condition_variable |
| Vision     | OpenCV (ALPR pipeline)                  |
| Database   | SQLite3                                 |
| HTTP Server| cpp-httplib (header-only)               |
| Frontend   | HTML / CSS / JS — cyberpunk dashboard   |

---

## 📁 Project Structure

```
SmartParking/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── main.cpp                   # SFML GUI mode
│   ├── main_server.cpp            # Web server mode
│   ├── ParkingSlotManager.h/.cpp  # Thread-safe slot manager
│   ├── Dijkstra.h/.cpp            # Shortest path routing
│   ├── Gate.h/.cpp                # Entry & exit gate logic
│   ├── ALPR.h/.cpp                # OpenCV license plate detection
│   ├── Database.h/.cpp            # SQLite3 vehicle logs
│   ├── Display.h/.cpp             # Console grid display
│   ├── GUI.h/.cpp                 # SFML graphical interface
│   ├── Server.h                   # REST API (cpp-httplib)
│   └── httplib.h                  # Header-only HTTP library
└── web/
    ├── index.html                 # Dashboard UI
    ├── css/style.css              # Cyberpunk styling
    └── js/app.js                  # Real-time polling logic
```

---

## 🚀 Build & Run

### Prerequisites (Ubuntu / WSL)
```bash
sudo apt install -y build-essential cmake libopencv-dev libsqlite3-dev libssl-dev
```

### Download httplib (required, not bundled)
```bash
curl -o src/httplib.h https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h
```

### Build
```bash
mkdir build && cd build
cmake .. && make -j$(nproc)
```

### Run — Web Server Mode
```bash
./SmartParkingServer          # default: 6x8 grid, port 8080
./SmartParkingServer 8 10 9090  # custom: 8x10 grid, port 9090
```
Then open **http://localhost:8080** in your browser 🚀

---

## 🌐 REST API Endpoints

| Method | Endpoint       | Description                          |
|--------|----------------|--------------------------------------|
| GET    | `/api/status`  | Full lot state as JSON               |
| GET    | `/api/logs`    | Recent 50 event logs                 |
| POST   | `/api/entry`   | Simulate car entry `{ "gate": 0 }`   |
| POST   | `/api/exit`    | Simulate car exit `{ "plate": "..." }`|
| GET    | `/`            | Serves the web dashboard             |

---

## 💡 Interview Talking Points

- **Why Dijkstra?** Guarantees shortest path from gate to nearest free slot. Min-heap gives O(log V) per extraction.
- **Why condition_variable?** Gates sleep when lot is full — zero CPU usage vs busy-waiting.
- **Why SQLite?** Lightweight embedded DB, no server needed, ACID compliant for concurrent writes.
- **Thread safety?** Single mutex guards the slot manager. Gates contend only at assignment, not during routing.

---

## 📄 License

This project is licensed under the **MIT License** — see the [LICENSE](LICENSE) file for details.

---

<div align="center">
Made with ❤️ and C++17
</div>
