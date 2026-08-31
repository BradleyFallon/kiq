# CoreMIDI Integration Implementation

> Historical milestone note: the parsing/interface classes described here
> remain in the repository, but the current standalone bridge wires only its
> thread-safe note-on callback. Note-off, CC mapping, and pitch bend are not
> currently connected to the app's synthesis controls. See
> [STANDALONE_APP_GUIDE.md](STANDALONE_APP_GUIDE.md) for current behavior.

## Overview

This document describes the implementation of CoreMIDI integration for the Kick Drum Synthesizer standalone macOS application (Task 17.3).

## Implementation Summary

### Files Created/Modified

**New Files:**
- `src/platform/midi/CoreMIDIInterface.h` - CoreMIDI interface header
- `src/platform/midi/CoreMIDIInterface.cpp` - CoreMIDI implementation
- `tests/unit/platform/CoreMIDIInterfaceTest.cpp` - Unit tests
- `tests/manual/test_coremidi_integration.cpp` - Manual integration test
- `test_coremidi_compile.sh` - Build and test script

**Modified Files:**
- `tests/unit/CMakeLists.txt` - Added CoreMIDI test
- `tests/manual/CMakeLists.txt` - Added manual test
- `src/audio_engine/CMakeLists.txt` - Added ParameterEventQueue.cpp

## Features Implemented

### 1. MIDI Device Enumeration

The `CoreMIDIInterface` class provides methods to enumerate available MIDI input devices:

```cpp
std::vector<DeviceInfo> getAvailableDevices() const;
size_t getDeviceCount() const;
DeviceInfo getDeviceInfo(size_t index) const;
```

Each `DeviceInfo` contains:
- `MIDIEndpointRef endpoint` - CoreMIDI endpoint reference
- `std::string name` - Device name
- `std::string manufacturer` - Manufacturer name
- `bool isOnline` - Online status

### 2. MIDI Input Connection

Connect to MIDI devices by endpoint or index:

```cpp
bool connectToDevice(MIDIEndpointRef endpoint);
bool connectToDeviceByIndex(size_t deviceIndex);
void disconnect();
bool isConnected() const;
```

### 3. MIDI Packet Parsing

The implementation parses CoreMIDI packets and converts them to `MIDIMessage` objects:

**Supported Message Types:**
- **Note On** (0x90) - Triggers synthesis with velocity
- **Note Off** (0x80) - Releases voice
- **Control Change** (0xB0) - Maps to parameters
- **Pitch Bend** (0xE0) - Applies pitch modulation

**Parsing Features:**
- Handles multiple messages per packet
- Treats Note On with velocity 0 as Note Off
- Extracts 14-bit pitch bend values
- Validates message lengths
- Skips system real-time messages

### 4. Message Routing

MIDI messages are routed to the `MIDIHandler` which:
- Allocates voices for Note On messages
- Releases voices for Note Off messages
- Updates parameters for CC messages
- Applies pitch bend to active voices

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                  CoreMIDI System                         │
│                  (macOS Framework)                       │
└─────────────────────┬───────────────────────────────────┘
                      │ MIDIPacketList
                      ▼
┌─────────────────────────────────────────────────────────┐
│              CoreMIDIInterface                           │
│  ┌────────────────────────────────────────────────┐    │
│  │  • Enumerate devices                            │    │
│  │  • Connect to device                            │    │
│  │  • Parse MIDI packets                           │    │
│  │  • Convert to MIDIMessage                       │    │
│  └────────────────────────────────────────────────┘    │
└─────────────────────┬───────────────────────────────────┘
                      │ MIDIMessage
                      ▼
