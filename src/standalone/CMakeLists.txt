# Graphical macOS standalone application. VSTGUI supplies the native entry
# point and event loop; the same KiqMainView is used by the VST3 editor.
vstgui_add_executable(KickDrumSynthStandalone
    "StandaloneUIApp.cpp;StandaloneUIBridge.cpp;StandaloneUIBridge.h"
)

target_include_directories(KickDrumSynthStandalone
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src/platform
        ${CMAKE_SOURCE_DIR}/src/audio_engine
        ${CMAKE_SOURCE_DIR}/src/audio_engine/parameters
        ${VST3_SDK_PATH}/vstgui4
)

target_link_libraries(KickDrumSynthStandalone
    PRIVATE
        kick_drum_ui
        kick_drum_platform
)

vstgui_set_target_bundle_id(
    KickDrumSynthStandalone
    "com.kickdrum.synthesizer.standalone"
)

# Platform-specific settings
if(APPLE)
    set_target_properties(KickDrumSynthStandalone PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
        MACOSX_BUNDLE_BUNDLE_NAME "Kiq"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.kickdrum.synthesizer.standalone"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    )
    
    # Install to Applications folder
    install(TARGETS KickDrumSynthStandalone
        BUNDLE DESTINATION /Applications
    )
endif()
