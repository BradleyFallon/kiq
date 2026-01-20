# Preset Class Implementation

## Overview

The Preset class has been successfully implemented as part of task 12.1. This document provides an overview of the implementation and usage examples.

## Implementation Details

### Files Created
- `src/audio_engine/presets/Preset.h` - Header file with class declaration
- `src/audio_engine/presets/Preset.cpp` - Implementation file
- `tests/unit/presets/PresetTest.cpp` - Comprehensive unit tests (35 tests)

### Class Structure

```cpp
class Preset {
public:
    // Constructors
    Preset();
    Preset(const std::string& name, const std::string& version = "1.0.0");
    
    // Name and version management
    const std::string& getName() const;
    void setName(const std::string& name);
    const std::string& getVersion() const;
    void setVersion(const std::string& version);
    
    // Parameter management
    const std::map<std::string, float>& getParameters() const;
    void setParameters(const std::map<std::string, float>& parameters);
    float getParameter(const std::string& id, float defaultValue = 0.0f) const;
    void setParameter(const std::string& id, float value);
    bool hasParameter(const std::string& id) const;
    size_t getParameterCount() const;
    void clearParameters();
    
    // JSON serialization
    std::string toJSON() const;
    static Preset fromJSON(const std::string& json);
    bool loadFromJSON(const std::string& json);
    static bool validateJSON(const std::string& json);
    
    // Utility
    bool isEmpty() const;
};
```

## Features

### 1. Name and Version Management
- Each preset has a user-friendly name
- Version string for compatibility tracking (default: "1.0.0")
- Supports empty names and versions

### 2. Parameter Storage
- Parameters stored as `std::map<std::string, float>`
- Individual parameter get/set operations
- Bulk parameter operations (get/set all)
- Parameter existence checking
- Parameter count tracking

### 3. JSON Serialization
- **toJSON()**: Converts preset to JSON string
- **fromJSON()**: Creates preset from JSON string (static factory method)
- **loadFromJSON()**: Updates existing preset from JSON
- **validateJSON()**: Validates JSON structure without loading

### 4. Error Handling
- Invalid JSON returns false without modifying preset state
- Missing required fields (name, version, parameters) cause failure
- Malformed JSON is detected and rejected
- Extra fields in JSON are ignored (forward compatibility)

### 5. JSON Format

The JSON format follows the design document specification:

```json
{
  "name": "Deep Sub Kick",
  "version": "1.0.0",
  "parameters": {
    "basePitch": 50.0,
    "sineLevel": 0.8,
    "harmonicRatio": 2.0,
    "harmonicLevel": 0.3,
    "harmonicModDepth": 0.5,
    "noiseLevel": 0.2,
    "noiseModDepth": 0.7,
    "warmUpDuration": 20.0,
    "warmUpStartFreq": 10.0,
    "warmUpAmplitude": 0.5,
    "attack": 0.001,
    "decay": 0.5,
    "sustain": 0.0,
    "release": 0.1,
    "pitchEnvelopeDepth": 500.0,
    "compressorThreshold": -12.0,
    "compressorRatio": 4.0,
    "compressorMix": 0.5,
    "reverbRoomSize": 0.3,
    "reverbDecayTime": 1.0,
    "reverbMix": 0.1,
    "masterLevel": 0.8
  }
}
```

## Usage Examples

### Creating a Preset

```cpp
#include "presets/Preset.h"

// Create with default name and version
Preset preset1;

// Create with custom name
Preset preset2("Deep Sub Kick");

// Create with custom name and version
Preset preset3("Deep Sub Kick", "1.2.0");
```

### Setting Parameters

```cpp
Preset preset("My Kick");

// Set individual parameters
preset.setParameter("basePitch", 50.0f);
preset.setParameter("sineLevel", 0.8f);
preset.setParameter("harmonicRatio", 2.0f);

// Set multiple parameters at once
std::map<std::string, float> params;
params["basePitch"] = 50.0f;
params["sineLevel"] = 0.8f;
preset.setParameters(params);
```

### Getting Parameters

```cpp
// Get individual parameter
float pitch = preset.getParameter("basePitch");

// Get with default value if not found
float level = preset.getParameter("unknownParam", 0.5f);

// Check if parameter exists
if (preset.hasParameter("basePitch")) {
    // Parameter exists
}

// Get all parameters
const auto& params = preset.getParameters();
for (const auto& [id, value] : params) {
    std::cout << id << ": " << value << std::endl;
}
```

### Serialization

```cpp
// Serialize to JSON
std::string json = preset.toJSON();

// Save to file (using file I/O)
std::ofstream file("preset.kdpreset");
file << json;
file.close();
```

### Deserialization

