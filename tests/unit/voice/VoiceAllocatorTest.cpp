#include <gtest/gtest.h>

#include "audio_engine/voice/VoiceAllocator.h"

#include <algorithm>
#include <array>

using namespace KickDrum;

TEST(VoiceAllocatorTest, CoversMaximumAuditionLoopOverlap) {
    VoiceAllocator allocator;
    allocator.initialize(48000.0f);
    EXPECT_EQ(allocator.getNumVoices(), 9);
    for (int hit = 0; hit < 9; ++hit) {
        EXPECT_NE(allocator.allocateVoice(36, 1.0f), nullptr);
    }
    EXPECT_EQ(allocator.getNumActiveVoices(), 9);
}

TEST(VoiceAllocatorTest, TenthHitStealsOldestVoice) {
    VoiceAllocator allocator;
    allocator.initialize(48000.0f);
    Voice* first = allocator.allocateVoice(36, 1.0f);
    std::array<float, 10> buffer {};
    allocator.renderBuffer(buffer.data(), static_cast<int>(buffer.size()));
    for (int hit = 1; hit < 9; ++hit) {
        allocator.allocateVoice(36 + hit, 1.0f);
    }
    EXPECT_EQ(allocator.allocateVoice(50, 1.0f), first);
}

TEST(VoiceAllocatorTest, NoteOffLeavesOneShotRunningAndPanicStopsIt) {
    VoiceAllocator allocator;
    allocator.initialize(48000.0f);
    allocator.allocateVoice(36, 1.0f);
    allocator.releaseVoice(36);
    EXPECT_EQ(allocator.getNumActiveVoices(), 1);
    allocator.releaseAll();
    EXPECT_EQ(allocator.getNumActiveVoices(), 0);
}

TEST(VoiceAllocatorTest, RenderClearsAndFillsBuffer) {
    VoiceAllocator allocator;
    allocator.initialize(48000.0f);
    std::array<float, 128> buffer;
    buffer.fill(99.0f);
    allocator.renderBuffer(buffer.data(), static_cast<int>(buffer.size()));
    EXPECT_TRUE(std::all_of(buffer.begin(), buffer.end(),
                            [](float sample) { return sample == 0.0f; }));

    allocator.allocateVoice(36, 1.0f);
    allocator.renderBuffer(buffer.data(), static_cast<int>(buffer.size()));
    EXPECT_TRUE(std::any_of(buffer.begin(), buffer.end(),
                            [](float sample) { return sample != 0.0f; }));
}

TEST(VoiceAllocatorTest, ActiveHitKeepsItsPhysicalSnapshot) {
    VoiceAllocator allocator;
    allocator.initialize(48000.0f);
    Voice* active = allocator.allocateVoice(36, 1.0f);
    ASSERT_NE(active, nullptr);

    KickParams next = kDefaultKickParams;
    next.pitch[0].value = 440.0f;
    next.transient.impactLevel = 0.0f;
    next.outputGain = 0.25f;
    allocator.setParams(next);

    EXPECT_FLOAT_EQ(active->getParams().pitch[0].value,
                    kDefaultKickParams.pitch[0].value);
    EXPECT_FLOAT_EQ(active->getParams().transient.impactLevel,
                    kDefaultKickParams.transient.impactLevel);
    // Output is intentionally the one live parameter; Voice smooths it.
    EXPECT_FLOAT_EQ(active->getParams().outputGain, 0.25f);

    Voice* following = allocator.allocateVoice(37, 1.0f);
    ASSERT_NE(following, nullptr);
    EXPECT_FLOAT_EQ(following->getParams().pitch[0].value, 440.0f);
    EXPECT_FLOAT_EQ(following->getParams().transient.impactLevel, 0.0f);
}
