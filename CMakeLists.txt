cmake_minimum_required(VERSION 3.22)
project(ChoirCrowd VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

# Windows: runtime C++ estático (/MT) -> o utilizador NÃO precisa de instalar o VC++ Redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Copiar o plugin para as pastas do sistema após compilar (útil localmente; desligado no CI)
option(CC_COPY_AFTER_BUILD "Copy plugin to the system plug-in folders after building" ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

# Formatos por plataforma:
#   macOS   -> AU (Logic, GarageBand, MainStage) + VST3 (Ableton, Cubase, FL, Studio One, Reaper, Bitwig...) + Standalone
#   Windows -> VST3 (todas as DAWs modernas) + Standalone
#   Linux   -> VST3 + Standalone (para testes/CI)
if(APPLE)
    set(CC_FORMATS AU VST3 Standalone)
else()
    set(CC_FORMATS VST3 Standalone)
endif()

juce_add_plugin(ChoirCrowd
    COMPANY_NAME "Logogueto"
    COMPANY_WEBSITE "https://github.com/kidymusiic-crypto/Mungoz-Choir"
    BUNDLE_ID "com.logogueto.choircrowd"
    PRODUCT_NAME "Choir Crowd"
    VERSION ${PROJECT_VERSION}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    PLUGIN_MANUFACTURER_CODE Lgue
    PLUGIN_CODE Chcr
    FORMATS ${CC_FORMATS}
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Synth
    HARDENED_RUNTIME_ENABLED TRUE
    COPY_PLUGIN_AFTER_BUILD ${CC_COPY_AFTER_BUILD})

target_sources(ChoirCrowd PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h)

target_compile_definitions(ChoirCrowd PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(ChoirCrowd
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Teste offline de renderização (cmake -DCC_BUILD_TESTS=ON)
option(CC_BUILD_TESTS "Build the offline render test" OFF)
if(CC_BUILD_TESTS)
    juce_add_console_app(ChoirCrowdTest PRODUCT_NAME "ChoirCrowdTest")
    target_sources(ChoirCrowdTest PRIVATE Tests/RenderTest.cpp Source/PluginProcessor.cpp)
    target_compile_definitions(ChoirCrowdTest PRIVATE
        JucePlugin_Name="Choir Crowd" JucePlugin_IsSynth=1 JucePlugin_WantsMidiInput=1
        JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1)
    target_link_libraries(ChoirCrowdTest PRIVATE
        juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
endif()
