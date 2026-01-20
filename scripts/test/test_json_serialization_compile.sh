#!/bin/bash

# Simple compilation test for JSON Serialization
# This verifies the code compiles and basic functionality works

echo "=== Testing JSON Serialization ==="

# Create a temporary directory for compilation
TEMP_DIR=$(mktemp -d)
echo "Using temp directory: $TEMP_DIR"

# Copy source files
cp src/audio_engine/parameters/Parameter.cpp $TEMP_DIR/
cp src/audio_engine/parameters/Parameter.h $TEMP_DIR/
cp src/audio_engine/parameters/ParameterManager.cpp $TEMP_DIR/
cp src/audio_engine/parameters/ParameterManager.h $TEMP_DIR/
cp src/audio_engine/utils/JSONSerializer.cpp $TEMP_DIR/
cp src/audio_engine/utils/JSONSerializer.h $TEMP_DIR/

# Create a simple test file
cat > $TEMP_DIR/test_main.cpp << 'EOF'
#include "ParameterManager.h"
#include "JSONSerializer.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace KickDrum;

bool approxEqual(float a, float b, float epsilon = 0.001f) {
    return std::abs(a - b) < epsilon;
}

void testBasicSerialization() {
    ParameterManager manager;
    manager.registerParameter(Parameter("param1", "Param 1", 25.0f, 0.0f, 100.0f, "%"));
    manager.registerParameter(Parameter("param2", "Param 2", 50.0f, 0.0f, 100.0f, "%"));
    manager.registerParameter(Parameter("param3", "Param 3", 75.0f, 0.0f, 100.0f, "%"));
    
    // Serialize
    std::string json = manager.serializeToJSON("1.0.0");
    std::cout << "Serialized JSON:" << std::endl;
    std::cout << json << std::endl;
    std::cout << std::endl;
    
    // Check that JSON contains expected content
    assert(json.find("\"version\"") != std::string::npos);
    assert(json.find("\"1.0.0\"") != std::string::npos);
    assert(json.find("\"parameters\"") != std::string::npos);
    assert(json.find("\"param1\"") != std::string::npos);
    assert(json.find("\"param2\"") != std::string::npos);
    assert(json.find("\"param3\"") != std::string::npos);
    std::cout << "✓ Serialization produces valid JSON structure" << std::endl;
}

void testRoundTrip() {
    ParameterManager manager1;
    manager1.registerParameter(Parameter("basePitch", "Base Pitch", 50.0f, 20.0f, 200.0f, "Hz"));
    manager1.registerParameter(Parameter("sineLevel", "Sine Level", 80.0f, 0.0f, 100.0f, "%"));
    manager1.registerParameter(Parameter("harmonicRatio", "Harmonic Ratio", 2.0f, 0.5f, 8.0f, "x"));
    
    // Modify some values
    manager1.setParameterValue("basePitch", 100.0f);
    manager1.setParameterValue("sineLevel", 60.0f);
    manager1.setParameterValue("harmonicRatio", 4.5f);
    
    // Serialize
    std::string json = manager1.serializeToJSON("1.0.0");
    
    // Create new manager and deserialize
    ParameterManager manager2;
    manager2.registerParameter(Parameter("basePitch", "Base Pitch", 50.0f, 20.0f, 200.0f, "Hz"));
    manager2.registerParameter(Parameter("sineLevel", "Sine Level", 80.0f, 0.0f, 100.0f, "%"));
    manager2.registerParameter(Parameter("harmonicRatio", "Harmonic Ratio", 2.0f, 0.5f, 8.0f, "x"));
    
    std::string version;
    assert(manager2.deserializeFromJSON(json, version));
    assert(version == "1.0.0");
    
    // Check values match
    assert(approxEqual(manager2.getParameterValue("basePitch"), 100.0f));
    assert(approxEqual(manager2.getParameterValue("sineLevel"), 60.0f));
    assert(approxEqual(manager2.getParameterValue("harmonicRatio"), 4.5f));
    
    std::cout << "✓ Round-trip serialization/deserialization works" << std::endl;
}

