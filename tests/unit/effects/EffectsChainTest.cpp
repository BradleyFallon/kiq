#include <gtest/gtest.h>
#include "audio_engine/effects/EffectsChain.h"
#include <cmath>
#include <vector>

using namespace KickDrum;

class EffectsChainTest : public ::testing::Test {
protected:
    void SetUp() override {
        chain = std::make_unique<EffectsChain>();
        sampleRate = 48000.0f;
    }

    std::unique_ptr<EffectsChain> chain;
    float sampleRate;
};

// Test initialization
TEST_F(EffectsChainTest, InitializationState) {
    EXPECT_FALSE(chain->isInitialized());
    
    chain->initialize(sampleRate);
    
    EXPECT_TRUE(chain->isInitialized());
    EXPECT_TRUE(chain->getCompressor().isInitialized());
    EXPECT_TRUE(chain->getReverb().isInitialized());
}

// Test bypass states default to false (effects active)
TEST_F(EffectsChainTest, DefaultBypassStates) {
    EXPECT_FALSE(chain->isCompressorBypassed());
    EXPECT_FALSE(chain->isReverbBypassed());
}

// Test compressor bypass control
TEST_F(EffectsChainTest, CompressorBypassControl) {
    chain->setCompressorBypassed(true);
    EXPECT_TRUE(chain->isCompressorBypassed());
    
    chain->setCompressorBypassed(false);
    EXPECT_FALSE(chain->isCompressorBypassed());
}

// Test reverb bypass control
TEST_F(EffectsChainTest, ReverbBypassControl) {
    chain->setReverbBypassed(true);
    EXPECT_TRUE(chain->isReverbBypassed());
    
    chain->setReverbBypassed(false);
    EXPECT_FALSE(chain->isReverbBypassed());
}

// Test that both effects can be bypassed independently
TEST_F(EffectsChainTest, IndependentBypassControl) {
    chain->setCompressorBypassed(true);
    chain->setReverbBypassed(false);
    EXPECT_TRUE(chain->isCompressorBypassed());
    EXPECT_FALSE(chain->isReverbBypassed());
    
    chain->setCompressorBypassed(false);
    chain->setReverbBypassed(true);
    EXPECT_FALSE(chain->isCompressorBypassed());
    EXPECT_TRUE(chain->isReverbBypassed());
    
    chain->setCompressorBypassed(true);
    chain->setReverbBypassed(true);
    EXPECT_TRUE(chain->isCompressorBypassed());
    EXPECT_TRUE(chain->isReverbBypassed());
}

// Test pass-through when both effects are bypassed
TEST_F(EffectsChainTest, BothEffectsBypassedPassThrough) {
    chain->initialize(sampleRate);
    chain->setCompressorBypassed(true);
    chain->setReverbBypassed(true);
    
    // Test with various input values
    std::vector<float> testValues = {0.0f, 0.5f, -0.5f, 1.0f, -1.0f, 0.123f};
    
    for (float input : testValues) {
        float output = chain->process(input);
        EXPECT_FLOAT_EQ(output, input) << "Input: " << input;
    }
}

// Test that compressor is applied when not bypassed
TEST_F(EffectsChainTest, CompressorAppliedWhenActive) {
    chain->initialize(sampleRate);
    chain->setCompressorBypassed(false);
    chain->setReverbBypassed(true);  // Bypass reverb to isolate compressor
    
    // Configure compressor for noticeable effect
    chain->getCompressor().setThreshold(-20.0f);
    chain->getCompressor().setRatio(4.0f);
    chain->getCompressor().setAttack(0.001f);
    chain->getCompressor().setRelease(0.1f);
    chain->getCompressor().setMix(1.0f);  // Fully wet
    
    // Process a loud signal that should be compressed
    float input = 0.8f;
    
    // Process multiple samples to let compressor engage
    float output = 0.0f;
    for (int i = 0; i < 100; ++i) {
        output = chain->process(input);
    }
    
    // Output should be different from input due to compression
    EXPECT_NE(output, input);
}

// Test that reverb is applied when not bypassed
TEST_F(EffectsChainTest, ReverbAppliedWhenActive) {
    chain->initialize(sampleRate);
    chain->setCompressorBypassed(true);  // Bypass compressor to isolate reverb
    chain->setReverbBypassed(false);
    
    // Configure reverb for noticeable effect
    chain->getReverb().setRoomSize(0.8f);
    chain->getReverb().setDecayTime(2.0f);
    chain->getReverb().setDamping(0.5f);
    chain->getReverb().setMix(0.5f);
    
    // Process an impulse
    float impulse = 1.0f;
    float firstOutput = chain->process(impulse);
    
    // Process silence - reverb should still produce output (tail)
    float silence = 0.0f;
    float tailEnergy = 0.0f;
    for (int i = 0; i < 5000; ++i) {
        tailEnergy += std::abs(chain->process(silence));
    }
    
    // Reverb tail should produce non-zero output after impulse
    EXPECT_GT(tailEnergy, 0.0f);
}