┌─────────────────────────────────────────────────────────┐
│                  MIDIHandler                             │
│  ┌────────────────────────────────────────────────┐    │
│  │  • Route Note On/Off to VoiceAllocator         │    │
│  │  • Map CC to ParameterManager                   │    │
│  │  • Apply pitch bend to voices                   │    │
│  └────────────────────────────────────────────────┘    │
└─────────────────────┬───────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────┐
│                  AudioEngine                             │
│  • Voice synthesis                                       │
│  • Parameter updates                                     │
│  • Audio output                                          │
└─────────────────────────────────────────────────────────┘
```

## Implementation Details

### CoreMIDI Client Setup

```cpp
bool CoreMIDIInterface::initialize() {
    // Create MIDI client
    CFStringRef clientName = CFStringCreateWithCString(
        nullptr, "KickDrumSynth", kCFStringEncodingUTF8);
    OSStatus status = MIDIClientCreate(
        clientName, nullptr, nullptr, &midiClient_);
    CFRelease(clientName);

    // Create input port
    CFStringRef portName = CFStringCreateWithCString(
        nullptr, "KickDrumInput", kCFStringEncodingUTF8);
    status = MIDIInputPortCreate(
        midiClient_, portName, midiReadCallback, this, &inputPort_);
    CFRelease(portName);

    return (status == noErr);
}
```

### MIDI Read Callback

The callback is a C-style function required by CoreMIDI:

```cpp
static void midiReadCallback(
    const MIDIPacketList* packetList,
    void* readProcRefCon,
    void* srcConnRefCon)
{
    CoreMIDIInterface* interface = 
        static_cast<CoreMIDIInterface*>(readProcRefCon);
    interface->processMIDIPacketList(packetList);
}
```

### Packet Processing

Each packet can contain multiple MIDI messages:

```cpp
void CoreMIDIInterface::processMIDIPacket(const MIDIPacket* packet) {
    const Byte* data = packet->data;
    UInt16 length = packet->length;

    for (UInt16 i = 0; i < length; ) {
        Byte statusByte = data[i];
        Byte messageType = statusByte & 0xF0;
        Byte channel = statusByte & 0x0F;

        MIDIMessage message;
        message.channel = channel;
        message.timestamp = packet->timeStamp;

        switch (messageType) {
            case 0x90: // Note On
                message.type = MIDIMessageType::NOTE_ON;
                message.data1 = data[i + 1] & 0x7F;
                message.data2 = data[i + 2] & 0x7F;
                midiHandler_->processMIDIMessage(message);
                i += 3;
                break;
            // ... other message types
        }
    }
}
```

## Testing

### Unit Tests

**Test Coverage:**
- Construction and initialization
- Device enumeration
- Device connection/disconnection
- MIDI handler management
- Error handling (invalid indices, uninitialized state)
- Cleanup on destruction

**Running Unit Tests:**
```bash
./build/bin/kick_drum_tests --gtest_filter="CoreMIDIInterface*"
```

**Results:**
```
[==========] Running 10 tests from 1 test suite.
[----------] 10 tests from CoreMIDIInterfaceTest
[ RUN      ] CoreMIDIInterfaceTest.Construction
[       OK ] CoreMIDIInterfaceTest.Construction (0 ms)
[ RUN      ] CoreMIDIInterfaceTest.Initialization
[       OK ] CoreMIDIInterfaceTest.Initialization (678 ms)
[ RUN      ] CoreMIDIInterfaceTest.EnumerateDevices
[       OK ] CoreMIDIInterfaceTest.EnumerateDevices (26 ms)
[ RUN      ] CoreMIDIInterfaceTest.GetDeviceInfo
[       OK ] CoreMIDIInterfaceTest.GetDeviceInfo (0 ms)
[ RUN      ] CoreMIDIInterfaceTest.ConnectByIndex
[       OK ] CoreMIDIInterfaceTest.ConnectByIndex (0 ms)
[ RUN      ] CoreMIDIInterfaceTest.ConnectBeforeInit
[       OK ] CoreMIDIInterfaceTest.ConnectBeforeInit (0 ms)
[ RUN      ] CoreMIDIInterfaceTest.SetMIDIHandler
[       OK ] CoreMIDIInterfaceTest.SetMIDIHandler (0 ms)
[ RUN      ] CoreMIDIInterfaceTest.DisconnectWhenNotConnected
[       OK ] CoreMIDIInterfaceTest.DisconnectWhenNotConnected (0 ms)
[ RUN      ] CoreMIDIInterfaceTest.MultipleInitialization
[       OK ] CoreMIDIInterfaceTest.MultipleInitialization (0 ms)
[ RUN      ] CoreMIDIInterfaceTest.CleanupOnDestruction
[       OK ] CoreMIDIInterfaceTest.CleanupOnDestruction (0 ms)
[----------] 10 tests from CoreMIDIInterfaceTest (706 ms total)

[  PASSED  ] 10 tests.
```

### Manual Integration Test

The manual test demonstrates real-world usage:

```bash
./build/bin/test_coremidi_integration
```

**Test Features:**
- Lists all available MIDI devices
- Connects to the first device
- Listens for MIDI messages for 60 seconds
- Logs all received messages to console
- Displays statistics at the end

**Example Output:**
```
=== CoreMIDI Integration Test ===

Initializing CoreMIDI...
CoreMIDI initialized successfully

