#include <gtest/gtest.h>

#include "audio_engine/midi/MIDIHandler.h"

using namespace KickDrum;

class MIDIHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        allocator.initialize(48000.0f);
        parameters.registerAllSynthesisParameters();
    }

    VoiceAllocator allocator;
    ParameterManager parameters;
};

TEST_F(MIDIHandlerTest, NoteOnCreatesVelocityScaledOneShot) {
    MIDIHandler handler(&allocator, &parameters);
    handler.processMIDIMessage(MIDIMessage(MIDIMessageType::NOTE_ON, 0, 36, 64));
    ASSERT_EQ(allocator.getNumActiveVoices(), 1);
    EXPECT_EQ(allocator.getVoice(0).getNote(), 36);
}

TEST_F(MIDIHandlerTest, NoteOnCanBeRoutedToAnAudioThreadSink) {
    MIDIHandler handler(nullptr, &parameters);
    int receivedNote = -1;
    float receivedVelocity = 0.0f;
    handler.setNoteOnCallback([&](int note, float velocity) {
        receivedNote = note;
        receivedVelocity = velocity;
    });

    handler.handleNoteOn(42, 64);

    EXPECT_EQ(receivedNote, 42);
    EXPECT_NEAR(receivedVelocity, 64.0f / 127.0f, 1.0e-6f);
    EXPECT_EQ(allocator.getNumActiveVoices(), 0);
}

TEST_F(MIDIHandlerTest, NoteOffDoesNotTruncateKick) {
    MIDIHandler handler(&allocator, &parameters);
    handler.handleNoteOn(36, 127);
    handler.handleNoteOff(36);
    EXPECT_EQ(allocator.getNumActiveVoices(), 1);
}

TEST_F(MIDIHandlerTest, CCControlsTrajectoryParameter) {
    MIDIHandler handler(&allocator, &parameters);
    ASSERT_TRUE(handler.mapCCToParameter(74, "pitch0Hz"));
    handler.handleCC(74, 127);
    EXPECT_FLOAT_EQ(parameters.getParameterValue("pitch0Hz"), 1000.0f);
}

TEST_F(MIDIHandlerTest, CCLearnRejectsRemovedParameter) {
    MIDIHandler handler(&allocator, &parameters);
    EXPECT_FALSE(handler.enableCCLearn("basePitch"));
    EXPECT_TRUE(handler.enableCCLearn("airLevel"));
    handler.handleCC(7, 64);
    EXPECT_EQ(handler.getMappedParameter(7), "airLevel");
}

TEST_F(MIDIHandlerTest, PitchBendAppliesRatioToActiveBody) {
    MIDIHandler handler(&allocator, &parameters);
    handler.handleNoteOn(36, 127);
    handler.handlePitchBend(127, 127);
    EXPECT_GT(allocator.getVoice(0).getCurrentPitchHz(), 220.0f);
}
