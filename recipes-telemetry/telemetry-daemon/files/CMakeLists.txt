cmake_minimum_required(VERSION 3.16)
project(TelemetryDaemon VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CRITICAL FIX: Instructs CMake to invoke the Qt Meta-Object Compiler 
# for headers containing the Q_OBJECT macro to resolve vtable references.
set(CMAKE_AUTOMOC ON)

# Locate Qt6 modules required for headless execution and WebSocket protocols
find_package(Qt6 REQUIRED COMPONENTS Core WebSockets Network)

add_executable(telemetry_daemon
    main.cpp
    TelemetryServer.cpp
    TelemetryServer.h
)

# Link native Qt libraries to the compiled binary target
target_link_libraries(telemetry_daemon PRIVATE 
    Qt6::Core 
    Qt6::WebSockets 
    Qt6::Network
)