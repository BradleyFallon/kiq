#include <gtest/gtest.h>
#include "audio_engine/include/AudioEngine.h"
#include "audio_engine/parameters/ParameterManager.h"
#include "audio_engine/effects/EffectsChain.h"
#include "audio_engine/voice/VoiceAllocator.h"
#include <vector>
#include <cmath>
#include <limits>

using namespace KickDrum;

class AudioEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine = std::make_unique<AudioEngine>();
        engine->initialize(48000.0f);
    }

    void TearDown() override {
        engine.reset();
    }

    std::unique_ptr<AudioEngine> engine;
};

// ============================================================================
// Master Level Control Tests
// ============================================================================

TEST_F(AudioEngineTest, MasterLevelDefaultValue) {
    // Default master level should be 0.8
    EXPECT_FLOAT_EQ(engine->getMasterLevel(), 0.8f);
}

TEST_F(AudioEngineTest, SetMasterLevelUpdatesValue) {
    engine->setMasterLevel(0.5f);
    EXPECT_FLOAT_EQ(engine->getMasterLevel(), 0.5f);
    
    engine->setMasterLevel(1.0f);
    EXPECT_FLOAT_EQ(engine->getMasterLevel(), 1.0f);
    
    engine->setMasterLevel(0.0f);
    EXPECT_FLOAT_EQ(engine->getMasterLevel(), 0.0f);
}

TEST_F(AudioEngineTest, MasterLevelClampsToValidRange) {
    // Test clamping above 1.0
    engine->setMasterLevel(2.0f);
    EXPECT_FLOAT_EQ(engine->getMasterLevel(), 1.0f);
    
    // Test clamping below 0.0
    engine->setMasterLevel(-0.5f);
    EXPECT_FLOAT_EQ(engine->getMasterLevel(), 0.0f);
}

TEST_F(AudioEngineTest, MasterLevelAffectsOutput) {
    // Trigger a note to generate audio
    engine->noteOn(60, 0.8f);
    
    // Process with full master level
    std::vector<float> buffer1(512, 0.0f);
    engine->setMasterLevel(1.0f);
    engine->processBlock(buffer1.data(), 512, 1);
    
    // Calculate RMS of output
    float rms1 = 0.0f;
    for (float sample : buffer1) {
        rms1 += sample * sample;
    }
    rms1 = std::sqrt(rms1 / buffer1.size());
    
    // Reset and process with half master level
    engine->allNotesOff();
    engine->noteOn(60, 0.8f);
    
    std::vector<float> buffer2(512, 0.0f);
    engine->setMasterLevel(0.5f);
    engine->processBlock(buffer2.data(), 512, 1);
    
    // Calculate RMS of output
    float rms2 = 0.0f;
    for (float sample : buffer2) {
        rms2 += sample * sample;
    }
    rms2 = std::sqrt(rms2 / buffer2.size());
    
    // RMS with 0.5 level should be approximately half of RMS with 1.0 level
    // (allowing for some tolerance due to envelope differences)
    if (rms1 > 0.0f) {
        EXPECT_LT(rms2, rms1);
        EXPECT_NEAR(rms2 / rms1, 0.5f, 0.2f);
    }
}

TEST_F(AudioEngineTest, MasterLevelZeroSilencesOutput) {
    // Trigger a note
    engine->noteOn(60, 0.8f);
    
    // Set master level to zero
    engine->setMasterLevel(0.0f);
    
    // Process audio
    std::vector<float> buffer(512, 0.0f);
    engine->processBlock(buffer.data(), 512, 1);
    
    // All samples should be zero
    for (float sample : buffer) {
        EXPECT_FLOAT_EQ(sample, 0.0f);
    }
}

// ============================================================================
// Soft Clipping Tests
// ============================================================================

TEST_F(AudioEngineTest, SoftClippingEnabledByDefault) {
    EXPECT_TRUE(engine->isSoftClippingEnabled());
}

TEST_F(AudioEngineTest, SetSoftClippingEnabled) {
    engine->setSoftClippingEnabled(false);
    EXPECT_FALSE(engine->isSoftClippingEnabled());
    
    engine->setSoftClippingEnabled(true);
    EXPECT_TRUE(engine->isSoftClippingEnabled());
}

TEST_F(AudioEngineTest, SoftClippingLimitsOutput) {
    // Enable soft clipping
    engine->setSoftClippingEnabled(true);
    
    // Set master level very high to potentially cause clipping
    engine->setMasterLevel(1.0f);
    
    // Trigger multiple notes to create loud output
    for (int note = 60; note < 68; ++note) {
        engine->noteOn(note, 1.0f);
    }
    
    // Process audio
    std::vector<float> buffer(1024, 0.0f);
    engine->processBlock(buffer.data(), 1024, 1);
    
    // All samples should be within [-1.0, 1.0]
    for (size_t i = 0; i < buffer.size(); ++i) {
        EXPECT_GE(buffer[i], -1.0f) << "Sample " << i << " below -1.0";
        EXPECT_LE(buffer[i], 1.0f) << "Sample " << i << " above 1.0";
    }
}

