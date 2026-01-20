#!/bin/bash

# Simple compilation test for PresetManager
# This verifies the code compiles and basic functionality works

echo "=== Testing PresetManager Compilation ==="

# Create a temporary directory for compilation
TEMP_DIR=$(mktemp -d)
echo "Using temp directory: $TEMP_DIR"

# Copy source files
cp src/audio_engine/presets/PresetManager.cpp $TEMP_DIR/
cp src/audio_engine/presets/PresetManager.h $TEMP_DIR/
cp src/audio_engine/presets/Preset.cpp $TEMP_DIR/
cp src/audio_engine/presets/Preset.h $TEMP_DIR/

# Create test directories
TEST_FACTORY_DIR="$TEMP_DIR/factory_presets"
TEST_USER_DIR="$TEMP_DIR/user_presets"
mkdir -p "$TEST_FACTORY_DIR"
mkdir -p "$TEST_USER_DIR"

# Create a simple test file
cat > $TEMP_DIR/test_main.cpp << 'EOF'
#include "PresetManager.h"
#include <iostream>
#include <cassert>
#include <fstream>

using namespace KickDrum;

void createTestPreset(const std::string& path, const std::string& name, float basePitch) {
    Preset preset(name, "1.0.0");
    preset.setParameter("basePitch", basePitch);
    preset.setParameter("sineLevel", 0.8f);
    
    std::ofstream file(path);
    file << preset.toJSON();
    file.close();
}

void testBasicFunctionality() {
    std::cout << "Testing basic functionality..." << std::endl;
    
    // Create test directories
    std::string factoryDir = "/tmp/test_factory_presets";
    std::string userDir = "/tmp/test_user_presets";
    
    system(("mkdir -p " + factoryDir).c_str());
    system(("mkdir -p " + userDir).c_str());
    
    // Create factory presets
    createTestPreset(factoryDir + "/Factory1.kdpreset", "Factory Preset 1", 50.0f);
    createTestPreset(factoryDir + "/Factory2.kdpreset", "Factory Preset 2", 60.0f);
    createTestPreset(factoryDir + "/Factory3.kdpreset", "Factory Preset 3", 70.0f);
    
    // Create preset manager
    PresetManager manager(factoryDir, userDir);
    assert(manager.initialize());
    std::cout << "✓ PresetManager initialized" << std::endl;
    
    // Test preset count
    assert(manager.getPresetCount() == 3);
    assert(manager.getFactoryPresetCount() == 3);
    assert(manager.getUserPresetCount() == 0);
    std::cout << "✓ Preset counts correct" << std::endl;
    
    // Test current preset
    assert(manager.getCurrentPresetIndex() == 0);
    const Preset* current = manager.getCurrentPreset();
    assert(current != nullptr);
    std::cout << "✓ Current preset accessible" << std::endl;
    
    // Test get preset by index
    const Preset* preset1 = manager.getPreset(0);
    assert(preset1 != nullptr);
    // Preset names may be in any order depending on directory listing
    std::string name = preset1->getName();
    assert(name == "Factory Preset 1" || name == "Factory Preset 2" || name == "Factory Preset 3");
    std::cout << "✓ Get preset by index works" << std::endl;
    
    // Test preset name
    std::string name1 = manager.getPresetName(1);
    assert(name1 == "Factory Preset 1" || name1 == "Factory Preset 2" || name1 == "Factory Preset 3");
    std::cout << "✓ Get preset name works" << std::endl;
    
    // Test is factory preset
    assert(manager.isFactoryPreset(0) == true);
    std::cout << "✓ Is factory preset works" << std::endl;
    
    // Cleanup
    system(("rm -rf " + factoryDir).c_str());
    system(("rm -rf " + userDir).c_str());
}

void testNavigation() {
    std::cout << "\nTesting navigation..." << std::endl;
    
    std::string factoryDir = "/tmp/test_factory_presets";
    std::string userDir = "/tmp/test_user_presets";
    
    system(("mkdir -p " + factoryDir).c_str());
    system(("mkdir -p " + userDir).c_str());
    
    createTestPreset(factoryDir + "/Preset1.kdpreset", "Preset 1", 50.0f);
    createTestPreset(factoryDir + "/Preset2.kdpreset", "Preset 2", 60.0f);
    createTestPreset(factoryDir + "/Preset3.kdpreset", "Preset 3", 70.0f);
    
    PresetManager manager(factoryDir, userDir);
    manager.initialize();
    
    // Test load preset
    assert(manager.loadPreset(1));
    assert(manager.getCurrentPresetIndex() == 1);
    std::cout << "✓ Load preset works" << std::endl;
    
    // Test next preset
    assert(manager.nextPreset());
    assert(manager.getCurrentPresetIndex() == 2);
    std::cout << "✓ Next preset works" << std::endl;
    
    // Test next preset wrap around
    assert(manager.nextPreset());
    assert(manager.getCurrentPresetIndex() == 0);
    std::cout << "✓ Next preset wraps around" << std::endl;
    
    // Test previous preset
    assert(manager.previousPreset());
    assert(manager.getCurrentPresetIndex() == 2);
    std::cout << "✓ Previous preset wraps around" << std::endl;
    
    assert(manager.previousPreset());
    assert(manager.getCurrentPresetIndex() == 1);
    std::cout << "✓ Previous preset works" << std::endl;
    
    // Cleanup
    system(("rm -rf " + factoryDir).c_str());
    system(("rm -rf " + userDir).c_str());
}

