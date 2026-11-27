cmake_minimum_required(VERSION 3.22)
project(Zephyr VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

# tronc commun Foudre Audio (code partage Orage / Zephyr : accords du sequenceur...). Fige sur un commit precis : le build reste reproductible.
FetchContent_Declare(FoudreCore
    GIT_REPOSITORY https://github.com/anthonyangelierleplan-design/Foudre-core.git
    GIT_TAG 8192015c4eb29a12d389b53021c4dd577e70aeed)
FetchContent_MakeAvailable(FoudreCore)

if(DEFINED ENV{CI})
    set(ZEPHYR_COPY FALSE)   # sur GitHub Actions : pas de copie dans les dossiers systeme
else()
    set(ZEPHYR_COPY TRUE)
endif()

if(APPLE)
    set(ZEPHYR_FORMATS VST3 AU Standalone)
else()
    set(ZEPHYR_FORMATS VST3 Standalone)
endif()

# ------------------------------------------------------------------ Zephyr FX (effet : le rack seul, pour les pistes audio)
juce_add_plugin(ZephyrFx
    COMPANY_NAME "Foudre Audio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Fdre
    PLUGIN_CODE ZpFx
    FORMATS ${ZEPHYR_FORMATS}
    PRODUCT_NAME "Zephyr FX"
    COPY_PLUGIN_AFTER_BUILD ${ZEPHYR_COPY})

target_sources(ZephyrFx PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/SynthEditor.cpp)

target_compile_definitions(ZephyrFx PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(ZephyrFx
    PRIVATE
        juce::juce_audio_utils
        foudre::core
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)



# ------------------------------------------------------------------ Zephyr (instrument : synthe 12 voix + effets, MIDI)
juce_add_plugin(Zephyr
    COMPANY_NAME "Foudre Audio"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Fdre
    PLUGIN_CODE ZpSy
    FORMATS ${ZEPHYR_FORMATS}
    PRODUCT_NAME "Zephyr"
    COPY_PLUGIN_AFTER_BUILD ${ZEPHYR_COPY})

target_sources(Zephyr PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/SynthEditor.cpp)

target_compile_definitions(Zephyr PUBLIC
    ZEPHYR_SYNTH=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Zephyr
    PRIVATE
        juce::juce_audio_utils
        foudre::core
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
