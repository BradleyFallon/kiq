add_library(kick_drum_ui STATIC
    KiqMainView.cpp
    KiqFactoryPresets.cpp
    include/KiqFactoryPresets.h
    include/KiqMainView.h
    include/KiqUIBridge.h
)

target_include_directories(kick_drum_ui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/audio_engine/analysis
        ${CMAKE_SOURCE_DIR}/src/audio_engine/parameters
        ${CMAKE_SOURCE_DIR}/src/audio_engine/envelopes
        ${CMAKE_SOURCE_DIR}/src/audio_engine/workflow
        ${VST3_SDK_PATH}/vstgui4
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src/audio_engine/export
        ${CMAKE_SOURCE_DIR}/src/audio_engine/presets
        ${CMAKE_SOURCE_DIR}/src/audio_engine/utils
        ${CMAKE_SOURCE_DIR}/src/audio_engine/voice
)

target_link_libraries(kick_drum_ui
    PUBLIC
        kick_drum_audio_engine
        vstgui
)
