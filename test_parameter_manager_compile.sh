#!/bin/bash

# Simple compilation test for ParameterManager
# This verifies the code compiles and basic functionality works

echo "=== Testing ParameterManager Compilation ==="

# Create a temporary directory for compilation
TEMP_DIR=$(mktemp -d)
echo "Using temp directory: $TEMP_DIR"

# Copy source files
cp src/audio_engine/parameters/Parameter.cpp $TEMP_DIR/
cp src/audio_engine/parameters/Parameter.h $TEMP_DIR/
cp src/audio_engine/parameters/ParameterManager.cpp $TEMP_DIR/
cp src/audio_engine/parameters/ParameterManager.h $TEMP_DIR/

# Create a simple test file
cat > $TEMP_DIR/test_main.cpp << 'EOF'
#include "ParameterManager.h"
#include <iostream>
#include <cassert>
#include <algorithm>

using namespace KickDrum;

void testBasicFunctionality() {
    ParameterManager manager;
    
    // Test empty manager
    assert(manager.getParameterCount() == 0);
    assert(!manager.hasParameter("anything"));
    std::cout << "✓ Empty manager works" << std::endl;
    
    // Test parameter registration
    Parameter param("testParam", "Test Parameter", 50.0f, 0.0f, 100.0f, "%");
    assert(manager.registerParameter(param));
    assert(manager.hasParameter("testParam"));
    assert(manager.getParameterCount() == 1);
    std::cout << "✓ Parameter registration works" << std::endl;
    
    // Test duplicate registration
    Parameter duplicate("testParam", "Duplicate", 75.0f, 0.0f, 100.0f, "%");
    assert(!manager.registerParameter(duplicate));
    assert(manager.getParameterCount() == 1);
    std::cout << "✓ Duplicate registration prevention works" << std::endl;
    
    // Test getParameter
    Parameter* retrieved = manager.getParameter("testParam");
    assert(retrieved != nullptr);
    assert(retrieved->getId() == "testParam");
    assert(retrieved->getValue() == 50.0f);
    std::cout << "✓ getParameter works" << std::endl;
    
    // Test setParameterValue and getParameterValue
    assert(manager.setParameterValue("testParam", 75.0f));
    assert(manager.getParameterValue("testParam") == 75.0f);
    std::cout << "✓ setParameterValue and getParameterValue work" << std::endl;
    
    // Test normalized access
    assert(manager.setParameterNormalized("testParam", 0.5f));
    assert(manager.getParameterValue("testParam") == 50.0f);
    assert(manager.getParameterNormalized("testParam") == 0.5f);
    std::cout << "✓ Normalized parameter access works" << std::endl;
    
    // Test reset
    manager.setParameterValue("testParam", 100.0f);
    assert(manager.resetParameter("testParam"));
    assert(manager.getParameterValue("testParam") == 50.0f);
    std::cout << "✓ resetParameter works" << std::endl;
}

void testMultipleParameters() {
    ParameterManager manager;
    
    manager.registerParameter(Parameter("param1", "Param 1", 25.0f, 0.0f, 100.0f, "%"));
    manager.registerParameter(Parameter("param2", "Param 2", 50.0f, 0.0f, 100.0f, "%"));
    manager.registerParameter(Parameter("param3", "Param 3", 75.0f, 0.0f, 100.0f, "%"));
    
    assert(manager.getParameterCount() == 3);
    std::cout << "✓ Multiple parameter registration works" << std::endl;
    
    // Test getParameterIds
    std::vector<std::string> ids = manager.getParameterIds();
    assert(ids.size() == 3);
    assert(std::find(ids.begin(), ids.end(), "param1") != ids.end());
    assert(std::find(ids.begin(), ids.end(), "param2") != ids.end());
    assert(std::find(ids.begin(), ids.end(), "param3") != ids.end());
    std::cout << "✓ getParameterIds works" << std::endl;
    
    // Modify all parameters
    manager.setParameterValue("param1", 100.0f);
    manager.setParameterValue("param2", 0.0f);
    manager.setParameterValue("param3", 50.0f);
    
    // Reset all
    manager.resetAllParameters();
    assert(manager.getParameterValue("param1") == 25.0f);
    assert(manager.getParameterValue("param2") == 50.0f);
    assert(manager.getParameterValue("param3") == 75.0f);
    std::cout << "✓ resetAllParameters works" << std::endl;
}

