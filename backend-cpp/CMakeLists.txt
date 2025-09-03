cmake_minimum_required(VERSION 3.25)

# Force MSVC LLVM/Clang compiler
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_C_COMPILER "clang")

# Set MSVC LLVM toolset
set(CMAKE_GENERATOR_TOOLSET "ClangCL")

# Require MSVC 2022 (17.0) or later for C++23 support
if(MSVC AND MSVC_VERSION LESS 1930)
    message(FATAL_ERROR "Microsoft Visual Studio 2022 or later is required for C++23 support")
endif()
project(pc-monitor-cpp VERSION 1.0.0 LANGUAGES CXX)

# Set target architecture to x64
set(CMAKE_GENERATOR_PLATFORM x64)
if(MSVC)
    set(CMAKE_VS_PLATFORM_NAME "x64")
endif()

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable modern C++ features
if(MSVC)
    if(CMAKE_GENERATOR_TOOLSET STREQUAL "ClangCL")
        # MSVC LLVM/Clang-cl specific options
        add_compile_options(/std:c++latest /utf-8)
        add_compile_options(-Wno-unused-command-line-argument)
        add_compile_options(-Wno-unknown-argument)
    else()
        # Regular MSVC
        add_compile_options(/std:c++20 /utf-8)
    endif()
    add_compile_definitions(_WIN32_WINNT=0x0A00) # Windows 10+
    add_compile_definitions(WIN32_LEAN_AND_MEAN)
    add_compile_definitions(NOMINMAX)
else()
    add_compile_options(-std=c++20 -fcoroutines)
endif()

# Include FetchContent module
include(FetchContent)

# Fetch nlohmann/json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
    GIT_SHALLOW TRUE
)

# Fetch httplib (lightweight HTTP library, header-only)
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.15.3
    GIT_SHALLOW TRUE
)

# Make dependencies available
FetchContent_MakeAvailable(nlohmann_json httplib)

# Find required packages
find_package(Threads REQUIRED)

# Create executable
add_executable(pc-monitor-cpp
    include/system_monitor.hpp
    include/web_server.hpp
    src/main.cpp
    src/system_monitor.cpp
    src/web_server.cpp
)

# Link libraries
target_link_libraries(pc-monitor-cpp PRIVATE
    Threads::Threads
    nlohmann_json::nlohmann_json
    httplib::httplib
)

# Windows specific libraries
if(WIN32)
    target_link_libraries(pc-monitor-cpp PRIVATE
        pdh.lib
        psapi.lib
        kernel32.lib
        user32.lib
        advapi32.lib
    )
endif()

# Include directories
target_include_directories(pc-monitor-cpp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Compiler specific options
if(MSVC OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(pc-monitor-cpp PRIVATE /W4)  # High warning level
    # target_compile_options(pc-monitor-cpp PRIVATE /WX)  # Treat warnings as errors
    target_compile_options(pc-monitor-cpp PRIVATE /permissive-) # Strict conformance
    # Disable warnings for external dependencies
    target_compile_options(pc-monitor-cpp PRIVATE /external:W0)    
    target_compile_options(pc-monitor-cpp PRIVATE /external:anglebrackets)
else()
    target_compile_options(pc-monitor-cpp PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# Enable coroutines
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(pc-monitor-cpp PRIVATE -fcoroutines)
endif()

# Set output directory
set_target_properties(pc-monitor-cpp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../dist/cpp
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/../dist/cpp
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/../dist/cpp
)

# Specific settings for MSVC LLVM toolset
if(CMAKE_GENERATOR_TOOLSET STREQUAL "ClangCL")
    target_compile_options(pc-monitor-cpp PRIVATE -Wno-unused-command-line-argument)
    target_compile_options(pc-monitor-cpp PRIVATE /external:W0)    
    target_compile_options(pc-monitor-cpp PRIVATE /external:anglebrackets)
endif()

