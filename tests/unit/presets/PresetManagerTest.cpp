#include <gtest/gtest.h>
#include "../../../src/audio_engine/presets/PresetManager.h"
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

using namespace KickDrum;

/**
 * Test suite for PresetManager class
 * 
 * Tests cover:
 * - Initialization and directory management
 * - Preset loading and navigation (next/previous)
 * - Preset saving and deletion
 * - File I/O with .kdpreset extension
 * - Factory vs user preset handling
 * - Error handling
 * - Callback functionality
 */

class PresetManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary test directories
        testFactoryDir_ = "/tmp/kick_drum_test_factory_presets";
        testUserDir_ = "/tmp/kick_drum_test_user_presets";

        // Clean up any existing test directories
        cleanupTestDirectories();

        // Create test directories
        mkdir(testFactoryDir_.c_str(), 0755);
        mkdir(testUserDir_.c_str(), 0755);

        // Create some factory presets
        createFactoryPreset("Factory Preset 1", 50.0f, 0.8f);
        createFactoryPreset("Factory Preset 2", 60.0f, 0.9f);
        createFactoryPreset("Factory Preset 3", 70.0f, 0.7f);
    }

    void TearDown() override {
        cleanupTestDirectories();
    }

    void cleanupTestDirectories() {
        // Remove all files in test directories
        removeDirectory(testFactoryDir_);
        removeDirectory(testUserDir_);
    }

    void removeDirectory(const std::string& path) {
        // Remove all files in directory
        DIR* dir = opendir(path.c_str());
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                std::string fileName = entry->d_name;
                if (fileName != "." && fileName != "..") {
                    std::string filePath = path + "/" + fileName;
                    unlink(filePath.c_str());
                }
            }
            closedir(dir);
        }

        // Remove directory
        rmdir(path.c_str());
    }

    void createFactoryPreset(const std::string& name, float basePitch, float sineLevel) {
        Preset preset(name, "1.0.0");
        preset.setParameter("basePitch", basePitch);
        preset.setParameter("sineLevel", sineLevel);

        std::string filePath = testFactoryDir_ + "/" + name + ".kdpreset";
        std::string json = preset.toJSON();

        std::ofstream file(filePath);
        file << json;
        file.close();
    }

    void createUserPreset(const std::string& name, float basePitch, float sineLevel) {
        Preset preset(name, "1.0.0");
        preset.setParameter("basePitch", basePitch);
        preset.setParameter("sineLevel", sineLevel);

        std::string filePath = testUserDir_ + "/" + name + ".kdpreset";
        std::string json = preset.toJSON();

        std::ofstream file(filePath);
        file << json;
        file.close();
    }

    bool fileExists(const std::string& path) {
        struct stat st;
        return stat(path.c_str(), &st) == 0;
    }

    std::string testFactoryDir_;
    std::string testUserDir_;
};

// Test: Default constructor
TEST_F(PresetManagerTest, DefaultConstructor) {
    PresetManager manager;

    EXPECT_EQ(manager.getPresetCount(), 0);
    EXPECT_EQ(manager.getCurrentPresetIndex(), -1);
    EXPECT_EQ(manager.getCurrentPreset(), nullptr);
}

// Test: Constructor with paths
TEST_F(PresetManagerTest, ConstructorWithPaths) {
    PresetManager manager(testFactoryDir_, testUserDir_);

    EXPECT_EQ(manager.getFactoryPresetsPath(), testFactoryDir_);
    EXPECT_EQ(manager.getUserPresetsPath(), testUserDir_);
}

// Test: Initialize loads factory presets
TEST_F(PresetManagerTest, InitializeLoadsFactoryPresets) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    EXPECT_TRUE(manager.initialize());

    EXPECT_EQ(manager.getPresetCount(), 3);
    EXPECT_EQ(manager.getFactoryPresetCount(), 3);
    EXPECT_EQ(manager.getUserPresetCount(), 0);
}

// Test: Initialize loads user presets
TEST_F(PresetManagerTest, InitializeLoadsUserPresets) {
    // Create user presets
    createUserPreset("User Preset 1", 80.0f, 0.6f);
    createUserPreset("User Preset 2", 90.0f, 0.5f);

    PresetManager manager(testFactoryDir_, testUserDir_);
    EXPECT_TRUE(manager.initialize());

    EXPECT_EQ(manager.getPresetCount(), 5);
    EXPECT_EQ(manager.getFactoryPresetCount(), 3);
    EXPECT_EQ(manager.getUserPresetCount(), 2);
}

