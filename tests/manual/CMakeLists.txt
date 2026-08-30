# Manual Tests
# These are interactive tests that require user observation/interaction

# Test for sample-accurate parameter updates
add_executable(test_sample_accurate_params
    test_sample_accurate_params.cpp
)

target_link_libraries(test_sample_accurate_params
    PRIVATE
        kick_drum_audio_engine
)

target_include_directories(test_sample_accurate_params
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/audio_engine
)

# CoreAudio integration test (macOS only)
if(APPLE)
    add_executable(test_coreaudio_integration
        test_coreaudio_integration.cpp
    )
    
    target_link_libraries(test_coreaudio_integration
        PRIVATE
            kick_drum_audio_engine
            kick_drum_platform
    )
    
    target_include_directories(test_coreaudio_integration
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src/audio_engine/include
            ${CMAKE_SOURCE_DIR}/src/platform/audio
    )
    
    # CoreMIDI integration test (macOS only)
    add_executable(test_coremidi_integration
        test_coremidi_integration.cpp
    )
    
    target_link_libraries(test_coremidi_integration
        PRIVATE
            kick_drum_audio_engine
            kick_drum_platform
    )
    
    target_include_directories(test_coremidi_integration
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src/audio_engine/include
            ${CMAKE_SOURCE_DIR}/src/platform/midi
    )
endif()