Available MIDI Input Devices:
  [0] USB MIDI Interface (Manufacturer) - Online

Connecting to device [0]: USB MIDI Interface
Connected successfully

Listening for MIDI messages...
Play some notes on your MIDI controller
Press Ctrl+C to exit

NOTE ON:  Note=60 Velocity=100
NOTE OFF: Note=60
CC:       Number=1 Value=64
PITCH BEND: Value=8192

=== Statistics ===
Note On messages:   1
Note Off messages:  1
CC messages:        1
Pitch Bend messages:1
```

## Requirements Validation

### Requirement 9.3: MIDI Input from Connected Devices

✅ **Implemented:**
- Enumerates all available MIDI input devices
- Displays device names and manufacturers
- Connects to selected devices
- Receives MIDI input in real-time

### Requirement 9.6: MIDI Input Routing Configuration

✅ **Implemented:**
- Routes MIDI messages to MIDIHandler
- MIDIHandler routes to VoiceAllocator and ParameterManager
- Supports CC mapping to parameters
- Applies pitch bend to voices

## Integration with Standalone App

To use CoreMIDI in the standalone application:

```cpp
#include "platform/midi/CoreMIDIInterface.h"
#include "audio_engine/midi/MIDIHandler.h"

// Create MIDI handler
MIDIHandler midiHandler(&voiceAllocator, &parameterManager);

// Create CoreMIDI interface
CoreMIDIInterface coreMIDI(&midiHandler);

// Initialize
if (coreMIDI.initialize()) {
    // Enumerate devices
    auto devices = coreMIDI.getAvailableDevices();
    
    // Connect to first device
    if (!devices.empty()) {
        coreMIDI.connectToDevice(devices[0].endpoint);
    }
}

// MIDI messages are now automatically routed to the audio engine
```

## Error Handling

The implementation handles various error conditions:

1. **Initialization Failure:**
   - Returns false if MIDI client or port creation fails
   - Logs error messages with OSStatus codes

2. **Device Connection Errors:**
   - Validates device indices
   - Checks initialization state before connecting
   - Disconnects from previous device before connecting to new one

3. **Invalid MIDI Data:**
   - Validates message lengths
   - Skips malformed messages
   - Handles system real-time messages gracefully

4. **Cleanup:**
   - Disconnects from devices on destruction
   - Disposes of MIDI ports and clients
   - Safe to call cleanup multiple times

## Performance Considerations

1. **Real-Time Callback:**
   - MIDI read callback runs on CoreMIDI's real-time thread
   - Minimal processing in callback (just parsing and routing)
   - No memory allocation in callback

2. **Message Parsing:**
   - Efficient byte-by-byte parsing
   - No string operations in hot path
   - Direct conversion to MIDIMessage struct

3. **Device Enumeration:**
   - Cached device list (call getAvailableDevices() when needed)
   - Efficient CoreFoundation string handling

## Future Enhancements

Potential improvements for future versions:

1. **Hot-Plug Support:**
   - Register for MIDI device notifications
   - Automatically reconnect when devices are added/removed

2. **Multiple Device Support:**
   - Connect to multiple MIDI devices simultaneously
   - Merge MIDI streams from multiple sources

3. **MIDI Clock/Sync:**
   - Support MIDI clock messages
   - Tempo synchronization

4. **SysEx Support:**
   - Parse System Exclusive messages
   - Device-specific parameter control

5. **MIDI Learn UI:**
   - Visual feedback for CC learn mode
   - Display current CC mappings

## Build Instructions

### Quick Build and Test

```bash
./test_coremidi_compile.sh
```

### Manual Build

```bash
# Configure
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_VST3=OFF -DBUILD_STANDALONE=OFF

# Build platform library
cmake --build build --target kick_drum_platform

# Build and run unit tests
cmake --build build --target kick_drum_tests
./build/bin/kick_drum_tests --gtest_filter="CoreMIDIInterface*"

# Build manual test
cmake --build build --target test_coremidi_integration
./build/bin/test_coremidi_integration
```

## Conclusion

The CoreMIDI integration is complete and fully functional. It provides:

✅ Device enumeration
✅ MIDI input connection
✅ Packet parsing
✅ Message routing to audio engine
✅ Comprehensive unit tests
✅ Manual integration test
✅ Error handling
✅ Clean resource management

The implementation satisfies requirements 9.3 and 9.6, enabling the standalone macOS application to receive MIDI input from connected controllers and route messages to the audio engine for synthesis.