// Test: Initialize selects first preset
TEST_F(PresetManagerTest, InitializeSelectsFirstPreset) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    EXPECT_TRUE(manager.initialize());

    EXPECT_EQ(manager.getCurrentPresetIndex(), 0);
    EXPECT_NE(manager.getCurrentPreset(), nullptr);
}

// Test: Initialize with empty directories
TEST_F(PresetManagerTest, InitializeWithEmptyDirectories) {
    // Remove factory presets
    cleanupTestDirectories();
    mkdir(testFactoryDir_.c_str(), 0755);
    mkdir(testUserDir_.c_str(), 0755);

    PresetManager manager(testFactoryDir_, testUserDir_);
    EXPECT_TRUE(manager.initialize());

    EXPECT_EQ(manager.getPresetCount(), 0);
    EXPECT_EQ(manager.getCurrentPresetIndex(), -1);
}

// Test: Initialize creates user presets directory if missing
TEST_F(PresetManagerTest, InitializeCreatesUserPresetsDirectory) {
    // Remove user presets directory
    removeDirectory(testUserDir_);

    PresetManager manager(testFactoryDir_, testUserDir_);
    EXPECT_TRUE(manager.initialize());

    // Directory should be created
    EXPECT_TRUE(fileExists(testUserDir_));
}

// Test: Get preset by index
TEST_F(PresetManagerTest, GetPresetByIndex) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    const Preset* preset = manager.getPreset(0);
    EXPECT_NE(preset, nullptr);
    EXPECT_EQ(preset->getName(), "Factory Preset 1");
}

// Test: Get preset with invalid index
TEST_F(PresetManagerTest, GetPresetInvalidIndex) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    EXPECT_EQ(manager.getPreset(999), nullptr);
}

// Test: Get preset name by index
TEST_F(PresetManagerTest, GetPresetName) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    EXPECT_EQ(manager.getPresetName(0), "Factory Preset 1");
    EXPECT_EQ(manager.getPresetName(1), "Factory Preset 2");
    EXPECT_EQ(manager.getPresetName(2), "Factory Preset 3");
}

// Test: Get preset name with invalid index
TEST_F(PresetManagerTest, GetPresetNameInvalidIndex) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    EXPECT_EQ(manager.getPresetName(999), "");
}

// Test: Is factory preset
TEST_F(PresetManagerTest, IsFactoryPreset) {
    createUserPreset("User Preset 1", 80.0f, 0.6f);

    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    EXPECT_TRUE(manager.isFactoryPreset(0));
    EXPECT_TRUE(manager.isFactoryPreset(1));
    EXPECT_TRUE(manager.isFactoryPreset(2));
    EXPECT_FALSE(manager.isFactoryPreset(3)); // User preset
}

// Test: Load preset by index
TEST_F(PresetManagerTest, LoadPresetByIndex) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    EXPECT_TRUE(manager.loadPreset(1));
    EXPECT_EQ(manager.getCurrentPresetIndex(), 1);

    const Preset* preset = manager.getCurrentPreset();
    EXPECT_NE(preset, nullptr);
    EXPECT_EQ(preset->getName(), "Factory Preset 2");
}

// Test: Load preset with invalid index
TEST_F(PresetManagerTest, LoadPresetInvalidIndex) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    EXPECT_FALSE(manager.loadPreset(999));
    EXPECT_NE(manager.getLastError(), "");
}

// Test: Next preset
TEST_F(PresetManagerTest, NextPreset) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    EXPECT_EQ(manager.getCurrentPresetIndex(), 0);

    EXPECT_TRUE(manager.nextPreset());
    EXPECT_EQ(manager.getCurrentPresetIndex(), 1);

    EXPECT_TRUE(manager.nextPreset());
    EXPECT_EQ(manager.getCurrentPresetIndex(), 2);
}

