#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Engine.h"
#ifndef ZEPHYR_SYNTH
 #define ZEPHYR_SYNTH 0
#endif
#include "Synth.h"
#include "Arp.h"

class ZephyrAudioProcessor : public juce::AudioProcessor
{
public:
    ZephyrAudioProcessor();
    ~ZephyrAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    #if ZEPHYR_SYNTH
    const juce::String getName() const override { return "Zephyr"; }
    bool acceptsMidi() const override  { return true; }
#else
    const juce::String getName() const override { return "Zephyr FX"; }
    bool acceptsMidi() const override  { return false; }
#endif
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- presets et remises a zero (thread de l'interface)
    static int getNumFactoryPresets();
    static juce::String getFactoryPresetName (int index);
    void loadFactoryPreset (int index);
    juce::File getPresetDir() const;
    juce::StringArray getUserPresetNames() const;
    static juce::String getFactoryPresetCategory (int index);
    static juce::StringArray getCategories();
    juce::String getUserPresetCategory (const juce::String& name) const;
    bool saveUserPreset (const juce::String& name, const juce::String& category);
    bool loadUserPreset (const juce::String& name);
    bool deleteUserPreset (const juce::String& name);
    void resetModules();        // remet tous les reglages par defaut (rack et patchbay intacts)
    void clearRack();           // vide le rack : INPUT est relie directement a OUTPUT
    void defaultRack();         // remet les modules dans l'ordre d'origine
    void clearPatchbay();       // retire tous les cables
    void setParamReal (const char* id, float realValue);

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;  // clavier a l'ecran (version instrument)
    std::atomic<int> macroCC[8] { 16, 17, 18, 19, 20, 21, 22, 23 }, learnMacro { -1 };   // CC MIDI des macros (16..23 par defaut = macros 1..8 de l'Hydrasynth) et macro en apprentissage
    zephyr::Synth synth;                     // utilise seulement par la version instrument (ZEPHYR_SYNTH)
    zephyr::Arp arp;                         // arpegiateur (version instrument)
    zephyr::Engine engine;                   // rack + patchbay vivent ici ; l'editeur les lit et les modifie
    juce::String presetName { "Init" }, presetCategory;

    // oscilloscope de l'en-tete : tampon circulaire alimente par la sortie finale (lecture sans verrou par l'interface)
    static constexpr int kScope = 8192;
    std::atomic<int> headerScope { 0 };                // 0 : prisme, 1 : oscilloscope (clic sur la fenetre)
    void copyScope (float* dst, int n) const
    {
        const int w = scopeW.load (std::memory_order_acquire);
        for (int i = 0; i < n; ++i) dst[i] = scopeBuf[(w - n + i) & (kScope - 1)];
    }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    zephyr::Params readParams() const;
    zephyr::SynthParams readSynth();
    zephyr::ArpParams arpParams;
    struct MidiEv { int pos; int type; int a; float v; int b = -1; };       // type : 0 note on, 1 note off, 2 all off, 3 bend, 4 molette, 5 aftertouch poly, 6 aftertouch canal, 7/8/9 notes brutes de l'arpegiateur (simple / pile unison / off), 10/11 VCO du mode POLY (on / off, b = VCO)
    std::vector<MidiEv> evs;
    int lastArpOn = 0, lastAssign = 0, chordN = 3, chordIv[6] { 4, 7, 11, 0, 0, 0 }, learnN = 0, learnNotes[6] {};
    bool chordLearn = false, lastLearn = false;
    bool handleMacroCC (int cc, int value);
    float arpStepHz (double bpm) const;
    float arpRateKnob = 0.55f; int arpSyncDiv = 0;
    std::vector<std::atomic<float>*> rawSyn;
    juce::ValueTree captureState();
    void applyState (juce::ValueTree state);

    std::vector<std::atomic<float>*> raw;
    float scopeBuf[kScope] {};
    std::atomic<int> scopeW { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZephyrAudioProcessor)
};
