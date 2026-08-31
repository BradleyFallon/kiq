# VST3 Plugin
if(NOT EXISTS ${VST3_SDK_PATH})
    message(WARNING "VST3 SDK not found at ${VST3_SDK_PATH}. VST3 plugin will not be built.")
    message(WARNING "Please download VST3 SDK from https://github.com/steinbergmedia/vst3sdk")
    message(WARNING "and set VST3_SDK_PATH to the SDK location.")
    return()
endif()

# Steinberg's plug-in helper resolves the platform entry point through this
# directory-scoped variable. The SDK sets it only inside its own subdirectory,
# so make it available where the Kiq target is declared.
set(public_sdk_SOURCE_DIR "${VST3_SDK_PATH}/public.sdk")

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

smtg_target_add_plugin_resources(${VST3_PLUGIN_NAME}
    RESOURCES
        resources/kiq.uidesc
)

target_include_directories(${VST3_PLUGIN_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src/audio_engine/include
        ${CMAKE_SOURCE_DIR}/src/audio_engine/parameters
)

target_link_libraries(${VST3_PLUGIN_NAME}
    PRIVATE
        kick_drum_ui
        sdk
        vstgui_support
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