// Test: Next preset wraps around
TEST_F(PresetManagerTest, NextPresetWrapsAround) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    // Go to last preset
    manager.loadPreset(2);
    EXPECT_EQ(manager.getCurrentPresetIndex(), 2);

    // Next should wrap to first
    EXPECT_TRUE(manager.nextPreset());
    EXPECT_EQ(manager.getCurrentPresetIndex(), 0);
}

// Test: Previous preset
TEST_F(PresetManagerTest, PreviousPreset) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    // Go to preset 2
    manager.loadPreset(2);
    EXPECT_EQ(manager.getCurrentPresetIndex(), 2);

    EXPECT_TRUE(manager.previousPreset());
    EXPECT_EQ(manager.getCurrentPresetIndex(), 1);

    EXPECT_TRUE(manager.previousPreset());
    EXPECT_EQ(manager.getCurrentPresetIndex(), 0);
}

// Test: Previous preset wraps around
TEST_F(PresetManagerTest, PreviousPresetWrapsAround) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    EXPECT_EQ(manager.getCurrentPresetIndex(), 0);

    // Previous should wrap to last
    EXPECT_TRUE(manager.previousPreset());
    EXPECT_EQ(manager.getCurrentPresetIndex(), 2);
}

// Test: Next preset with no presets
TEST_F(PresetManagerTest, NextPresetNoPresets) {
    cleanupTestDirectories();
    mkdir(testFactoryDir_.c_str(), 0755);
    mkdir(testUserDir_.c_str(), 0755);

    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    EXPECT_FALSE(manager.nextPreset());
    EXPECT_NE(manager.getLastError(), "");
}

// Test: Previous preset with no presets
TEST_F(PresetManagerTest, PreviousPresetNoPresets) {
    cleanupTestDirectories();
    mkdir(testFactoryDir_.c_str(), 0755);
    mkdir(testUserDir_.c_str(), 0755);

    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    EXPECT_FALSE(manager.previousPreset());
    EXPECT_NE(manager.getLastError(), "");
}

// Test: Save preset
TEST_F(PresetManagerTest, SavePreset) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    std::map<std::string, float> params;
    params["basePitch"] = 100.0f;
    params["sineLevel"] = 0.95f;

    EXPECT_TRUE(manager.savePreset("New User Preset", params));
    EXPECT_EQ(manager.getPresetCount(), 4);
    EXPECT_EQ(manager.getUserPresetCount(), 1);

    // Verify file was created
    EXPECT_TRUE(fileExists(testUserDir_ + "/New_User_Preset.kdpreset"));
}

// Test: Save preset object
TEST_F(PresetManagerTest, SavePresetObject) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    Preset preset("Test Preset", "1.0.0");
    preset.setParameter("basePitch", 110.0f);
    preset.setParameter("sineLevel", 0.85f);

    EXPECT_TRUE(manager.savePreset(preset));
    EXPECT_EQ(manager.getPresetCount(), 4);

    // Verify file was created
    EXPECT_TRUE(fileExists(testUserDir_ + "/Test_Preset.kdpreset"));
}

// Test: Save preset with special characters in name
TEST_F(PresetManagerTest, SavePresetSpecialCharacters) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    std::map<std::string, float> params;
    params["basePitch"] = 100.0f;

    EXPECT_TRUE(manager.savePreset("Test/Preset:Name*", params));

    // Special characters should be sanitized
    EXPECT_TRUE(fileExists(testUserDir_ + "/Test_Preset_Name_.kdpreset"));
}

// Test: Save preset with duplicate name
TEST_F(PresetManagerTest, SavePresetDuplicateName) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    std::map<std::string, float> params;
    params["basePitch"] = 100.0f;

    // Save first preset
    EXPECT_TRUE(manager.savePreset("Duplicate", params));
    EXPECT_TRUE(fileExists(testUserDir_ + "/Duplicate.kdpreset"));

    // Save second preset with same name
    EXPECT_TRUE(manager.savePreset("Duplicate", params));
    EXPECT_TRUE(fileExists(testUserDir_ + "/Duplicate_1.kdpreset"));
}

