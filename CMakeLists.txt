cmake_minimum_required(VERSION 3.22)

# Must be set before project() so it applies to every target.
# 11.0 covers Apple Silicon + Intel and every macOS that runs Live 11.3 (incl. Sonoma 14.x).
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

project(DaveGrain VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(DAVEGRAIN_BUILD_TESTS "Build the headless engine tests" ON)

# ---------------------------------------------------------------------------
# JUCE (fetched automatically). To use a local checkout instead, configure with
#   -DFETCHCONTENT_SOURCE_DIR_JUCE=/path/to/JUCE
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.9
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

set(DAVEGRAIN_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND DAVEGRAIN_FORMATS AU)
endif()

juce_add_plugin(DaveGrain
    PRODUCT_NAME              "Dave Grain"
    COMPANY_NAME              "Dave Grain Audio"
    COMPANY_WEBSITE           "https://github.com/lorca7/dave-grain"
    BUNDLE_ID                 com.davegrainaudio.davegrain
    PLUGIN_MANUFACTURER_CODE  Dvgr
    PLUGIN_CODE               Dgrn
    IS_SYNTH                  TRUE
    NEEDS_MIDI_INPUT          TRUE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
    FORMATS                   ${DAVEGRAIN_FORMATS}
    VST3_CATEGORIES           Instrument Synth Sampler
    AU_MAIN_TYPE              kAudioUnitType_MusicDevice
    MICROPHONE_PERMISSION_ENABLED FALSE)

set(DAVEGRAIN_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Parameters.cpp
    Source/PresetManager.cpp
    Source/FactoryPresets.cpp
    Source/dsp/GrainEngine.cpp
    Source/dsp/BuiltinSources.cpp
    Source/dsp/Effects.cpp
    Source/ui/DaveLookAndFeel.cpp
    Source/ui/Controls.cpp
    Source/ui/WaveformView.cpp
    Source/ui/PresetBar.cpp)

target_sources(DaveGrain PRIVATE ${DAVEGRAIN_SOURCES})
target_include_directories(DaveGrain PRIVATE Source)

target_compile_definitions(DaveGrain PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_USE_MP3AUDIOFORMAT=0)

target_link_libraries(DaveGrain
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Headless tests: render audio through the real processor, check presets/state.
# ---------------------------------------------------------------------------
if(DAVEGRAIN_BUILD_TESTS)
    juce_add_console_app(DaveGrainTests PRODUCT_NAME "DaveGrainTests")
    target_sources(DaveGrainTests PRIVATE tests/EngineTests.cpp ${DAVEGRAIN_SOURCES})
    target_include_directories(DaveGrainTests PRIVATE Source)
    target_compile_definitions(DaveGrainTests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_MODAL_LOOPS_PERMITTED=1
        JucePlugin_Name="Dave Grain"
        JucePlugin_IsSynth=1
        JucePlugin_WantsMidiInput=1
        JucePlugin_ProducesMidiOutput=0
        JucePlugin_IsMidiEffect=0
        JucePlugin_VersionString="${PROJECT_VERSION}")
    target_link_libraries(DaveGrainTests
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)
endif()
