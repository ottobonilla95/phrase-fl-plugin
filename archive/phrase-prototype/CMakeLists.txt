cmake_minimum_required(VERSION 3.22)
project(Phrase VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS")
include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.12 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)
juce_add_plugin(Phrase
    COMPANY_NAME "Otto Audio" BUNDLE_ID "audio.otto.phrase"
    PLUGIN_MANUFACTURER_CODE OtAu PLUGIN_CODE Phrs
    FORMATS VST3 Standalone PRODUCT_NAME "Phrase"
    IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE VST3_CATEGORIES Instrument Tools)
target_sources(Phrase PRIVATE Source/Music.cpp Source/AI.cpp Source/Processor.cpp Source/Editor.cpp)
target_compile_definitions(Phrase PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(Phrase PRIVATE juce::juce_audio_utils PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
juce_add_console_app(PhraseTests PRODUCT_NAME "Phrase tests")
target_sources(PhraseTests PRIVATE Tests/MusicTests.cpp Source/Music.cpp Source/AI.cpp)
target_include_directories(PhraseTests PRIVATE Source)
target_compile_definitions(PhraseTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(PhraseTests PRIVATE juce::juce_audio_basics juce::juce_events)
enable_testing()
add_test(NAME music COMMAND PhraseTests)
juce_add_console_app(PhraseEngineTests PRODUCT_NAME "Phrase engine tests")
target_sources(PhraseEngineTests PRIVATE Tests/EngineTests.cpp)
target_include_directories(PhraseEngineTests PRIVATE Source "${juce_SOURCE_DIR}/modules")
target_compile_definitions(PhraseEngineTests PRIVATE JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1)
target_link_libraries(PhraseEngineTests PRIVATE Phrase)
add_test(NAME engine COMMAND PhraseEngineTests)