// Test: Overwrite user preset
TEST_F(PresetManagerTest, OverwriteUserPreset) {
    createUserPreset("User Preset 1", 80.0f, 0.6f);

    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    size_t userPresetIndex = 3; // After 3 factory presets

    std::map<std::string, float> newParams;
    newParams["basePitch"] = 120.0f;
    newParams["sineLevel"] = 0.4f;

    EXPECT_TRUE(manager.overwritePreset(userPresetIndex, newParams));

    // Verify preset was updated
    const Preset* preset = manager.getPreset(userPresetIndex);
    EXPECT_EQ(preset->getParameter("basePitch"), 120.0f);
    EXPECT_EQ(preset->getParameter("sineLevel"), 0.4f);
}

// Test: Cannot overwrite factory preset
TEST_F(PresetManagerTest, CannotOverwriteFactoryPreset) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    std::map<std::string, float> newParams;
    newParams["basePitch"] = 120.0f;

    EXPECT_FALSE(manager.overwritePreset(0, newParams)); // Factory preset
    EXPECT_NE(manager.getLastError(), "");
}

// Test: Overwrite with invalid index
TEST_F(PresetManagerTest, OverwriteInvalidIndex) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    std::map<std::string, float> newParams;
    newParams["basePitch"] = 120.0f;

    EXPECT_FALSE(manager.overwritePreset(999, newParams));
    EXPECT_NE(manager.getLastError(), "");
}

// Test: Delete user preset
TEST_F(PresetManagerTest, DeleteUserPreset) {
    createUserPreset("User Preset 1", 80.0f, 0.6f);

    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    size_t userPresetIndex = 3; // After 3 factory presets
    EXPECT_EQ(manager.getPresetCount(), 4);

    EXPECT_TRUE(manager.deletePreset(userPresetIndex));
    EXPECT_EQ(manager.getPresetCount(), 3);

    // Verify file was deleted
    EXPECT_FALSE(fileExists(testUserDir_ + "/User Preset 1.kdpreset"));
}

// Test: Cannot delete factory preset
TEST_F(PresetManagerTest, CannotDeleteFactoryPreset) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    EXPECT_FALSE(manager.deletePreset(0)); // Factory preset
    EXPECT_NE(manager.getLastError(), "");
}

// Test: Delete with invalid index
TEST_F(PresetManagerTest, DeleteInvalidIndex) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    EXPECT_FALSE(manager.deletePreset(999));
    EXPECT_NE(manager.getLastError(), "");
}

// Test: Delete adjusts current preset index
TEST_F(PresetManagerTest, DeleteAdjustsCurrentPresetIndex) {
    createUserPreset("User Preset 1", 80.0f, 0.6f);
    createUserPreset("User Preset 2", 90.0f, 0.5f);

    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    // Load second user preset (index 4)
    manager.loadPreset(4);
    EXPECT_EQ(manager.getCurrentPresetIndex(), 4);

    // Delete first user preset (index 3)
    manager.deletePreset(3);

    // Current index should be adjusted
    EXPECT_EQ(manager.getCurrentPresetIndex(), 3);
}

// Test: Load preset from file
TEST_F(PresetManagerTest, LoadPresetFromFile) {
    PresetManager manager(testFactoryDir_, testUserDir_);

    std::string filePath = testFactoryDir_ + "/Factory Preset 1.kdpreset";
    Preset preset;

    EXPECT_TRUE(manager.loadPresetFromFile(filePath, preset));
    EXPECT_EQ(preset.getName(), "Factory Preset 1");
    EXPECT_EQ(preset.getParameter("basePitch"), 50.0f);
}

// Test: Load preset from non-existent file
TEST_F(PresetManagerTest, LoadPresetFromNonExistentFile) {
    PresetManager manager(testFactoryDir_, testUserDir_);

    Preset preset;
    EXPECT_FALSE(manager.loadPresetFromFile("/nonexistent/file.kdpreset", preset));
}

// Test: Save preset to file
TEST_F(PresetManagerTest, SavePresetToFile) {
    PresetManager manager(testFactoryDir_, testUserDir_);

    Preset preset("Test Save", "1.0.0");
    preset.setParameter("basePitch", 130.0f);

    std::string filePath = testUserDir_ + "/test_save.kdpreset";
    EXPECT_TRUE(manager.savePresetToFile(preset, filePath));

    // Verify file exists
    EXPECT_TRUE(fileExists(filePath));

    // Load and verify
    Preset loaded;
    EXPECT_TRUE(manager.loadPresetFromFile(filePath, loaded));
    EXPECT_EQ(loaded.getName(), "Test Save");
    EXPECT_EQ(loaded.getParameter("basePitch"), 130.0f);
}

