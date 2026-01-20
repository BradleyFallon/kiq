#include <gtest/gtest.h>
#include "../../../src/audio_engine/presets/Preset.h"

using namespace KickDrum;

/**
 * Test suite for Preset class
 * 
 * Tests cover:
 * - Construction and basic getters/setters
 * - Parameter management
 * - JSON serialization (toJSON)
 * - JSON deserialization (fromJSON)
 * - Round-trip serialization/deserialization
 * - Error handling for invalid JSON
 * - Edge cases
 */

// Test: Default constructor
TEST(PresetTest, DefaultConstructor) {
    Preset preset;
    
    EXPECT_EQ(preset.getName(), "Untitled");
    EXPECT_EQ(preset.getVersion(), "1.0.0");
    EXPECT_EQ(preset.getParameterCount(), 0);
    EXPECT_TRUE(preset.isEmpty());
}

// Test: Constructor with name and version
TEST(PresetTest, ConstructorWithNameAndVersion) {
    Preset preset("Deep Sub Kick", "1.2.3");
    
    EXPECT_EQ(preset.getName(), "Deep Sub Kick");
    EXPECT_EQ(preset.getVersion(), "1.2.3");
    EXPECT_EQ(preset.getParameterCount(), 0);
}

// Test: Constructor with name only (default version)
TEST(PresetTest, ConstructorWithNameOnly) {
    Preset preset("Test Preset");
    
    EXPECT_EQ(preset.getName(), "Test Preset");
    EXPECT_EQ(preset.getVersion(), "1.0.0");
}

// Test: Set and get name
TEST(PresetTest, SetAndGetName) {
    Preset preset;
    preset.setName("My Kick");
    
    EXPECT_EQ(preset.getName(), "My Kick");
}

// Test: Set and get version
TEST(PresetTest, SetAndGetVersion) {
    Preset preset;
    preset.setVersion("2.0.0");
    
    EXPECT_EQ(preset.getVersion(), "2.0.0");
}

// Test: Set and get single parameter
TEST(PresetTest, SetAndGetParameter) {
    Preset preset;
    preset.setParameter("basePitch", 50.0f);
    
    EXPECT_TRUE(preset.hasParameter("basePitch"));
    EXPECT_EQ(preset.getParameter("basePitch"), 50.0f);
    EXPECT_EQ(preset.getParameterCount(), 1);
    EXPECT_FALSE(preset.isEmpty());
}

// Test: Get parameter with default value
TEST(PresetTest, GetParameterWithDefault) {
    Preset preset;
    
    EXPECT_EQ(preset.getParameter("nonexistent", 42.0f), 42.0f);
}

// Test: Set multiple parameters
TEST(PresetTest, SetMultipleParameters) {
    Preset preset;
    preset.setParameter("basePitch", 50.0f);
    preset.setParameter("sineLevel", 0.8f);
    preset.setParameter("harmonicRatio", 2.0f);
    
    EXPECT_EQ(preset.getParameterCount(), 3);
    EXPECT_EQ(preset.getParameter("basePitch"), 50.0f);
    EXPECT_EQ(preset.getParameter("sineLevel"), 0.8f);
    EXPECT_EQ(preset.getParameter("harmonicRatio"), 2.0f);
}

// Test: Set parameters using map
TEST(PresetTest, SetParametersMap) {
    Preset preset;
    std::map<std::string, float> params;
    params["basePitch"] = 60.0f;
    params["sineLevel"] = 0.9f;
    
    preset.setParameters(params);
    
    EXPECT_EQ(preset.getParameterCount(), 2);
    EXPECT_EQ(preset.getParameter("basePitch"), 60.0f);
    EXPECT_EQ(preset.getParameter("sineLevel"), 0.9f);
}

// Test: Get parameters map
TEST(PresetTest, GetParametersMap) {
    Preset preset;
    preset.setParameter("basePitch", 50.0f);
    preset.setParameter("sineLevel", 0.8f);
    
    const auto& params = preset.getParameters();
    
    EXPECT_EQ(params.size(), 2);
    EXPECT_EQ(params.at("basePitch"), 50.0f);
    EXPECT_EQ(params.at("sineLevel"), 0.8f);
}