```cpp
// Load from file
std::ifstream file("preset.kdpreset");
std::string json((std::istreambuf_iterator<char>(file)),
                  std::istreambuf_iterator<char>());
file.close();

// Create preset from JSON
Preset preset = Preset::fromJSON(json);

// Or update existing preset
Preset existingPreset;
if (existingPreset.loadFromJSON(json)) {
    // Successfully loaded
} else {
    // Failed to load - preset unchanged
}
```

### Validation

```cpp
// Validate JSON before loading
if (Preset::validateJSON(json)) {
    Preset preset = Preset::fromJSON(json);
    // Use preset
} else {
    // Invalid JSON
}
```

### Round-Trip Example

```cpp
// Create and configure preset
Preset original("Test Preset", "1.0.0");
original.setParameter("basePitch", 50.0f);
original.setParameter("sineLevel", 0.8f);

// Serialize
std::string json = original.toJSON();

// Deserialize
Preset restored = Preset::fromJSON(json);

// Verify
assert(restored.getName() == original.getName());
assert(restored.getVersion() == original.getVersion());
assert(restored.getParameter("basePitch") == 50.0f);
assert(restored.getParameter("sineLevel") == 0.8f);
```

## Test Coverage

The implementation includes 35 comprehensive unit tests covering:

### Basic Functionality (11 tests)
- Default constructor
- Constructor with name and version
- Name and version getters/setters
- Parameter get/set operations
- Parameter map operations
- Clear parameters

### JSON Serialization (4 tests)
- Basic toJSON functionality
- Empty parameters
- Special characters in names
- Large parameter sets

### JSON Deserialization (9 tests)
- Basic fromJSON functionality
- Empty parameters
- Many parameters
- Negative numbers
- Scientific notation
- loadFromJSON updates
- Round-trip serialization

### Error Handling (7 tests)
- Invalid JSON (missing brace)
- Missing required fields (name, version, parameters)
- Malformed parameters
- Unterminated strings
- Extra fields (ignored)
- Validation

### Edge Cases (4 tests)
- Overwrite parameter values
- Zero values
- Large parameter count (100 parameters)
- Empty name and version
- Very long names (1000 characters)

## Requirements Validation

### Requirement 10.1
✅ **"WHEN the user saves a preset, THE Kick_Synth SHALL store all current parameter values to a Preset file"**

The Preset class provides:
- `toJSON()` method to serialize all parameters
- Stores name, version, and all parameter values
- JSON format suitable for file storage

### Requirement 10.2
✅ **"WHEN the user loads a preset, THE Kick_Synth SHALL restore all parameter values from the Preset file"**

The Preset class provides:
- `fromJSON()` static method to create preset from JSON
- `loadFromJSON()` method to update existing preset
- Restores name, version, and all parameter values
- Error handling maintains state on failure

### Additional Requirements Met

- **Requirement 14.1**: JSON format for parameter encoding ✅
- **Requirement 14.2**: JSON validation before applying parameters ✅
- **Requirement 14.3**: Error handling maintains current state ✅
- **Requirement 14.4**: Version information in preset files ✅
- **Requirement 14.5**: .kdpreset file extension (handled by PresetManager)

## Implementation Notes

### JSON Parser
- Custom lightweight JSON parser implementation
- No external dependencies (no nlohmann/json or similar)
- Supports:
  - String values with escape sequences
  - Numeric values (including negative and scientific notation)
  - Nested objects (one level for parameters)
  - Whitespace handling
  - Error detection and recovery

### String Escaping
- Properly escapes special characters in JSON strings
- Handles: `"`, `\`, `\b`, `\f`, `\n`, `\r`, `\t`
- Control characters converted to unicode escapes

### Error Handling Philosophy
- **Fail-safe**: Invalid operations don't crash or corrupt state
- **Preserve state**: Failed loads don't modify existing preset
- **Clear feedback**: Boolean return values indicate success/failure
- **Validation**: Separate validation method for pre-checking

### Performance Considerations
- Efficient map-based parameter storage (O(log n) access)
- Minimal string copying
- Single-pass JSON parsing
- No dynamic memory allocation in hot paths

## Next Steps

The Preset class is now ready for integration with:
1. **PresetManager** (Task 12.2) - Will manage collections of presets
2. **File I/O** - PresetManager will handle .kdpreset file operations
3. **UI Integration** - Preset browser will use this class
4. **Parameter Manager** - Will provide parameter values to presets

## Testing

All tests pass successfully:

```bash
cd build
cmake ..
make -j4
./bin/kick_drum_tests --gtest_filter="PresetTest.*"
```

Result: **35/35 tests passed** ✅

## Conclusion

Task 12.1 is complete. The Preset class provides a robust foundation for preset management with:
- Clean API for parameter storage and retrieval
- Reliable JSON serialization/deserialization
- Comprehensive error handling
- Extensive test coverage
- Full compliance with requirements 10.1 and 10.2
