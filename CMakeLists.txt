cmake_minimum_required(VERSION 3.22)

   set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build for Intel and Apple Silicon" FORCE)
   set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version" FORCE)

   project(SubOsc VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Fetch JUCE automatically. You need an internet connection the first time
# you run cmake. If you'd rather use a JUCE copy you already have on disk,
# comment out the FetchContent block and use:
#   add_subdirectory(/path/to/your/JUCE JUCE)
# instead.
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
  JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG        8.0.4
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(SubOsc
    COMPANY_NAME "David Test Plugin"
    PLUGIN_MANUFACTURER_CODE Subo
    PLUGIN_CODE Sub1
    FORMATS VST3 Standalone
    PRODUCT_NAME "SubOsc"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES "Instrument" "Synth"
)

juce_generate_juce_header(SubOsc)

target_sources(SubOsc
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_compile_definitions(SubOsc
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(SubOsc
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