// Test: Save preset to file adds .kdpreset extension
TEST_F(PresetManagerTest, SavePresetToFileAddsExtension) {
    PresetManager manager(testFactoryDir_, testUserDir_);

    Preset preset("Test", "1.0.0");
    preset.setParameter("basePitch", 140.0f);

    std::string filePath = testUserDir_ + "/test_no_extension";
    EXPECT_TRUE(manager.savePresetToFile(preset, filePath));

    // Verify file with extension exists
    EXPECT_TRUE(fileExists(testUserDir_ + "/test_no_extension.kdpreset"));
}

// Test: Refresh reloads presets
TEST_F(PresetManagerTest, RefreshReloadsPresets) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    EXPECT_EQ(manager.getPresetCount(), 3);

    // Add a new user preset file manually
    createUserPreset("New Preset", 150.0f, 0.3f);

    // Refresh
    EXPECT_TRUE(manager.refresh());
    EXPECT_EQ(manager.getPresetCount(), 4);
}

// Test: Preset loaded callback
TEST_F(PresetManagerTest, PresetLoadedCallback) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    bool callbackInvoked = false;
    std::string loadedPresetName;

    manager.setPresetLoadedCallback([&](const Preset& preset) {
        callbackInvoked = true;
        loadedPresetName = preset.getName();
    });

    manager.loadPreset(1);

    EXPECT_TRUE(callbackInvoked);
    EXPECT_EQ(loadedPresetName, "Factory Preset 2");
}

// Test: Preset loaded callback on next
TEST_F(PresetManagerTest, PresetLoadedCallbackOnNext) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    int callbackCount = 0;

    manager.setPresetLoadedCallback([&](const Preset& preset) {
        callbackCount++;
    });

    manager.nextPreset();
    manager.nextPreset();

    EXPECT_EQ(callbackCount, 2);
}

// Test: Preset loaded callback on previous
TEST_F(PresetManagerTest, PresetLoadedCallbackOnPrevious) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    int callbackCount = 0;

    manager.setPresetLoadedCallback([&](const Preset& preset) {
        callbackCount++;
    });

    manager.previousPreset();
    manager.previousPreset();

    EXPECT_EQ(callbackCount, 2);
}

// Test: Clear preset loaded callback
TEST_F(PresetManagerTest, ClearPresetLoadedCallback) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    bool callbackInvoked = false;

    manager.setPresetLoadedCallback([&](const Preset& preset) {
        callbackInvoked = true;
    });

    manager.clearPresetLoadedCallback();
    manager.loadPreset(1);

    EXPECT_FALSE(callbackInvoked);
}

// Test: Set paths after construction
TEST_F(PresetManagerTest, SetPathsAfterConstruction) {
    PresetManager manager;

    manager.setFactoryPresetsPath(testFactoryDir_);
    manager.setUserPresetsPath(testUserDir_);

    EXPECT_EQ(manager.getFactoryPresetsPath(), testFactoryDir_);
    EXPECT_EQ(manager.getUserPresetsPath(), testUserDir_);

    EXPECT_TRUE(manager.initialize());
    EXPECT_EQ(manager.getPresetCount(), 3);
}

// Test: Error messages are cleared on success
TEST_F(PresetManagerTest, ErrorMessagesClearedOnSuccess) {
    PresetManager manager(testFactoryDir_, testUserDir_);
    manager.initialize();

    // Cause an error
    manager.loadPreset(999);
    EXPECT_NE(manager.getLastError(), "");

    // Successful operation should clear error
    manager.loadPreset(0);
    EXPECT_EQ(manager.getLastError(), "");
}

// Test: Save without user presets path set
TEST_F(PresetManagerTest, SaveWithoutUserPresetsPath) {
    PresetManager manager(testFactoryDir_, "");
    manager.initialize();

    std::map<std::string, float> params;
    params["basePitch"] = 100.0f;

    EXPECT_FALSE(manager.savePreset("Test", params));
    EXPECT_NE(manager.getLastError(), "");
}
