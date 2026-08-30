# Building Kick Drum Synthesizer

This document provides detailed instructions for building the Kick Drum Synthesizer from source.

## Prerequisites

### Required Tools

- **CMake** 3.20 or later
- **C++17 compatible compiler**:
  - macOS: Xcode Command Line Tools (Clang)
  - Linux: GCC 7+ or Clang 5+
  - Windows: Visual Studio 2019 or later
- **Git** (for cloning dependencies)

### Platform-Specific Requirements

#### macOS
- macOS 11.0 (Big Sur) or later
- Xcode Command Line Tools: `xcode-select --install`
- CoreAudio and CoreMIDI frameworks (included with macOS)

#### Linux
- ALSA development libraries: `sudo apt-get install libasound2-dev`
- JACK development libraries (optional): `sudo apt-get install libjack-dev`

#### Windows
- Visual Studio 2019 or later with C++ desktop development workload

### Optional Dependencies

#### VST3 SDK (Required for VST3 Plugin)

The VST3 SDK is required to build the VST3 plugin. Download it from:
https://github.com/steinbergmedia/vst3sdk

```bash
mkdir -p external
cd external
git clone --recursive https://github.com/steinbergmedia/vst3sdk.git
cd ..
```

#### Node.js (Required for Property-Based Tests)

Node.js 16+ is required to run property-based tests with fast-check.

Download from: https://nodejs.org/

## Quick Build

### Using the Build Script (macOS/Linux)

```bash
chmod +x build.sh
./build.sh
```

### Manual Build

```bash
# Create build directory
mkdir build
cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_VST3=OFF

# Build
cmake --build . --config Release

# Run tests
ctest --output-on-failure
```

## Build Options

Configure the build with CMake options:

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_VST3=ON \
    -DBUILD_STANDALONE=ON \
    -DBUILD_TESTS=ON \
    -DVST3_SDK_PATH=/path/to/vst3sdk
```

### Available Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_VST3` | ON | Build VST3 plugin |
| `BUILD_STANDALONE` | ON | Build standalone application |
| `BUILD_TESTS` | ON | Build test suite |
| `VST3_SDK_PATH` | `external/vst3sdk` | Path to VST3 SDK |
| `CMAKE_BUILD_TYPE` | Release | Build type (Debug/Release) |

## Build Configurations

### Debug Build

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug
```

Debug builds include:
- Debug symbols
- Assertions enabled
- No optimizations
- Verbose logging

### Release Build

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

Release builds include:
- Full optimizations
- No debug symbols
- Assertions disabled
- Minimal logging

### RelWithDebInfo Build

```bash
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build . --config RelWithDebInfo
```

Useful for profiling and debugging optimized code.

## Building Individual Components

### Audio Engine Only

```bash
cmake .. -DBUILD_VST3=OFF -DBUILD_STANDALONE=OFF
cmake --build . --target kick_drum_audio_engine
```

### VST3 Plugin Only (macOS)

```bash
cmake -G Xcode -S . -B cmake-build-vst3-xcode \
  -DBUILD_VST3=ON -DBUILD_STANDALONE=OFF -DBUILD_TESTS=OFF
cmake --build cmake-build-vst3-xcode --config Release --target KickDrumSynth
```

The Xcode generator is required by the bundled SDK's macOS Objective-C++
targets. Its post-build step runs Steinberg's validator.

### Standalone App Only

```bash
cmake .. -DBUILD_VST3=OFF
cmake --build . --target KickDrumSynthStandalone
```

### Tests Only

```bash
cmake .. -DBUILD_VST3=OFF -DBUILD_STANDALONE=OFF
cmake --build . --target kick_drum_tests
```

## Running Tests

### C++ Unit Tests

```bash
cd build
ctest --output-on-failure
# or
./bin/kick_drum_tests
```

### Property-Based Tests

```bash
cd tests/property
npm install
npm test
```

### Run All Tests

```bash
# C++ tests
cd build
ctest

