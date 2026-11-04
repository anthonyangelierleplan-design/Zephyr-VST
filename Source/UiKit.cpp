#include "UiKit.h"

namespace ui
{
juce::String fr (const char* utf8) { return juce::String::fromUTF8 (utf8); }
juce::Font font (float h, bool bold) { return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain)); }

juce::Colour heatColour (float t)
{
    t = juce::jlimit (0.f, 1.f, t);
    const juce::Colour a (0xff1a1412), b (0xffa11a08), c (0xffff7a1a), d (0xffffe9b0);
    if (t < 0.4f)  return a.interpolatedWith (b, t / 0.4f);
    if (t < 0.75f) return b.interpolatedWith (c, (t - 0.4f) / 0.35f);
    return c.interpolatedWith (d, (t - 0.75f) / 0.25f);
}

// ecaillures de peinture : une tuile transparente repetee par-dessus les panneaux
const juce::Image& speckTile()
{
    static const juce::Image img = []
    {
        juce::Image i (juce::Image::ARGB, 256, 256, true);
        juce::Graphics g (i);
        juce::Random rr (99);
        for (int k = 0; k < 820; ++k)
        {
            g.setColour (look::paint.withAlpha (0.92f));
            g.fillRect (rr.nextInt (256), rr.nextInt (256), 1 + rr.nextInt (2), 1 + rr.nextInt (2));
        }
        return i;
    }();
    return img;
}

void drawSpecks (juce::Graphics&, juce::Rectangle<int>) {}          // version sobre : plus de poussieres

void drawPlate (juce::Graphics& g, juce::Rectangle<float> b)
{
    g.setColour (juce::Colour (0xff0e0e10).withAlpha (0.55f));                  // panneau sobre
    g.fillRoundedRectangle (b.reduced (1.f), 3.f);
    g.setColour (look::print.withAlpha (0.30f));
    g.drawRoundedRectangle (b.reduced (1.f), 3.f, 1.f);
}

// pictogrammes d'ondes (selecteurs de forme d'onde, facon MS20)
bool drawGlyph (juce::Graphics& g, const juce::String& name, juce::Rectangle<float> r)
{
    juce::Path p;
    auto pt = [&r] (float x, float y) { return juce::Point<float> (r.getX() + x * r.getWidth(), r.getY() + y * r.getHeight()); };
    if (name == "SAW")                { p.startNewSubPath (pt (0.f, 1.f)); p.lineTo (pt (0.5f, 0.f)); p.lineTo (pt (0.5f, 1.f)); p.lineTo (pt (1.f, 0.f)); }
    else if (name == "PULSE" || name == "SQR") { p.startNewSubPath (pt (0.f, 1.f)); p.lineTo (pt (0.f, 0.f)); p.lineTo (pt (0.5f, 0.f)); p.lineTo (pt (0.5f, 1.f)); p.lineTo (pt (1.f, 1.f)); p.lineTo (pt (1.f, 0.f)); }
    else if (name == "TRI")           { p.startNewSubPath (pt (0.f, 0.5f)); p.lineTo (pt (0.25f, 0.f)); p.lineTo (pt (0.75f, 1.f)); p.lineTo (pt (1.f, 0.5f)); }
    else if (name == "SIN")
    {
        for (int i = 0; i <= 24; ++i)
        {
            const auto q = pt ((float) i / 24.f, 0.5f - 0.5f * std::sin ((float) i / 24.f * 6.2831853f));
            if (i == 0) p.startNewSubPath (q); else p.lineTo (q);
        }
    }
    else if (name == "S&H")
    {
        p.startNewSubPath (pt (0.f, 0.7f)); p.lineTo (pt (0.25f, 0.7f)); p.lineTo (pt (0.25f, 0.15f)); p.lineTo (pt (0.5f, 0.15f));
        p.lineTo (pt (0.5f, 0.9f)); p.lineTo (pt (0.75f, 0.9f)); p.lineTo (pt (0.75f, 0.4f)); p.lineTo (pt (1.f, 0.4f));
    }
    else return false;
    g.strokePath (p, juce::PathStrokeType (1.5f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));
    return true;
}
} // namespace ui
using namespace ui;

