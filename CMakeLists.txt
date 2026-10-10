# Must be set BEFORE project(): build for both Apple Silicon and Intel
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "" FORCE)

cmake_minimum_required(VERSION 3.22)
project(FormantShifter VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(external/JUCE)

juce_add_plugin(FormantShifter
    COMPANY_NAME                "Alias4garmore"        
    PLUGIN_MANUFACTURER_CODE    Yrco                   # 4 chars, at least one uppercase
    PLUGIN_CODE                 Fmsh                   # 4 chars, exactly one uppercase
    PRODUCT_NAME                "Formant Shifter"
    FORMATS                     AU VST3
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    AU_MAIN_TYPE                kAudioUnitType_Effect
    VST3_CATEGORIES             Fx "Pitch Shift"
    COPY_PLUGIN_AFTER_BUILD     TRUE)

target_sources(FormantShifter PRIVATE
    src/PluginProcessor.h
    src/PluginProcessor.cpp)

target_compile_definitions(FormantShifter PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(FormantShifter
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)