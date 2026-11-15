// Editeur de la version instrument : quatre onglets (SYNTH / ARP / EFFECTS / MATRIX) + clavier a l'ecran.
// L'onglet EFFECTS est l'editeur de Zephyr FX tel quel ; l'onglet SYNTH se pose par-dessus son corps (l'en-tete et les presets restent visibles).
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
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtt;
    std::vector<std::shared_ptr<void>> idAtt;       // attachements par identifiant (menus groupes)
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAtt;
};


// Etiquette de serigraphie (texte pose sur une plaque)
class Tag : public juce::Component
{
public:
    explicit Tag (const juce::String& t, juce::Justification j = juce::Justification::centredLeft, float h = 9.f) : text (t), just (j), size (h) { setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics&) override;
    juce::String text; juce::Justification just; float size;
};

// Grille de pas des notes : clic = silence / note / accent, rangee du bas = ratchet (1 a 4 repetitions)
class StepGrid : public juce::Component, private juce::Timer
{
public:
    explicit StepGrid (ZephyrAudioProcessor&);
    ~StepGrid() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void randomize();
    void clear();
private:
    void timerCallback() override;
    void apply (const juce::MouseEvent&, bool first);
    ZephyrAudioProcessor& proc;
    int shown = -2, paintState = 1, recSig = -1;
    bool ratchetRow = false;
};

// Piste de modulation : 16 barres reglables a la souris (clic droit = zero), longueur propre a chaque piste
class LaneEditor : public juce::Component, private juce::Timer
{
public:
    LaneEditor (ZephyrAudioProcessor&, int lane);
    ~LaneEditor() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override { apply (e); }
    void mouseDrag (const juce::MouseEvent& e) override { apply (e); }
    void randomize();
    void clear();
private:
    void timerCallback() override { const int c = proc.arpStepShown.load(); if (c != shown) { shown = c; repaint(); } }
    void apply (const juce::MouseEvent&);
    ZephyrAudioProcessor& proc;
    int lane, shown = -2;
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

class SynthPanel : public juce::Component, private juce::Timer
{
public:
    SynthPanel (ZephyrAudioProcessor&, int page);
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
    int page = 0;                                   // 0 : SYNTH, 1 : ARP
    std::vector<std::unique_ptr<ModuleUnit>> units;
    std::vector<std::unique_ptr<KnobUnit>> knobs;
    std::vector<std::unique_ptr<juce::Component>> owned;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAtt;
    std::vector<LfoView*> lfoViews;
    std::vector<std::shared_ptr<void>> idAttachments;
    juce::Image tex;
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
    ZephyrEditor fx;
    WoodCheek cheekL { true }, cheekR { false };
    std::unique_ptr<SynthPanel> synthPanel, arpPanel;
    std::unique_ptr<MatrixDrawer> matrix;
    juce::TextButton synthTab { "SYNTH" }, arpTab { "ARP" }, fxTab { "EFFECTS" }, matrixBtn { "MATRIX" };
    juce::ComponentBoundsConstrainer constrainer;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthEditor)
};
