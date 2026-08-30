#include <gtest/gtest.h>

#include "audio_engine/voice/VoiceAllocator.h"

#include <algorithm>
#include <array>

using namespace KickDrum;

TEST(VoiceAllocatorTest, ProvidesFourVoicesForOverlappingHits) {
    VoiceAllocator allocator;
    allocator.initialize(48000.0f);
    EXPECT_EQ(allocator.getNumVoices(), 4);
    for (int hit = 0; hit < 4; ++hit) {
        EXPECT_NE(allocator.allocateVoice(36, 1.0f), nullptr);
    }
    EXPECT_EQ(allocator.getNumActiveVoices(), 4);
}

TEST(VoiceAllocatorTest, FifthHitStealsOldestVoice) {
    VoiceAllocator allocator;
    allocator.initialize(48000.0f);
    Voice* first = allocator.allocateVoice(36, 1.0f);
    std::array<float, 10> buffer {};
    allocator.renderBuffer(buffer.data(), static_cast<int>(buffer.size()));
    for (int hit = 1; hit < 4; ++hit) {
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
