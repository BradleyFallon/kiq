#!/bin/bash

# Simple test script for sample-accurate parameter updates

echo "=========================================="
echo "Testing Sample-Accurate Parameter Updates"
echo "=========================================="

# Create a temporary directory for compilation
TEMP_DIR=$(mktemp -d)
echo "Using temp directory: $TEMP_DIR"

# Copy all necessary source files with directory structure
echo "Copying source files..."

# Create directory structure
mkdir -p $TEMP_DIR/core
mkdir -p $TEMP_DIR/include
mkdir -p $TEMP_DIR/utils
mkdir -p $TEMP_DIR/voice
mkdir -p $TEMP_DIR/generators
mkdir -p $TEMP_DIR/modulation
mkdir -p $TEMP_DIR/envelopes
mkdir -p $TEMP_DIR/effects
mkdir -p $TEMP_DIR/parameters

# Core
cp src/audio_engine/core/AudioEngine.cpp $TEMP_DIR/core/
cp src/audio_engine/include/AudioEngine.h $TEMP_DIR/include/

# Utils
cp src/audio_engine/utils/DSPUtils.cpp $TEMP_DIR/utils/
cp src/audio_engine/utils/DSPUtils.h $TEMP_DIR/utils/
cp src/audio_engine/utils/JSONSerializer.cpp $TEMP_DIR/utils/
cp src/audio_engine/utils/JSONSerializer.h $TEMP_DIR/utils/

# Voice
cp src/audio_engine/voice/Voice.cpp $TEMP_DIR/voice/
cp src/audio_engine/voice/Voice.h $TEMP_DIR/voice/
cp src/audio_engine/voice/VoiceAllocator.cpp $TEMP_DIR/voice/
cp src/audio_engine/voice/VoiceAllocator.h $TEMP_DIR/voice/

# Generators
cp src/audio_engine/generators/SineDriver.cpp $TEMP_DIR/generators/
cp src/audio_engine/generators/SineDriver.h $TEMP_DIR/generators/
cp src/audio_engine/generators/HarmonicMembrane.cpp $TEMP_DIR/generators/
cp src/audio_engine/generators/HarmonicMembrane.h $TEMP_DIR/generators/
cp src/audio_engine/generators/NoiseGenerator.cpp $TEMP_DIR/generators/
cp src/audio_engine/generators/NoiseGenerator.h $TEMP_DIR/generators/

# Modulation
cp src/audio_engine/modulation/RingModulator.cpp $TEMP_DIR/modulation/
cp src/audio_engine/modulation/RingModulator.h $TEMP_DIR/modulation/
cp src/audio_engine/modulation/GeneratorMixer.cpp $TEMP_DIR/modulation/
cp src/audio_engine/modulation/GeneratorMixer.h $TEMP_DIR/modulation/

# Envelopes
cp src/audio_engine/envelopes/EnvelopeCurves.cpp $TEMP_DIR/envelopes/
cp src/audio_engine/envelopes/EnvelopeCurves.h $TEMP_DIR/envelopes/
cp src/audio_engine/envelopes/DualPhaseEnvelope.cpp $TEMP_DIR/envelopes/
cp src/audio_engine/envelopes/DualPhaseEnvelope.h $TEMP_DIR/envelopes/
cp src/audio_engine/envelopes/PitchEnvelope.cpp $TEMP_DIR/envelopes/
cp src/audio_engine/envelopes/PitchEnvelope.h $TEMP_DIR/envelopes/

# Effects
cp src/audio_engine/effects/Compressor.cpp $TEMP_DIR/effects/
cp src/audio_engine/effects/Compressor.h $TEMP_DIR/effects/
cp src/audio_engine/effects/Reverb.cpp $TEMP_DIR/effects/
cp src/audio_engine/effects/Reverb.h $TEMP_DIR/effects/
cp src/audio_engine/effects/EffectsChain.cpp $TEMP_DIR/effects/
cp src/audio_engine/effects/EffectsChain.h $TEMP_DIR/effects/

# Parameters
cp src/audio_engine/parameters/Parameter.cpp $TEMP_DIR/parameters/
cp src/audio_engine/parameters/Parameter.h $TEMP_DIR/parameters/
cp src/audio_engine/parameters/ParameterManager.cpp $TEMP_DIR/parameters/
cp src/audio_engine/parameters/ParameterManager.h $TEMP_DIR/parameters/
cp src/audio_engine/parameters/ParameterEvent.h $TEMP_DIR/parameters/
cp src/audio_engine/parameters/ParameterEventQueue.cpp $TEMP_DIR/parameters/
cp src/audio_engine/parameters/ParameterEventQueue.h $TEMP_DIR/parameters/

# Test file
cp tests/manual/test_sample_accurate_params.cpp $TEMP_DIR/

echo "Compiling..."
g++ -std=c++17 \
    -I$TEMP_DIR/include \
    -I$TEMP_DIR \
    -o $TEMP_DIR/test_sample_accurate \
    $TEMP_DIR/core/AudioEngine.cpp \
    $TEMP_DIR/utils/DSPUtils.cpp \
    $TEMP_DIR/utils/JSONSerializer.cpp \
    $TEMP_DIR/voice/Voice.cpp \
    $TEMP_DIR/voice/VoiceAllocator.cpp \
    $TEMP_DIR/generators/SineDriver.cpp \
    $TEMP_DIR/generators/HarmonicMembrane.cpp \
    $TEMP_DIR/generators/NoiseGenerator.cpp \
    $TEMP_DIR/modulation/RingModulator.cpp \
    $TEMP_DIR/modulation/GeneratorMixer.cpp \
    $TEMP_DIR/envelopes/EnvelopeCurves.cpp \
    $TEMP_DIR/envelopes/DualPhaseEnvelope.cpp \
    $TEMP_DIR/envelopes/PitchEnvelope.cpp \
    $TEMP_DIR/effects/Compressor.cpp \
    $TEMP_DIR/effects/Reverb.cpp \
    $TEMP_DIR/effects/EffectsChain.cpp \
    $TEMP_DIR/parameters/Parameter.cpp \
    $TEMP_DIR/parameters/ParameterManager.cpp \
    $TEMP_DIR/parameters/ParameterEventQueue.cpp \
    $TEMP_DIR/test_sample_accurate_params.cpp

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful"
    echo ""
    echo "Running tests..."
    echo ""
    $TEMP_DIR/test_sample_accurate
    TEST_RESULT=$?
    echo ""
    
    # Cleanup
    rm -rf $TEMP_DIR
    
    if [ $TEST_RESULT -eq 0 ]; then
        echo "=========================================="
        echo "All tests passed!"
        echo "=========================================="
        exit 0
    else
        echo "=========================================="
        echo "Tests failed"
        echo "=========================================="
        exit 1
    fi
else
    echo "✗ Compilation failed"
    rm -rf $TEMP_DIR
    exit 1
fi
