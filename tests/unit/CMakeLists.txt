# C++ Unit Tests

# Test executable
add_executable(kick_drum_tests
    # Generator tests
    generators/SineDriverTest.cpp
    generators/HarmonicMembraneTest.cpp
    generators/NoiseGeneratorTest.cpp
    
    # Modulation tests
    modulation/RingModulatorTest.cpp
    modulation/MixerTest.cpp
    
    # Envelope tests
    envelopes/EnvelopeCurvesTest.cpp
    envelopes/DualPhaseEnvelopeTest.cpp
    envelopes/PitchEnvelopeTest.cpp
    
    # Voice tests
    voice/VoiceTest.cpp
    voice/VoiceAllocatorTest.cpp
    
    # Effects tests
    effects/CompressorTest.cpp
    effects/ReverbTest.cpp
    effects/EffectsChainTest.cpp
    
    # Parameter tests
    parameters/ParameterTest.cpp
    parameters/ParameterManagerTest.cpp
    
    # Preset tests
    presets/PresetTest.cpp
    presets/PresetManagerTest.cpp
    
    # MIDI tests
    midi/MIDIMessageTest.cpp
    midi/MIDIHandlerTest.cpp
    
    # Core tests
    core/AudioEngineTest.cpp
    
    # Utilities tests
    utils/DSPUtilsTest.cpp
    utils/JSONSerializerTest.cpp
    
    # Main test runner
    main.cpp
)

target_link_libraries(kick_drum_tests
    PRIVATE
        kick_drum_audio_engine
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(kick_drum_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)

# Discover tests
gtest_discover_tests(kick_drum_tests)

# Add custom target to run tests
add_custom_target(run_unit_tests
    COMMAND kick_drum_tests
    DEPENDS kick_drum_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running C++ unit tests..."
)
