# Must be set BEFORE project(): build for both Apple Silicon and Intel
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "" FORCE)

cmake_minimum_required(VERSION 3.22)
project(FormantShifter VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(external/JUCE)
# Third-party DSP: header-only, pinned submodules. SYSTEM keeps their warnings out of our build.
add_library(signalsmith_dsp INTERFACE)
target_include_directories(signalsmith_dsp SYSTEM INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/external/signalsmith-stretch
    ${CMAKE_CURRENT_SOURCE_DIR}/external/signalsmith-linear/include)

# DSP engine library: JUCE-free, shared by the plugin, tests and offline harness
add_library(fsdsp STATIC
    src/dsp/PitchFormantEngine.h
    src/dsp/PassthroughEngine.h
    src/dsp/PassthroughEngine.cpp
    src/dsp/SignalsmithEngine.h
    src/dsp/SignalsmithEngine.cpp
    src/dsp/EngineFactory.h
    src/dsp/EngineFactory.cpp
    src/dsp/OfflineRenderer.h
    src/dsp/OfflineRenderer.cpp
    src/dsp/ProcessChunked.h
    src/dsp/ProcessChunked.cpp)

target_include_directories(fsdsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(fsdsp PUBLIC cxx_std_20)
target_link_libraries(fsdsp PRIVATE signalsmith_dsp juce::juce_recommended_warning_flags)

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
        fsdsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# --- Tests ---
enable_testing()
add_subdirectory(external/Catch2)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/external/Catch2/extras)
include(Catch)

add_executable(fsdsp_tests
    tests/PassthroughEngineTests.cpp
    tests/ProcessChunkedTests.cpp
    tests/OfflineRendererTests.cpp
    tests/SignalsmithEngineTests.cpp
    tests/support/RecordingEngine.h
    tests/support/DelayEngine.h)

target_include_directories(fsdsp_tests PRIVATE tests)
target_link_libraries(fsdsp_tests PRIVATE fsdsp Catch2::Catch2WithMain)
set_target_properties(fsdsp_tests PROPERTIES
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED YES
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-")
catch_discover_tests(fsdsp_tests DISCOVERY_MODE PRE_TEST)

# Offline Render tool
juce_add_console_app(offline_render
    PRODUCT_NAME "Offline Render")

target_sources(offline_render PRIVATE
    tools/offline/main.cpp)

target_compile_definitions(offline_render PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
    
target_link_libraries(offline_render
    PRIVATE
        fsdsp 
        juce::juce_audio_formats
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

set_target_properties(offline_render PROPERTIES
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED YES
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-")