#!/bin/bash

# Test script for sample-accurate parameter updates

set -e  # Exit on error

echo "=========================================="
echo "Testing Sample-Accurate Parameter Updates"
echo "=========================================="

# Create build directory
BUILD_DIR="build_test_sample_accurate"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Compile ParameterEventQueue
echo ""
echo "Compiling ParameterEventQueue..."
g++ -std=c++17 -DTEST_BUILD \
    -I../src/audio_engine \
    -I/opt/homebrew/include \
    -c ../src/audio_engine/parameters/ParameterEventQueue.cpp \
    -o ParameterEventQueue.o

# Compile ParameterEventQueue tests
echo ""
echo "Compiling ParameterEventQueue tests..."
g++ -std=c++17 -DTEST_BUILD \
    -I../src/audio_engine \
    -I/opt/homebrew/include \
    -c ../tests/unit/parameters/ParameterEventQueueTest.cpp \
    -o ParameterEventQueueTest.o

# Link and run ParameterEventQueue tests
echo ""
echo "Linking ParameterEventQueue tests..."
g++ -std=c++17 \
    ParameterEventQueueTest.o \
    ParameterEventQueue.o \
    -L/opt/homebrew/lib \
    -lgtest -lgtest_main -pthread \
    -o test_parameter_event_queue

echo ""
echo "Running ParameterEventQueue tests..."
./test_parameter_event_queue

# Compile all dependencies for AudioEngine tests
echo ""
echo "Compiling dependencies for AudioEngine tests..."

# Generators
g++ -std=c++17 -DTEST_BUILD -I../src/audio_engine -c ../src/audio_engine/generators/SineDriver.cpp -o SineDriver.o
g++ -std=c++17 -DTEST_BUILD -I../src/audio_engine -c ../src/audio_engine/generators/HarmonicMembrane.cpp -o HarmonicMembrane.o
g++ -std=c++17 -DTEST_BUILD -I../src/audio_engine -c ../src/audio_engine/generators/NoiseGenerator.cpp -o NoiseGenerator.o

# Modulation
g++ -std=c++17 -DTEST_BUILD -I../src/audio_engine -c ../src/audio_engine/modulation/RingModulator.cpp -o RingModulator.o

# Envelopes
g++ -std=c++17 -DTEST_BUILD -I../src/audio_engine -c ../src/audio_engine/envelopes/DualPhaseEnvelope.cpp -o DualPhaseEnvelope.o
g++ -std=c++17 -DTEST_BUILD -I../src/audio_engine -c ../src/audio_engine/envelopes/PitchEnvelope.cpp -o PitchEnvelope.o

# Voice
g++ -std=c++17 -DTEST_BUILD -I../src/audio_engine -c ../src/audio_engine/voice/Voice.cpp -o Voice.o
g++ -std=c++17 -DTEST_BUILD -I../src/audio_engine -c ../src/audio_engine/voice/VoiceAllocator.cpp -o VoiceAllocator.o

# Effects
g++ -std=c++17 -DTEST_BUILD -I../src/audio_engine -c ../src/audio_engine/effects/Compressor.cpp -o Compressor.o
g++ -std=c++17 -DTEST_BUILD -I../src/audio_engine -c ../src/audio_engine/effects/Reverb.cpp -o Reverb.o
g++ -std=c++17 -DTEST_BUILD -I../src/audio_engine -c ../src/audio_engine/effects/EffectsChain.cpp -o EffectsChain.o

# Parameters
g++ -std=c++17 -DTEST_BUILD -I../src/audio_engine -c ../src/audio_engine/parameters/Parameter.cpp -o Parameter.o
g++ -std=c++17 -DTEST_BUILD -I../src/audio_engine -c ../src/audio_engine/parameters/ParameterManager.cpp -o ParameterManager.o

# Utils
g++ -std=c++17 -DTEST_BUILD -I../src/audio_engine -c ../src/audio_engine/utils/DSPUtils.cpp -o DSPUtils.o
g++ -std=c++17 -DTEST_BUILD -I../src/audio_engine -c ../src/audio_engine/utils/JSONSerializer.cpp -o JSONSerializer.o

# AudioEngine
g++ -std=c++17 -DTEST_BUILD -I../src/audio_engine -c ../src/audio_engine/core/AudioEngine.cpp -o AudioEngine.o

# Compile sample-accurate parameter tests
echo ""
echo "Compiling sample-accurate parameter tests..."
g++ -std=c++17 -DTEST_BUILD \
    -I../src/audio_engine \
    -I/opt/homebrew/include \
    -c ../tests/unit/core/SampleAccurateParameterTest.cpp \
    -o SampleAccurateParameterTest.o

# Link and run sample-accurate parameter tests
echo ""
echo "Linking sample-accurate parameter tests..."
g++ -std=c++17 \
    SampleAccurateParameterTest.o \
    AudioEngine.o \
    VoiceAllocator.o \
    Voice.o \
    EffectsChain.o \
    Compressor.o \
    Reverb.o \
    ParameterManager.o \
    Parameter.o \
    ParameterEventQueue.o \
    DualPhaseEnvelope.o \
    PitchEnvelope.o \
    SineDriver.o \
    HarmonicMembrane.o \
    NoiseGenerator.o \
    RingModulator.o \
    DSPUtils.o \
    JSONSerializer.o \
    -L/opt/homebrew/lib \
    -lgtest -lgtest_main -pthread \
    -o test_sample_accurate_params

echo ""
echo "Running sample-accurate parameter tests..."
./test_sample_accurate_params

echo ""
echo "=========================================="
echo "All tests passed!"
echo "=========================================="

cd ..
