# Audio Engine Library
add_library(kick_drum_audio_engine STATIC
    # Generators
    generators/SineDriver.cpp
    generators/MembraneModel.cpp
    generators/NoiseGenerator.cpp
    generators/TransientGenerator.cpp
    
    # Envelopes
    envelopes/Trajectory.cpp
    
    # Voice Management
    voice/Voice.cpp
    voice/VoiceAllocator.cpp
    
    # Parameters
    parameters/Parameter.cpp
    parameters/KickParams.cpp
    parameters/ParameterManager.cpp
    parameters/ParameterEventQueue.cpp
    
    # Presets
    presets/Preset.cpp
    presets/PresetManager.cpp
    presets/KickPresetIO.cpp

    # Reference analysis and editor workflow
    analysis/ReferenceAudio.cpp
    export/KickWavExporter.cpp
    workflow/KickParamsHistory.cpp
    
    # MIDI
    midi/MIDIMessage.cpp
    midi/MIDIHandler.cpp
    
    # Core Engine
    core/AudioEngine.cpp
    core/AudioBuffer.cpp
    
    # Utilities
    utils/DSPUtils.cpp
    utils/JSONSerializer.cpp
)

target_include_directories(kick_drum_audio_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis
        ${CMAKE_CURRENT_SOURCE_DIR}/export
        ${CMAKE_CURRENT_SOURCE_DIR}/presets
        ${CMAKE_CURRENT_SOURCE_DIR}/workflow
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(kick_drum_audio_engine
    PUBLIC
        Threads::Threads
)

# MSVC's C runtime exposes these functions without a separate libm target.
if(NOT MSVC)
    target_link_libraries(kick_drum_audio_engine PRIVATE m)
endif()

# Platform-specific settings
if(APPLE)
    target_compile_definitions(kick_drum_audio_engine PRIVATE PLATFORM_MACOS)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(kick_drum_audio_engine PRIVATE /W4)
else()
    target_compile_options(kick_drum_audio_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()
