#include <gtest/gtest.h>
#include "../../../src/audio_engine/midi/MIDIMessage.h"

using namespace KickDrum;

// Test default constructor
TEST(MIDIMessageTest, DefaultConstructor) {
    MIDIMessage msg;
    
    EXPECT_EQ(msg.type, MIDIMessageType::UNKNOWN);
    EXPECT_EQ(msg.channel, 0);
    EXPECT_EQ(msg.data1, 0);
    EXPECT_EQ(msg.data2, 0);
    EXPECT_EQ(msg.timestamp, 0);
}

// Test parameterized constructor
TEST(MIDIMessageTest, ParameterizedConstructor) {
    MIDIMessage msg(MIDIMessageType::NOTE_ON, 5, 60, 100, 12345);
    
    EXPECT_EQ(msg.type, MIDIMessageType::NOTE_ON);
    EXPECT_EQ(msg.channel, 5);
    EXPECT_EQ(msg.data1, 60);
    EXPECT_EQ(msg.data2, 100);
    EXPECT_EQ(msg.timestamp, 12345);
}

// Test parsing note-on message
TEST(MIDIMessageTest, ParseNoteOn) {
    // Note-on on channel 0, note 60 (middle C), velocity 100
    uint8_t status = 0x90;  // Note-on, channel 0
    uint8_t data1 = 60;     // Note number
    uint8_t data2 = 100;    // Velocity
    uint64_t timestamp = 1000;
    
    MIDIMessage msg = parseMIDIMessage(status, data1, data2, timestamp);
    
    EXPECT_EQ(msg.type, MIDIMessageType::NOTE_ON);
    EXPECT_EQ(msg.channel, 0);
    EXPECT_EQ(msg.data1, 60);
    EXPECT_EQ(msg.data2, 100);
    EXPECT_EQ(msg.timestamp, 1000);
    EXPECT_TRUE(msg.isNoteOn());
    EXPECT_FALSE(msg.isNoteOff());
}

// Test parsing note-on on different channel
TEST(MIDIMessageTest, ParseNoteOnChannel15) {
    // Note-on on channel 15, note 48, velocity 64
    uint8_t status = 0x9F;  // Note-on, channel 15
    uint8_t data1 = 48;
    uint8_t data2 = 64;
    
    MIDIMessage msg = parseMIDIMessage(status, data1, data2);
    
    EXPECT_EQ(msg.type, MIDIMessageType::NOTE_ON);
    EXPECT_EQ(msg.channel, 15);
    EXPECT_EQ(msg.data1, 48);
    EXPECT_EQ(msg.data2, 64);
}

// Test parsing note-off message
TEST(MIDIMessageTest, ParseNoteOff) {
    // Note-off on channel 0, note 60, velocity 64
    uint8_t status = 0x80;  // Note-off, channel 0
    uint8_t data1 = 60;
    uint8_t data2 = 64;
    
    MIDIMessage msg = parseMIDIMessage(status, data1, data2);
    
    EXPECT_EQ(msg.type, MIDIMessageType::NOTE_OFF);
    EXPECT_EQ(msg.channel, 0);
    EXPECT_EQ(msg.data1, 60);
    EXPECT_EQ(msg.data2, 64);
    EXPECT_FALSE(msg.isNoteOn());
    EXPECT_TRUE(msg.isNoteOff());
}

// Test parsing note-on with velocity 0 (treated as note-off)
TEST(MIDIMessageTest, ParseNoteOnVelocityZero) {
    // Note-on with velocity 0 should be treated as note-off
    uint8_t status = 0x90;  // Note-on, channel 0
    uint8_t data1 = 60;
    uint8_t data2 = 0;      // Velocity 0
    
    MIDIMessage msg = parseMIDIMessage(status, data1, data2);
    
    EXPECT_EQ(msg.type, MIDIMessageType::NOTE_ON);
    EXPECT_FALSE(msg.isNoteOn());  // Should not be considered a note-on
    EXPECT_TRUE(msg.isNoteOff());  // Should be considered a note-off
}