void testSynthesisParametersRoundTrip() {
    ParameterManager manager1;
    manager1.registerAllSynthesisParameters();
    
    // Modify several parameters
    manager1.setParameterValue("basePitch", 75.0f);
    manager1.setParameterValue("sineLevel", 90.0f);
    manager1.setParameterValue("harmonicRatio", 3.5f);
    manager1.setParameterValue("attack", 5.0f);
    manager1.setParameterValue("decay", 1000.0f);
    manager1.setParameterValue("compressorThreshold", -20.0f);
    manager1.setParameterValue("masterLevel", 70.0f);
    
    // Serialize
    std::string json = manager1.serializeToJSON("1.0.0");
    
    // Create new manager and deserialize
    ParameterManager manager2;
    manager2.registerAllSynthesisParameters();
    
    assert(manager2.deserializeFromJSON(json));
    
    // Check all modified values
    assert(approxEqual(manager2.getParameterValue("basePitch"), 75.0f));
    assert(approxEqual(manager2.getParameterValue("sineLevel"), 90.0f));
    assert(approxEqual(manager2.getParameterValue("harmonicRatio"), 3.5f));
    assert(approxEqual(manager2.getParameterValue("attack"), 5.0f));
    assert(approxEqual(manager2.getParameterValue("decay"), 1000.0f));
    assert(approxEqual(manager2.getParameterValue("compressorThreshold"), -20.0f));
    assert(approxEqual(manager2.getParameterValue("masterLevel"), 70.0f));
    
    // Check unmodified values remain at defaults
    assert(approxEqual(manager2.getParameterValue("noiseLevel"), 20.0f));
    assert(approxEqual(manager2.getParameterValue("warmUpDuration"), 20.0f));
    
    std::cout << "✓ Full synthesis parameters round-trip works" << std::endl;
}

void testInvalidJSON() {
    ParameterManager manager;
    manager.registerParameter(Parameter("test", "Test", 50.0f, 0.0f, 100.0f, "%"));
    
    // Test invalid JSON
    assert(!manager.deserializeFromJSON("not json"));
    assert(!manager.deserializeFromJSON("{incomplete"));
    assert(!manager.deserializeFromJSON("{}"));  // Missing required fields
    
    // Parameter should remain unchanged
    assert(approxEqual(manager.getParameterValue("test"), 50.0f));
    
    std::cout << "✓ Invalid JSON handling works" << std::endl;
}

void testForwardCompatibility() {
    // Simulate loading JSON with unknown parameters (forward compatibility)
    ParameterManager manager;
    manager.registerParameter(Parameter("param1", "Param 1", 25.0f, 0.0f, 100.0f, "%"));
    manager.registerParameter(Parameter("param2", "Param 2", 50.0f, 0.0f, 100.0f, "%"));
    
    // JSON with extra parameter that doesn't exist in manager
    std::string json = R"({
  "version": "2.0.0",
  "parameters": {
    "param1": 75.0,
    "param2": 80.0,
    "futureParam": 99.0
  }
})";
    
    std::string version;
    assert(manager.deserializeFromJSON(json, version));
    assert(version == "2.0.0");
    
    // Known parameters should be updated
    assert(approxEqual(manager.getParameterValue("param1"), 75.0f));
    assert(approxEqual(manager.getParameterValue("param2"), 80.0f));
    
    // Unknown parameter should be silently ignored
    assert(!manager.hasParameter("futureParam"));
    
    std::cout << "✓ Forward compatibility (unknown parameters) works" << std::endl;
}

void testVersionInformation() {
    ParameterManager manager;
    manager.registerParameter(Parameter("test", "Test", 50.0f, 0.0f, 100.0f, "%"));
    
    // Serialize with custom version
    std::string json = manager.serializeToJSON("1.2.3");
    
    // Deserialize and check version
    ParameterManager manager2;
    manager2.registerParameter(Parameter("test", "Test", 50.0f, 0.0f, 100.0f, "%"));
    
    std::string version;
    assert(manager2.deserializeFromJSON(json, version));
    assert(version == "1.2.3");
    
    std::cout << "✓ Version information handling works" << std::endl;
}

int main() {
    std::cout << "Testing JSON Serialization..." << std::endl;
    std::cout << std::endl;
    
    try {
        testBasicSerialization();
        std::cout << std::endl;
        testRoundTrip();
        testSynthesisParametersRoundTrip();
        testInvalidJSON();
        testForwardCompatibility();
        testVersionInformation();
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
g++ -std=c++17 -I$TEMP_DIR -o $TEMP_DIR/test_json \
    $TEMP_DIR/Parameter.cpp \
    $TEMP_DIR/ParameterManager.cpp \
    $TEMP_DIR/JSONSerializer.cpp \
    $TEMP_DIR/test_main.cpp \
    -DTEST_BUILD

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful"
    echo ""
    echo "Running tests..."
    echo ""
    $TEMP_DIR/test_json
    TEST_RESULT=$?
    echo ""
    
    # Cleanup
    rm -rf $TEMP_DIR
    
    if [ $TEST_RESULT -eq 0 ]; then
        echo "=== JSON Serialization implementation verified! ==="
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