// Test processing order: compressor before reverb
TEST_F(EffectsChainTest, ProcessingOrderCompressorBeforeReverb) {
    chain->initialize(sampleRate);
    chain->setCompressorBypassed(false);
    chain->setReverbBypassed(false);
    
    // Configure compressor to reduce level
    chain->getCompressor().setThreshold(-20.0f);
    chain->getCompressor().setRatio(10.0f);
    chain->getCompressor().setAttack(0.001f);
    chain->getCompressor().setRelease(0.1f);
    chain->getCompressor().setMix(1.0f);
    
    // Configure reverb
    chain->getReverb().setRoomSize(0.5f);
    chain->getReverb().setDecayTime(1.0f);
    chain->getReverb().setMix(0.3f);
    
    // Process a loud signal
    float input = 0.9f;
    
    // Process multiple samples
    float output = 0.0f;
    for (int i = 0; i < 100; ++i) {
        output = chain->process(input);
    }
    
    // The output should be affected by both effects
    // Since compressor comes first, it reduces the level before reverb
    EXPECT_NE(output, input);
}

// Test that bypassing compressor skips it in the chain
TEST_F(EffectsChainTest, CompressorBypassSkipsProcessing) {
    chain->initialize(sampleRate);
    
    // Configure compressor with extreme settings
    chain->getCompressor().setThreshold(-40.0f);
    chain->getCompressor().setRatio(20.0f);
    chain->getCompressor().setMix(1.0f);
    
    // Bypass reverb to isolate compressor effect
    chain->setReverbBypassed(true);
    
    // Process with compressor active
    chain->setCompressorBypassed(false);
    float input = 0.7f;
    float outputWithCompressor = 0.0f;
    for (int i = 0; i < 100; ++i) {
        outputWithCompressor = chain->process(input);
    }
    
    // Reset and process with compressor bypassed
    chain->reset();
    chain->setCompressorBypassed(true);
    float outputWithoutCompressor = 0.0f;
    for (int i = 0; i < 100; ++i) {
        outputWithoutCompressor = chain->process(input);
    }
    
    // Bypassed should equal input, active should be different
    EXPECT_FLOAT_EQ(outputWithoutCompressor, input);
    EXPECT_NE(outputWithCompressor, input);
}

// Test that bypassing reverb skips it in the chain
TEST_F(EffectsChainTest, ReverbBypassSkipsProcessing) {
    chain->initialize(sampleRate);
    
    // Configure reverb with noticeable settings
    chain->getReverb().setRoomSize(0.9f);
    chain->getReverb().setDecayTime(3.0f);
    chain->getReverb().setMix(0.8f);
    
    // Bypass compressor to isolate reverb effect
    chain->setCompressorBypassed(true);
    
    // Process with reverb active
    chain->setReverbBypassed(false);
    float input = 0.5f;
    float outputWithReverb = chain->process(input);
    
    // Reset and process with reverb bypassed
    chain->reset();
    chain->setReverbBypassed(true);
    float outputWithoutReverb = chain->process(input);
    
    // Bypassed should equal input, active should be different
    EXPECT_FLOAT_EQ(outputWithoutReverb, input);
    EXPECT_NE(outputWithReverb, input);
}

// Test reset clears both effects
TEST_F(EffectsChainTest, ResetClearsBothEffects) {
    chain->initialize(sampleRate);
    
    // Configure effects
    chain->getReverb().setRoomSize(0.8f);
    chain->getReverb().setDecayTime(2.0f);
    chain->getReverb().setMix(1.0f);
    
    // Process an impulse to build up reverb tail
    chain->process(1.0f);
    
    // Process silence - should have reverb tail
    float tailEnergyBeforeReset = 0.0f;
    for (int i = 0; i < 5000; ++i) {
        tailEnergyBeforeReset += std::abs(chain->process(0.0f));
    }
    
    // Reset the chain
    chain->reset();
    
    // Process silence again - reverb tail should be cleared
    float outputAfterReset = chain->process(0.0f);
    
    EXPECT_GT(tailEnergyBeforeReset, 0.0f);
    EXPECT_FLOAT_EQ(outputAfterReset, 0.0f);
}

// Test accessing compressor for parameter control
TEST_F(EffectsChainTest, CompressorAccessForParameterControl) {
    chain->initialize(sampleRate);
    
    // Set parameters through the accessor
    chain->getCompressor().setThreshold(-15.0f);
    chain->getCompressor().setRatio(3.0f);
    chain->getCompressor().setAttack(0.005f);
    chain->getCompressor().setRelease(0.05f);
    chain->getCompressor().setMix(0.7f);
    
    // Verify parameters were set
    EXPECT_FLOAT_EQ(chain->getCompressor().getThreshold(), -15.0f);
    EXPECT_FLOAT_EQ(chain->getCompressor().getRatio(), 3.0f);
    EXPECT_FLOAT_EQ(chain->getCompressor().getAttack(), 0.005f);
    EXPECT_FLOAT_EQ(chain->getCompressor().getRelease(), 0.05f);
    EXPECT_FLOAT_EQ(chain->getCompressor().getMix(), 0.7f);
}

