# VST3 Plugin
if(NOT EXISTS ${VST3_SDK_PATH})
    message(WARNING "VST3 SDK not found at ${VST3_SDK_PATH}. VST3 plugin will not be built.")
    message(WARNING "Please download VST3 SDK from https://github.com/steinbergmedia/vst3sdk")
    message(WARNING "and set VST3_SDK_PATH to the SDK location.")
    return()
endif()

# Add VST3 SDK
add_subdirectory(${VST3_SDK_PATH} ${CMAKE_BINARY_DIR}/vst3sdk)

# VST3 Plugin target
set(VST3_PLUGIN_NAME "KickDrumSynth")

smtg_add_vst3plugin(${VST3_PLUGIN_NAME}
    KickSynthVST3.cpp
    KickSynthController.cpp
    KickSynthController.h
    KickSynthProcessor.cpp
    KickSynthProcessor.h
    KickSynthCIDs.h
)

target_include_directories(${VST3_PLUGIN_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src/audio_engine/include
        ${CMAKE_SOURCE_DIR}/src/audio_engine/parameters
)

target_link_libraries(${VST3_PLUGIN_NAME}
    PRIVATE
        kick_drum_audio_engine
        kick_drum_ui
        sdk
)

# Set plugin properties
smtg_target_set_bundle(${VST3_PLUGIN_NAME}
    BUNDLE_IDENTIFIER "com.kickdrum.synthesizer"
    COMPANY_NAME "KickDrum"
)

# Installation
if(APPLE)
    smtg_target_set_bundle(${VST3_PLUGIN_NAME}
        BUNDLE_IDENTIFIER "com.kickdrum.synthesizer"
        COMPANY_NAME "KickDrum"
    )
    install(TARGETS ${VST3_PLUGIN_NAME}
        DESTINATION "$ENV{HOME}/Library/Audio/Plug-Ins/VST3"
    )
endif()
