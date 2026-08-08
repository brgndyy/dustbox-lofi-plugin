cmake_minimum_required(VERSION 3.22)

project(DustBoxLoFi VERSION 0.2.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_binary_data(DustBoxLoFiAssets
    SOURCES
        assets/vinyl_crackle_source_original.wav
)

juce_add_plugin(DustBoxLoFi
    COMPANY_NAME "GNDY Audio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Gndy
    PLUGIN_CODE Dblf
    FORMATS VST3
    PRODUCT_NAME "DustBox LoFi"
    BUNDLE_ID "com.gndy.audio.dustboxlofi"
)

juce_generate_juce_header(DustBoxLoFi)

target_sources(DustBoxLoFi
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PluginProcessor.h
)

target_compile_definitions(DustBoxLoFi
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(DustBoxLoFi
    PRIVATE
        DustBoxLoFiAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

juce_add_console_app(DustBoxDSPTest PRODUCT_NAME "DustBox DSP Test")
set_target_properties(DustBoxDSPTest PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_sources(DustBoxDSPTest PRIVATE tests/dsp_age_render_test.cpp)
target_include_directories(DustBoxDSPTest PRIVATE Source ${CMAKE_CURRENT_BINARY_DIR}/DustBoxLoFi_artefacts/JuceLibraryCode)
target_link_libraries(DustBoxDSPTest PRIVATE DustBoxLoFi DustBoxLoFiAssets juce::juce_audio_utils juce::juce_dsp)