# Property tests
cd ../tests/property
npm test
```

## Installation

### Install to System

```bash
cd build
sudo cmake --install .
```

This installs:
- VST3 plugin to `~/Library/Audio/Plug-Ins/VST3/` (macOS)
- Standalone app to `/Applications/` (macOS)
- Libraries to `/usr/local/lib/`

### Custom Install Location

```bash
cmake --install . --prefix /custom/path
```

## Troubleshooting

### VST3 SDK Not Found

**Error**: `VST3 SDK not found at external/vst3sdk`

**Solution**: Download the VST3 SDK:
```bash
mkdir -p external
cd external
git clone --recursive https://github.com/steinbergmedia/vst3sdk.git
```

Or specify a custom path:
```bash
cmake .. -DVST3_SDK_PATH=/path/to/vst3sdk
```

### CMake Version Too Old

**Error**: `CMake 3.20 or higher is required`

**Solution**: Update CMake:
- macOS: `brew upgrade cmake`
- Linux: Download from https://cmake.org/download/
- Windows: Download installer from https://cmake.org/download/

### Compiler Not Found

**Error**: `No CMAKE_CXX_COMPILER could be found`

**Solution**:
- macOS: Install Xcode Command Line Tools: `xcode-select --install`
- Linux: Install GCC: `sudo apt-get install build-essential`
- Windows: Install Visual Studio with C++ workload

### CoreAudio/CoreMIDI Not Found (macOS)

**Error**: `Could not find CoreAudio framework`

**Solution**: Ensure Xcode Command Line Tools are installed:
```bash
xcode-select --install
```

### Google Test Download Fails

**Error**: `Failed to download googletest`

**Solution**: Check internet connection or manually download:
```bash
cd build/_deps
git clone https://github.com/google/googletest.git googletest-src
```

### Build Fails with "undefined reference"

**Solution**: Clean and rebuild:
```bash
rm -rf build
mkdir build
cd build
cmake ..
cmake --build .
```

## Platform-Specific Notes

### macOS

#### Universal Binary (Apple Silicon + Intel)

```bash
cmake .. -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build .
```

#### Code Signing

For distribution, sign the binaries:
```bash
codesign --force --deep --sign "Developer ID" build/bin/KickDrumSynthStandalone.app
```

### Linux

#### JACK Support

To build with JACK audio support:
```bash
sudo apt-get install libjack-dev
cmake .. -DUSE_JACK=ON
cmake --build .
```

### Windows

#### Visual Studio

Open the generated solution:
```bash
cmake .. -G "Visual Studio 16 2019"
start KickDrumSynthesizer.sln
```

Or build from command line:
```bash
cmake --build . --config Release
```

## Performance Optimization

### Enable Link-Time Optimization (LTO)

```bash
cmake .. -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
cmake --build .
```

### Native CPU Optimization

```bash
cmake .. -DCMAKE_CXX_FLAGS="-march=native"
cmake --build .
```

**Warning**: Binaries will only run on CPUs with the same instruction set.

## Development Builds

### Fast Incremental Builds

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=OFF
cmake --build . -j$(nproc)
```

### Verbose Build Output

```bash
cmake --build . --verbose
```

### Clean Build

```bash
cmake --build . --target clean
# or
rm -rf build
```

## Continuous Integration

### GitHub Actions Example

```yaml
name: Build

on: [push, pull_request]

jobs:
  build:
    runs-on: macos-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install dependencies
        run: |
          brew install cmake
      - name: Build
        run: |
          mkdir build
          cd build
          cmake .. -DBUILD_VST3=OFF
          cmake --build .
      - name: Test
        run: |
          cd build
          ctest --output-on-failure
```

## Getting Help

If you encounter build issues:

1. Check this document for troubleshooting steps
2. Ensure all prerequisites are installed
3. Try a clean build (`rm -rf build`)
4. Check the CMake output for specific errors
5. Open an issue on GitHub with:
   - Your OS and version
   - CMake version (`cmake --version`)
   - Compiler version
   - Full error message
   - CMake configuration command used