// ================================================================ look & feel
OrageLookAndFeel::OrageLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff0b0b0c));
    setColour (juce::ComboBox::textColourId, look::print);
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff6a6a70));
    setColour (juce::ComboBox::arrowColourId, look::print);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff141415));
    setColour (juce::PopupMenu::textColourId, look::print);
    setColour (juce::PopupMenu::headerTextColourId, look::cvRing);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff6a2412));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff232326));
    setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff6a2412));
    setColour (juce::TextButton::textColourOffId, look::print);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::ScrollBar::thumbColourId, juce::Colour (0xff6a6a70));
    setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff141415));
    setColour (juce::AlertWindow::textColourId, look::print);
    setColour (juce::AlertWindow::outlineColourId, juce::Colour (0xff6a6a70));
    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0b0b0c));
    setColour (juce::TextEditor::textColourId, look::print);
    setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff6a6a70));
}

void OrageLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, juce::Slider& s)
{
    const auto c = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).getCentre();
    const bool numbered = (bool) s.getProperties()["numbered"];
    const float r = (float) juce::jmin (w, h) * 0.5f, kr = r * (numbered ? 0.58f : 0.66f), tr = kr * 0.86f;
    const float angle = startAngle + pos * (endAngle - startAngle);
    juce::Random rr ((int) s.getProperties()["seed"]);
    const float alpha = s.isEnabled() ? 1.f : 0.5f;

    g.setColour (juce::Colours::black.withAlpha (0.6f * alpha));
    g.fillEllipse (c.x - kr + 2.f, c.y - kr + 5.f, kr * 2.f, kr * 2.f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff39393c), c.x - kr * 0.6f, c.y - kr * 0.7f,
                                             juce::Colour (0xff060607), c.x + kr * 0.7f, c.y + kr * 0.8f, true));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.f, kr * 2.f);
    g.setColour (juce::Colour (0xff77777c).withAlpha (0.55f));
    g.drawEllipse (c.x - kr + 0.5f, c.y - kr + 0.5f, kr * 2.f - 1.f, kr * 2.f - 1.f, 1.2f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2c2f), c.x - tr * 0.5f, c.y - tr * 0.6f,
                                             juce::Colour (0xff0b0b0c), c.x + tr * 0.7f, c.y + tr * 0.8f, true));
    g.fillEllipse (c.x - tr, c.y - tr, tr * 2.f, tr * 2.f);
    for (int i = 1; i < 5; ++i)
    {
        const float rr2 = tr * (float) i / 5.f;
        g.setColour (juce::Colours::white.withAlpha (0.035f));
        g.drawEllipse (c.x - rr2, c.y - rr2, rr2 * 2.f, rr2 * 2.f, 0.7f);
    }
    juce::Path p;
    p.addRoundedRectangle (-1.7f, -tr * 0.98f, 3.4f, tr * 0.64f, 1.2f);
    const auto xf = juce::AffineTransform::rotation (angle).translated (c.x, c.y);
    g.setColour (juce::Colour (0xfff0ece0).withAlpha (0.9f * alpha));
    g.fillPath (p, xf);
}

// ================================================================ potard
KnobUnit::KnobUnit (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, const juce::String& t,
                    const juce::String& lo, const juce::String& hi, int cvIdx)
    : text (t), loText (lo), hiText (hi), cvIndex (cvIdx), attachment (apvts, id, slider)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    slider.setPopupDisplayEnabled (true, true, nullptr);
    slider.setScrollWheelEnabled (false);                                   // la molette fait defiler le rack
    slider.getProperties().set ("seed", id.hashCode());
    if (auto* prm = apvts.getParameter (id))
        slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
    if (slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0) slider.detent = 0.0;           // cran au zero des reglages +/-
    addAndMakeVisible (slider);
}

void KnobUnit::resized() { slider.setBounds (getLocalBounds().withTrimmedBottom (16)); }

void KnobUnit::setCvActive (bool a) { if (a != cvActive) { cvActive = a; repaint(); } }