TEST_F(AudioEngineTest, OutputAlwaysFiniteWithSoftClipping) {
    // Enable soft clipping
    engine->setSoftClippingEnabled(true);
    
    // Trigger notes
    engine->noteOn(60, 1.0f);
    
    // Process multiple blocks
    for (int block = 0; block < 10; ++block) {
        std::vector<float> buffer(512, 0.0f);
        engine->processBlock(buffer.data(), 512, 1);
        
        // All samples should be finite
        for (size_t i = 0; i < buffer.size(); ++i) {
            EXPECT_TRUE(std::isfinite(buffer[i])) 
                << "Non-finite sample at block " << block << ", index " << i;
        }
    }
}

// ============================================================================
// NaN/Infinity Detection Tests
// ============================================================================

TEST_F(AudioEngineTest, NaNDetectionEnabledByDefault) {
    EXPECT_TRUE(engine->isNaNDetectionEnabled());
}

TEST_F(AudioEngineTest, SetNaNDetectionEnabled) {
    engine->setNaNDetectionEnabled(false);
    EXPECT_FALSE(engine->isNaNDetectionEnabled());
    
    engine->setNaNDetectionEnabled(true);
    EXPECT_TRUE(engine->isNaNDetectionEnabled());
}

TEST_F(AudioEngineTest, NormalOperationProducesValidOutput) {
    // Enable NaN detection
    engine->setNaNDetectionEnabled(true);
    
    // Trigger a note
    engine->noteOn(60, 0.8f);
    
    // Process multiple blocks
    for (int block = 0; block < 10; ++block) {
        std::vector<float> buffer(512, 0.0f);
        engine->processBlock(buffer.data(), 512, 1);
        
        // All samples should be valid (not NaN or infinity)
        for (size_t i = 0; i < buffer.size(); ++i) {
            EXPECT_FALSE(std::isnan(buffer[i])) 
                << "NaN at block " << block << ", index " << i;
            EXPECT_FALSE(std::isinf(buffer[i])) 
                << "Infinity at block " << block << ", index " << i;
        }
    }
}

// ============================================================================
// Multi-Channel Output Tests
// ============================================================================