// Test: Clear parameters
TEST(PresetTest, ClearParameters) {
    Preset preset;
    preset.setParameter("basePitch", 50.0f);
    preset.setParameter("sineLevel", 0.8f);
    
    EXPECT_EQ(preset.getParameterCount(), 2);
    
    preset.clearParameters();
    
    EXPECT_EQ(preset.getParameterCount(), 0);
    EXPECT_TRUE(preset.isEmpty());
}

// Test: toJSON basic functionality
TEST(PresetTest, ToJSONBasic) {
    Preset preset("Test Preset", "1.0.0");
    preset.setParameter("basePitch", 50.0f);
    preset.setParameter("sineLevel", 0.8f);
    
    std::string json = preset.toJSON();
    
    // Verify JSON contains expected fields
    EXPECT_NE(json.find("\"name\""), std::string::npos);
    EXPECT_NE(json.find("\"version\""), std::string::npos);
    EXPECT_NE(json.find("\"parameters\""), std::string::npos);
    EXPECT_NE(json.find("\"Test Preset\""), std::string::npos);
    EXPECT_NE(json.find("\"1.0.0\""), std::string::npos);
    EXPECT_NE(json.find("\"basePitch\""), std::string::npos);
    EXPECT_NE(json.find("50"), std::string::npos);
    EXPECT_NE(json.find("\"sineLevel\""), std::string::npos);
    EXPECT_NE(json.find("0.8"), std::string::npos);
}

// Test: toJSON with empty parameters
TEST(PresetTest, ToJSONEmptyParameters) {
    Preset preset("Empty Preset", "1.0.0");
    
    std::string json = preset.toJSON();
    
    EXPECT_NE(json.find("\"name\""), std::string::npos);
    EXPECT_NE(json.find("\"version\""), std::string::npos);
    EXPECT_NE(json.find("\"parameters\""), std::string::npos);
    EXPECT_NE(json.find("\"Empty Preset\""), std::string::npos);
}

// Test: toJSON with special characters in name
TEST(PresetTest, ToJSONSpecialCharacters) {
    Preset preset("Test \"Quoted\" Preset\nWith Newline", "1.0.0");
    
    std::string json = preset.toJSON();
    
    // Should escape quotes and newlines
    EXPECT_NE(json.find("\\\""), std::string::npos);
    EXPECT_NE(json.find("\\n"), std::string::npos);
}

// Test: fromJSON basic functionality
TEST(PresetTest, FromJSONBasic) {
    std::string json = R"({
        "name": "Test Preset",
        "version": "1.0.0",
        "parameters": {
            "basePitch": 50.0,
            "sineLevel": 0.8
        }
    })";
    
    Preset preset = Preset::fromJSON(json);
    
    EXPECT_EQ(preset.getName(), "Test Preset");
    EXPECT_EQ(preset.getVersion(), "1.0.0");
    EXPECT_EQ(preset.getParameterCount(), 2);
    EXPECT_EQ(preset.getParameter("basePitch"), 50.0f);
    EXPECT_EQ(preset.getParameter("sineLevel"), 0.8f);
}

// Test: fromJSON with empty parameters
TEST(PresetTest, FromJSONEmptyParameters) {
    std::string json = R"({
        "name": "Empty Preset",
        "version": "1.0.0",
        "parameters": {}
    })";
    
    Preset preset = Preset::fromJSON(json);
    
    EXPECT_EQ(preset.getName(), "Empty Preset");
    EXPECT_EQ(preset.getVersion(), "1.0.0");
    EXPECT_EQ(preset.getParameterCount(), 0);
    EXPECT_TRUE(preset.isEmpty());
}

// Test: fromJSON with many parameters
TEST(PresetTest, FromJSONManyParameters) {
    std::string json = R"({
        "name": "Complex Preset",
        "version": "1.0.0",
        "parameters": {
            "basePitch": 50.0,
            "sineLevel": 0.8,
            "harmonicRatio": 2.0,
            "harmonicLevel": 0.3,
            "noiseLevel": 0.2,
            "attack": 0.001,
            "decay": 0.5,
            "sustain": 0.0,
            "release": 0.1
        }
    })";
    
    Preset preset = Preset::fromJSON(json);
    
    EXPECT_EQ(preset.getParameterCount(), 9);
    EXPECT_EQ(preset.getParameter("basePitch"), 50.0f);
    EXPECT_EQ(preset.getParameter("attack"), 0.001f);
    EXPECT_EQ(preset.getParameter("release"), 0.1f);
}

