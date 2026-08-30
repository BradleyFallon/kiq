cmake_minimum_required(VERSION 3.20)

# CMake initializes the Apple toolchain during project(), so the deployment
# target must be selected first. Respect an explicit caller-provided value.
if(CMAKE_HOST_APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING
        "Minimum macOS deployment version" FORCE)
endif()

project(KickDrumSynthesizer VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_VST3 "Build VST3 plugin" ON)
option(BUILD_STANDALONE "Build standalone application" ON)
option(BUILD_TESTS "Build tests" ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find required packages
find_package(Threads REQUIRED)

# VST3 SDK path (user should set this or we'll try to find it)
set(VST3_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/vst3sdk" CACHE PATH "Path to VST3 SDK")

# Add subdirectories
add_subdirectory(src/audio_engine)
add_subdirectory(src/ui)
add_subdirectory(src/platform)

if(BUILD_VST3)
    add_subdirectory(src/vst3)
endif()

if(BUILD_STANDALONE)
    add_subdirectory(src/standalone)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation rules
install(TARGETS kick_drum_audio_engine
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
