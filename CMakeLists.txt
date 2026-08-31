cmake_minimum_required(VERSION 3.25)

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

# The graphical editor is shared by the VST3 and standalone targets. Make the
# bundled VSTGUI targets available before src/ui is configured. The full SDK is
# included for plug-in builds so the validator remains available; standalone-
# only builds include just VSTGUI.
if(BUILD_VST3 OR BUILD_STANDALONE)
    if(NOT EXISTS "${VST3_SDK_PATH}/vstgui4/CMakeLists.txt")
        message(FATAL_ERROR "Bundled VSTGUI not found below VST3_SDK_PATH: ${VST3_SDK_PATH}")
    endif()

    set(VSTGUI_STANDALONE ON CACHE BOOL "Build VSTGUI standalone support" FORCE)
    set(VSTGUI_STANDALONE_EXAMPLES OFF CACHE BOOL "Build VSTGUI examples" FORCE)
    set(VSTGUI_TOOLS OFF CACHE BOOL "Build VSTGUI tools" FORCE)
    set(VSTGUI_DISABLE_UNITTESTS ON CACHE BOOL "Build VSTGUI tests" FORCE)

    if(BUILD_VST3)
        set(SMTG_BUILD_UNIVERSAL_BINARY OFF CACHE BOOL "Build universal VST3 binaries")
        set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "Build SDK plug-in examples" FORCE)
        set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES ON CACHE BOOL "Build SDK validator" FORCE)
        add_subdirectory(${VST3_SDK_PATH} ${CMAKE_BINARY_DIR}/vst3sdk)
    else()
        add_subdirectory(${VST3_SDK_PATH}/vstgui4 ${CMAKE_BINARY_DIR}/vstgui4)
    endif()

    # VSTGUI 4.15 still uses std::wstring_convert on macOS. Xcode 26 marks it
    # deprecated, while the upstream target promotes warnings to errors.
    # Keep the warning visible for project code without letting this SDK-only
    # deprecation prevent an otherwise valid build.
    if(APPLE AND TARGET vstgui)
        target_compile_options(vstgui PRIVATE -Wno-error=deprecated-declarations)
    endif()
endif()

# Add subdirectories
add_subdirectory(src/audio_engine)
add_subdirectory(src/platform)

if(BUILD_VST3 OR BUILD_STANDALONE)
    add_subdirectory(src/ui)
endif()

if(BUILD_VST3)
    add_subdirectory(src/vst3)
endif()

if(BUILD_STANDALONE)
    if(NOT APPLE)
        message(FATAL_ERROR "The standalone application currently requires macOS")
    endif()
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