void KnobUnit::paint (juce::Graphics& g)
{
    const auto sb = slider.getBounds().toFloat();
    const auto c = sb.getCentre();
    const float r = juce::jmin (sb.getWidth(), sb.getHeight()) * 0.5f;
    const float sa = juce::MathConstants<float>::pi * 1.2f, ea = juce::MathConstants<float>::pi * 2.8f;
    const float alpha = slider.isEnabled() ? 1.f : 0.45f;
    const bool numbered = (bool) slider.getProperties()["numbered"] && sb.getWidth() >= 58.f;
    for (int i = 0; i <= 10; ++i)
    {
        const float a = sa + (float) i / 10.f * (ea - sa), rad = r * (numbered ? 0.74f : 0.93f), dot = (i % 5 == 0) ? 3.2f : 2.f;
        g.setColour (look::print.withAlpha (0.88f * alpha));
        g.fillEllipse (c.x + rad * std::sin (a) - dot * 0.5f, c.y - rad * std::cos (a) - dot * 0.5f, dot, dot);
        if (numbered)                                                              // chiffres 0..10 autour du bouton, comme le MS20
        {
            g.setFont (font (10.f));
            g.drawText (juce::String (i), (int) (c.x + r * 0.93f * std::sin (a)) - 7, (int) (c.y - r * 0.93f * std::cos (a)) - 5, 14, 10,
                        juce::Justification::centred, false);
        }
    }
    if (! numbered)
    {
        g.setFont (font (10.5f));
        g.setColour (look::print.withAlpha (0.8f * alpha));
        const int lw = juce::jmin (56, getWidth() / 2 - 1), ly = (int) (c.y + r * 0.88f);
        g.drawText (loText, juce::jmax (0, (int) (c.x - r * 0.78f) - lw / 2), ly, lw, 11, juce::Justification::centred, false);
        g.drawText (hiText, juce::jmin (getWidth() - lw, (int) (c.x + r * 0.78f) - lw / 2), ly, lw, 11, juce::Justification::centred, false);
    }
    if (slider.detent >= slider.getMinimum() && slider.detent <= slider.getMaximum())               // repere du cran (zero / centre)
    {
        const float pos = (float) slider.valueToProportionOfLength (slider.detent);
        const float a = sa + pos * (ea - sa), rad = r * (numbered ? 0.74f : 0.93f);
        g.setColour (look::cvRing.withAlpha (0.95f * alpha));
        g.fillEllipse (c.x + rad * std::sin (a) - 2.f, c.y - rad * std::cos (a) - 2.f, 4.f, 4.f);
    }
    g.setFont (font (getWidth() < 80 ? 11.5f : 12.5f));
    g.setColour (look::print.withAlpha (0.95f * alpha));
    g.drawText (text, -4, getHeight() - 15, getWidth() + 8, 14, juce::Justification::centred, false);
    if (cvActive)                                                            // un cable CV est branche sur ce reglage
    {
        g.setColour (look::cvRing.withAlpha (0.3f)); g.fillEllipse ((float) getWidth() - 17.f, 0.f, 15.f, 15.f);
        g.setColour (look::cvRing);                  g.fillEllipse ((float) getWidth() - 14.f, 3.f, 9.f, 9.f);
    }
}

// ================================================================ selecteur / interrupteurs
SegmentSelector::SegmentSelector (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, juce::StringArray names)
    : labels (std::move (names)),
      attachment (*apvts.getParameter (id), [this] (float v) { selected = juce::roundToInt (v); repaint(); })
{
    attachment.sendInitialUpdate();
}