TEST_F(AudioEngineTest, MonoOutputWorks) {
    engine->noteOn(60, 0.8f);
    
    std::vector<float> buffer(512, 0.0f);
    engine->processBlock(buffer.data(), 512, 1);
    
    // Should produce non-zero output
    bool hasNonZero = false;
    for (float sample : buffer) {
        if (sample != 0.0f) {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero);
}

TEST_F(AudioEngineTest, StereoOutputDuplicatesMono) {
    engine->noteOn(60, 0.8f);
    
    std::vector<float> buffer(512 * 2, 0.0f);  // Stereo interleaved
    engine->processBlock(buffer.data(), 512, 2);
    
    // Left and right channels should be identical
    for (size_t i = 0; i < 512; ++i) {
        float left = buffer[i * 2];
        float right = buffer[i * 2 + 1];
        EXPECT_FLOAT_EQ(left, right) << "Mismatch at sample " << i;
    }
}

TEST_F(AudioEngineTest, MultiChannelOutputWorks) {
    engine->noteOn(60, 0.8f);
    
    const size_t numChannels = 4;
    std::vector<float> buffer(512 * numChannels, 0.0f);
    engine->processBlock(buffer.data(), 512, numChannels);
    
    // All channels should be identical
    for (size_t i = 0; i < 512; ++i) {
        float firstChannel = buffer[i * numChannels];
        for (size_t ch = 1; ch < numChannels; ++ch) {
            EXPECT_FLOAT_EQ(buffer[i * numChannels + ch], firstChannel)
                << "Mismatch at sample " << i << ", channel " << ch;
        }
    }
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(AudioEngineTest, ProcessBlockHandlesNullBuffer) {
    // Should not crash with null buffer
    engine->processBlock(nullptr, 512, 1);
}

TEST_F(AudioEngineTest, ProcessBlockHandlesZeroSamples) {
    std::vector<float> buffer(512, 0.0f);
    // Should not crash with zero samples
    engine->processBlock(buffer.data(), 0, 1);
}

TEST_F(AudioEngineTest, ProcessBlockHandlesZeroChannels) {
    std::vector<float> buffer(512, 0.0f);
    // Should not crash with zero channels
    engine->processBlock(buffer.data(), 512, 0);
}

TEST_F(AudioEngineTest, NoteOnAndOffWork) {
    // Trigger a note
    engine->noteOn(60, 0.8f);
    
    // Process some audio
    std::vector<float> buffer1(512, 0.0f);
    engine->processBlock(buffer1.data(), 512, 1);
    
    // Should have non-zero output
    bool hasNonZero = false;
    for (float sample : buffer1) {
        if (sample != 0.0f) {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero);
    
    // Release the note
    engine->noteOff(60);
    
    // Process more audio (envelope should decay)
    std::vector<float> buffer2(512, 0.0f);
    engine->processBlock(buffer2.data(), 512, 1);
}

TEST_F(AudioEngineTest, AllNotesOffSilencesOutput) {
    // Trigger multiple notes
    for (int note = 60; note < 68; ++note) {
        engine->noteOn(note, 0.8f);
    }
    
    // Release all notes
    engine->allNotesOff();
    
    // Process enough audio for envelopes to complete
    for (int i = 0; i < 100; ++i) {
        std::vector<float> buffer(512, 0.0f);
        engine->processBlock(buffer.data(), 512, 1);
    }
    
    // Final buffer should be silent (or very quiet)
    std::vector<float> finalBuffer(512, 0.0f);
    engine->processBlock(finalBuffer.data(), 512, 1);
    
    float maxAbs = 0.0f;
    for (float sample : finalBuffer) {
        maxAbs = std::max(maxAbs, std::abs(sample));
    }
    EXPECT_LT(maxAbs, 0.01f);  // Should be very quiet
}

TEST_F(AudioEngineTest, GettersReturnValidPointers) {
    EXPECT_NE(engine->getEffectsChain(), nullptr);
    EXPECT_NE(engine->getVoiceAllocator(), nullptr);
}

TEST_F(AudioEngineTest, SampleRateIsCorrect) {
    EXPECT_FLOAT_EQ(engine->getSampleRate(), 48000.0f);
}

// ============================================================================
// Combined Feature Tests
// ============================================================================

TEST_F(AudioEngineTest, MasterLevelAndSoftClippingWorkTogether) {
    // Set high master level
    engine->setMasterLevel(1.0f);
    engine->setSoftClippingEnabled(true);
    
    // Trigger multiple notes
    for (int note = 60; note < 68; ++note) {
        engine->noteOn(note, 1.0f);
    }
    
    // Process audio
    std::vector<float> buffer(1024, 0.0f);
    engine->processBlock(buffer.data(), 1024, 1);
    
    // Output should be limited to [-1.0, 1.0]
    for (float sample : buffer) {
        EXPECT_GE(sample, -1.0f);
        EXPECT_LE(sample, 1.0f);
        EXPECT_TRUE(std::isfinite(sample));
    }
}

TEST_F(AudioEngineTest, AllSafetyFeaturesWorkTogether) {
    // Enable all safety features
    engine->setMasterLevel(0.8f);
    engine->setSoftClippingEnabled(true);
    engine->setNaNDetectionEnabled(true);
    
    // Trigger notes and process audio
    engine->noteOn(60, 1.0f);
    
    for (int block = 0; block < 20; ++block) {
        std::vector<float> buffer(512, 0.0f);
        engine->processBlock(buffer.data(), 512, 1);
        
        // Verify all safety constraints
        for (size_t i = 0; i < buffer.size(); ++i) {
            EXPECT_TRUE(std::isfinite(buffer[i]));
            EXPECT_GE(buffer[i], -1.0f);
            EXPECT_LE(buffer[i], 1.0f);
        }
    }
}

// ============================================================================
// Parameter Manager Integration Tests
// ============================================================================

TEST_F(AudioEngineTest, ParameterManagerIsInitialized) {
    EXPECT_NE(engine->getParameterManager(), nullptr);
}

TEST_F(AudioEngineTest, ParameterManagerHasAllParameters) {
    ParameterManager* pm = engine->getParameterManager();
    ASSERT_NE(pm, nullptr);
    
    // Check that key parameters exist
    EXPECT_TRUE(pm->hasParameter("basePitch"));
    EXPECT_TRUE(pm->hasParameter("sineLevel"));
    EXPECT_TRUE(pm->hasParameter("harmonicRatio"));
    EXPECT_TRUE(pm->hasParameter("harmonicLevel"));
    EXPECT_TRUE(pm->hasParameter("noiseLevel"));
    EXPECT_TRUE(pm->hasParameter("attack"));
    EXPECT_TRUE(pm->hasParameter("decay"));
    EXPECT_TRUE(pm->hasParameter("sustain"));
    EXPECT_TRUE(pm->hasParameter("release"));
    EXPECT_TRUE(pm->hasParameter("compressorThreshold"));
    EXPECT_TRUE(pm->hasParameter("reverbRoomSize"));
    EXPECT_TRUE(pm->hasParameter("masterLevel"));
}

TEST_F(AudioEngineTest, ParameterManagerCanSetAndGetValues) {
    ParameterManager* pm = engine->getParameterManager();
    ASSERT_NE(pm, nullptr);
    
    // Set and get a parameter value
    EXPECT_TRUE(pm->setParameterValue("basePitch", 60.0f));
    EXPECT_FLOAT_EQ(pm->getParameterValue("basePitch"), 60.0f);
    
    // Set and get another parameter
    EXPECT_TRUE(pm->setParameterValue("attack", 5.0f));
    EXPECT_FLOAT_EQ(pm->getParameterValue("attack"), 5.0f);
}

// ============================================================================
// Full Integration Tests
// ============================================================================

TEST_F(AudioEngineTest, FullAudioPipelineIntegration) {
    // This test verifies the complete audio processing pipeline:
    // Voice Allocator -> Effects Chain -> Master Level -> Soft Clipping
    
    ParameterManager* pm = engine->getParameterManager();
    ASSERT_NE(pm, nullptr);
    
    // Configure synthesis parameters
    pm->setParameterValue("basePitch", 50.0f);
    pm->setParameterValue("sineLevel", 80.0f);
    pm->setParameterValue("attack", 1.0f);
    pm->setParameterValue("decay", 500.0f);
    
    // Configure effects
    pm->setParameterValue("compressorThreshold", -12.0f);
    pm->setParameterValue("compressorRatio", 4.0f);
    pm->setParameterValue("reverbMix", 10.0f);
    
    // Set master level
    engine->setMasterLevel(0.8f);
    
    // Trigger a note
    engine->noteOn(60, 0.8f);
    
    // Process audio blocks
    std::vector<float> buffer(512, 0.0f);
    for (int block = 0; block < 10; ++block) {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        engine->processBlock(buffer.data(), 512, 1);
        
        // Verify output is valid
        for (size_t i = 0; i < buffer.size(); ++i) {
            EXPECT_TRUE(std::isfinite(buffer[i]));
            EXPECT_GE(buffer[i], -1.0f);
            EXPECT_LE(buffer[i], 1.0f);
        }
    }
}

TEST_F(AudioEngineTest, SampleRateConfigurationWorks) {
    // Test that sample rate can be configured
    auto engine2 = std::make_unique<AudioEngine>();
    
    // Initialize with different sample rates
    engine2->initialize(44100.0f);
    EXPECT_FLOAT_EQ(engine2->getSampleRate(), 44100.0f);
    
    engine2->initialize(96000.0f);
    EXPECT_FLOAT_EQ(engine2->getSampleRate(), 96000.0f);
    
    // Verify audio processing still works
    engine2->noteOn(60, 0.8f);
    std::vector<float> buffer(512, 0.0f);
    engine2->processBlock(buffer.data(), 512, 1);
    
    // Should produce non-zero output
    bool hasNonZero = false;
    for (float sample : buffer) {
        if (sample != 0.0f) {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero);
}

TEST_F(AudioEngineTest, EffectsChainIntegration) {
    // Verify effects chain is accessible and functional
    EffectsChain* effects = engine->getEffectsChain();
    ASSERT_NE(effects, nullptr);
    
    // Configure effects
    effects->getCompressor().setThreshold(-20.0f);
    effects->getCompressor().setRatio(4.0f);
    effects->getReverb().setRoomSize(0.5f);
    
    // Trigger a note and process
    engine->noteOn(60, 1.0f);
    std::vector<float> buffer(512, 0.0f);
    engine->processBlock(buffer.data(), 512, 1);
    
    // Output should be valid
    for (float sample : buffer) {
        EXPECT_TRUE(std::isfinite(sample));
    }
}

TEST_F(AudioEngineTest, VoiceAllocatorIntegration) {
    // Verify voice allocator is accessible and functional
    VoiceAllocator* voices = engine->getVoiceAllocator();
    ASSERT_NE(voices, nullptr);
    
    // Should have 8 voices
    EXPECT_EQ(voices->getNumVoices(), 8);
    
    // Initially no active voices
    EXPECT_EQ(voices->getNumActiveVoices(), 0);
    
    // Trigger some notes
    engine->noteOn(60, 0.8f);
    engine->noteOn(62, 0.8f);
    engine->noteOn(64, 0.8f);
    
    // Should have 3 active voices
    EXPECT_EQ(voices->getNumActiveVoices(), 3);
    
    // Release all
    engine->allNotesOff();
    
    // Process enough audio for envelopes to complete
    for (int i = 0; i < 100; ++i) {
        std::vector<float> buffer(512, 0.0f);
        engine->processBlock(buffer.data(), 512, 1);
    }
    
    // Should have no active voices
    EXPECT_EQ(voices->getNumActiveVoices(), 0);
}