// Test: fromJSON with negative numbers
TEST(PresetTest, FromJSONNegativeNumbers) {
    std::string json = R"({
        "name": "Negative Test",
        "version": "1.0.0",
        "parameters": {
            "threshold": -12.0,
            "offset": -0.5
        }
    })";
    
    Preset preset = Preset::fromJSON(json);
    
    EXPECT_EQ(preset.getParameter("threshold"), -12.0f);
    EXPECT_EQ(preset.getParameter("offset"), -0.5f);
}

// Test: fromJSON with scientific notation
TEST(PresetTest, FromJSONScientificNotation) {
    std::string json = R"({
        "name": "Scientific Test",
        "version": "1.0.0",
        "parameters": {
            "small": 1.5e-3,
            "large": 2.5e2
        }
    })";
    
    Preset preset = Preset::fromJSON(json);
    
    EXPECT_FLOAT_EQ(preset.getParameter("small"), 1.5e-3f);
    EXPECT_FLOAT_EQ(preset.getParameter("large"), 2.5e2f);
}

// Test: loadFromJSON updates existing preset
TEST(PresetTest, LoadFromJSONUpdatesExisting) {
    Preset preset("Original", "0.9.0");
    preset.setParameter("oldParam", 123.0f);
    
    std::string json = R"({
        "name": "Updated",
        "version": "1.0.0",
        "parameters": {
            "newParam": 456.0
        }
    })";
    
    bool success = preset.loadFromJSON(json);
    
    EXPECT_TRUE(success);
    EXPECT_EQ(preset.getName(), "Updated");
    EXPECT_EQ(preset.getVersion(), "1.0.0");
    EXPECT_EQ(preset.getParameterCount(), 1);
    EXPECT_FALSE(preset.hasParameter("oldParam"));
    EXPECT_TRUE(preset.hasParameter("newParam"));
}

// Test: Round-trip serialization (toJSON -> fromJSON)
TEST(PresetTest, RoundTripSerialization) {
    Preset original("Round Trip Test", "1.2.3");
    original.setParameter("basePitch", 50.0f);
    original.setParameter("sineLevel", 0.8f);
    original.setParameter("harmonicRatio", 2.0f);
    original.setParameter("attack", 0.001f);
    original.setParameter("decay", 0.5f);
    
    std::string json = original.toJSON();
    Preset restored = Preset::fromJSON(json);
    
    EXPECT_EQ(restored.getName(), original.getName());
    EXPECT_EQ(restored.getVersion(), original.getVersion());
    EXPECT_EQ(restored.getParameterCount(), original.getParameterCount());
    
    for (const auto& param : original.getParameters()) {
        EXPECT_TRUE(restored.hasParameter(param.first));
        EXPECT_FLOAT_EQ(restored.getParameter(param.first), param.second);
    }
}

// Test: fromJSON with invalid JSON (missing opening brace)
TEST(PresetTest, FromJSONInvalidMissingBrace) {
    std::string json = R"(
        "name": "Invalid",
        "version": "1.0.0",
        "parameters": {}
    )";
    
    Preset preset = Preset::fromJSON(json);
    
    // Should return empty preset on failure
    EXPECT_EQ(preset.getName(), "Untitled");
}

// Test: fromJSON with invalid JSON (missing name field)
TEST(PresetTest, FromJSONInvalidMissingName) {
    std::string json = R"({
        "version": "1.0.0",
        "parameters": {}
    })";
    
    Preset preset("Original", "0.9.0");
    bool success = preset.loadFromJSON(json);
    
    EXPECT_FALSE(success);
    // Preset should remain unchanged
    EXPECT_EQ(preset.getName(), "Original");
    EXPECT_EQ(preset.getVersion(), "0.9.0");
}

// Test: fromJSON with invalid JSON (missing version field)
TEST(PresetTest, FromJSONInvalidMissingVersion) {
    std::string json = R"({
        "name": "Test",
        "parameters": {}
    })";
    
    Preset preset("Original", "0.9.0");
    bool success = preset.loadFromJSON(json);
    
    EXPECT_FALSE(success);
    EXPECT_EQ(preset.getName(), "Original");
}

