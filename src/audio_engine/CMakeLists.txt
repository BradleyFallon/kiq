# Audio Engine Library
add_library(kick_drum_audio_engine STATIC
    # Generators
    generators/SineDriver.cpp
    generators/HarmonicMembrane.cpp
    generators/NoiseGenerator.cpp
    
    # Modulation
    modulation/RingModulator.cpp
    modulation/GeneratorMixer.cpp
    
    # Envelopes
    envelopes/EnvelopeCurves.cpp
    envelopes/DualPhaseEnvelope.cpp
    envelopes/PitchEnvelope.cpp
    
    # Effects
    effects/Compressor.cpp
    effects/Reverb.cpp
    effects/EffectsChain.cpp
    
    # Voice Management
    voice/Voice.cpp
    voice/VoiceAllocator.cpp
    
    # Parameters
    parameters/Parameter.cpp
    parameters/ParameterManager.cpp
    parameters/ParameterEventQueue.cpp
    
    # Presets
    presets/Preset.cpp
    presets/PresetManager.cpp
    
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
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link math library
target_link_libraries(kick_drum_audio_engine
    PUBLIC
        Threads::Threads
    PRIVATE
        m
)

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
