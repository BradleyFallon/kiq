# C++ Unit Tests

# Test executable
add_executable(kick_drum_tests
    # Generator tests
    generators/SineDriverTest.cpp
    generators/MembraneModelTest.cpp
    generators/NoiseGeneratorTest.cpp
    generators/TransientGeneratorTest.cpp
    
    # Envelope tests
    envelopes/TrajectoryTest.cpp
    
    # Voice tests
    voice/VoiceTest.cpp
    voice/VoiceAllocatorTest.cpp
    
    # Parameter tests
    parameters/ParameterTest.cpp
    parameters/ParameterManagerTest.cpp
    parameters/ParameterEventQueueTest.cpp
    
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

# Platform-specific tests
if(APPLE)
    target_sources(kick_drum_tests PRIVATE
        platform/CoreAudioInterfaceTest.cpp
        platform/CoreMIDIInterfaceTest.cpp
    )
endif()

target_link_libraries(kick_drum_tests
    PRIVATE
        kick_drum_audio_engine
        GTest::gtest
        GTest::gtest_main
)

# Platform-specific libraries
if(APPLE)
    target_link_libraries(kick_drum_tests
        PRIVATE
            kick_drum_platform
    )
endif()

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