// Test: fromJSON with invalid JSON (missing parameters field)
TEST(PresetTest, FromJSONInvalidMissingParameters) {
    std::string json = R"({
        "name": "Test",
        "version": "1.0.0"
    })";
    
    Preset preset("Original", "0.9.0");
    bool success = preset.loadFromJSON(json);
    
    EXPECT_FALSE(success);
    EXPECT_EQ(preset.getName(), "Original");
}

// Test: fromJSON with invalid JSON (malformed parameters)
TEST(PresetTest, FromJSONInvalidMalformedParameters) {
    std::string json = R"({
        "name": "Test",
        "version": "1.0.0",
        "parameters": "not an object"
    })";
    
    Preset preset("Original", "0.9.0");
    bool success = preset.loadFromJSON(json);
    
    EXPECT_FALSE(success);
    EXPECT_EQ(preset.getName(), "Original");
}

// Test: fromJSON with invalid JSON (unterminated string)
TEST(PresetTest, FromJSONInvalidUnterminatedString) {
    std::string json = R"({
        "name": "Test,
        "version": "1.0.0",
        "parameters": {}
    })";
    
    Preset preset("Original", "0.9.0");
    bool success = preset.loadFromJSON(json);
    
    EXPECT_FALSE(success);
}

// Test: fromJSON with extra fields (should be ignored)
TEST(PresetTest, FromJSONExtraFields) {
    std::string json = R"({
        "name": "Test",
        "version": "1.0.0",
        "author": "Unknown",
        "date": "2024-01-01",
        "parameters": {
            "basePitch": 50.0
        }
    })";
    
    Preset preset = Preset::fromJSON(json);
    
    EXPECT_EQ(preset.getName(), "Test");
    EXPECT_EQ(preset.getVersion(), "1.0.0");
    EXPECT_EQ(preset.getParameterCount(), 1);
}

// Test: validateJSON with valid JSON
TEST(PresetTest, ValidateJSONValid) {
    std::string json = R"({
        "name": "Test",
        "version": "1.0.0",
        "parameters": {}
    })";
    
    EXPECT_TRUE(Preset::validateJSON(json));
}

// Test: validateJSON with invalid JSON
TEST(PresetTest, ValidateJSONInvalid) {
    std::string json = R"({
        "name": "Test",
        "version": "1.0.0"
    })";
    
    EXPECT_FALSE(Preset::validateJSON(json));
}

// Test: Overwrite parameter value
TEST(PresetTest, OverwriteParameter) {
    Preset preset;
    preset.setParameter("basePitch", 50.0f);
    EXPECT_EQ(preset.getParameter("basePitch"), 50.0f);
    
    preset.setParameter("basePitch", 60.0f);
    EXPECT_EQ(preset.getParameter("basePitch"), 60.0f);
    EXPECT_EQ(preset.getParameterCount(), 1); // Still only one parameter
}

// Test: Zero values
TEST(PresetTest, ZeroValues) {
    Preset preset;
    preset.setParameter("value", 0.0f);
    
    EXPECT_EQ(preset.getParameter("value"), 0.0f);
    EXPECT_TRUE(preset.hasParameter("value"));
}

// Test: Large parameter count
TEST(PresetTest, LargeParameterCount) {
    Preset preset;
    
    // Add 100 parameters
    for (int i = 0; i < 100; i++) {
        preset.setParameter("param" + std::to_string(i), static_cast<float>(i));
    }
    
    EXPECT_EQ(preset.getParameterCount(), 100);
    
    // Verify all parameters
    for (int i = 0; i < 100; i++) {
        EXPECT_EQ(preset.getParameter("param" + std::to_string(i)), static_cast<float>(i));
    }
}

// Test: Empty name and version
TEST(PresetTest, EmptyNameAndVersion) {
    Preset preset("", "");
    
    EXPECT_EQ(preset.getName(), "");
    EXPECT_EQ(preset.getVersion(), "");
}

// Test: Very long name
TEST(PresetTest, VeryLongName) {
    std::string longName(1000, 'A');
    Preset preset(longName, "1.0.0");
    
    EXPECT_EQ(preset.getName(), longName);
    
    // Should still serialize/deserialize correctly
    std::string json = preset.toJSON();
    Preset restored = Preset::fromJSON(json);
    EXPECT_EQ(restored.getName(), longName);
}
