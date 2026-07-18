cmake_minimum_required(VERSION 3.16)
project(media_controller VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add Gui to the required Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core WebSockets Gui)
find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBCAMERA REQUIRED libcamera)

set(CMAKE_AUTOMOC ON)

add_executable(media_controller
    main.cpp
    MediaController.cpp
    MediaController.h
)

target_include_directories(media_controller PRIVATE ${LIBCAMERA_INCLUDE_DIRS})

target_compile_definitions(media_controller PRIVATE QT_NO_KEYWORDS)

# Link the Qt6::Gui library
target_link_libraries(media_controller PRIVATE
    Qt6::Core
    Qt6::WebSockets
    Qt6::Gui
    ${LIBCAMERA_LIBRARIES}
)