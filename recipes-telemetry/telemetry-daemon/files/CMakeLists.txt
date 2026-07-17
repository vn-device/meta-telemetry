cmake_minimum_required(VERSION 3.16)
project(telemetry_daemon VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core WebSockets)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED libgpiodcxx)

set(CMAKE_AUTOMOC ON)

add_executable(telemetry_daemon
    main.cpp
    TelemetryServer.cpp
    TelemetryServer.h
    HardwareMonitor.cpp
    HardwareMonitor.h
)

target_include_directories(telemetry_daemon PRIVATE ${GPIOD_INCLUDE_DIRS})

target_link_libraries(telemetry_daemon PRIVATE
    Qt6::Core
    Qt6::WebSockets
    ${GPIOD_LIBRARIES}
)