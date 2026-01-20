#!/bin/bash
set -e

echo "Compiling MIDI CC mapping test..."

# Get the workspace directory
WORKSPACE_DIR="$(pwd)"

# Compile the test
g++ -std=c++17 -I"${WORKSPACE_DIR}/tests" -I"${WORKSPACE_DIR}/src" \
    -DTEST_BUILD \
    "${WORKSPACE_DIR}/tests/unit/midi/MIDIHandlerTest.cpp" \
    "${WORKSPACE_DIR}/src/audio_engine/midi/MIDIHandler.cpp" \
    "${WORKSPACE_DIR}/src/audio_engine/midi/MIDIMessage.cpp" \
    "${WORKSPACE_DIR}/src/audio_engine/voice/VoiceAllocator.cpp" \
    "${WORKSPACE_DIR}/src/audio_engine/voice/Voice.cpp" \
    "${WORKSPACE_DIR}/src/audio_engine/generators/SineDriver.cpp" \
    "${WORKSPACE_DIR}/src/audio_engine/generators/HarmonicMembrane.cpp" \
    "${WORKSPACE_DIR}/src/audio_engine/generators/NoiseGenerator.cpp" \
    "${WORKSPACE_DIR}/src/audio_engine/modulation/RingModulator.cpp" \
    "${WORKSPACE_DIR}/src/audio_engine/envelopes/DualPhaseEnvelope.cpp" \
    "${WORKSPACE_DIR}/src/audio_engine/envelopes/PitchEnvelope.cpp" \
    "${WORKSPACE_DIR}/src/audio_engine/envelopes/EnvelopeCurves.cpp" \
    "${WORKSPACE_DIR}/src/audio_engine/parameters/Parameter.cpp" \
    "${WORKSPACE_DIR}/src/audio_engine/parameters/ParameterManager.cpp" \
    "${WORKSPACE_DIR}/src/audio_engine/utils/JSONSerializer.cpp" \
    -lgtest -lgtest_main -pthread \
    -o /tmp/midi_cc_test

echo "Compilation successful!"
echo "Running tests..."

# Run the tests
/tmp/midi_cc_test

echo "All tests passed!"