void testSaveAndDelete() {
    std::cout << "\nTesting save and delete..." << std::endl;
    
    std::string factoryDir = "/tmp/test_factory_presets";
    std::string userDir = "/tmp/test_user_presets";
    
    system(("mkdir -p " + factoryDir).c_str());
    system(("mkdir -p " + userDir).c_str());
    
    createTestPreset(factoryDir + "/Factory.kdpreset", "Factory", 50.0f);
    
    PresetManager manager(factoryDir, userDir);
    manager.initialize();
    
    // Test save preset
    std::map<std::string, float> params;
    params["basePitch"] = 100.0f;
    params["sineLevel"] = 0.9f;
    
    assert(manager.savePreset("User Preset", params));
    assert(manager.getPresetCount() == 2);
    assert(manager.getUserPresetCount() == 1);
    std::cout << "✓ Save preset works" << std::endl;
    
    // Test cannot delete factory preset
    assert(!manager.deletePreset(0));
    std::cout << "✓ Cannot delete factory preset" << std::endl;
    
    // Test delete user preset
    assert(manager.deletePreset(1));
    assert(manager.getPresetCount() == 1);
    assert(manager.getUserPresetCount() == 0);
    std::cout << "✓ Delete user preset works" << std::endl;
    
    // Cleanup
    system(("rm -rf " + factoryDir).c_str());
    system(("rm -rf " + userDir).c_str());
}

void testFileIO() {
    std::cout << "\nTesting file I/O..." << std::endl;
    
    std::string factoryDir = "/tmp/test_factory_presets";
    std::string userDir = "/tmp/test_user_presets";
    
    system(("mkdir -p " + factoryDir).c_str());
    system(("mkdir -p " + userDir).c_str());
    
    PresetManager manager(factoryDir, userDir);
    
    // Test save preset to file
    Preset preset("Test Preset", "1.0.0");
    preset.setParameter("basePitch", 80.0f);
    preset.setParameter("sineLevel", 0.7f);
    
    std::string filePath = userDir + "/test.kdpreset";
    assert(manager.savePresetToFile(preset, filePath));
    std::cout << "✓ Save preset to file works" << std::endl;
    
    // Test load preset from file
    Preset loaded;
    assert(manager.loadPresetFromFile(filePath, loaded));
    assert(loaded.getName() == "Test Preset");
    assert(loaded.getParameter("basePitch") == 80.0f);
    std::cout << "✓ Load preset from file works" << std::endl;
    
    // Test .kdpreset extension is added
    std::string filePathNoExt = userDir + "/test2";
    assert(manager.savePresetToFile(preset, filePathNoExt));
    
    Preset loaded2;
    assert(manager.loadPresetFromFile(filePathNoExt + ".kdpreset", loaded2));
    std::cout << "✓ .kdpreset extension is added automatically" << std::endl;
    
    // Cleanup
    system(("rm -rf " + factoryDir).c_str());
    system(("rm -rf " + userDir).c_str());
}

void testCallback() {
    std::cout << "\nTesting callback..." << std::endl;
    
    std::string factoryDir = "/tmp/test_factory_presets";
    std::string userDir = "/tmp/test_user_presets";
    
    system(("mkdir -p " + factoryDir).c_str());
    system(("mkdir -p " + userDir).c_str());
    
    createTestPreset(factoryDir + "/Preset1.kdpreset", "Preset 1", 50.0f);
    createTestPreset(factoryDir + "/Preset2.kdpreset", "Preset 2", 60.0f);
    
    PresetManager manager(factoryDir, userDir);
    manager.initialize();
    
    // Test callback
    bool callbackInvoked = false;
    std::string loadedName;
    
    manager.setPresetLoadedCallback([&](const Preset& preset) {
        callbackInvoked = true;
        loadedName = preset.getName();
    });
    
    manager.loadPreset(1);
    assert(callbackInvoked);
    assert(loadedName == "Preset 2");
    std::cout << "✓ Callback invoked on load" << std::endl;
    
    // Test callback on next
    callbackInvoked = false;
    manager.nextPreset();
    assert(callbackInvoked);
    std::cout << "✓ Callback invoked on next" << std::endl;
    
    // Test clear callback
    manager.clearPresetLoadedCallback();
    callbackInvoked = false;
    manager.previousPreset();
    assert(!callbackInvoked);
    std::cout << "✓ Clear callback works" << std::endl;
    
    // Cleanup
    system(("rm -rf " + factoryDir).c_str());
    system(("rm -rf " + userDir).c_str());
}

int main() {
    std::cout << "Testing PresetManager implementation..." << std::endl;
    std::cout << std::endl;
    
    try {
        testBasicFunctionality();
        testNavigation();
        testSaveAndDelete();
        testFileIO();
        testCallback();
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
g++ -std=c++17 -I$TEMP_DIR -o $TEMP_DIR/test_preset_manager \
    $TEMP_DIR/PresetManager.cpp \
    $TEMP_DIR/Preset.cpp \
    $TEMP_DIR/test_main.cpp

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful"
    echo ""
    echo "Running tests..."
    echo ""
    $TEMP_DIR/test_preset_manager
    TEST_RESULT=$?
    echo ""
    
    # Cleanup
    rm -rf $TEMP_DIR
    
    if [ $TEST_RESULT -eq 0 ]; then
        echo "=== PresetManager implementation verified! ==="
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
