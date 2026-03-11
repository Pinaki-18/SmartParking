cmake_minimum_required(VERSION 3.16)
project(SmartParking VERSION 3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)
find_package(SQLite3 QUIET)

if(NOT SQLite3_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SQLITE3 REQUIRED sqlite3)
    set(SQLite3_LIBRARIES    ${SQLITE3_LIBRARIES})
    set(SQLite3_INCLUDE_DIRS ${SQLITE3_INCLUDE_DIRS})
endif()

# ── Web Server (httplib is header-only, needs OpenSSL headers) ──
find_package(OpenSSL QUIET)

# ── SmartParking Web Server ──
add_executable(SmartParkingServer
    src/main_server.cpp
    src/ParkingSlotManager.cpp
    src/Dijkstra.cpp
    src/Gate.cpp
    src/ALPR.cpp
    src/Database.cpp
    src/Display.cpp
)

target_include_directories(SmartParkingServer PRIVATE
    src/
    ${OpenCV_INCLUDE_DIRS}
    ${SQLite3_INCLUDE_DIRS}
)

target_link_libraries(SmartParkingServer PRIVATE
    ${OpenCV_LIBS}
    Threads::Threads
    ${SQLite3_LIBRARIES}
)

if(OpenSSL_FOUND)
    target_link_libraries(SmartParkingServer PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(SmartParkingServer PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
endif()

target_compile_options(SmartParkingServer PRIVATE -Wall -O2)
