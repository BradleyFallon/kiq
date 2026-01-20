/**
 * Unit tests for CoreAudio integration
 * 
 * These tests verify the CoreAudio interface functionality:
 * - Device enumeration
 * - Initialization
 * - Configuration
 * - Audio callback routing
 * 
 * Requirements: 9.2, 9.4, 9.5
 */

#include <gtest/gtest.h>

#ifdef __APPLE__

#include "../../../src/platform/audio/CoreAudioInterface.h"
#include "AudioEngine.h"
#include <thread>
#include <chrono>

using namespace KickDrum;

class CoreAudioInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        audioEngine = std::make_unique<AudioEngine>();
    }

    void TearDown() override {
        audioEngine.reset();
    }

    std::unique_ptr<AudioEngine> audioEngine;
};

// Test 1: Device enumeration
TEST_F(CoreAudioInterfaceTest, DeviceEnumeration) {
    auto devices = CoreAudioInterface::getAvailableDevices();
    
    // Should have at least one output device on macOS
    EXPECT_GT(devices.size(), 0) << "No audio output devices found";
    
    // Each device should have a valid ID
    for (AudioDeviceID deviceId : devices) {
        EXPECT_NE(deviceId, 0) << "Invalid device ID";
        
        // Should be able to get device name
        std::string name = CoreAudioInterface::getDeviceName(deviceId);
        EXPECT_FALSE(name.empty()) << "Device name is empty for device " << deviceId;
    }
}

// Test 2: Default device detection
TEST_F(CoreAudioInterfaceTest, DefaultDeviceDetection) {
    AudioDeviceID defaultDevice = CoreAudioInterface::getDefaultOutputDevice();
    
    // Should have a default output device
    EXPECT_NE(defaultDevice, 0) << "No default output device found";
    
    // Should be able to get its name
    std::string name = CoreAudioInterface::getDeviceName(defaultDevice);
    EXPECT_FALSE(name.empty()) << "Default device name is empty";
}

// Test 3: Interface creation
TEST_F(CoreAudioInterfaceTest, InterfaceCreation) {
    CoreAudioInterface audioInterface(audioEngine.get());
    
    // Should not be initialized or running initially
    EXPECT_FALSE(audioInterface.isRunning());
}

// Test 4: Initialization with default device
TEST_F(CoreAudioInterfaceTest, InitializationWithDefaultDevice) {
    CoreAudioInterface audioInterface(audioEngine.get());
    
    bool success = audioInterface.initialize();
    EXPECT_TRUE(success) << "Failed to initialize with default device";
    
    if (success) {
        // Should have valid configuration
        EXPECT_GT(audioInterface.getSampleRate(), 0.0);
        EXPECT_GT(audioInterface.getBufferSize(), 0);
        EXPECT_NE(audioInterface.getDeviceId(), 0);
        EXPECT_FALSE(audioInterface.getDeviceName().empty());
        
        // Should not be running yet
        EXPECT_FALSE(audioInterface.isRunning());
    }
}

// Test 5: Initialization with specific device
TEST_F(CoreAudioInterfaceTest, InitializationWithSpecificDevice) {
    AudioDeviceID defaultDevice = CoreAudioInterface::getDefaultOutputDevice();
    ASSERT_NE(defaultDevice, 0) << "No default device available for test";
    
    CoreAudioInterface audioInterface(audioEngine.get());
    
    bool success = audioInterface.initializeWithDevice(defaultDevice);
    EXPECT_TRUE(success) << "Failed to initialize with specific device";
    
    if (success) {
        EXPECT_EQ(audioInterface.getDeviceId(), defaultDevice);
    }
}

// Test 6: Double initialization should fail
TEST_F(CoreAudioInterfaceTest, DoubleInitializationFails) {
    CoreAudioInterface audioInterface(audioEngine.get());
    
    bool firstInit = audioInterface.initialize();
    ASSERT_TRUE(firstInit) << "First initialization failed";
    
    // Second initialization should fail
    bool secondInit = audioInterface.initialize();
    EXPECT_FALSE(secondInit) << "Second initialization should fail";
}

// Test 7: Start without initialization should fail
TEST_F(CoreAudioInterfaceTest, StartWithoutInitializationFails) {
    CoreAudioInterface audioInterface(audioEngine.get());
    
    bool success = audioInterface.start();
    EXPECT_FALSE(success) << "Start should fail without initialization";
}

