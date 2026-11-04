#pragma once
#include <map>
#include "ZephyrProcessor.h"
#include "UiKit.h"

class Plate : public juce::Component
{
public:
    explicit Plate (juce::String t) : title (std::move (t)) {}
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    juce::String title;
    std::vector<std::pair<juce::String, juce::Rectangle<int>>> labels;      // serigraphies libres
};

class LedBar : public juce::Component
{
public:
    LedBar (float lo, float hi, juce::String cap = {}, bool horizontal = false) : lo_ (lo), hi_ (hi), caption (std::move (cap)), horiz (horizontal) {}
    void set (float v) { if (std::abs (v - value) > 0.004f) { value = v; repaint(); } }
    void paint (juce::Graphics&) override;
    bool levelColours = true;
private:
    float lo_, hi_, value = 0.f; juce::String caption; bool horiz;
};

class Lcd : public juce::Component
{
public:
    Lcd (juce::String l, bool big = false) : label (std::move (l)), large (big) {}
    void set (const juce::String& t) { if (t != text) { text = t; repaint(); } }
    void paint (juce::Graphics&) override;
private:
    juce::String label, text { "--" }; bool large;
};

// le tuyau : echelle des harmoniques (ou des octaves) et une perle par voix, qui monte avec le vent
class PipeView : public juce::Component
{
public:
    explicit PipeView (ZephyrProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
private:
    ZephyrProcessor& proc;
};

class ZephyrContent : public juce::Component, private juce::Timer
{
public:
    explicit ZephyrContent (ZephyrProcessor&);
    ~ZephyrContent() override;
    void paint (juce::Graphics&) override;
    static constexpr int designW = 1400, designH = 1040;
private:
    void timerCallback() override;
    void buildPanelTex();
    KnobUnit* knob (juce::Component& parent, eo::P id, const juce::String& text, const char* lo = "", const char* hi = "", double detent = -1e30);
    LedToggle* toggle (juce::Component& parent, eo::P id, const juce::String& text);
    juce::ComboBox* combo (juce::Component& parent, eo::P id);
    SegmentSelector* selector (juce::Component& parent, eo::P id);
    void drawTube (juce::Graphics&, juce::Rectangle<float>, float glow, float arc);
    void drawBolt (juce::Graphics&, juce::Point<float>, juce::Point<float>);
    void stepPreset (int dir);
    void presetMenu();
    void afterPreset();
    void saveDialog();
    void showDialog (const juce::String&, const juce::String&, bool, const juce::String&, const juce::String&, std::function<void (bool, const juce::String&)>);
    void confirm (const juce::String&, const juce::String&, std::function<void()>);

    ZephyrProcessor& proc;
    OrageLookAndFeel laf;
    juce::Image panelTex;
    juce::Random rnd;
    float heatShown = 0.f, arcShown = 0.f;
    int lastMidi = 0, midiFlash = 0;

    Plate pipePlate { "THE PIPE" }, resPlate { "RESONATOR  -  FEEDBACK LOOP" }, spacePlate { "SPACE  &  OUTPUT" },
          presPlate { "AFTERTOUCH  -  WHAT PRESSURE DOES" }, perfPlate { "PERFORMANCE" },
          fmPlate { "FM SYNTH  -  WHAT EXCITES THE PIPE" }, dlPlate { "DELAY  -  SPRING / PSYCHO  &  FX ROUTING" };
    PipeView pipeView;

    std::vector<std::unique_ptr<KnobUnit>> knobs;
    KnobUnit* byId[eo::kNumP] {};
    std::vector<std::unique_ptr<LedToggle>> toggles;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> att;
    std::vector<std::unique_ptr<juce::ComboBox>> combos;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtt;
    std::vector<std::unique_ptr<juce::Component>> owned;

    LedBar* outBar = nullptr; LedBar* voiceBar[eo::kVoices] {}; LedBar* pressBar = nullptr;
    Lcd *lcdPress = nullptr, *lcdVoices = nullptr, *lcdPeak = nullptr;
    LedLamp midiLamp { "MIDI" };
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, nameBtn, saveBtn { "SAVE" };
    std::unique_ptr<OrageDialog> dialog;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZephyrContent)
};

class ZephyrEditor : public juce::AudioProcessorEditor
{
public:
    explicit ZephyrEditor (ZephyrProcessor&);
    ~ZephyrEditor() override { setLookAndFeel (nullptr); }
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colour (0xff0b0b0c)); }
    void resized() override;
    ZephyrContent content;
private:
    juce::ComponentBoundsConstrainer constrainer;
};
