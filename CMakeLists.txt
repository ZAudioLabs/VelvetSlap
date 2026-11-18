cmake_minimum_required(VERSION 3.22)
project(VelvetSlap VERSION 1.0.0)
include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.6)
FetchContent_MakeAvailable(JUCE)
juce_add_binary_data(VelvetSlapAssets SOURCES Resources/bg.png Resources/logo.jpg)
set(VS_FORMATS VST3 Standalone)
if(APPLE)
  list(APPEND VS_FORMATS AU)
endif()
if(DEFINED ENV{CI})
  set(VS_COPY FALSE)
else()
  set(VS_COPY TRUE)
endif()
juce_add_plugin(VelvetSlap
  COMPANY_NAME "Z Audio Labs" PLUGIN_MANUFACTURER_CODE ZAlb PLUGIN_CODE VSlp
  FORMATS ${VS_FORMATS} PRODUCT_NAME "Velvet Slap"
  IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE COPY_PLUGIN_AFTER_BUILD ${VS_COPY})
juce_generate_juce_header(VelvetSlap)
target_sources(VelvetSlap PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(VelvetSlap PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(VelvetSlap PRIVATE VelvetSlapAssets juce::juce_audio_utils juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)
