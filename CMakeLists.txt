cmake_minimum_required(VERSION 3.22)

project(TapeDelayVST VERSION 1.0.3)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(JUCE_PATH "" CACHE PATH "Path to JUCE")
if(NOT JUCE_PATH)
    set(JUCE_PATH "${CMAKE_CURRENT_LIST_DIR}/../JUCE")
endif()

if(APPLE)
    message(FATAL_ERROR "macOS is not supported for this project")
endif()

if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found at ${JUCE_PATH}. Set -DJUCE_PATH=/path/to/JUCE")
endif()

add_subdirectory(${JUCE_PATH} JUCE)

juce_add_plugin(TapeDelay
    COMPANY_NAME "Ystu"
    PRODUCT_NAME "TapeDelay"
    PLUGIN_MANUFACTURER_CODE Ystu
    PLUGIN_CODE Tdvs
    FORMATS VST3
    VST3_CATEGORIES "Fx"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

juce_generate_juce_header(TapeDelay)

set_target_properties(TapeDelay PROPERTIES JUCE_PLUGIN_NAME "CockAmps: Tape Delay")

juce_add_binary_data(TapeDelayResources
    NAMESPACE BinaryData
    HEADER_NAME BinaryData.h
    SOURCES Source/CockDelayBG.png
)
set_target_properties(TapeDelayResources PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_sources(TapeDelay
    PRIVATE
        Source/PluginEntry.cpp
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/TapeDelay.cpp
        Source/TapeDelay.h
        Source/BBDDelay.cpp
        Source/BBDDelay.h
)

target_compile_definitions(TapeDelay
    PRIVATE
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(TapeDelay
    PRIVATE
        TapeDelayResources
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_extra
)

if(WIN32)
    target_compile_definitions(TapeDelay PRIVATE NOMINMAX _USE_MATH_DEFINES)
endif()

if(UNIX AND NOT APPLE)
    target_compile_options(TapeDelay PRIVATE -fvisibility=hidden)
endif()
