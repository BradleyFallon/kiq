#ifdef __APPLE__

#include <iostream>
#include <thread>
#include <chrono>
#include "../../src/platform/midi/CoreMIDIInterface.h"
#include "../../src/audio_engine/midi/MIDIHandler.h"
#include "../../src/audio_engine/voice/VoiceAllocator.h"
#include "../../src/audio_engine/parameters/ParameterManager.h"

using namespace KickDrum;

/**
 * Manual test for CoreMIDI integration
 * 
 * This test demonstrates:
 * 1. Enumerating MIDI input devices
 * 2. Connecting to a MIDI device
 * 3. Receiving MIDI messages
 * 4. Routing messages to the audio engine
 * 
 * To run this test:
 * 1. Connect a MIDI controller to your Mac
 * 2. Build and run this test
 * 3. Play notes on your MIDI controller
 * 4. Observe the console output showing received MIDI messages
 */

// Global counters for message statistics
int noteOnCount = 0;
int noteOffCount = 0;
int ccCount = 0;
int pitchBendCount = 0;

// Wrapper MIDI handler that logs messages
class LoggingMIDIHandler : public MIDIHandler {
public:
    LoggingMIDIHandler(VoiceAllocator* voiceAllocator, ParameterManager* parameterManager)
        : MIDIHandler(voiceAllocator, parameterManager)
    {
    }

    void processMIDIMessage(const MIDIMessage& message) {
        // Log the message before processing
        if (message.isNoteOn()) {
            noteOnCount++;
            std::cout << "NOTE ON:  Note=" << message.data1 
                      << " Velocity=" << message.data2 
                      << " Channel=" << static_cast<int>(message.channel) << std::endl;
        } else if (message.isNoteOff()) {
            noteOffCount++;
            std::cout << "NOTE OFF: Note=" << message.data1 
                      << " Channel=" << static_cast<int>(message.channel) << std::endl;
        } else if (message.type == KickDrum::MIDIMessageType::CC) {
            ccCount++;
            std::cout << "CC:       Number=" << message.data1 
                      << " Value=" << message.data2 
                      << " Channel=" << static_cast<int>(message.channel) << std::endl;
        } else if (message.type == KickDrum::MIDIMessageType::PITCH_BEND) {
            pitchBendCount++;
            int pitchBend14bit = (message.data2 << 7) | message.data1;
            std::cout << "PITCH BEND: Value=" << pitchBend14bit 
                      << " Channel=" << static_cast<int>(message.channel) << std::endl;
        }
        
        // Call base class implementation
        MIDIHandler::processMIDIMessage(message);
    }
};

int main() {
    std::cout << "=== CoreMIDI Integration Test ===" << std::endl;
    std::cout << std::endl;

    // Create audio engine components
    VoiceAllocator voiceAllocator;
    voiceAllocator.initialize(48000.0f);
    ParameterManager parameterManager;
    LoggingMIDIHandler midiHandler(&voiceAllocator, &parameterManager);

    // Create CoreMIDI interface
    CoreMIDIInterface coreMIDI(&midiHandler);

    // Initialize CoreMIDI
    std::cout << "Initializing CoreMIDI..." << std::endl;
    if (!coreMIDI.initialize()) {
        std::cerr << "Failed to initialize CoreMIDI" << std::endl;
        return 1;
    }
    std::cout << "CoreMIDI initialized successfully" << std::endl;
    std::cout << std::endl;

    // Enumerate MIDI devices
    std::cout << "Available MIDI Input Devices:" << std::endl;
    std::vector<CoreMIDIInterface::DeviceInfo> devices = coreMIDI.getAvailableDevices();
    
    if (devices.empty()) {
        std::cout << "No MIDI input devices found" << std::endl;
        std::cout << "Please connect a MIDI controller and try again" << std::endl;
        return 0;
    }

    for (size_t i = 0; i < devices.size(); ++i) {
        std::cout << "  [" << i << "] " << devices[i].name;
        if (!devices[i].manufacturer.empty()) {
            std::cout << " (" << devices[i].manufacturer << ")";
        }
        std::cout << " - " << (devices[i].isOnline ? "Online" : "Offline") << std::endl;
    }
    std::cout << std::endl;

    // Connect to first available device
    std::cout << "Connecting to device [0]: " << devices[0].name << std::endl;
    if (!coreMIDI.connectToDeviceByIndex(0)) {
        std::cerr << "Failed to connect to MIDI device" << std::endl;
        return 1;
    }
    std::cout << "Connected successfully" << std::endl;
    std::cout << std::endl;

    // Listen for MIDI messages
    std::cout << "Listening for MIDI messages..." << std::endl;
    std::cout << "Play some notes on your MIDI controller" << std::endl;
    std::cout << "Press Ctrl+C to exit" << std::endl;
    std::cout << std::endl;

    // Run for 60 seconds
    auto startTime = std::chrono::steady_clock::now();
    while (true) {
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();
        
        if (elapsed >= 60) {
            break;
        }

        // Sleep to avoid busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Print statistics
    std::cout << std::endl;
    std::cout << "=== Statistics ===" << std::endl;
    std::cout << "Note On messages:   " << noteOnCount << std::endl;
    std::cout << "Note Off messages:  " << noteOffCount << std::endl;
    std::cout << "CC messages:        " << ccCount << std::endl;
    std::cout << "Pitch Bend messages:" << pitchBendCount << std::endl;
    std::cout << std::endl;

    // Disconnect
    std::cout << "Disconnecting..." << std::endl;
    coreMIDI.disconnect();
    std::cout << "Test complete" << std::endl;

    return 0;
}

#else

#include <iostream>

int main() {
    std::cout << "This test is only available on macOS" << std::endl;
    return 0;
}

#endif // __APPLE__
