cmake_minimum_required(VERSION 3.24)

project(RapReadyOne VERSION 0.2.0 LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.13
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

set(RAPREADY_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND RAPREADY_FORMATS AU)
endif()

juce_add_plugin(RapReadyOne
    COMPANY_NAME "Bedroom Labs"
    BUNDLE_ID "com.bedroomlabs.rapreadyone"
    PLUGIN_MANUFACTURER_CODE BdRm
    PLUGIN_CODE RpR1
    FORMATS ${RAPREADY_FORMATS}
    PRODUCT_NAME "RapReady One"
    DESCRIPTION "One-knob mix-ready rap vocal processor"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "RapReady One uses the selected microphone for live vocal processing."
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx Dynamics Filter
)

juce_generate_juce_header(RapReadyOne)

add_library(RapReadyDSP STATIC
    Source/Dsp/RapReadyDSP.cpp
    Source/Dsp/RapReadyDSP.h
)

target_include_directories(RapReadyDSP PUBLIC Source)
target_link_libraries(RapReadyDSP PUBLIC juce::juce_audio_basics juce::juce_dsp)
target_compile_definitions(RapReadyDSP PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

add_library(RapReadyOffline STATIC
    Source/OfflineRenderer.cpp
    Source/OfflineRenderer.h
)
target_include_directories(RapReadyOffline PUBLIC Source)
target_link_libraries(RapReadyOffline PUBLIC
    RapReadyDSP
    juce::juce_audio_formats
)
target_compile_definitions(RapReadyOffline PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_sources(RapReadyOne PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/AdvancedPanel.cpp
    Source/AdvancedPanel.h
    Source/Parameters.h
    Source/Theme.h
)

target_link_libraries(RapReadyOne PRIVATE
    RapReadyDSP
    RapReadyOffline
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

target_compile_definitions(RapReadyOne PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

juce_add_console_app(RapReadyRender PRODUCT_NAME "RapReady File Renderer")
set_target_properties(RapReadyRender PROPERTIES OUTPUT_NAME "RapReady File Renderer")
target_sources(RapReadyRender PRIVATE tools/RenderMain.cpp)
target_link_libraries(RapReadyRender PRIVATE
    RapReadyOffline
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

if(BUILD_TESTING)
    juce_add_console_app(RapReadyTests PRODUCT_NAME "RapReady DSP Tests")
    set_target_properties(RapReadyTests PROPERTIES OUTPUT_NAME "RapReady DSP Tests")
    target_sources(RapReadyTests PRIVATE tests/DspTests.cpp)
    target_link_libraries(RapReadyTests PRIVATE
        # Link processor implementations without inheriting plugin-wrapper compile definitions.
        $<LINK_ONLY:RapReadyOne>
        RapReadyDSP
        RapReadyOffline
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
    )

    add_test(NAME RapReadyDSP COMMAND RapReadyTests)

    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(
        NAME RapReadyRendererTail
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/renderer_tail_test.py
                $<TARGET_FILE:RapReadyRender>
    )
endif()
