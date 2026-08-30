# VST3 Plugin
if(NOT EXISTS ${VST3_SDK_PATH})
    message(WARNING "VST3 SDK not found at ${VST3_SDK_PATH}. VST3 plugin will not be built.")
    message(WARNING "Please download VST3 SDK from https://github.com/steinbergmedia/vst3sdk")
    message(WARNING "and set VST3_SDK_PATH to the SDK location.")
    return()
endif()

# The bundled Steinberg SDK defaults plugin targets to a universal binary when
# using Xcode, while project libraries remain native unless the whole build is
# explicitly configured as universal. Keep all targets on the same native
# architecture by default. Callers can override this cache option together
# with CMAKE_OSX_ARCHITECTURES for a universal release build.
set(SMTG_BUILD_UNIVERSAL_BINARY OFF CACHE BOOL "Build universal VST3 binaries")

# Add VST3 SDK
add_subdirectory(${VST3_SDK_PATH} ${CMAKE_BINARY_DIR}/vst3sdk)

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