void SegmentSelector::paint (juce::Graphics& g)
{
    const int n = labels.size();
    const float w = (float) getWidth() / (float) juce::jmax (1, n);
    for (int i = 0; i < n; ++i)
    {
        auto r = juce::Rectangle<float> (w * (float) i, 0.f, w, (float) getHeight()).reduced (2.f);
        const bool on = (i == selected);
        g.setGradientFill (juce::ColourGradient (on ? juce::Colour (0xff0d0d0e) : juce::Colour (0xff2b2b2e), r.getX(), r.getY(),
                                                 on ? juce::Colour (0xff1c1c1e) : juce::Colour (0xff111112), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 3.f);
        g.setColour (juce::Colour (0xff8a8a90).withAlpha (0.6f));
        g.drawRoundedRectangle (r, 3.f, 1.f);
        const auto led = juce::Rectangle<float> (6.f, 6.f).withCentre ({ r.getCentreX(), r.getY() + 6.f });
        if (on) { g.setColour (juce::Colour (0xffff7a1a).withAlpha (0.35f)); g.fillEllipse (led.expanded (3.f)); }
        g.setColour (on ? juce::Colour (0xffff9a3a) : juce::Colour (0xff3a1408));
        g.fillEllipse (led);
        g.setColour (on ? look::print : look::print.withAlpha (0.6f));
        const auto glyphArea = r.withTrimmedTop (13.f).withTrimmedBottom (r.getHeight() > 36.f ? 11.f : 0.f).reduced (7.f, 2.f);
        if (r.getHeight() > 36.f && drawGlyph (g, labels[i], glyphArea))
        {
            g.setFont (font (10.f));
            g.drawText (labels[i], r.withTrimmedTop (r.getHeight() - 12.f), juce::Justification::centred, false);
        }
        else
        {
            g.setFont (font (11.f));
            g.drawText (labels[i], r.withTrimmedTop (10.f), juce::Justification::centred, false);
        }
    }
}

void SegmentSelector::mouseDown (const juce::MouseEvent& e)
{
    const int n = labels.size();
    attachment.setValueAsCompleteGesture ((float) juce::jlimit (0, n - 1, (int) (e.position.x / ((float) getWidth() / (float) n))));
}

void LedToggle::paintButton (juce::Graphics& g, bool, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (2.f);
    const bool on = getToggleState();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2b2b2e), r.getX(), r.getY(), juce::Colour (0xff111112), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (down ? r.reduced (0.6f) : r, 3.f);
    g.setColour (juce::Colour (0xff8a8a90).withAlpha (0.6f));
    g.drawRoundedRectangle (r, 3.f, 1.f);
    const auto led = juce::Rectangle<float> (7.f, 7.f).withCentre ({ r.getX() + 11.f, r.getCentreY() });
    if (on) { g.setColour (juce::Colour (0xffff7a1a).withAlpha (0.35f)); g.fillEllipse (led.expanded (3.f)); }
    g.setColour (on ? juce::Colour (0xffff9a3a) : juce::Colour (0xff3a1408));
    g.fillEllipse (led);
    g.setColour (on ? look::print : look::print.withAlpha (0.6f));
    g.setFont (font (11.5f));
    g.drawText (getButtonText(), r.withTrimmedLeft (22.f), juce::Justification::centredLeft, false);
}

void FootSwitch::paintButton (juce::Graphics& g, bool, bool down)
{
    auto b = getLocalBounds().toFloat();
    const bool on = getToggleState();
    const auto led = juce::Rectangle<float> (b.getCentreX() - 6.f, b.getY() + 2.f, 12.f, 12.f);
    if (on) { g.setColour (juce::Colour (0xffff2a1a).withAlpha (0.3f)); g.fillEllipse (led.expanded (6.f)); }
    g.setColour (on ? juce::Colour (0xffff3b2a) : juce::Colour (0xff3a0c08));
    g.fillEllipse (led);
    g.setColour (look::print.withAlpha (0.6f));
    g.drawEllipse (led, 1.f);
    const float d = juce::jmin (b.getWidth() - 8.f, b.getHeight() - 22.f) * (down ? 0.93f : 1.f);
    auto s = juce::Rectangle<float> (d, d).withCentre ({ b.getCentreX(), b.getY() + 20.f + (b.getHeight() - 20.f) * 0.5f });
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillEllipse (s.translated (2.f, 4.f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd9d9dc), s.getX(), s.getY(), juce::Colour (0xff4b4b51), s.getRight(), s.getBottom(), true));
    g.fillEllipse (s);
    g.setColour (juce::Colour (0xff202022));
    g.drawEllipse (s.reduced (d * 0.2f), 2.f);
    g.setColour (juce::Colour (0xff111111));
    g.setFont (font (12.5f));
    g.drawText ("ON", s, juce::Justification::centred, false);
}

