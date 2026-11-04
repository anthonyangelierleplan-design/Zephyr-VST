#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Zephyr.h"

class ZephyrProcessor : public juce::AudioProcessor
{
public:
    ZephyrProcessor();
    ~ZephyrProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Zephyr"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static int getNumFactoryPresets();
    static juce::String getFactoryPresetName (int);
    void loadFactoryPreset (int);
    juce::File getPresetDir() const;
    juce::StringArray getUserPresetNames() const;
    bool saveUserPreset (const juce::String&);
    bool loadUserPreset (const juce::String&);
    bool deleteUserPreset (const juce::String&);
    static juce::StringArray choiceNames (int paramIndex);

    juce::AudioProcessorValueTreeState apvts;
    eo::Zephyr synth;
    juce::String presetName { "Init" };
    std::atomic<int> learnArm { 0 }, learnCC[2] { { -1 }, { -1 } };       // apprentissage MIDI : 0 = rien, 1 = A, 2 = B

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();
    juce::ValueTree captureState();
    void applyState (const juce::ValueTree&);
    void handle (const juce::MidiMessage&);
    std::atomic<float>* raw[eo::kNumP] {};
    float values[eo::kNumP] {};
    juce::AudioBuffer<float> extBuf;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZephyrProcessor)
};
