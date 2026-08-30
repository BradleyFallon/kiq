#!/bin/bash
# Build script for Kick Drum Synthesizer

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Kick Drum Synthesizer Build Script ===${NC}"

# Check if VST3 SDK exists
if [ ! -d "external/vst3sdk" ]; then
    echo -e "${YELLOW}Warning: VST3 SDK not found at external/vst3sdk${NC}"
    echo -e "${YELLOW}VST3 plugin will not be built.${NC}"
    echo -e "${YELLOW}To build VST3 plugin, run:${NC}"
    echo -e "${YELLOW}  mkdir -p external${NC}"
    echo -e "${YELLOW}  cd external${NC}"
    echo -e "${YELLOW}  git clone --recursive https://github.com/steinbergmedia/vst3sdk.git${NC}"
    echo ""
fi

# Create build directory
echo -e "${GREEN}Creating build directory...${NC}"
mkdir -p build
cd build

# Configure with CMake
echo -e "${GREEN}Configuring with CMake...${NC}"
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_VST3=ON \
    -DBUILD_STANDALONE=ON \
    -DBUILD_TESTS=ON

# Build
echo -e "${GREEN}Building...${NC}"
cmake --build . --config Release -j$(sysctl -n hw.ncpu)

# Run tests
echo -e "${GREEN}Running tests...${NC}"
ctest --output-on-failure

echo -e "${GREEN}=== Build Complete ===${NC}"
echo ""
echo -e "${GREEN}Binaries are located in:${NC}"
echo -e "  build/bin/"
echo ""
echo -e "${GREEN}To install:${NC}"
echo -e "  cd build && cmake --install ."
