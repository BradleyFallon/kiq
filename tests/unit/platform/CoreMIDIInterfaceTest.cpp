#ifdef __APPLE__

#include <gtest/gtest.h>
#include "../../../src/platform/midi/CoreMIDIInterface.h"
#include "../../../src/audio_engine/midi/MIDIHandler.h"
#include "../../../src/audio_engine/voice/VoiceAllocator.h"
#include "../../../src/audio_engine/parameters/ParameterManager.h"

using namespace KickDrum;

class CoreMIDIInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create voice allocator and parameter manager
        voiceAllocator = std::make_unique<VoiceAllocator>();
        voiceAllocator->initialize(48000.0f);
        parameterManager = std::make_unique<ParameterManager>();
        
        // Create MIDI handler
        midiHandler = std::make_unique<MIDIHandler>(voiceAllocator.get(), parameterManager.get());
        
        // Create CoreMIDI interface
        coreMIDI = std::make_unique<CoreMIDIInterface>(midiHandler.get());
    }

    void TearDown() override {
        coreMIDI.reset();
        midiHandler.reset();
        parameterManager.reset();
        voiceAllocator.reset();
    }

    std::unique_ptr<VoiceAllocator> voiceAllocator;
    std::unique_ptr<ParameterManager> parameterManager;
    std::unique_ptr<MIDIHandler> midiHandler;
    std::unique_ptr<CoreMIDIInterface> coreMIDI;
};

// Test: CoreMIDI interface can be constructed
TEST_F(CoreMIDIInterfaceTest, Construction) {
    EXPECT_NE(coreMIDI, nullptr);
    EXPECT_EQ(coreMIDI->getMIDIHandler(), midiHandler.get());
    EXPECT_FALSE(coreMIDI->isConnected());
}

// Test: CoreMIDI interface can be initialized
TEST_F(CoreMIDIInterfaceTest, Initialization) {
    bool initialized = coreMIDI->initialize();
    
    // Initialization may fail if no MIDI system is available
    // (e.g., in CI environment), so we just check it doesn't crash
    if (initialized) {
        EXPECT_FALSE(coreMIDI->isConnected());
    }
}

// Test: Can enumerate MIDI devices
TEST_F(CoreMIDIInterfaceTest, EnumerateDevices) {
    bool initialized = coreMIDI->initialize();
    
    if (initialized) {
        std::vector<CoreMIDIInterface::DeviceInfo> devices = coreMIDI->getAvailableDevices();
        size_t deviceCount = coreMIDI->getDeviceCount();
        
        EXPECT_EQ(devices.size(), deviceCount);
        
        // Check device info structure
        for (const auto& device : devices) {
            EXPECT_NE(device.endpoint, 0);
            // Name may be empty for some devices, so we don't assert on it
        }
    }
}

// Test: Can get device info by index
TEST_F(CoreMIDIInterfaceTest, GetDeviceInfo) {
    bool initialized = coreMIDI->initialize();
    
    if (initialized) {
        size_t deviceCount = coreMIDI->getDeviceCount();
        
        if (deviceCount > 0) {
            // Get first device info
            CoreMIDIInterface::DeviceInfo info = coreMIDI->getDeviceInfo(0);
            EXPECT_NE(info.endpoint, 0);
        }
        
        // Out of range should return empty info
        CoreMIDIInterface::DeviceInfo emptyInfo = coreMIDI->getDeviceInfo(9999);
        EXPECT_EQ(emptyInfo.endpoint, 0);
        EXPECT_EQ(emptyInfo.name, "");
    }
}

// Test: Can connect to device by index
TEST_F(CoreMIDIInterfaceTest, ConnectByIndex) {
    bool initialized = coreMIDI->initialize();
    
    if (initialized) {
        size_t deviceCount = coreMIDI->getDeviceCount();
        
        if (deviceCount > 0) {
            // Try to connect to first device
            bool connected = coreMIDI->connectToDeviceByIndex(0);
            
            if (connected) {
                EXPECT_TRUE(coreMIDI->isConnected());
                EXPECT_NE(coreMIDI->getConnectedDevice(), 0);
                
                // Disconnect
                coreMIDI->disconnect();
                EXPECT_FALSE(coreMIDI->isConnected());
                EXPECT_EQ(coreMIDI->getConnectedDevice(), 0);
            }
        }
        
        // Out of range should fail
        bool connected = coreMIDI->connectToDeviceByIndex(9999);
        EXPECT_FALSE(connected);
    }
}

// Test: Cannot connect before initialization
TEST_F(CoreMIDIInterfaceTest, ConnectBeforeInit) {
    bool connected = coreMIDI->connectToDeviceByIndex(0);
    EXPECT_FALSE(connected);
    EXPECT_FALSE(coreMIDI->isConnected());
}

// Test: Can set and get MIDI handler
TEST_F(CoreMIDIInterfaceTest, SetMIDIHandler) {
    EXPECT_EQ(coreMIDI->getMIDIHandler(), midiHandler.get());
    
    // Create a new MIDI handler
    auto newHandler = std::make_unique<MIDIHandler>(voiceAllocator.get());
    coreMIDI->setMIDIHandler(newHandler.get());
    
    EXPECT_EQ(coreMIDI->getMIDIHandler(), newHandler.get());
    
    // Restore original handler
    coreMIDI->setMIDIHandler(midiHandler.get());
}

// Test: Disconnect when not connected is safe
TEST_F(CoreMIDIInterfaceTest, DisconnectWhenNotConnected) {
    EXPECT_FALSE(coreMIDI->isConnected());
    
    // Should not crash
    coreMIDI->disconnect();
    
    EXPECT_FALSE(coreMIDI->isConnected());
}

// Test: Multiple initialization calls are safe
TEST_F(CoreMIDIInterfaceTest, MultipleInitialization) {
    bool init1 = coreMIDI->initialize();
    bool init2 = coreMIDI->initialize();
    
    // Second initialization should succeed (idempotent)
    if (init1) {
        EXPECT_TRUE(init2);
    }
}

// Test: Cleanup on destruction
TEST_F(CoreMIDIInterfaceTest, CleanupOnDestruction) {
    bool initialized = coreMIDI->initialize();
    
    if (initialized) {
        size_t deviceCount = coreMIDI->getDeviceCount();
        
        if (deviceCount > 0) {
            coreMIDI->connectToDeviceByIndex(0);
        }
    }
    
    // Destructor should clean up without crashing
    coreMIDI.reset();
    
    EXPECT_EQ(coreMIDI, nullptr);
}

#endif // __APPLE__
