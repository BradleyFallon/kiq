# CoreAudio Integration Implementation

## Overview

This document describes the CoreAudio integration for the Kick Drum Synthesizer standalone macOS application. The implementation provides a bridge between the AudioEngine and macOS CoreAudio framework for real-time audio output.

## Requirements

This implementation satisfies the following requirements:

- **Requirement 9.2**: Initialize CoreAudio and present user interface when launched
- **Requirement 9.4**: Output audio to selected system audio device
- **Requirement 9.5**: Configure AudioEngine for device's sample rate and buffer size

## Architecture

### Components

#### CoreAudioInterface Class

The `CoreAudioInterface` class (`src/platform/audio/CoreAudioInterface.h/cpp`) provides:

1. **Device Management**
   - Enumerate available audio output devices
   - Get default output device
   - Query device properties (name, sample rate, buffer size)

2. **Initialization**
   - Create and configure AudioUnit (CoreAudio's audio processing component)
   - Query device format (sample rate, channels, buffer size)
   - Initialize AudioEngine with device sample rate

3. **Audio Callback**
   - Real-time render callback invoked by CoreAudio
   - Routes audio processing to AudioEngine
   - Handles format conversion (interleaved ↔ non-interleaved)

4. **Lifecycle Management**
   - Start/stop audio processing
   - Clean up resources on destruction

### Data Flow

```
CoreAudio System
      ↓
AudioUnit (Output)
      ↓
Render Callback (real-time thread)
      ↓
CoreAudioInterface::render()
      ↓
AudioEngine::processBlock()
      ↓
Voice Rendering + Effects
      ↓
Audio Output Buffer
      ↓
CoreAudio System → Speakers
```

## Implementation Details

### Audio Format

CoreAudio uses **non-interleaved** float format:
- Sample Rate: Device native rate (typically 44.1kHz or 48kHz)
- Format: 32-bit float
- Channels: Typically 2 (stereo)
- Layout: Non-interleaved (separate buffers per channel)

The AudioEngine uses **interleaved** format, so the CoreAudioInterface performs conversion:

```cpp
// In render callback:
// 1. Allocate interleaved buffer
std::vector<float> interleavedBuffer(numFrames * numChannels);

// 2. Process through AudioEngine (interleaved)
audioEngine->processBlock(interleavedBuffer.data(), numFrames, numChannels);

// 3. Convert to non-interleaved for CoreAudio
for (channel in channels) {
    for (frame in frames) {
        outputBuffer[channel][frame] = interleavedBuffer[frame * numChannels + channel];
    }
}
```

### Initialization Sequence

1. **Get Default Device**
   ```cpp
   AudioDeviceID deviceId = CoreAudioInterface::getDefaultOutputDevice();
   ```

2. **Query Device Format**
   - Sample rate (kAudioDevicePropertyNominalSampleRate)
   - Buffer size (kAudioDevicePropertyBufferFrameSize)
   - Channel count (kAudioDevicePropertyStreamConfiguration)

3. **Create AudioUnit**
   ```cpp
   AudioComponentDescription desc;
   desc.componentType = kAudioUnitType_Output;
   desc.componentSubType = kAudioUnitSubType_DefaultOutput;
   // ... find and instantiate component
   ```

4. **Configure AudioUnit**
   - Set stream format (sample rate, channels, float format)
   - Set render callback
   - Initialize AudioUnit

5. **Initialize AudioEngine**
   ```cpp
   audioEngine->initialize(sampleRate);
   ```

### Real-Time Considerations

The render callback runs on a **real-time audio thread** with strict requirements:

- **No memory allocation** (pre-allocate buffers)
- **No blocking operations** (no locks, I/O, or system calls)
- **Deterministic execution** (must complete within buffer duration)
- **No exceptions** (catch and handle all errors)

The implementation follows these rules:
- Uses pre-allocated temporary buffer for format conversion
- All AudioEngine processing is lock-free
- No dynamic memory allocation in render path

### Error Handling

1. **Initialization Errors**
   - Device not found → Return false, log error
   - AudioUnit creation failed → Cleanup and return false
   - Format configuration failed → Cleanup and return false

2. **Runtime Errors**
   - Invalid audio data → Silence output, continue
   - AudioEngine returns NaN/infinity → Handled by AudioEngine's safety layer

3. **Device Changes**
   - Device disconnection → Stop audio, allow reinitialization
   - Sample rate change → Requires reinitialization

## Usage Example

### Basic Usage

```cpp
#include "CoreAudioInterface.h"
#include "AudioEngine.h"

// Create audio engine
AudioEngine audioEngine;

// Create CoreAudio interface
CoreAudioInterface audioInterface(&audioEngine);

// Initialize with default device
if (!audioInterface.initialize()) {
    std::cerr << "Failed to initialize audio" << std::endl;
    return;
}

// Start audio
if (!audioInterface.start()) {
    std::cerr << "Failed to start audio" << std::endl;
    return;
}

// Trigger notes
audioEngine.noteOn(60, 0.8f);

// ... application runs ...

// Stop audio
audioInterface.stop();
```

### Device Selection

```cpp
// List available devices
auto devices = CoreAudioInterface::getAvailableDevices();
for (AudioDeviceID deviceId : devices) {
    std::cout << CoreAudioInterface::getDeviceName(deviceId) << std::endl;
}

// Initialize with specific device
AudioDeviceID selectedDevice = devices[0];
audioInterface.initializeWithDevice(selectedDevice);
```

### Query Configuration

```cpp
// After initialization
std::cout << "Sample Rate: " << audioInterface.getSampleRate() << " Hz" << std::endl;
std::cout << "Buffer Size: " << audioInterface.getBufferSize() << " frames" << std::endl;
std::cout << "Device: " << audioInterface.getDeviceName() << std::endl;
```

## Testing

### Unit Tests

Location: `tests/unit/platform/CoreAudioInterfaceTest.cpp`

Tests cover:
- Device enumeration
- Default device detection
- Initialization (default and specific device)
- Start/stop lifecycle
- Audio callback routing
- Configuration queries
- Error handling (double init, start without init, etc.)
- Cleanup on destruction

Run tests:
```bash
./build/tests/unit/kick_drum_tests --gtest_filter=CoreAudioInterface*
```

### Manual Tests

Location: `tests/manual/test_coreaudio_integration.cpp`

Interactive test that:
1. Lists available devices
2. Initializes with default device
3. Starts audio
4. Triggers test notes (you should hear kick drums)
5. Tests stop/restart
6. Verifies cleanup

Run manual test:
```bash
./build/tests/manual/test_coreaudio_integration
```

**Note**: This test plays audio through your default output device. Adjust volume accordingly!

## Build Configuration

The CoreAudio integration is automatically included on macOS builds:

```cmake
# In src/platform/CMakeLists.txt
if(APPLE)
    target_sources(kick_drum_platform PRIVATE
        audio/CoreAudioInterface.cpp
    )
    
    target_link_libraries(kick_drum_platform PRIVATE
        ${COREAUDIO_FRAMEWORK}
        ${AUDIOTOOLBOX_FRAMEWORK}
        ${COREFOUNDATION_FRAMEWORK}
    )
endif()
```

## Future Enhancements

Potential improvements:

1. **Device Hot-Plugging**
   - Listen for device connection/disconnection events
   - Automatically switch to new default device
   - Notify application of device changes

2. **Buffer Size Control**
   - Allow application to request specific buffer size
   - Handle buffer size changes at runtime

3. **Sample Rate Conversion**
   - Support devices with non-standard sample rates
   - Automatic resampling if needed

4. **Multi-Channel Support**
   - Support surround sound configurations
   - Channel mapping for multi-output devices

5. **Latency Reporting**
   - Calculate and report total system latency
   - Include device latency + buffer latency

6. **Error Recovery**
   - Automatic recovery from device errors
   - Graceful handling of overruns/underruns

## References

- [Apple CoreAudio Documentation](https://developer.apple.com/documentation/coreaudio)
- [Audio Unit Programming Guide](https://developer.apple.com/library/archive/documentation/MusicAudio/Conceptual/AudioUnitProgrammingGuide/)
- [Core Audio Overview](https://developer.apple.com/library/archive/documentation/MusicAudio/Conceptual/CoreAudioOverview/)

## Implementation Status

✅ **Completed**:
- Device enumeration and selection
- Default device detection
- AudioUnit creation and configuration
- Audio callback routing to AudioEngine
- Format conversion (interleaved ↔ non-interleaved)
- Start/stop lifecycle management
- Error handling and cleanup
- Unit tests
- Manual integration test
- Documentation

**Task 17.2 Complete**: CoreAudio integration is fully implemented and tested.
