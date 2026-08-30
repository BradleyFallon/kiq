# Standalone Application
add_executable(KickDrumSynthStandalone
    main.cpp
    StandaloneApp.cpp
)

target_include_directories(KickDrumSynthStandalone
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(KickDrumSynthStandalone
    PRIVATE
        kick_drum_audio_engine
        kick_drum_ui
        kick_drum_platform
)

# Platform-specific settings
if(APPLE)
    set_target_properties(KickDrumSynthStandalone PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
        MACOSX_BUNDLE_BUNDLE_NAME "Kick Drum Synthesizer"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.kickdrum.synthesizer.standalone"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.kickdrum.synthesizer.standalone"
    )
    
    # Install to Applications folder
    install(TARGETS KickDrumSynthStandalone
        BUNDLE DESTINATION /Applications
    )
endif()
