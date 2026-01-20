# UI Library
add_library(kick_drum_ui STATIC
    # Waveform Display
    waveform/WaveformDisplay.cpp
    
    # Parameter Controls
    controls/ParameterControl.cpp
    controls/Knob.cpp
    controls/Slider.cpp
    
    # Visualizers
    visualizers/EnvelopeVisualizer.cpp
    
    # Preset Browser
    preset_browser/PresetBrowser.cpp
    
    # Main UI
    MainUI.cpp
)

target_include_directories(kick_drum_ui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(kick_drum_ui
    PUBLIC
        kick_drum_audio_engine
)

# Platform-specific UI frameworks
if(APPLE)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore)
    target_link_libraries(kick_drum_ui
        PRIVATE
            ${COCOA_FRAMEWORK}
            ${QUARTZCORE_FRAMEWORK}
    )
endif()
