# Platform Layer Library
add_library(kick_drum_platform STATIC
    # Audio Interface
    audio/AudioInterface.cpp
    
    # MIDI Interface
    midi/MIDIInterface.cpp
    
    # File System
    filesystem/FileSystem.cpp
)

target_include_directories(kick_drum_platform
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(kick_drum_platform
    PUBLIC
        kick_drum_audio_engine
)

# Platform-specific frameworks
if(APPLE)
    find_library(COREAUDIO_FRAMEWORK CoreAudio)
    find_library(COREMIDI_FRAMEWORK CoreMIDI)
    find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
    
    target_sources(kick_drum_platform PRIVATE
        audio/CoreAudioInterface.cpp
        midi/CoreMIDIInterface.cpp
    )
    
    target_link_libraries(kick_drum_platform
        PRIVATE
            ${COREAUDIO_FRAMEWORK}
            ${COREMIDI_FRAMEWORK}
            ${AUDIOTOOLBOX_FRAMEWORK}
            ${COREFOUNDATION_FRAMEWORK}
    )
    
    target_compile_definitions(kick_drum_platform PRIVATE PLATFORM_MACOS)
endif()