// Test parsing control change message
TEST(MIDIMessageTest, ParseControlChange) {
    // CC on channel 0, controller 7 (volume), value 100
    uint8_t status = 0xB0;  // CC, channel 0
    uint8_t data1 = 7;      // Controller number
    uint8_t data2 = 100;    // Controller value
    
    MIDIMessage msg = parseMIDIMessage(status, data1, data2);
    
    EXPECT_EQ(msg.type, MIDIMessageType::CC);
    EXPECT_EQ(msg.channel, 0);
    EXPECT_EQ(msg.data1, 7);
    EXPECT_EQ(msg.data2, 100);
}

// Test parsing control change on different channel
TEST(MIDIMessageTest, ParseControlChangeChannel10) {
    // CC on channel 10, controller 1 (modulation), value 64
    uint8_t status = 0xBA;  // CC, channel 10
    uint8_t data1 = 1;
    uint8_t data2 = 64;
    
    MIDIMessage msg = parseMIDIMessage(status, data1, data2);
    
    EXPECT_EQ(msg.type, MIDIMessageType::CC);
    EXPECT_EQ(msg.channel, 10);
    EXPECT_EQ(msg.data1, 1);
    EXPECT_EQ(msg.data2, 64);
}

// Test parsing pitch bend message
TEST(MIDIMessageTest, ParsePitchBend) {
    // Pitch bend on channel 0, center position (8192)
    uint8_t status = 0xE0;  // Pitch bend, channel 0
    uint8_t data1 = 0x00;   // LSB
    uint8_t data2 = 0x40;   // MSB (0x40 << 7 = 8192)
    
    MIDIMessage msg = parseMIDIMessage(status, data1, data2);
    
    EXPECT_EQ(msg.type, MIDIMessageType::PITCH_BEND);
    EXPECT_EQ(msg.channel, 0);
    EXPECT_EQ(msg.data1, 0x00);
    EXPECT_EQ(msg.data2, 0x40);
}

// Test pitch bend value calculation - center position
TEST(MIDIMessageTest, PitchBendValueCenter) {
    // Center position: LSB=0, MSB=64 (8192 in 14-bit)
    MIDIMessage msg(MIDIMessageType::PITCH_BEND, 0, 0x00, 0x40);
    
    float value = msg.getPitchBendValue();
    EXPECT_NEAR(value, 0.0f, 0.001f);
}

// Test pitch bend value calculation - maximum up
TEST(MIDIMessageTest, PitchBendValueMaxUp) {
    // Maximum up: LSB=127, MSB=127 (16383 in 14-bit)
    MIDIMessage msg(MIDIMessageType::PITCH_BEND, 0, 0x7F, 0x7F);
    
    float value = msg.getPitchBendValue();
    EXPECT_NEAR(value, 1.0f, 0.001f);
}

// Test pitch bend value calculation - maximum down
TEST(MIDIMessageTest, PitchBendValueMaxDown) {
    // Maximum down: LSB=0, MSB=0 (0 in 14-bit)
    MIDIMessage msg(MIDIMessageType::PITCH_BEND, 0, 0x00, 0x00);
    
    float value = msg.getPitchBendValue();
    EXPECT_NEAR(value, -1.0f, 0.001f);
}

// Test pitch bend value calculation - halfway up
TEST(MIDIMessageTest, PitchBendValueHalfwayUp) {
    // Halfway up: approximately 12288 in 14-bit
    MIDIMessage msg(MIDIMessageType::PITCH_BEND, 0, 0x00, 0x60);
    
    float value = msg.getPitchBendValue();
    EXPECT_NEAR(value, 0.5f, 0.01f);
}

// Test pitch bend value for non-pitch-bend message
TEST(MIDIMessageTest, PitchBendValueNonPitchBend) {
    // Non-pitch-bend message should return 0.0
    MIDIMessage msg(MIDIMessageType::NOTE_ON, 0, 60, 100);
    
    float value = msg.getPitchBendValue();
    EXPECT_EQ(value, 0.0f);
}

