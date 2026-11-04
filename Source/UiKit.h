#pragma once
#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace look
{
inline const juce::Colour paint  { 0xff111112 };       // peinture noire
inline const juce::Colour print  { 0xffe9e5d8 };       // serigraphie blanc casse
inline const juce::Colour cvRing { 0xffe0662a };       // anneau orange des prises CV
}

namespace ui
{
juce::String fr (const char* utf8);
juce::Font font (float h, bool bold = true);
juce::Colour heatColour (float t);
void drawSpecks (juce::Graphics& g, juce::Rectangle<int> r);
void drawPlate (juce::Graphics& g, juce::Rectangle<float> b);
}

class OrageLookAndFeel : public juce::LookAndFeel_V4
{
public:
    OrageLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
};

// ---------------------------------------------------------------- potard (echelle et nom serigraphies dans paint())
// potard avec "cran" : un aimant leger retient la valeur au zero / au centre (double-clic = valeur par defaut)
class DetentSlider : public juce::Slider
{
public:
    double detent = -1e30;                                   // valeur du cran (hors plage = aucun)
    double snapValue (double v, DragMode mode) override
    {
        if (mode == notDragging || detent < getMinimum() || detent > getMaximum()) return v;
        return std::abs (v - detent) < (getMaximum() - getMinimum()) * 0.022 ? detent : v;
    }
};

class KnobUnit : public juce::Component
{
public:
    KnobUnit (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& text,
              const juce::String& lo, const juce::String& hi, int cvIdx = -1);
    void paint (juce::Graphics&) override;
    void resized() override;
    void setCvActive (bool);

    DetentSlider slider;
    juce::String text, loText, hiText;
    int cvIndex;                                     // destination CV associee (ou -1)
    bool cvActive = false;
private:
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};

// ---------------------------------------------------------------- boutons-poussoirs (parametre "choice")
class SegmentSelector : public juce::Component
{
public:
    SegmentSelector (juce::AudioProcessorValueTreeState&, const juce::String& id, juce::StringArray names);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    juce::StringArray labels;
    int selected = 0;
    juce::ParameterAttachment attachment;
};

class LedToggle : public juce::ToggleButton
{
public:
    void paintButton (juce::Graphics&, bool over, bool down) override;
};

class FootSwitch : public juce::ToggleButton
{
public:
    void paintButton (juce::Graphics&, bool over, bool down) override;
};

// ---------------------------------------------------------------- vumeter a aiguille (reduction de gain du compresseur)
class VuMeter : public juce::Component
{
public:
    void setValue (float grDb) { target = grDb; }
    void tick();
    void paint (juce::Graphics&) override;
private:
    float target = 0.f, shown = 0.f, lastDrawn = -1.f;
};

// ---------------------------------------------------------------- voyant (gate ouvert)
class LedLamp : public juce::Component
{
public:
    explicit LedLamp (juce::String t) : text (std::move (t)) {}
    void set (bool on) { if (on != lit) { lit = on; repaint(); } }
    void paint (juce::Graphics&) override;
private:
    juce::String text;
    bool lit = true;
};

// ---------------------------------------------------------------- prise jack du patchbay

// ---------------------------------------------------------------- boite de dialogue integree (plaque metal, assortie au design)
class OrageDialog : public juce::Component
{
public:
    OrageDialog (juce::String title, juce::String message, bool askText, juce::String initialText, juce::String okText,
                 std::function<void (bool, const juce::String&)> callback);
    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void focus();

    std::function<void()> onFinished;
private:
    void finish (bool ok);
    juce::String titleText, messageText;
    bool hasText;
    juce::TextEditor editor;
    juce::TextButton okBtn, cancelBtn;
    std::function<void (bool, const juce::String&)> cb;
    juce::Rectangle<int> box;
    bool done = false;
};