void testSynthesisParameters() {
    ParameterManager manager;
    
    manager.registerAllSynthesisParameters();
    
    // Check that all expected parameters are registered
    assert(manager.hasParameter("basePitch"));
    assert(manager.hasParameter("sineLevel"));
    assert(manager.hasParameter("harmonicRatio"));
    assert(manager.hasParameter("harmonicLevel"));
    assert(manager.hasParameter("harmonicModDepth"));
    assert(manager.hasParameter("noiseLevel"));
    assert(manager.hasParameter("noiseModDepth"));
    assert(manager.hasParameter("warmUpDuration"));
    assert(manager.hasParameter("attack"));
    assert(manager.hasParameter("decay"));
    assert(manager.hasParameter("sustain"));
    assert(manager.hasParameter("release"));
    assert(manager.hasParameter("pitchEnvelopeDepth"));
    assert(manager.hasParameter("compressorThreshold"));
    assert(manager.hasParameter("compressorRatio"));
    assert(manager.hasParameter("reverbRoomSize"));
    assert(manager.hasParameter("reverbDecayTime"));
    assert(manager.hasParameter("masterLevel"));
    assert(manager.hasParameter("pitchTracking"));
    
    std::cout << "✓ All synthesis parameters registered" << std::endl;
    std::cout << "  Total parameters: " << manager.getParameterCount() << std::endl;
    
    // Test some default values
    assert(manager.getParameterValue("basePitch") == 50.0f);
    assert(manager.getParameterValue("sineLevel") == 80.0f);
    assert(manager.getParameterValue("harmonicRatio") == 2.0f);
    assert(manager.getParameterValue("masterLevel") == 80.0f);
    std::cout << "✓ Synthesis parameter defaults correct" << std::endl;
    
    // Test parameter ranges
    const Parameter* basePitch = manager.getParameter("basePitch");
    assert(basePitch != nullptr);
    assert(basePitch->getMinValue() == 20.0f);
    assert(basePitch->getMaxValue() == 200.0f);
    std::cout << "✓ Synthesis parameter ranges correct" << std::endl;
    
    // Test modification
    manager.setParameterValue("basePitch", 100.0f);
    assert(manager.getParameterValue("basePitch") == 100.0f);
    manager.resetParameter("basePitch");
    assert(manager.getParameterValue("basePitch") == 50.0f);
    std::cout << "✓ Synthesis parameter modification works" << std::endl;
}

int main() {
    std::cout << "Testing ParameterManager implementation..." << std::endl;
    std::cout << std::endl;
    
    try {
        testBasicFunctionality();
        std::cout << std::endl;
        testMultipleParameters();
        std::cout << std::endl;
        testSynthesisParameters();
        std::cout << std::endl;
        std::cout << "=== All tests passed! ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
EOF

# Compile
echo ""
echo "Compiling..."
g++ -std=c++17 -I$TEMP_DIR -o $TEMP_DIR/test_parameter_manager \
    $TEMP_DIR/Parameter.cpp \
    $TEMP_DIR/ParameterManager.cpp \
    $TEMP_DIR/test_main.cpp

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful"
    echo ""
    echo "Running tests..."
    echo ""
    $TEMP_DIR/test_parameter_manager
    TEST_RESULT=$?
    echo ""
    
    # Cleanup
    rm -rf $TEMP_DIR
    
    if [ $TEST_RESULT -eq 0 ]; then
        echo "=== ParameterManager implementation verified! ==="
        exit 0
    else
        echo "=== Tests failed ==="
        exit 1
    fi
else
    echo "✗ Compilation failed"
    rm -rf $TEMP_DIR
    exit 1
fi