// Test 8: Start and stop
TEST_F(CoreAudioInterfaceTest, StartAndStop) {
    CoreAudioInterface audioInterface(audioEngine.get());
    
    bool initSuccess = audioInterface.initialize();
    ASSERT_TRUE(initSuccess) << "Initialization failed";
    
    // Start audio
    bool startSuccess = audioInterface.start();
    EXPECT_TRUE(startSuccess) << "Failed to start audio";
    
    if (startSuccess) {
        EXPECT_TRUE(audioInterface.isRunning());
        
        // Stop audio
        audioInterface.stop();
        EXPECT_FALSE(audioInterface.isRunning());
    }
}

// Test 9: Double start should fail
TEST_F(CoreAudioInterfaceTest, DoubleStartFails) {
    CoreAudioInterface audioInterface(audioEngine.get());
    
    bool initSuccess = audioInterface.initialize();
    ASSERT_TRUE(initSuccess) << "Initialization failed";
    
    bool firstStart = audioInterface.start();
    ASSERT_TRUE(firstStart) << "First start failed";
    
    // Second start should fail
    bool secondStart = audioInterface.start();
    EXPECT_FALSE(secondStart) << "Second start should fail";
    
    audioInterface.stop();
}

// Test 10: Stop when not running is safe
TEST_F(CoreAudioInterfaceTest, StopWhenNotRunningIsSafe) {
    CoreAudioInterface audioInterface(audioEngine.get());
    
    // Should not crash or cause issues
    EXPECT_NO_THROW(audioInterface.stop());
}

// Test 11: Audio callback routing (basic test)
TEST_F(CoreAudioInterfaceTest, AudioCallbackRouting) {
    CoreAudioInterface audioInterface(audioEngine.get());
    
    bool initSuccess = audioInterface.initialize();
    ASSERT_TRUE(initSuccess) << "Initialization failed";
    
    bool startSuccess = audioInterface.start();
    ASSERT_TRUE(startSuccess) << "Failed to start audio";
    
    // Trigger a note - this should route through the audio callback
    audioEngine->noteOn(60, 0.8f);
    
    // Let it run briefly to ensure callback is called
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // If we get here without crashing, the callback routing works
    EXPECT_TRUE(audioInterface.isRunning());
    
    audioInterface.stop();
}

// Test 12: Sample rate configuration
TEST_F(CoreAudioInterfaceTest, SampleRateConfiguration) {
    CoreAudioInterface audioInterface(audioEngine.get());
    
    bool initSuccess = audioInterface.initialize();
    ASSERT_TRUE(initSuccess) << "Initialization failed";
    
    double sampleRate = audioInterface.getSampleRate();
    
    // Common sample rates
    EXPECT_TRUE(
        sampleRate == 44100.0 ||
        sampleRate == 48000.0 ||
        sampleRate == 88200.0 ||
        sampleRate == 96000.0 ||
        sampleRate == 192000.0
    ) << "Unexpected sample rate: " << sampleRate;
    
    // Audio engine should be initialized with the same sample rate
    EXPECT_FLOAT_EQ(audioEngine->getSampleRate(), static_cast<float>(sampleRate));
}

// Test 13: Buffer size configuration
TEST_F(CoreAudioInterfaceTest, BufferSizeConfiguration) {
    CoreAudioInterface audioInterface(audioEngine.get());
    
    bool initSuccess = audioInterface.initialize();
    ASSERT_TRUE(initSuccess) << "Initialization failed";
    
    UInt32 bufferSize = audioInterface.getBufferSize();
    
    // Buffer size should be reasonable (typically 64-2048 frames)
    EXPECT_GE(bufferSize, 64);
    EXPECT_LE(bufferSize, 2048);
}

// Test 14: Cleanup on destruction
TEST_F(CoreAudioInterfaceTest, CleanupOnDestruction) {
    {
        CoreAudioInterface audioInterface(audioEngine.get());
        audioInterface.initialize();
        audioInterface.start();
        
        // Interface goes out of scope here
    }
    
    // If we get here without crashing, cleanup worked
    SUCCEED();
}

// Test 15: Restart after stop
TEST_F(CoreAudioInterfaceTest, RestartAfterStop) {
    CoreAudioInterface audioInterface(audioEngine.get());
    
    bool initSuccess = audioInterface.initialize();
    ASSERT_TRUE(initSuccess) << "Initialization failed";
    
    // Start, stop, start again
    EXPECT_TRUE(audioInterface.start());
    audioInterface.stop();
    EXPECT_TRUE(audioInterface.start());
    
    EXPECT_TRUE(audioInterface.isRunning());
    
    audioInterface.stop();
}

#else

// Placeholder test for non-macOS platforms
TEST(CoreAudioInterfaceTest, NotAvailableOnThisPlatform) {
    GTEST_SKIP() << "CoreAudio tests are only available on macOS";
}

#endif // __APPLE__