// ================================================================ vumeter a aiguille (patine, papier jauni)
void VuMeter::tick()
{
    shown += (target > shown ? 0.45f : 0.12f) * (target - shown);          // balistique : montee rapide, retombee lente
    if (std::fabs (shown - lastDrawn) > 0.03f) { lastDrawn = shown; repaint(); }
}

void VuMeter::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (1.f);
    juce::Graphics::ScopedSaveState ss (g);
    juce::Path clip; clip.addRoundedRectangle (b, 4.f);
    g.reduceClipRegion (clip);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffddd3ab), b.getX(), b.getY(), juce::Colour (0xff9a8e66), b.getX(), b.getBottom(), false));
    g.fillRect (b);
    juce::Random rr (77);
    for (int k = 0; k < 70; ++k)                                                // taches de vieillissement
    {
        g.setColour (juce::Colour (0xff5a4a28).withAlpha (0.03f + 0.07f * rr.nextFloat()));
        const float s = 2.f + rr.nextFloat() * 7.f;
        g.fillEllipse (b.getX() + rr.nextFloat() * b.getWidth(), b.getY() + rr.nextFloat() * b.getHeight(), s, s);
    }

    const juce::Point<float> pivot (b.getCentreX(), b.getBottom() + 14.f);
    const float R = b.getHeight() * 0.98f + 6.f, a0 = -0.62f, a1 = 0.62f;
    auto dir = [] (float a) { return juce::Point<float> (std::sin (a), -std::cos (a)); };

    for (int i = 0; i <= 24; ++i)                                               // graduations 0..24 dB
    {
        const float a = a0 + (a1 - a0) * (float) i / 24.f, len = (i % 6 == 0) ? 9.f : 5.f;
        const auto p1 = pivot + dir (a) * (R - len), p2 = pivot + dir (a) * R;
        g.setColour (i >= 18 ? juce::Colour (0xffa81c10) : juce::Colour (0xff2a2418));
        g.drawLine (p1.x, p1.y, p2.x, p2.y, (i % 6 == 0) ? 1.6f : 1.f);
        if (i % 6 == 0)
        {
            const auto t = pivot + dir (a) * (R - 19.f);
            g.setFont (font (10.5f));
            g.drawText (juce::String (i), (int) t.x - 10, (int) t.y - 6, 20, 12, juce::Justification::centred, false);
        }
    }
    g.setColour (juce::Colour (0xff2a2418));
    g.setFont (font (10.f));
    g.drawText ("COMPRESSION", b.withTrimmedTop (3.f).withHeight (12.f), juce::Justification::centred, false);
    g.drawText ("dB", b.withTrimmedLeft (6.f).withTrimmedTop (b.getHeight() - 15.f), juce::Justification::centredLeft, false);

    const float v = juce::jlimit (0.f, 25.f, shown), a = a0 + (a1 - a0) * v / 24.f;
    const auto tip = pivot + dir (a) * (R - 3.f);
    g.setColour (juce::Colours::black.withAlpha (0.25f));                        // ombre de l'aiguille
    g.drawLine (pivot.x + 2.f, pivot.y + 1.f, tip.x + 2.f, tip.y + 1.f, 1.6f);
    g.setColour (juce::Colour (0xff101010));
    g.drawLine (pivot.x, pivot.y, tip.x, tip.y, 1.6f);

    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.20f), b.getX(), b.getY(),      // reflet du verre
                                             juce::Colours::white.withAlpha (0.f), b.getX() + b.getWidth() * 0.6f, b.getBottom(), false));
    g.fillRect (b);
    g.setColour (juce::Colour (0xff1b1b1d));
    g.drawRoundedRectangle (b, 4.f, 3.f);
}

void LedLamp::paint (juce::Graphics& g)
{
    const auto led = juce::Rectangle<float> (14.f, 14.f).withCentre ({ (float) getWidth() * 0.5f, 10.f });
    if (lit) { g.setColour (juce::Colour (0xff5be36b).withAlpha (0.35f)); g.fillEllipse (led.expanded (5.f)); }
    g.setColour (lit ? juce::Colour (0xff6bff7a) : juce::Colour (0xff10260f));
    g.fillEllipse (led);
    g.setColour (look::print.withAlpha (0.6f));
    g.drawEllipse (led, 1.f);
    g.setColour (look::print.withAlpha (0.9f));
    g.setFont (font (10.5f));
    g.drawText (text, 0, 22, getWidth(), 12, juce::Justification::centred, false);
}

