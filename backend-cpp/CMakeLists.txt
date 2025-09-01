cmake_minimum_required(VERSION 3.25)
project(pc-monitor-cpp VERSION 1.0.0 LANGUAGES CXX)

# Set C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable modern C++ features
if(MSVC)
    add_compile_options(/std:c++latest /permissive-)
    add_compile_definitions(_WIN32_WINNT=0x0A00) # Windows 10+
else()
    add_compile_options(-std=c++23 -fcoroutines)
endif()

# Find required packages
find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(httplib REQUIRED)

# Create executable
add_executable(pc-monitor
    src/main.cpp
    src/system_monitor.cpp
    src/web_server.cpp
    src/cpu_monitor.cpp
    src/memory_monitor.cpp
    src/websocket_handler.cpp
)

# Link libraries
target_link_libraries(pc-monitor PRIVATE
    Threads::Threads
    nlohmann_json::nlohmann_json
    httplib::httplib
)

# Windows specific libraries
if(WIN32)
    target_link_libraries(pc-monitor PRIVATE
        pdh.lib
        psapi.lib
        kernel32.lib
        user32.lib
        advapi32.lib
    )
endif()

# Include directories
target_include_directories(pc-monitor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Compiler specific options
if(MSVC)
    target_compile_options(pc-monitor PRIVATE /W4 /WX)
else()
    target_compile_options(pc-monitor PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# Enable coroutines
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(pc-monitor PRIVATE -fcoroutines)
endif()