// Test parsing from byte array
TEST(MIDIMessageTest, ParseFromByteArray) {
    uint8_t bytes[3] = {0x90, 60, 100};  // Note-on, note 60, velocity 100
    uint64_t timestamp = 5000;
    
    MIDIMessage msg = parseMIDIMessage(bytes, timestamp);
    
    EXPECT_EQ(msg.type, MIDIMessageType::NOTE_ON);
    EXPECT_EQ(msg.channel, 0);
    EXPECT_EQ(msg.data1, 60);
    EXPECT_EQ(msg.data2, 100);
    EXPECT_EQ(msg.timestamp, 5000);
}

// Test parsing from null byte array
TEST(MIDIMessageTest, ParseFromNullByteArray) {
    MIDIMessage msg = parseMIDIMessage(nullptr, 0);
    
    EXPECT_EQ(msg.type, MIDIMessageType::UNKNOWN);
}

// Test parsing unknown message type
TEST(MIDIMessageTest, ParseUnknownMessageType) {
    // Program change (0xC0) is not supported
    uint8_t status = 0xC0;
    uint8_t data1 = 10;
    uint8_t data2 = 0;
    
    MIDIMessage msg = parseMIDIMessage(status, data1, data2);
    
    EXPECT_EQ(msg.type, MIDIMessageType::UNKNOWN);
}

// Test data byte masking (safety check)
TEST(MIDIMessageTest, DataByteMasking) {
    // Send data bytes with bit 7 set (invalid MIDI data)
    uint8_t status = 0x90;
    uint8_t data1 = 0xFF;  // Should be masked to 0x7F (127)
    uint8_t data2 = 0x80;  // Should be masked to 0x00 (0)
    
    MIDIMessage msg = parseMIDIMessage(status, data1, data2);
    
    EXPECT_EQ(msg.data1, 127);
    EXPECT_EQ(msg.data2, 0);
}

// Test all MIDI channels (0-15)
TEST(MIDIMessageTest, AllChannels) {
    for (int channel = 0; channel < 16; ++channel) {
        uint8_t status = 0x90 | channel;  // Note-on with channel
        MIDIMessage msg = parseMIDIMessage(status, 60, 100);
        
        EXPECT_EQ(msg.channel, channel);
        EXPECT_EQ(msg.type, MIDIMessageType::NOTE_ON);
    }
}

// Test all note numbers (0-127)
TEST(MIDIMessageTest, AllNoteNumbers) {
    for (int note = 0; note <= 127; ++note) {
        MIDIMessage msg = parseMIDIMessage(0x90, note, 100);
        
        EXPECT_EQ(msg.data1, note);
        EXPECT_EQ(msg.type, MIDIMessageType::NOTE_ON);
    }
}

// Test all velocity values (0-127)
TEST(MIDIMessageTest, AllVelocityValues) {
    for (int velocity = 0; velocity <= 127; ++velocity) {
        MIDIMessage msg = parseMIDIMessage(0x90, 60, velocity);
        
        EXPECT_EQ(msg.data2, velocity);
        
        if (velocity > 0) {
            EXPECT_TRUE(msg.isNoteOn());
            EXPECT_FALSE(msg.isNoteOff());
        } else {
            EXPECT_FALSE(msg.isNoteOn());
            EXPECT_TRUE(msg.isNoteOff());
        }
    }
}

// Test timestamp preservation
TEST(MIDIMessageTest, TimestampPreservation) {
    uint64_t timestamps[] = {0, 1, 100, 1000, 10000, 100000, 1000000, UINT64_MAX};
    
    for (uint64_t ts : timestamps) {
        MIDIMessage msg = parseMIDIMessage(0x90, 60, 100, ts);
        EXPECT_EQ(msg.timestamp, ts);
    }
}
