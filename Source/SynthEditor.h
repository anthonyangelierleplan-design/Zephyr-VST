// Editeur de la version instrument : deux onglets (SYNTH / EFFECTS) + clavier a l'ecran.
// L'onglet EFFECTS est l'editeur d'Orage FX tel quel ; l'onglet SYNTH se pose par-dessus son corps (l'en-tete et les presets restent visibles).
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginEditor.h"

class LfoView : public juce::Component
{
public:
    LfoView (juce::AudioProcessorValueTreeState& a, const juce::String& waveId, const juce::String& slewId) : apvts (a), wave (waveId), slew (slewId) {}
    void paint (juce::Graphics&) override;
private:
    float value (const juce::String& id) const;
    juce::AudioProcessorValueTreeState& apvts;
    juce::String wave, slew;
};

// Liaison menu <-> parametre "choice" par identifiant d'element (et non par position) : les menus de la matrice sont ranges par familles
struct IdComboAttachment
{
    IdComboAttachment (juce::RangedAudioParameter& p, juce::ComboBox& b)
        : box (b), att (p, [this] (float v) { box.setSelectedId (juce::roundToInt (v) + 1, juce::dontSendNotification); })
    {
        box.onChange = [this] { if (box.getSelectedId() > 0) att.setValueAsCompleteGesture ((float) (box.getSelectedId() - 1)); };
        att.sendInitialUpdate();
    }
    juce::ComboBox& box;
    juce::ParameterAttachment att;
};

// Volet de la matrice de modulation : un menu deroulant au-dessus des deux onglets (SYNTH et EFFECTS)
class MatrixDrawer : public juce::Component
{
public:
    explicit MatrixDrawer (juce::AudioProcessorValueTreeState&);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    struct Row { juce::ComboBox src, dst; DetentSlider amt { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox }; };
    std::vector<std::unique_ptr<Row>> rows;
    std::vector<std::unique_ptr<IdComboAttachment>> comboAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAtt;
};

class SynthPanel : public juce::Component, private juce::Timer
{
public:
    explicit SynthPanel (ZephyrAudioProcessor&);
    ~SynthPanel() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void rebuild();                                 // changement de theme

private:
    void timerCallback() override;
    void mouseDown (const juce::MouseEvent&) override;
    std::vector<KnobUnit*> macroKnobs;
    KnobUnit* knob (juce::Component& parent, const char* id, const juce::String& text, const char* lo = "0", const char* hi = "10");
    ModuleUnit* unit (const juce::String& title);

    ZephyrAudioProcessor& proc;
    std::vector<std::unique_ptr<ModuleUnit>> units;
    std::vector<std::unique_ptr<KnobUnit>> knobs;
    std::vector<std::unique_ptr<juce::Component>> owned;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAtt;
    std::vector<LfoView*> lfoViews;
    juce::Image tex;
};

// Flanc en bois (gauche ou droite) : veinage procedural, mis en cache, suit le theme
class WoodCheek : public juce::Component
{
public:
    explicit WoodCheek (bool leftSide) : left (leftSide) { setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics&) override;
    void resized() override { cache = {}; }
    void themeChanged() { cache = {}; repaint(); }
private:
    bool left;
    juce::Image cache;
    int cachedTheme = -1;
};

// Fenetre redimensionnable : tout le contenu est dessine a la taille de conception puis mis a l'echelle.
class SynthEditor : public juce::AudioProcessorEditor
{
public:
    explicit SynthEditor (ZephyrAudioProcessor&);
    ~SynthEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void showTab (int t);
    int designHeight() const;
    int designWidth() const;
    ZephyrLookAndFeel laf;
    ZephyrAudioProcessor& proc;
    juce::Component content;
    WoodCheek cheekL { true }, cheekR { false };
    ZephyrEditor fx;
    std::unique_ptr<SynthPanel> synthPanel;
    std::unique_ptr<MatrixDrawer> matrix;
    juce::TextButton synthTab { "SYNTH" }, fxTab { "EFFECTS" }, matrixBtn { "MATRIX" };
    juce::ComponentBoundsConstrainer constrainer;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthEditor)
};