// ================================================================ boite de dialogue integree
OrageDialog::OrageDialog (juce::String t, juce::String m, bool askText, juce::String initialText, juce::String okText,
                          std::function<void (bool, const juce::String&)> callback)
    : titleText (std::move (t)), messageText (std::move (m)), hasText (askText), cb (std::move (callback))
{
    setWantsKeyboardFocus (true);
    okBtn.setButtonText (okText);
    cancelBtn.setButtonText ("ANNULER");
    okBtn.onClick = [this] { finish (true); };
    cancelBtn.onClick = [this] { finish (false); };
    addAndMakeVisible (okBtn);
    addAndMakeVisible (cancelBtn);
    if (hasText)
    {
        editor.setText (initialText, false);
        editor.setFont (font (16.f));
        editor.setSelectAllWhenFocused (true);
        editor.setIndents (8, 6);
        editor.onReturnKey = [this] { finish (true); };
        editor.onEscapeKey = [this] { finish (false); };
        addAndMakeVisible (editor);
    }
}

void OrageDialog::focus()
{
    if (hasText) editor.grabKeyboardFocus(); else grabKeyboardFocus();
}

bool OrageDialog::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::escapeKey) { finish (false); return true; }
    if (k == juce::KeyPress::returnKey) { finish (true); return true; }
    return true;                                                      // modal : rien ne passe a travers
}

void OrageDialog::finish (bool ok)
{
    if (done) return;
    done = true;
    const auto text = editor.getText();
    setVisible (false);
    if (cb) cb (ok, text);
    if (onFinished) onFinished();
}

void OrageDialog::resized()
{
    const int w = 520, h = hasText ? 230 : 200;
    box = juce::Rectangle<int> (w, h).withCentre (getLocalBounds().getCentre());
    if (hasText) editor.setBounds (box.getX() + 28, box.getY() + 96, w - 56, 34);
    cancelBtn.setBounds (box.getRight() - 28 - 140 - 10 - 140, box.getBottom() - 28 - 34, 140, 34);
    okBtn.setBounds (box.getRight() - 28 - 140, box.getBottom() - 28 - 34, 140, 34);
}

void OrageDialog::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.62f));
    const auto b = box.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRect (b.translated (4.f, 6.f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1c1c1f), 0.f, b.getY(), juce::Colour (0xff0d0d0f), 0.f, b.getBottom(), false));
    g.fillRect (b);
    g.setColour (look::print.withAlpha (0.8f));
    g.drawRect (b, 1.6f);
    g.drawRect (b.reduced (6.f), 0.8f);
    for (auto p : { juce::Point<float> (b.getX() + 15.f, b.getY() + 15.f), { b.getRight() - 15.f, b.getY() + 15.f },
                    { b.getX() + 15.f, b.getBottom() - 15.f }, { b.getRight() - 15.f, b.getBottom() - 15.f } })
    {
        g.setColour (juce::Colour (0xff8a8a90)); g.fillEllipse (p.x - 4.f, p.y - 4.f, 8.f, 8.f);
        g.setColour (juce::Colour (0xff2a2a2d)); g.drawLine (p.x - 3.f, p.y - 1.f, p.x + 3.f, p.y + 1.f, 1.2f);
    }
    g.setColour (look::print);
    g.setFont (font (16.f));
    g.drawText (titleText, box.getX() + 32, box.getY() + 22, box.getWidth() - 64, 22, juce::Justification::centredLeft, false);
    g.setColour (juce::Colour (0xffc8281c));
    g.fillRect (box.getX() + 32, box.getY() + 48, 60, 3);
    g.setColour (look::print.withAlpha (0.9f));
    g.setFont (font (13.f, false));
    g.drawFittedText (messageText, box.getX() + 32, box.getY() + 60, box.getWidth() - 64, hasText ? 30 : 70, juce::Justification::topLeft, 3);
    drawSpecks (g, box);
}