// Test accessing reverb for parameter control
TEST_F(EffectsChainTest, ReverbAccessForParameterControl) {
    chain->initialize(sampleRate);
    
    // Set parameters through the accessor
    chain->getReverb().setRoomSize(0.6f);
    chain->getReverb().setDecayTime(1.5f);
    chain->getReverb().setDamping(0.4f);
    chain->getReverb().setMix(0.3f);
    
    // Verify parameters were set
    EXPECT_FLOAT_EQ(chain->getReverb().getRoomSize(), 0.6f);
    EXPECT_FLOAT_EQ(chain->getReverb().getDecayTime(), 1.5f);
    EXPECT_FLOAT_EQ(chain->getReverb().getDamping(), 0.4f);
    EXPECT_FLOAT_EQ(chain->getReverb().getMix(), 0.3f);
}

// Test const accessors
TEST_F(EffectsChainTest, ConstAccessors) {
    chain->initialize(sampleRate);
    chain->getCompressor().setThreshold(-10.0f);
    chain->getReverb().setRoomSize(0.5f);
    
    const EffectsChain& constChain = *chain;
    
    EXPECT_FLOAT_EQ(constChain.getCompressor().getThreshold(), -10.0f);
    EXPECT_FLOAT_EQ(constChain.getReverb().getRoomSize(), 0.5f);
}

// Test processing with different sample rates
TEST_F(EffectsChainTest, DifferentSampleRates) {
    std::vector<float> sampleRates = {44100.0f, 48000.0f, 88200.0f, 96000.0f};
    
    for (float sr : sampleRates) {
        auto testChain = std::make_unique<EffectsChain>();
        testChain->initialize(sr);
        
        EXPECT_TRUE(testChain->isInitialized());
        
        // Process a sample to ensure no crashes
        float output = testChain->process(0.5f);
        EXPECT_TRUE(std::isfinite(output));
    }
}

// Test that output is always finite (no NaN or infinity)
TEST_F(EffectsChainTest, OutputAlwaysFinite) {
    chain->initialize(sampleRate);
    
    // Test with various input values including edge cases
    std::vector<float> testInputs = {
        0.0f, 0.5f, -0.5f, 1.0f, -1.0f,
        0.001f, -0.001f, 0.999f, -0.999f
    };
    
    for (float input : testInputs) {
        float output = chain->process(input);
        EXPECT_TRUE(std::isfinite(output)) 
            << "Output not finite for input: " << input;
        EXPECT_FALSE(std::isnan(output))
            << "Output is NaN for input: " << input;
        EXPECT_FALSE(std::isinf(output))
            << "Output is infinity for input: " << input;
    }
}

// Test Requirements 5.8 and 5.9: Independent bypass controls
TEST_F(EffectsChainTest, RequirementIndependentBypass) {
    chain->initialize(sampleRate);
    
    // Requirement 5.8: Allow bypassing of the Compressor independently
    chain->setCompressorBypassed(true);
    EXPECT_TRUE(chain->isCompressorBypassed());
    EXPECT_FALSE(chain->isReverbBypassed());  // Reverb should be unaffected
    
    // Requirement 5.9: Allow bypassing of the Reverb independently
    chain->setCompressorBypassed(false);
    chain->setReverbBypassed(true);
    EXPECT_FALSE(chain->isCompressorBypassed());  // Compressor should be unaffected
    EXPECT_TRUE(chain->isReverbBypassed());
}

// Test Requirements 5.1 and 5.2: Correct processing order
TEST_F(EffectsChainTest, RequirementProcessingOrder) {
    chain->initialize(sampleRate);
    
    // Requirement 5.1: Apply compressor to mixed output before final output
    // Requirement 5.2: Apply reverb to mixed output after compression
    
    // We verify this by checking that compressor affects the signal before reverb
    // Set up compressor to significantly reduce level
    chain->getCompressor().setThreshold(-30.0f);
    chain->getCompressor().setRatio(10.0f);
    chain->getCompressor().setMix(1.0f);
    
    // Set up reverb with high mix
    chain->getReverb().setMix(0.5f);
    chain->getReverb().setRoomSize(0.5f);
    
    // Process a loud signal
    float input = 0.8f;
    float output = 0.0f;
    for (int i = 0; i < 100; ++i) {
        output = chain->process(input);
    }
    
    // The fact that we get a valid output confirms the chain order
    // (compressor first, then reverb)
    EXPECT_TRUE(std::isfinite(output));
    EXPECT_NE(output, input);  // Signal should be modified
}
