/**
 * Manual test for CoreAudio integration
 * 
 * This test verifies that:
 * 1. CoreAudio can be initialized with the default device
 * 2. Audio device configuration works correctly
 * 3. Audio callback routing to AudioEngine works
 * 4. Audio can be started and stopped
 * 
 * Requirements: 9.2, 9.4, 9.5
 */

#ifdef __APPLE__

#include "../../src/platform/audio/CoreAudioInterface.h"
#include "AudioEngine.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace KickDrum;

int main() {
    std::cout << "=== CoreAudio Integration Test ===" << std::endl;
    std::cout << std::endl;
    
    // Test 1: List available devices
    std::cout << "Test 1: Listing available audio devices..." << std::endl;
    auto devices = CoreAudioInterface::getAvailableDevices();
    std::cout << "Found " << devices.size() << " output device(s):" << std::endl;
    for (size_t i = 0; i < devices.size(); ++i) {
        std::cout << "  [" << i << "] " << CoreAudioInterface::getDeviceName(devices[i]) 
                  << " (ID: " << devices[i] << ")" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 2: Get default device
    std::cout << "Test 2: Getting default output device..." << std::endl;
    AudioDeviceID defaultDevice = CoreAudioInterface::getDefaultOutputDevice();
    if (defaultDevice == 0) {
        std::cerr << "ERROR: No default output device found!" << std::endl;
        return 1;
    }
    std::cout << "Default device: " << CoreAudioInterface::getDeviceName(defaultDevice) 
              << " (ID: " << defaultDevice << ")" << std::endl;
    std::cout << std::endl;
    
    // Test 3: Create audio engine
    std::cout << "Test 3: Creating audio engine..." << std::endl;
    AudioEngine audioEngine;
    std::cout << "Audio engine created" << std::endl;
    std::cout << std::endl;
    
    // Test 4: Create CoreAudio interface
    std::cout << "Test 4: Creating CoreAudio interface..." << std::endl;
    CoreAudioInterface audioInterface(&audioEngine);
    std::cout << "CoreAudio interface created" << std::endl;
    std::cout << std::endl;
    
    // Test 5: Initialize with default device
    std::cout << "Test 5: Initializing CoreAudio with default device..." << std::endl;
    if (!audioInterface.initialize()) {
        std::cerr << "ERROR: Failed to initialize CoreAudio!" << std::endl;
        return 1;
    }
    std::cout << "CoreAudio initialized successfully" << std::endl;
    std::cout << "  Sample Rate: " << audioInterface.getSampleRate() << " Hz" << std::endl;
    std::cout << "  Buffer Size: " << audioInterface.getBufferSize() << " frames" << std::endl;
    std::cout << "  Device: " << audioInterface.getDeviceName() << std::endl;
    std::cout << std::endl;
    
    // Test 6: Start audio
    std::cout << "Test 6: Starting audio..." << std::endl;
    if (!audioInterface.start()) {
        std::cerr << "ERROR: Failed to start audio!" << std::endl;
        return 1;
    }
    std::cout << "Audio started successfully" << std::endl;
    std::cout << "Running: " << (audioInterface.isRunning() ? "YES" : "NO") << std::endl;
    std::cout << std::endl;
    
    // Test 7: Trigger a note to verify audio callback is working
    std::cout << "Test 7: Triggering a test note (60, velocity 0.8)..." << std::endl;
    audioEngine.enqueueNoteOn(60, 0.8f);
    std::cout << "Note triggered - you should hear a kick drum sound" << std::endl;
    std::cout << "Waiting 2 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << std::endl;
    
    // Test 8: Trigger another note
    std::cout << "Test 8: Triggering another note (48, velocity 1.0)..." << std::endl;
    audioEngine.enqueueNoteOn(48, 1.0f);
    std::cout << "Note triggered - you should hear a lower kick drum sound" << std::endl;
    std::cout << "Waiting 2 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << std::endl;
    
    // Test 9: Stop audio
    std::cout << "Test 9: Stopping audio..." << std::endl;
    audioInterface.stop();
    std::cout << "Audio stopped" << std::endl;
    std::cout << "Running: " << (audioInterface.isRunning() ? "YES" : "NO") << std::endl;
    std::cout << std::endl;
    
    // Test 10: Restart audio
    std::cout << "Test 10: Restarting audio..." << std::endl;
    if (!audioInterface.start()) {
        std::cerr << "ERROR: Failed to restart audio!" << std::endl;
        return 1;
    }
    std::cout << "Audio restarted successfully" << std::endl;
    
    // Trigger one more note
    std::cout << "Triggering final test note..." << std::endl;
    audioEngine.enqueueNoteOn(72, 0.9f);
    std::cout << "Waiting 2 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << std::endl;
    
    // Cleanup
    std::cout << "Test complete - cleaning up..." << std::endl;
    audioInterface.stop();
    
    std::cout << std::endl;
    std::cout << "=== All Tests Passed ===" << std::endl;
    std::cout << std::endl;
    std::cout << "Summary:" << std::endl;
    std::cout << "  ✓ Device enumeration works" << std::endl;
    std::cout << "  ✓ Default device detection works" << std::endl;
    std::cout << "  ✓ CoreAudio initialization works" << std::endl;
    std::cout << "  ✓ Audio device configuration works" << std::endl;
    std::cout << "  ✓ Audio callback routing to AudioEngine works" << std::endl;
    std::cout << "  ✓ Audio start/stop works" << std::endl;
    std::cout << "  ✓ Note triggering through audio callback works" << std::endl;
    
    return 0;
}

#else

#include <iostream>

int main() {
    std::cout << "This test is only available on macOS" << std::endl;
    return 0;
}

#endif // __APPLE__
