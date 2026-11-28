// Editeur de la version instrument : quatre onglets (SYNTH / ARP / EFFECTS / MATRIX) + clavier a l'ecran.
// L'onglet EFFECTS est l'editeur de Zephyr FX tel quel ; l'onglet SYNTH se pose par-dessus son corps (l'en-tete et les presets restent visibles).
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginEditor.h"

class LfoView : public juce::Component, private juce::Timer
{
public:
    // v7 : anime : l'onde defile comme sur un oscilloscope, a la vraie vitesse du LFO (60 images/s, seulement quand l'ecran est visible)
    LfoView (juce::AudioProcessorValueTreeState& a, const juce::String& waveId, const juce::String& slewId, const zephyr::Synth* s = nullptr, int index = 0)
        : apvts (a), wave (waveId), slew (slewId), synth (s), idx (index) { if (synth != nullptr) startTimerHz (60); }
    ~LfoView() override { stopTimer(); }
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override { if (isShowing()) repaint(); }
    float value (const juce::String& id) const;
    float shapeAt (float ph, int w, float sl) const;
    juce::AudioProcessorValueTreeState& apvts;
    juce::String wave, slew;
    const zephyr::Synth* synth; int idx;
};


// Cadre d'ecran (lunette metal biseautee) autour d'un petit ecran ; renvoie la zone interieure
juce::Rectangle<float> drawBezel (juce::Graphics& g, juce::Rectangle<float> b);

// Vue animee : se redessine toute seule des qu'un des reglages lus change (courbes qui bougent avec les potards)
class LiveView : public juce::Component, private juce::Timer
{
public:
    LiveView (juce::AudioProcessorValueTreeState& a, std::vector<juce::String> ids) : apvts (a), pids (std::move (ids)) { startTimerHz (20); }
    ~LiveView() override { stopTimer(); }
protected:
    float v (int i) const { auto* p = apvts.getRawParameterValue (pids[(size_t) i]); return p != nullptr ? p->load() : 0.f; }
    juce::AudioProcessorValueTreeState& apvts;
    std::vector<juce::String> pids;
private:
    void timerCallback() override { float h = 0.f; for (size_t i = 0; i < pids.size(); ++i) h += v ((int) i) * (float) (i * 37 + 11); if (h != last) { last = h; repaint(); } }
    float last = -1.f;
};

// Reponse en frequence du filtre (modele choisi, coupure, resonance, passe-haut) : bouge avec les potards
class FilterView : public LiveView
{
public:
    explicit FilterView (juce::AudioProcessorValueTreeState& a) : LiveView (a, { "s_ftype", "s_lpf", "s_lpfRes", "s_hpf", "s_hpfRes", "s_semMorph", "s_drive", "s_fltEnv", "s_key" }) {}
    void paint (juce::Graphics&) override;
};

// Courbe ADSR (filtre ou ampli) : bouge avec les potards. prefix : 'f' ou 'a'
class EnvView : public LiveView
{
public:
    EnvView (juce::AudioProcessorValueTreeState& a, char prefix)
        : LiveView (a, { juce::String ("s_") + prefix + "A", juce::String ("s_") + prefix + "D", juce::String ("s_") + prefix + "S", juce::String ("s_") + prefix + "R" }) {}
    void paint (juce::Graphics&) override;
};

// Flancs en bois, vis, aerations : decor de la fenetre (derriere l'editeur)
class WoodFrame : public juce::Component
{
public:
    static constexpr int kFlank = 26;
    WoodFrame() { setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics&) override;
};

// Volet de la matrice de modulation : un menu deroulant au-dessus des deux onglets (SYNTH et EFFECTS)
class MatrixDrawer : public juce::Component
{
public:
    explicit MatrixDrawer (juce::AudioProcessorValueTreeState&);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    struct Row { juce::ComboBox src, dst, dir; DetentSlider amt { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox }; };      // dir : sens (avant / arriere / les deux)
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
    void setSpeedRow (bool s) { speedRow = s; repaint(); }       // rangee du bas : false = RATCHET, true = STEP SPEED (v4)
    // v7 : selection (clic sur les numeros, ou MAJ + clic), CTRL+D = dupliquer, CTRL+Z = annuler, CTRL+Y / CTRL+MAJ+Z = refaire, ECHAP = deselectionner
    bool keyPressed (const juce::KeyPress&) override;
    void pushUndo (int coalesce = -1);                          // a appeler AVANT chaque modification du motif (coalesce >= 0 : molette, une seule etape par geste)
    void duplicate();
    void undoRedo (bool redo);
private:
    int selA = -1, selB = -1; bool selecting = false;
    float mainBottom() const { return 16.f + ((float) getHeight() - 16.f) * 0.64f; }   // v7 : ligne des numeros (16 px) puis cases des notes, puis rangee du bas
    void timerCallback() override;
    void apply (const juce::MouseEvent&, bool first);
    bool speedRow = false;
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
    void mouseDown (const juce::MouseEvent& e) override { grabKeyboardFocus(); if (grid != nullptr) grid->pushUndo(); apply (e); }
    void mouseDrag (const juce::MouseEvent& e) override { apply (e); }
    bool keyPressed (const juce::KeyPress& k) override { return grid != nullptr && grid->keyPressed (k); }   // v7 : CTRL+Z / CTRL+D aussi depuis les pistes
    void randomize();
    void clear();
    StepGrid* grid = nullptr;
private:
    void timerCallback() override { const int c = proc.arpStepShown.load(); if (c != shown) { shown = c; repaint(); } }
    void apply (const juce::MouseEvent&);
    ZephyrAudioProcessor& proc;
    int lane, shown = -2;
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
    StepGrid* stepGrid = nullptr;                               // v7 : les pistes passent leurs CTRL+Z / CTRL+D a la grille
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAtt;
    std::vector<LfoView*> lfoViews;
    std::vector<std::function<void()>> refreshers;   // v5 : reglages grises selon le mode (ARP / SEQ), libelles qui suivent l'etat
    std::vector<std::shared_ptr<void>> idAttachments;
    juce::Image tex;
    ModuleUnit* envLinkUnit = nullptr; ModuleUnit* envFltUnit = nullptr; juce::Component* envLinkView = nullptr; juce::ComboBox* envLinkDst = nullptr; KnobUnit* envLinkAmt = nullptr;   // LINK des enveloppes
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
    std::unique_ptr<SynthPanel> synthPanel, arpPanel;
    std::unique_ptr<MatrixDrawer> matrix;
    WoodFrame wood;
    juce::TextButton synthTab { "SYNTH" }, arpTab { "ARP" }, fxTab { "EFFECTS" }, matrixBtn { "MATRIX" };
    juce::ComponentBoundsConstrainer constrainer;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthEditor)
};
