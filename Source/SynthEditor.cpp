#include "SynthEditor.h"

using namespace zephyr;

namespace { juce::Font bold (float h) { return juce::Font (juce::FontOptions (h, juce::Font::bold)); } }

// ---------------------------------------------------------------- flancs en bois (veinage procedural)
void WoodCheek::paint (juce::Graphics& g)
{
    const int W = getWidth(), H = getHeight();
    if (W <= 0 || H <= 0) return;
    if (cache.isNull() || cachedTheme != look::themeIdx || cache.getWidth() != W || cache.getHeight() != H)
    {
        cachedTheme = look::themeIdx;
        const auto& th = look::T();
        const juce::Colour a (th.woodA), b (th.woodB);
        cache = juce::Image (juce::Image::RGB, W, H, false);
        juce::Graphics wg (cache);
        wg.setGradientFill (juce::ColourGradient (a, 0.f, 0.f, b, (float) W, 0.f, false));
        wg.fillAll();
        juce::Random rr (left ? 1234 : 5678);
        for (int k = 0; k < 26; ++k)                                                    // fibres longues, legerement ondulees
        {
            const float x0 = rr.nextFloat() * (float) W, amp = 0.6f + rr.nextFloat() * 2.2f, freq = 0.004f + rr.nextFloat() * 0.01f, ph = rr.nextFloat() * 6.28f;
            juce::Path p;
            for (int y = 0; y <= H; y += 6)
            {
                const float x = x0 + amp * std::sin ((float) y * freq + ph) + 1.2f * std::sin ((float) y * freq * 3.1f + ph * 2.f);
                if (y == 0) p.startNewSubPath (x, (float) y); else p.lineTo (x, (float) y);
            }
            const bool dark = rr.nextFloat() < 0.6f;
            wg.setColour ((dark ? b.darker (0.5f) : a.brighter (0.25f)).withAlpha (0.18f + 0.25f * rr.nextFloat()));
            wg.strokePath (p, juce::PathStrokeType (0.6f + rr.nextFloat() * 1.6f));
        }
        for (int k = 0; k < 3; ++k)                                                     // quelques noeuds allonges
        {
            const float cx = (float) W * (0.25f + 0.5f * rr.nextFloat()), cy = (float) H * rr.nextFloat();
            for (int r = 0; r < 5; ++r)
            {
                wg.setColour (b.darker (0.4f).withAlpha (0.18f));
                wg.drawEllipse (cx - 2.f - (float) r * 1.6f, cy - 9.f - (float) r * 7.f, 4.f + (float) r * 3.2f, 18.f + (float) r * 14.f, 0.8f);
            }
        }
        wg.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.16f), left ? (float) W * 0.3f : (float) W * 0.7f, 0.f,       // vernis : reflet vertical
                                                  juce::Colours::white.withAlpha (0.f), left ? (float) W : 0.f, 0.f, false));
        wg.fillAll();
        wg.setColour (juce::Colours::black.withAlpha (0.35f));                           // chanfreins
        wg.drawVerticalLine (left ? W - 1 : 0, 0.f, (float) H);
        wg.setColour (juce::Colours::white.withAlpha (0.12f));
        wg.drawVerticalLine (left ? 0 : W - 1, 0.f, (float) H);
        wg.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.35f), 0.f, 0.f, juce::Colours::transparentBlack, 0.f, 26.f, false));
        wg.fillRect (0, 0, W, 26);
        wg.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, 0.f, (float) H - 26.f, juce::Colours::black.withAlpha (0.35f), 0.f, (float) H, false));
        wg.fillRect (0, H - 26, W, 26);
    }
    g.drawImageAt (cache, 0, 0);
}

// ---------------------------------------------------------------- volet de la matrice
namespace
{
struct Fam { const char* title; std::vector<int> items; std::vector<Fam> subs {}; };
// Les menus sont groupes par famille : l'ordre d'affichage n'est pas l'ordre du parametre. L'attachement standard de JUCE se fonde sur la
// position dans la liste ; celui-ci se fonde sur l'identifiant (id = index du parametre + 1).
struct IdComboAttachment
{
    IdComboAttachment (juce::AudioProcessorValueTreeState& a, const juce::String& pid, juce::ComboBox& c)
        : cb (c), att (*a.getParameter (pid), [this] (float v) { cb.setSelectedId (juce::roundToInt (v) + 1, juce::dontSendNotification); })
    {
        att.sendInitialUpdate();
        cb.onChange = [this] { const int id = cb.getSelectedId(); if (id > 0) att.setValueAsCompleteGesture ((float) (id - 1)); };
    }
    ~IdComboAttachment() { cb.onChange = nullptr; }
    juce::ComboBox& cb; juce::ParameterAttachment att;
};
void addFam (juce::PopupMenu& menu, const Fam& f, const char* const* names)
{
    for (int i : f.items) menu.addItem (i + 1, names[i]);                          // id = index du parametre + 1
    for (const auto& sf : f.subs) { juce::PopupMenu sub; addFam (sub, sf, names); menu.addSubMenu (sf.title, sub); }
}
// menus en sous-menus par module (plus de defilement interminable)
void fillGrouped (juce::ComboBox& cb, const std::vector<Fam>& fams, const char* const* names)
{
    auto* root = cb.getRootMenu();
    root->addItem (1, "-");
    for (const auto& f : fams) { juce::PopupMenu sub; addFam (sub, f, names); root->addSubMenu (f.title, sub); }
}
const std::vector<Fam>& srcFamilies()
{
    static const std::vector<Fam> f { { "LFO", { MsLfo1, MsLfo2, MsLfo3, MsLfo4 } }, { "MACROS", { MsMac1, MsMac2, MsMac3, MsMac4, MsMac5, MsMac6, MsMac7, MsMac8 } },
                                      { "ENVELOPES", { MsEnv, MsAmpEnv } }, { "PERFORMANCE", { MsVel, MsWheel, MsKey, MsChanAT, MsPolyAT } }, { "RANDOM", { MsRandom } },
                                      { "ARP LANES", { MsArp1, MsArp2, MsArp3, MsArp4, MsArp5, MsArp6, MsArp7, MsArp8 } } };
    return f;
}
const std::vector<Fam>& dstFamilies()
{
    static const std::vector<Fam> f {
        { "OSCILLATORS", { MdPitch, MdPitch2, MdPw1, MdPw2, MdLvl1, MdMix, MdSub, MdNoise, MdGlide, MdDrift } },
        { "FM / RING / POLY-MOD / FOLD", { MdFm, MdRing, MdPolyMod, MdFold, MdFoldSym } },
        { "FILTER", { MdHpf, MdHpfReso, MdLpf, MdReso, MdMorph, MdFltEnv, MdSat, MdKeyTrack } },
        { "ENVELOPES", {}, { { "AMP ENVELOPE", { MdAmpA, MdAmpD, MdAmpS, MdAmpR } }, { "FILTER ENVELOPE", { MdFltA, MdFltD, MdFltS, MdFltR } } } },
        { "LFO", { MdLfo1Rate, MdLfo2Rate, MdLfo3Rate, MdLfo4Rate } },
        { "ARPEGGIATOR", { MdArpRate, MdArpGate, MdArpSwing, MdArpChance } },
        { "OUTPUT", { MdLevel, MdPan } },
        { "FX RACK", { MdFxMix }, {
            { "STEREO WIDE", { MdWideWidth, MdWideSpread } },
            { "SATURATION", { MdSatDrive, MdSatTone } },
            { "EQ", { MdEqLow, MdEqMid, MdEqAir } },
            { "FILTER", { MdFxHpf, MdFxLpf, MdFxPeak } },
            { "PHASER", { MdPhMix, MdPhRate, MdPhFb } },
            { "COMP", { MdCompAmt } },
            { "ENSEMBLE", { MdEnsMix, MdEnsDepth, MdEnsSpeed } },
            { "STUTTER", { MdStutOn, MdStutMode, MdStutDiv, MdStutFeel, MdStutLen, MdStutDecay, MdStutPitch, MdStutMix } },
            { "DELAY", { MdDlyTime, MdDlyFb, MdDlyMix, MdDlyTone, MdDlyMod } },
            { "REVERB", { MdRevMix, MdRevDecay, MdRevDamp, MdRevSize, MdRevPre, MdRevShimmer, MdRevWidth } } } } };
    return f;
}
}

// ---------------------------------------------------------------- apercu de la forme d'onde du LFO
float LfoView::value (const juce::String& id) const
{
    if (auto* p = apvts.getRawParameterValue (id)) return p->load();
    return 0.f;
}

void LfoView::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff050506));
    g.fillRect (b);
    g.setColour (juce::Colour (0xff3a1c10));
    for (int i = 1; i < 4; ++i) g.drawHorizontalLine ((int) (b.getY() + b.getHeight() * (float) i / 4.f), b.getX() + 2.f, b.getRight() - 2.f);
    for (int i = 1; i < 4; ++i) g.drawVerticalLine ((int) (b.getX() + b.getWidth() * (float) i / 4.f), b.getY() + 2.f, b.getBottom() - 2.f);
    const int w = juce::roundToInt (value (wave));
    const float sl = value (slew);
    static const float steps[4] { 0.55f, -0.7f, 0.25f, -0.25f };                // valeurs types d'un S&H, un cycle = 4 marches
    juce::Path p;
    const int n = (int) b.getWidth();
    for (int i = 0; i <= n; ++i)
    {
        const float ph = (float) i / (float) n * 0.9999f;
        float y;
        if (w == 4)
        {
            const float pos = ph * 4.f;
            const int k = juce::jlimit (0, 3, (int) pos);
            const float frac = pos - (float) k, r = juce::jmax (0.001f, sl * 0.9f);
            const float t = juce::jlimit (0.f, 1.f, (frac - (1.f - r)) / r);
            y = steps[k] + (steps[(k + 1) % 4] - steps[k]) * t * t * (3.f - 2.f * t);
        }
        else
            y = zephyr::Synth::lfoWave (ph, w, 0.f);
        const float px = b.getX() + (float) i, py = b.getCentreY() - y * (b.getHeight() * 0.38f);
        if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
    }
    g.setColour (juce::Colour (0xffff7a1a).withAlpha (0.3f));
    g.strokePath (p, juce::PathStrokeType (4.f));
    g.setColour (juce::Colour (0xffffb05a));
    g.strokePath (p, juce::PathStrokeType (1.6f));
    g.setColour (juce::Colour (0xffe9e5d8).withAlpha (0.6f));
    g.drawRect (b, 1.2f);
}


// ---------------------------------------------------------------- sequenceur : etiquettes, grille de pas, pistes de modulation
void Tag::paint (juce::Graphics& g)
{
    g.setColour (look::print.withAlpha (0.65f));
    g.setFont (bold (size));
    g.drawText (text, getLocalBounds(), just, true);
}

namespace
{
int paramInt (juce::AudioProcessorValueTreeState& a, const juce::String& id) { if (auto* p = a.getRawParameterValue (id)) return juce::roundToInt (p->load()); return 0; }
juce::Colour ledCol() { return juce::Colour (look::T().led); }
void drawWell (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colour (look::T().screenBg)); g.fillRoundedRectangle (r, 3.f);
    g.setColour (juce::Colour (look::T().accent).withAlpha (0.55f)); g.drawRoundedRectangle (r.reduced (0.5f), 3.f, 1.f);
}
}

StepGrid::StepGrid (ZephyrAudioProcessor& p) : proc (p) { startTimerHz (20); }

void StepGrid::timerCallback()
{
    const int c = proc.arpStepShown.load(), r = paramInt (proc.apvts, "s_seqRec") * 100 + proc.arpPat.recPos.load() + 1000 * paramInt (proc.apvts, "s_arpType");
    if (c != shown || r != recSig) { shown = c; recSig = r; repaint(); }
}

namespace
{
juce::String noteName (int n)
{
    static const char* const nm[12] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return juce::String (nm[n % 12]) + juce::String (n / 12 - 2);              // do central (60) = C3, comme dans Ableton
}
}

void StepGrid::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (paramInt (proc.apvts, "s_arpType") != 1 || (float) e.y > (float) getHeight() * 0.64f) return;
    const int k = juce::jlimit (0, 15, (int) ((float) e.x / ((float) getWidth() / 16.f)));
    const int d = (w.deltaY > 0 ? 1 : -1) * (e.mods.isShiftDown() ? 12 : 1);
    proc.arpPat.seqNote[k] = juce::jlimit (0, 127, proc.arpPat.seqNote[k].load() + d);
    repaint();
}

void StepGrid::paint (juce::Graphics& g)
{
    const float cw = (float) getWidth() / 16.f, mainH = (float) getHeight() * 0.64f;
    const int len = paramInt (proc.apvts, "s_arpLen") + 1;
    const int cur = shown >= 0 ? shown % len : -1;
    const bool seqType = paramInt (proc.apvts, "s_arpType") == 1, recOn = seqType && paramInt (proc.apvts, "s_seqRec") != 0;
    const int recAt = proc.arpPat.recPos.load();
    for (int k = 0; k < 16; ++k)
    {
        const float dim = k < len ? 1.f : 0.3f;
        auto cell = juce::Rectangle<float> ((float) k * cw + 2.f, 2.f, cw - 4.f, mainH - 4.f);
        const int st = proc.arpPat.step[k].load(), rt = proc.arpPat.ratchet[k].load();
        drawWell (g, cell);
        if (st > 0)
        {
            auto lit = cell.reduced (3.f);
            g.setGradientFill (juce::ColourGradient (ledCol().brighter (st == 2 ? 0.5f : 0.f).withAlpha (dim), lit.getX(), lit.getY(),
                                                     ledCol().darker (st == 2 ? 0.1f : 0.6f).withAlpha (dim), lit.getX(), lit.getBottom(), false));
            g.fillRoundedRectangle (lit, 2.f);
            if (st == 2) { g.setColour (ledCol().withAlpha (0.25f * dim)); g.fillRoundedRectangle (cell.expanded (2.f), 4.f); g.setColour (juce::Colours::white.withAlpha (0.5f * dim)); g.fillRect (lit.getCentreX() - 1.5f, lit.getY() + 4.f, 3.f, lit.getHeight() - 8.f); }
        }
        g.setColour (look::print.withAlpha (0.55f * dim)); g.setFont (bold (9.f));
        g.drawText (juce::String (k + 1), cell.toNearestInt().withTrimmedTop (juce::roundToInt (cell.getHeight()) - 12), juce::Justification::centred, false);
        if (seqType && st > 0)
        {
            g.setColour (juce::Colours::black.withAlpha (0.85f * dim)); g.setFont (bold (13.f));
            g.drawText (noteName (proc.arpPat.seqNote[k].load()), cell.toNearestInt().withTrimmedBottom (10), juce::Justification::centred, false);
            if (proc.arpPat.tie[k].load() != 0) { g.setColour (juce::Colours::black.withAlpha (0.8f * dim)); g.fillRoundedRectangle (cell.getRight() - 9.f, cell.getCentreY() - 3.f, 12.f, 6.f, 2.f); }
        }
        // ratchet : 1 a 4 petits points
        auto rc = juce::Rectangle<float> ((float) k * cw + 2.f, mainH + 2.f, cw - 4.f, (float) getHeight() - mainH - 4.f);
        drawWell (g, rc);
        for (int d = 0; d < 4; ++d)
        {
            const float px = rc.getX() + rc.getWidth() * (0.2f + 0.2f * (float) d), py = rc.getCentreY();
            g.setColour (d < rt && st > 0 ? ledCol().withAlpha (dim) : look::print.withAlpha (0.12f * dim));
            g.fillEllipse (px - 3.f, py - 3.f, 6.f, 6.f);
        }
        if (k == cur) { g.setColour (juce::Colours::white.withAlpha (0.85f)); g.drawRoundedRectangle (juce::Rectangle<float> ((float) k * cw + 1.f, 1.f, cw - 2.f, (float) getHeight() - 2.f), 3.f, 1.6f); }
        if (recOn && k == recAt) { g.setColour (juce::Colour (0xffff4a3a)); g.drawRoundedRectangle (juce::Rectangle<float> ((float) k * cw + 1.f, 1.f, cw - 2.f, (float) getHeight() - 2.f), 3.f, 2.4f); }
    }
}

void StepGrid::apply (const juce::MouseEvent& e, bool first)
{
    const float cw = (float) getWidth() / 16.f, mainH = (float) getHeight() * 0.64f;
    const int k = juce::jlimit (0, 15, (int) ((float) e.x / cw));
    if (first) ratchetRow = (float) e.y > mainH;
    if (ratchetRow)
    {
        if (first) { const int r = proc.arpPat.ratchet[k].load(); paintState = e.mods.isPopupMenu() ? 1 : (r % 4) + 1; }
        proc.arpPat.ratchet[k] = paintState;
    }
    else
    {
        if (first) paintState = e.mods.isPopupMenu() ? 0 : (proc.arpPat.step[k].load() + 1) % 3;
        proc.arpPat.step[k] = paintState;
    }
    repaint();
}
void StepGrid::mouseDown (const juce::MouseEvent& e) { apply (e, true); }
void StepGrid::mouseDrag (const juce::MouseEvent& e) { apply (e, false); }

void StepGrid::randomize()
{
    juce::Random r;
    for (int k = 0; k < 16; ++k)
    {
        const float x = r.nextFloat();
        proc.arpPat.step[k] = x < 0.22f ? 0 : (x < 0.42f ? 2 : 1);
        proc.arpPat.ratchet[k] = r.nextFloat() < 0.18f ? 2 + r.nextInt (3) : 1;
    }
    repaint();
}
void StepGrid::clear() { for (int k = 0; k < 16; ++k) { proc.arpPat.step[k] = 1; proc.arpPat.ratchet[k] = 1; } repaint(); }

LaneEditor::LaneEditor (ZephyrAudioProcessor& p, int l) : proc (p), lane (l) { startTimerHz (20); }

void LaneEditor::paint (juce::Graphics& g)
{
    const float cw = (float) getWidth() / 16.f, H = (float) getHeight();
    const int len = paramInt (proc.apvts, "s_lane" + juce::String (lane + 1) + "Len") + 1;
    const int cur = shown >= 0 ? shown % len : -1;
    for (int k = 0; k < 16; ++k)
    {
        const float dim = k < len ? 1.f : 0.3f, v = proc.arpPat.lane[lane][k].load();
        auto cell = juce::Rectangle<float> ((float) k * cw + 2.f, 2.f, cw - 4.f, H - 4.f);
        drawWell (g, cell);
        g.setColour (look::print.withAlpha (0.06f * dim));
        for (int q = 1; q < 4; ++q) g.fillRect (cell.getX() + 2.f, cell.getY() + cell.getHeight() * (float) q / 4.f, cell.getWidth() - 4.f, 1.f);
        if (v > 0.001f)
        {
            auto bar = cell.reduced (3.f); bar = bar.withTop (bar.getBottom() - bar.getHeight() * v);
            g.setGradientFill (juce::ColourGradient (ledCol().brighter (0.3f).withAlpha (dim), bar.getX(), bar.getY(), ledCol().darker (0.6f).withAlpha (dim), bar.getX(), bar.getBottom(), false));
            g.fillRoundedRectangle (bar, 2.f);
            g.setColour (juce::Colours::white.withAlpha (0.55f * dim)); g.fillRect (bar.getX(), bar.getY(), bar.getWidth(), 1.5f);
        }
        if (k == cur) { g.setColour (juce::Colours::white.withAlpha (0.85f)); g.drawRoundedRectangle (juce::Rectangle<float> ((float) k * cw + 1.f, 1.f, cw - 2.f, H - 2.f), 3.f, 1.6f); }
    }
}

void LaneEditor::apply (const juce::MouseEvent& e)
{
    const float cw = (float) getWidth() / 16.f;
    const int k = juce::jlimit (0, 15, (int) ((float) e.x / cw));
    float v = 1.f - (float) (e.y - 5) / (float) (getHeight() - 10);
    v = juce::jlimit (0.f, 1.f, v);
    if (v < 0.04f) v = 0.f;
    proc.arpPat.lane[lane][k] = e.mods.isPopupMenu() ? 0.f : v;
    repaint();
}
void LaneEditor::randomize() { juce::Random r; for (int k = 0; k < 16; ++k) proc.arpPat.lane[lane][k] = r.nextFloat() < 0.15f ? 0.f : r.nextFloat(); repaint(); }
void LaneEditor::clear() { for (int k = 0; k < 16; ++k) proc.arpPat.lane[lane][k] = 0.f; repaint(); }

// ---------------------------------------------------------------- panneau synthe
KnobUnit* SynthPanel::knob (juce::Component& parent, const char* id, const juce::String& text, const char* lo, const char* hi)
{
    knobs.push_back (std::make_unique<KnobUnit> (proc.apvts, id, text, lo, hi));
    auto* k = knobs.back().get();
    parent.addAndMakeVisible (*k);
    return k;
}

ModuleUnit* SynthPanel::unit (const juce::String& title)
{
    units.push_back (std::make_unique<ModuleUnit> (nullptr, -1, title, 100, true));
    addAndMakeVisible (*units.back());
    return units.back().get();
}

SynthPanel::SynthPanel (ZephyrAudioProcessor& p, int pg) : proc (p), page (pg)
{
    auto selector = [this] (juce::Component& parent, const char* id, const juce::StringArray& names)
    {
        auto s = std::make_unique<SegmentSelector> (proc.apvts, id, names);
        auto* r = s.get(); parent.addAndMakeVisible (*r); owned.push_back (std::move (s)); return r;
    };
    auto toggle = [this] (juce::Component& parent, const char* id, const char* label)
    {
        auto b = std::make_unique<LedToggle>();
        b->setButtonText (label);
        parent.addAndMakeVisible (*b);
        buttonAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, *b));
        auto* r = b.get(); owned.push_back (std::move (b)); return r;
    };
    auto combo = [this] (juce::Component& parent, const char* id)
    {
        auto cb = std::make_unique<juce::ComboBox>();
        if (auto* ch = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id))) cb->addItemList (ch->choices, 1);
        cb->setScrollWheelEnabled (false);
        parent.addAndMakeVisible (*cb);
        comboAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, id, *cb));
        auto* r = cb.get(); owned.push_back (std::move (cb)); return r;
    };
    // potards alignes verticalement au centre de la zone, a taille fixe
    auto place = [] (juce::Rectangle<int> r, std::vector<KnobUnit*> ks, int x0, int cellW)
    {
        cellW = juce::jmin (cellW, (r.getWidth() - x0) / juce::jmax (1, (int) ks.size()));     // jamais plus large que le module
        int x = r.getX() + x0;
        const int kh = juce::jmin (r.getHeight(), cellW + 28), y = r.getY() + (r.getHeight() - kh) / 2;
        for (auto* k : ks) { k->setBounds (x, y, cellW, kh); x += cellW; }
    };
    const juce::StringArray waves { "SAW", "PULSE", "TRI", "SIN" }, octs { "-2", "-1", "0", "+1", "+2" };
    const juce::StringArray lfoWaves { "SIN", "TRI", "SAW", "SQR", "S&H" };


    // ================================================================ page ARP : arpegiateur / sequenceur + 4 pistes de modulation
    if (page == 1)
    {
        auto& idAtt = idAttachments;
        auto tag = [this] (juce::Component& parent, const juce::String& t, juce::Justification j = juce::Justification::centredLeft, float h = 9.f)
        { auto c = std::make_unique<Tag> (t, j, h); auto* r = c.get(); parent.addAndMakeVisible (*r); owned.push_back (std::move (c)); return r; };
        auto button = [this] (juce::Component& parent, const juce::String& t, std::function<void()> f)
        { auto b = std::make_unique<juce::TextButton> (t); b->onClick = std::move (f); parent.addAndMakeVisible (*b); auto* r = b.get(); owned.push_back (std::move (b)); return r; };
        auto dstCombo = [this, &idAtt] (juce::Component& parent, const char* id)
        {
            auto cb = std::make_unique<juce::ComboBox>();
            fillGrouped (*cb, dstFamilies(), kModDstNames);
            cb->setScrollWheelEnabled (false);
            parent.addAndMakeVisible (*cb);
            idAtt.push_back (std::make_shared<IdComboAttachment> (proc.apvts, id, *cb));
            auto* r = cb.get(); owned.push_back (std::move (cb)); return r;
        };
        // ---- rang 1 : arpegiateur et horloge (meme ordre que la face avant de l'Hydrasynth)
        {
            auto* u = unit ("ARPEGGIATOR  /  SEQUENCER");
            auto* on = selector (*u, "s_arpOn", juce::StringArray { "OFF", "ON", "LATCH" });
            auto* trip = toggle (*u, "s_arpTriplet", "TRIPLET");
            auto* tap = button (*u, "TAP TEMPO", [this]
            {
                static double last = 0.0, avg = 0.0;
                const double now = juce::Time::getMillisecondCounterHiRes() * 0.001, dt = now - last; last = now;
                if (dt < 0.12 || dt > 2.5) { avg = 0.0; return; }                          // premier tap ou pause trop longue : on repart
                avg = avg <= 0.0 ? dt : 0.5 * avg + 0.5 * dt;
                const double hz = juce::jlimit (0.5, 20.0, 1.0 / avg);                      // 1 tap = 1 pas ; l'horloge passe en mode libre
                if (auto* sy = proc.apvts.getParameter ("s_arpSync")) sy->setValueNotifyingHost (0.f);
                if (auto* rt = proc.apvts.getParameter ("s_arpRate")) rt->setValueNotifyingHost ((float) (std::log (hz / 0.5) / std::log (40.0)));
            });
            auto* dir = knob (*u, "s_arpMode", "MODE");  dir->setChoices ({ "UP", "DOWN", "UP-DN", "PLAYED", "RANDOM", "CONVRG" });
            auto* oct = knob (*u, "s_arpOct", "OCTAVE"); oct->setChoices ({ "1", "2", "3", "4" });
            auto* free = toggle (*u, "s_arpFree", "FREE");
            auto* shift = knob (*u, "s_arpShift", "SHIFT"); auto* clus = knob (*u, "s_arpCluster", "CLUSTER");     // mode POLY (rotation sur 4 VCO) : decalages
            auto* tp = tag (*u, "POLY : 4 VCO", juce::Justification::centred);
            auto* type = selector (*u, "s_arpType", juce::StringArray { "ARP", "SEQ" });
            auto* t1 = tag (*u, "ON / LATCH"); auto* t4 = tag (*u, "TYPE  (SEQ = one stored note per step)");
            u->layoutFn = [=] (juce::Rectangle<int> r)
            {
                const int w = r.getWidth(), kw = juce::jmin (112, (int) (w * 0.2f)), sw2 = juce::jmin (84, (int) (w * 0.15f));
                dir->setBounds (r.getX(), r.getY(), kw, r.getHeight());          oct->setBounds (r.getX() + kw + 2, r.getY(), kw, r.getHeight());
                const int sx = r.getX() + 2 * kw + 10;
                tp->setBounds (sx, r.getY(), 2 * sw2, 10);
                shift->setBounds (sx, r.getY() + 12, sw2, r.getHeight() - 12); clus->setBounds (sx + sw2, r.getY() + 12, sw2, r.getHeight() - 12);
                const int rx = sx + 2 * sw2 + 12, rw = r.getRight() - rx, y = r.getY();
                t1->setBounds (rx, y, 100, 10);                                  on->setBounds (rx, y + 12, rw, 34);
                trip->setBounds (rx, y + 52, rw / 2 - 3, 32);                    tap->setBounds (rx + rw / 2 + 3, y + 52, rw - rw / 2 - 3, 32);
                t4->setBounds (rx, y + 92, rw, 10);                              type->setBounds (rx, y + 104, rw / 2 - 3, 32);
                free->setBounds (rx + rw / 2 + 3, y + 104, rw - rw / 2 - 3, 32);
            };
        }
        {
            auto* u = unit ("CLOCK  &  FEEL");
            auto* rate = knob (*u, "s_arpRate", "TEMPO"); auto* sync = combo (*u, "s_arpSync");
            auto* rat = knob (*u, "s_arpRatchet", "RATCHET"); rat->setChoices ({ "1x", "2x", "3x", "4x" });
            auto* chance = knob (*u, "s_arpChance", "CHANCE"); auto* gate = knob (*u, "s_arpGate", "GATE"); auto* swing = knob (*u, "s_arpSwing", "SWING");
            auto* ts = tag (*u, "DIVISION");
            u->layoutFn = [=] (juce::Rectangle<int> r)
            {
                ts->setBounds (r.getX(), r.getY(), 80, 10); sync->setBounds (r.getX(), r.getY() + 12, 150, 28);
                const int cw = juce::jmin (100, (r.getWidth() - 170) / 5), x0 = r.getX() + 170;
                int x = x0; for (auto* k : { rate, rat, chance, gate, swing }) { k->setBounds (x, r.getY(), cw, r.getHeight()); x += cw; }
            };
        }
        // ---- rang 2 : pas des notes
        {
            auto* u = unit ("NOTE STEPS  (click : rest / note / accent  -  lower row : ratchet)");
            auto* len = combo (*u, "s_arpLen");
            auto* grid = new StepGrid (proc); { std::unique_ptr<juce::Component> o (grid); u->addAndMakeVisible (*grid); owned.push_back (std::move (o)); }
            auto* rnd = button (*u, "RANDOM", [grid] { grid->randomize(); }); auto* clr = button (*u, "CLEAR", [grid] { grid->clear(); });
            auto* rec = toggle (*u, "s_seqRec", "REC");
            auto stepLen = [this] { return juce::jlimit (1, 16, juce::roundToInt (proc.apvts.getRawParameterValue ("s_arpLen")->load()) + 1); };
            auto* rest = button (*u, "REST", [this, stepLen, grid] { const int p = juce::jlimit (0, 15, proc.arpPat.recPos.load()); proc.arpPat.step[p] = 0; proc.arpPat.recPos = (p + 1) % stepLen(); grid->repaint(); });
            auto* tie = button (*u, "TIE", [this, stepLen, grid] { const int p = (proc.arpPat.recPos.load() + stepLen() - 1) % stepLen(); proc.arpPat.tie[p] = 1 - proc.arpPat.tie[p].load(); grid->repaint(); });
            auto* back = button (*u, "BACK", [this, stepLen, grid] { proc.arpPat.recPos = (proc.arpPat.recPos.load() + stepLen() - 1) % stepLen(); grid->repaint(); });
            auto* t1 = tag (*u, "PATTERN LENGTH"); auto* t3 = tag (*u, "STEP RECORD  (type SEQ)");
            auto* t2 = tag (*u, "SEQ : REC, then play keys - one step per note. Wheel on a step = pitch.", juce::Justification::topLeft, 9.f);
            u->layoutFn = [=] (juce::Rectangle<int> r)
            {
                t1->setBounds (r.getX(), r.getY(), 120, 10); len->setBounds (r.getX(), r.getY() + 12, 150, 28);
                rnd->setBounds (r.getX(), r.getY() + 46, 72, 22); clr->setBounds (r.getX() + 78, r.getY() + 46, 72, 22);
                t3->setBounds (r.getX() + 164, r.getY(), 150, 10);
                rec->setBounds (r.getX() + 164, r.getY() + 12, 74, 28); rest->setBounds (r.getX() + 244, r.getY() + 14, 70, 24);
                tie->setBounds (r.getX() + 164, r.getY() + 46, 74, 22); back->setBounds (r.getX() + 244, r.getY() + 46, 70, 22);
                t2->setBounds (r.getX(), r.getY() + 74, 316, 22);
                grid->setBounds (r.getX() + 330, r.getY(), r.getWidth() - 330, r.getHeight());
            };
        }
        // ---- rang 3 : pistes de modulation
        {
            auto* u = unit ("MODULATION LANES  (8 lanes, each drives any destination, with its own length)");
            struct L { juce::ComboBox *dst, *len; KnobUnit *amt, *glide; LaneEditor* ed; juce::TextButton *rnd, *clr; Tag* tg; };
            auto lanes = std::make_shared<std::vector<L>>();
            auto page = std::make_shared<int>(0);                                 // 0 : pistes 1-4, 1 : pistes 5-8
            for (int i = 0; i < kLanes; ++i)
            {
                const juce::String q ("s_lane" + juce::String (i + 1));
                L l;
                l.dst = dstCombo (*u, (q + "Dst").toRawUTF8()); l.len = combo (*u, (q + "Len").toRawUTF8());
                l.amt = knob (*u, (q + "Amt").toRawUTF8(), "AMOUNT", "-", "+"); l.glide = knob (*u, (q + "Glide").toRawUTF8(), "GLIDE");
                l.ed = new LaneEditor (proc, i); { std::unique_ptr<juce::Component> o (l.ed); u->addAndMakeVisible (*l.ed); owned.push_back (std::move (o)); }
                auto* ed = l.ed;
                l.rnd = button (*u, "RND", [ed] { ed->randomize(); }); l.clr = button (*u, "CLR", [ed] { ed->clear(); });
                l.tg = tag (*u, "LANE " + juce::String (i + 1), juce::Justification::centredLeft, 10.f);
                lanes->push_back (l);
            }
            auto* p1 = button (*u, "LANES 1-4", nullptr); auto* p2 = button (*u, "LANES 5-8", nullptr);
            for (auto* b : { p1, p2 }) { b->setClickingTogglesState (true); b->setRadioGroupId (4711); }
            p1->setToggleState (true, juce::dontSendNotification);
            auto relayout = [u] { u->resized(); u->repaint(); };
            p1->onClick = [page, relayout] { *page = 0; relayout(); };
            p2->onClick = [page, relayout] { *page = 1; relayout(); };
            u->layoutFn = [lanes, page, p1, p2] (juce::Rectangle<int> r)
            {
                const int rh = r.getHeight() / 4;
                p1->setBounds (r.getRight() - 170, r.getY() - 25, 80, 18); p2->setBounds (r.getRight() - 86, r.getY() - 25, 80, 18);
                for (int i = 0; i < kLanes; ++i)
                {
                    auto& l = (*lanes)[(size_t) i];
                    const bool shown = (i / 4) == *page;
                    for (juce::Component* c : { (juce::Component*) l.dst, (juce::Component*) l.len, (juce::Component*) l.amt, (juce::Component*) l.glide, (juce::Component*) l.ed, (juce::Component*) l.rnd, (juce::Component*) l.clr, (juce::Component*) l.tg })
                        c->setVisible (shown);
                    if (! shown) continue;
                    const int y = r.getY() + (i % 4) * rh;
                    l.tg->setBounds (r.getX(), y + 1, 60, 12);
                    l.dst->setBounds (r.getX(), y + 13, 150, 22); l.len->setBounds (r.getX(), y + 37, 150, 22);
                    l.rnd->setBounds (r.getX(), y + 61, 72, 18); l.clr->setBounds (r.getX() + 78, y + 61, 72, 18);
                    l.amt->setBounds (r.getX() + 160, y + 4, 80, rh - 8); l.glide->setBounds (r.getX() + 240, y + 4, 80, rh - 8);
                    l.ed->setBounds (r.getX() + 330, y + 3, r.getWidth() - 330, rh - 6);
                }
            };
        }
        return;
    }

    // ---- rang A : oscillateurs, melangeur, modulation croisee (indices 0..3)
    {
        auto* u = unit ("VOLTAGE CONTROLLED OSCILLATOR 1");
        auto* w = selector (*u, "s_wave1", waves); auto* o = selector (*u, "s_oct1", octs);
        auto* pw = knob (*u, "s_pw1", "PW");
        auto* vb = knob (*u, "s_vib", "LFO AMT"); auto* vw = knob (*u, "s_vibWheel", "+ WHEEL"); auto* va = knob (*u, "s_vibAT", "+ AFTER");
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            const int sw = juce::jmin (r.getWidth(), 196);
            w->setBounds (r.getX(), r.getY() + 2, sw, 44); o->setBounds (r.getX(), r.getY() + 50, sw, 28);
            place (r.withTrimmedTop (80), { pw, vb, vw, va }, 0, juce::jmin (76, r.getWidth() / 4));
        };
    }
    {
        auto* u = unit ("VOLTAGE CONTROLLED OSCILLATOR 2");
        auto* w = selector (*u, "s_wave2", waves); auto* o = selector (*u, "s_oct2", octs);
        auto* tu = knob (*u, "s_tune2", "PITCH", "-12", "+12"); auto* fi = knob (*u, "s_fine2", "FINE", "-", "+"); auto* pw = knob (*u, "s_pw2", "PW");
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            w->setBounds (r.getX(), r.getY() + 6, 170, 46); o->setBounds (r.getX(), r.getY() + 62, 170, 34);
            place (r, { tu, fi, pw }, 180, 88);
        };
    }
    {
        auto* u = unit ("MIXER");
        auto* a = knob (*u, "s_lvl1", "VCO 1"); auto* b = knob (*u, "s_lvl2", "VCO 2"); auto* sb = knob (*u, "s_sub", "SUB");
        auto* c = knob (*u, "s_noise", "NOISE"); auto* d = knob (*u, "s_master", "VOLUME");
        u->layoutFn = [=] (juce::Rectangle<int> r) { place (r, { a, b, sb, c, d }, 0, juce::jmin (80, r.getWidth() / 5)); };
    }
    {
        auto* u = unit ("MODULATION  &  DRIFT");
        auto* sy = toggle (*u, "s_sync", "HARD SYNC");
        auto* rg = knob (*u, "s_ring", "RING"); auto* gl = knob (*u, "s_glide", "GLIDE"); auto* dr = knob (*u, "s_drift", "DRIFT");
        auto* bd = combo (*u, "s_bendRange");
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            sy->setBounds (r.getX(), r.getY() + 6, 120, 30);
            bd->setBounds (r.getX() + 128, r.getY() + 8, juce::jmin (150, r.getWidth() - 128), 26);
            place (r.withTrimmedTop (38), { rg, gl, dr }, 0, juce::jmin (96, r.getWidth() / 3));
        };
    }

    // ---- rang B : filtres et enveloppes (indices 4..7)
    {
        auto* u = unit ("HIGHPASS");
        auto* a = knob (*u, "s_hpf", "CUTOFF"); auto* b = knob (*u, "s_hpfRes", "PEAK");
        u->layoutFn = [=] (juce::Rectangle<int> r) { place (r, { a, b }, 0, 88); };
    }
    {
        auto* u = unit ("VOLTAGE CONTROLLED FILTER");
        auto* a = knob (*u, "s_lpf", "CUTOFF"); auto* b = knob (*u, "s_lpfRes", "RESONANCE"); auto* c = knob (*u, "s_fltEnv", "EG AMOUNT");
        auto* d = knob (*u, "s_key", "KEY TRACK"); auto* e = knob (*u, "s_drive", "SATURATION"); auto* mo = knob (*u, "s_semMorph", "SEM MORPH");
        auto* lf = knob (*u, "s_fltLfo", "LFO 1");
        auto* ft = selector (*u, "s_ftype", juce::StringArray { "MS-10", "CEM3320", "SEM", "PROPHET" });
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            ft->setBounds (r.getX(), r.getY() + 2, juce::jmin (620, r.getWidth()), 34);
            place (r.withTrimmedTop (38), { a, b, c, d, e, lf, mo }, 0, juce::jmin (88, r.getWidth() / 7));
        };
    }
    {
        auto* u = unit ("ENVELOPE GENERATOR 1  (FILTER)");
        auto* a = knob (*u, "s_fA", "ATTACK"); auto* d = knob (*u, "s_fD", "DECAY"); auto* s = knob (*u, "s_fS", "SUSTAIN"); auto* r2 = knob (*u, "s_fR", "RELEASE");
        u->layoutFn = [=] (juce::Rectangle<int> r) { place (r, { a, d, s, r2 }, 0, 88); };
    }
    {
        auto* u = unit ("ENVELOPE GENERATOR 2  (AMP)");
        auto* a = knob (*u, "s_aA", "ATTACK"); auto* d = knob (*u, "s_aD", "DECAY"); auto* s = knob (*u, "s_aS", "SUSTAIN"); auto* r2 = knob (*u, "s_aR", "RELEASE");
        u->layoutFn = [=] (juce::Rectangle<int> r) { place (r, { a, d, s, r2 }, 0, 88); };
    }

    // ---- rang C : un LFO (choisi dans un menu), wavefolder, macros, arpegiateur (indices 8..11)
    {
        auto* u = unit ("MODULATION GENERATOR");
        struct LfoSet { LfoView* view; SegmentSelector* wave; KnobUnit *rate, *slew, *fade; juce::ComboBox* sync; LedToggle* trig; };
        auto sets = std::make_shared<std::vector<LfoSet>>();
        for (int i = 1; i <= kLfos; ++i)
        {
            const juce::String pre ("s_l" + juce::String (i));
            LfoSet st;
            st.wave  = selector (*u, (pre + "Wave").toRawUTF8(), lfoWaves);
            st.rate  = knob (*u, (pre + "Rate").toRawUTF8(), "FREQUENCY");
            st.slew  = knob (*u, (pre + "Slew").toRawUTF8(), "S&H SLEW");
            st.fade  = knob (*u, (pre + "Fade").toRawUTF8(), "FADE-IN");
            st.sync  = combo (*u, (pre + "Sync").toRawUTF8());
            st.trig  = toggle (*u, (pre + "Trig").toRawUTF8(), "RETRIG");
            auto view = std::make_unique<LfoView> (proc.apvts, pre + "Wave", pre + "Slew");
            st.view = view.get(); u->addAndMakeVisible (*view); lfoViews.push_back (st.view); owned.push_back (std::move (view));
            sets->push_back (st);
        }
        auto pick = std::make_unique<juce::ComboBox>();
        for (int i = 1; i <= kLfos; ++i) pick->addItem ("LFO " + juce::String (i), i);
        pick->setSelectedId (1, juce::dontSendNotification);
        pick->setScrollWheelEnabled (false);
        u->addAndMakeVisible (*pick);
        auto* pk = pick.get(); owned.push_back (std::move (pick));
        auto show = [sets, pk]
        {
            const int sel = pk->getSelectedId() - 1;
            for (size_t k = 0; k < sets->size(); ++k)
            {
                auto& st = (*sets)[k]; const bool on = (int) k == sel;
                for (juce::Component* c : { (juce::Component*) st.view, (juce::Component*) st.wave, (juce::Component*) st.rate, (juce::Component*) st.slew,
                                            (juce::Component*) st.fade, (juce::Component*) st.sync, (juce::Component*) st.trig })
                    c->setVisible (on);
            }
        };
        pk->onChange = show; show();
        u->layoutFn = [sets, pk, show] (juce::Rectangle<int> r)
        {
            show();                                                    // ModuleUnit::resized() remet tout visible : on ne garde que le LFO choisi
            pk->setBounds (r.getX(), r.getY() + 2, 104, 28);
            for (auto& st : *sets)
            {
                st.view->setBounds (r.getX() + 114, r.getY(), r.getWidth() - 114, 40);
                st.wave->setBounds (r.getX(), r.getY() + 46, r.getWidth(), 46);
                st.sync->setBounds (r.getX(), r.getY() + 98, r.getWidth() / 2 - 4, 28);
                st.trig->setBounds (r.getX() + r.getWidth() / 2, r.getY() + 96, r.getWidth() / 2, 32);
                const int cw = r.getWidth() / 3, y = r.getY() + 132, h = r.getHeight() - 132;
                st.rate->setBounds (r.getX(), y, cw, h); st.slew->setBounds (r.getX() + cw, y, cw, h); st.fade->setBounds (r.getX() + 2 * cw, y, cw, h);
            }
        };
    }
    {
        auto* u = unit ("WAVEFOLDER");
        auto* a = knob (*u, "s_fold", "FOLD"); auto* b = knob (*u, "s_foldSym", "SYMMETRY", "-", "+");
        u->layoutFn = [=] (juce::Rectangle<int> r) { const int cw = juce::jmin (96, r.getWidth() / 2 - 4); place (r, { a, b }, (r.getWidth() - 2 * cw) / 2, cw); };
    }
    {
        auto* u = unit ("MACROS  (CC MIDI : clic droit = learn)");
        auto ks = std::make_shared<std::vector<KnobUnit*>>();
        for (int i = 1; i <= kMacros; ++i)
        {
            auto* k = knob (*u, ("s_m" + juce::String (i)).toRawUTF8(), "MACRO " + juce::String (i));
            k->slider.addMouseListener (this, false);
            ks->push_back (k); macroKnobs.push_back (k);
        }
        u->layoutFn = [ks] (juce::Rectangle<int> r)
        {
            const int cw = r.getWidth() / 4, kh = juce::jmin (r.getHeight() / 2, cw + 22);
            for (int i = 0; i < 8; ++i) (*ks)[(size_t) i]->setBounds (r.getX() + (i % 4) * cw, r.getY() + (i / 4) * kh, cw, kh);
        };
    }
    {
        auto* u = unit ("KEY ASSIGN");
        auto* as = selector (*u, "s_assign", juce::StringArray { "POLY", "MONO", "UNISON", "SHARE", "CHORD" });
        auto* hold = toggle (*u, "s_hold", "HOLD"); auto* learn = toggle (*u, "s_chordLearn", "LEARN CHORD");
        auto* det = knob (*u, "s_uniDetune", "DETUNE");
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            as->setBounds (r.getX(), r.getY(), r.getWidth(), 46);
            hold->setBounds (r.getX(), r.getY() + 54, r.getWidth() / 2 - 4, 32); learn->setBounds (r.getX() + r.getWidth() / 2, r.getY() + 54, r.getWidth() / 2, 32);
            det->setBounds (r.getX() + (r.getWidth() - 110) / 2, r.getY() + 92, 110, r.getHeight() - 92);
        };
    }
    {
        auto* u = unit ("FM  (cross modulation)");
        auto* depth = knob (*u, "s_fm", "DEPTH");
        auto* dir = selector (*u, "s_fmDir", juce::StringArray { "2>1", "1>2", "BOTH" });
        auto* ratio = combo (*u, "s_fmRatio"); auto* wave = selector (*u, "s_fmWave", juce::StringArray { "OSC", "SIN", "TRI", "SQR" });
        auto* envAmt = knob (*u, "s_fmEnvAmt", "ENV AMT"); auto* att = knob (*u, "s_fmAtt", "ATTACK"); auto* dec = knob (*u, "s_fmDec", "DECAY");
        auto* fine = knob (*u, "s_fmFine", "FINE", "-", "+"); auto* vel = knob (*u, "s_fmVel", "VELOCITY"); auto* at = knob (*u, "s_fmAT", "AFTERTOUCH");
        auto tag = [this, u] (const juce::String& t) { auto c = std::make_unique<Tag> (t); auto* r = c.get(); u->addAndMakeVisible (*r); owned.push_back (std::move (c)); return r; };
        auto* tPal = tag ("PALETTE"); auto* tDir = tag ("DIRECTION"); auto* tRat = tag ("RATIO"); auto* tWav = tag ("MODULATOR");
        // palettes : un clic regle ratio, onde, profondeur et enveloppe ; tout reste ensuite ajustable
        auto pal = std::make_unique<juce::ComboBox>();
        pal->addItem ("choose...", 1); pal->addItem ("BELL", 2); pal->addItem ("E-PIANO", 3); pal->addItem ("BRASS", 4); pal->addItem ("METAL", 5); pal->addItem ("GROWL", 6); pal->addItem ("PAD", 7);
        pal->setSelectedId (1, juce::dontSendNotification); pal->setScrollWheelEnabled (false);
        u->addAndMakeVisible (*pal);
        auto* palP = pal.get(); owned.push_back (std::move (pal));
        palP->onChange = [this, palP]
        {
            //                       fm   ratio wave dir  envAmt att  dec  fine  vel  at
            static const float T[6][10] { { 0.35f, 10, 1, 0, 0.60f, 0.00f, 0.55f, 0.00f, 0.3f, 0.0f },     // BELL    : 7:2, sinus, forte attaque qui s'eteint
                                          { 0.16f, 3, 1, 0, 0.50f, 0.00f, 0.35f, 0.00f, 0.5f, 0.0f },      // E-PIANO : 1:1, sinus, impact court, suit la velocite
                                          { 0.12f, 3, 1, 0, 0.50f, 0.45f, 0.50f, 0.00f, 0.4f, 0.4f },      // BRASS   : 1:1, la profondeur monte puis retombe
                                          { 0.45f, 11, 2, 2, 0.30f, 0.00f, 0.60f, 0.15f, 0.0f, 0.0f },     // METAL   : x1.41, triangle, FM croisee, leger desaccord
                                          { 0.55f, 1, 3, 0, 0.00f, 0.00f, 0.40f, 0.20f, 0.0f, 0.3f },      // GROWL   : 1:2, carre, profondeur constante
                                          { 0.12f, 4, 1, 0, 0.20f, 0.60f, 0.80f, 0.00f, 0.0f, 0.4f } };    // PAD     : 3:2, sinus, lente
            const int id = palP->getSelectedId();
            if (id < 2) return;
            const auto* t = T[id - 2];
            proc.setParamReal ("s_fm", t[0]); proc.setParamReal ("s_fmRatio", t[1]); proc.setParamReal ("s_fmWave", t[2]); proc.setParamReal ("s_fmDir", t[3]);
            proc.setParamReal ("s_fmEnvAmt", t[4]); proc.setParamReal ("s_fmAtt", t[5]); proc.setParamReal ("s_fmDec", t[6]); proc.setParamReal ("s_fmFine", t[7]);
            proc.setParamReal ("s_fmVel", t[8]); proc.setParamReal ("s_fmAT", t[9]);
        };
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            const int dw = juce::jmin (96, r.getWidth() / 4), x = r.getX() + dw + 6, w = r.getWidth() - dw - 6;
            tPal->setBounds (x, r.getY(), 90, 10);  palP->setBounds (x, r.getY() + 12, w / 2 - 4, 26);
            tDir->setBounds (x + w / 2, r.getY(), 90, 10); dir->setBounds (x + w / 2, r.getY() + 12, w / 2, 30);
            tRat->setBounds (x, r.getY() + 46, 90, 10);  ratio->setBounds (x, r.getY() + 58, w / 2 - 4, 26);
            tWav->setBounds (x + w / 2, r.getY() + 46, 90, 10); wave->setBounds (x + w / 2, r.getY() + 58, w / 2, 30);
            const int ky = r.getY() + 92, kh = (r.getHeight() - 92) / 2, cw = w / 3;
            int i = 0; for (auto* k : { envAmt, att, dec, fine, vel, at }) { k->setBounds (x + (i % 3) * cw, ky + (i / 3) * kh, cw, kh); ++i; }
            depth->setBounds (r.getX(), r.getY() + 12, dw, r.getHeight() - 12);
        };
    }
    {   // index 13 : poly-mod facon Prophet-5 (enveloppe de filtre et VCO 2 -> frequence / largeur du VCO 1, filtre)
        auto* u = unit ("POLY-MOD");
        auto* a = knob (*u, "s_pmEnv", "FILTER ENV"); auto* b = knob (*u, "s_pmOsc", "OSC 2");
        auto* t1 = toggle (*u, "s_pmFreq", "FREQ A"); auto* t2 = toggle (*u, "s_pmPw", "PW A"); auto* t3 = toggle (*u, "s_pmFilt", "FILTER");
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            const int tw = juce::jmin (96, r.getWidth() / 3), kx = r.getX() + tw + 6, cw = (r.getRight() - kx) / 2;
            int y = r.getY() + 4; for (auto* t : { t1, t2, t3 }) { t->setBounds (r.getX(), y, tw, 30); y += 36; }
            a->setBounds (kx, r.getY(), cw, r.getHeight()); b->setBounds (kx + cw, r.getY(), cw, r.getHeight());
        };
    }
    startTimerHz (10);
}

void SynthPanel::timerCallback()
{
    for (auto* v : lfoViews) v->repaint();
    // le nom de chaque macro = destination de la premiere ligne de matrice qu'elle pilote
    for (size_t k = 0; k < macroKnobs.size(); ++k)
    {
        juce::String name = "MACRO " + juce::String ((int) k + 1);
        if (proc.isLearning ("s_m" + juce::String ((int) k + 1))) name = "LEARN CC...";
        else
            for (int sl = 1; sl <= kSlots; ++sl)
            {
                auto* sp = proc.apvts.getRawParameterValue ("s_slot" + juce::String (sl) + "Src");
                auto* dp = proc.apvts.getRawParameterValue ("s_slot" + juce::String (sl) + "Dst");
                if (sp != nullptr && dp != nullptr && (int) sp->load() == macroSrc ((int) k) && (int) dp->load() > 0) { name = juce::String (kModDstNames[(int) dp->load()]); break; }
            }
        if (macroKnobs[k]->text != name) { macroKnobs[k]->text = name; macroKnobs[k]->repaint(); }
    }
}

void SynthPanel::mouseDown (const juce::MouseEvent&) {}          // le menu MIDI (clic droit) est porte par chaque potard

void SynthPanel::paint (juce::Graphics& g) { g.drawImageAt (tex, 0, 0); }
void SynthPanel::rebuild() { paintFacade (tex, getWidth(), getHeight(), false); repaint(); for (auto& u : units) u->repaint(); }

void SynthPanel::resized()
{
    const int W = getWidth(), gap = 8;
    auto row = [&] (int y, int h, std::initializer_list<std::pair<size_t, float>> items)
    {
        float total = 0.f; for (auto& it : items) total += it.second;
        const int usable = W - 20 - gap * ((int) items.size() - 1);
        int x = 10;
        for (auto& it : items)
        {
            const int w = juce::roundToInt ((float) usable * it.second / total);
            units[it.first]->fullHeight = h;
            units[it.first]->setBounds (x, y, w, h);
            x += w + gap;
        }
    };
    if (page == 1)
    {
        row (0, 200, { { 0, 520.f }, { 1, 880.f } });
        row (208, 130, { { 2, 1.f } });
        row (346, 386, { { 3, 1.f } });
        paintFacade (tex, W, getHeight(), false);
        return;
    }
    row (0, 212, { { 0, 290.f }, { 1, 450.f }, { 2, 372.f }, { 3, 300.f } });
    row (220, 212, { { 4, 170.f }, { 5, 560.f }, { 6, 320.f }, { 7, 320.f }, { 13, 300.f } });
    row (440, 276, { { 8, 290.f }, { 9, 205.f }, { 10, 330.f }, { 11, 250.f }, { 12, 395.f } });

    paintFacade (tex, W, getHeight(), false);
}

MatrixDrawer::MatrixDrawer (juce::AudioProcessorValueTreeState& apvts)
{
    for (int i = 1; i <= kSlots; ++i)
    {
        const juce::String pre ("s_slot" + juce::String (i));
        auto r = std::make_unique<Row>();
        for (auto* cb : { &r->src, &r->dst }) cb->setScrollWheelEnabled (false);
        fillGrouped (r->src, srcFamilies(), kModSrcNames);
        fillGrouped (r->dst, dstFamilies(), kModDstNames);
        r->amt.detent = 0.0; r->amt.showMark = true;
        r->amt.setPopupDisplayEnabled (true, true, nullptr);
        r->amt.setScrollWheelEnabled (false);
        r->amt.setDoubleClickReturnValue (true, 0.0);
        addAndMakeVisible (r->src); addAndMakeVisible (r->dst); addAndMakeVisible (r->amt);
        idAtt.push_back (std::make_shared<IdComboAttachment> (apvts, pre + "Src", r->src));
        idAtt.push_back (std::make_shared<IdComboAttachment> (apvts, pre + "Dst", r->dst));
        sliderAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, pre + "Amt", r->amt));
        rows.push_back (std::move (r));
    }
}

void MatrixDrawer::paint (juce::Graphics& g)
{
    const auto& th = look::T();
    const auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (b.translated (0.f, 3.f), 7.f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (th.plateTop), 0.f, 0.f, juce::Colour (th.plateBot), 0.f, b.getBottom(), false));
    g.fillRoundedRectangle (b, 7.f);
    g.setColour (juce::Colour (th.accent).withAlpha (0.7f));
    g.drawRoundedRectangle (b.reduced (1.f), 7.f, 1.3f);
    g.setColour (look::print.withAlpha (0.95f));
    g.setFont (bold (13.f));
    g.drawText ("MODULATION MATRIX   (reaches the filters, the FX rack, delay and reverb)", 14, 6, getWidth() - 28, 18, juce::Justification::centredLeft, false);
    g.setColour (juce::Colour (th.accent).withAlpha (0.45f));
    g.fillRect (14, 28, getWidth() - 28, 1);
    g.setColour (look::print.withAlpha (0.6f));
    g.setFont (bold (9.f));
    g.drawText ("SOURCE", 14, 30, 150, 8, juce::Justification::centredLeft, false);
    g.drawText ("DESTINATION", 172, 30, 176, 8, juce::Justification::centredLeft, false);
    g.drawText ("AMOUNT", 358, 30, 100, 8, juce::Justification::centredLeft, false);
}

void MatrixDrawer::resized()
{
    const int rowH = 32;
    for (size_t i = 0; i < rows.size(); ++i)
    {
        const int y = 42 + (int) i * rowH;
        rows[i]->src.setBounds (14, y, 150, 26);
        rows[i]->dst.setBounds (172, y, 176, 26);
        rows[i]->amt.setBounds (358, y, getWidth() - 358 - 16, 26);
    }
}

// ---------------------------------------------------------------- fenetre : onglets (synthe) + mise a l'echelle

int SynthEditor::designHeight() const { return ZEPHYR_SYNTH ? 30 + kFxH : kFxH; }
int SynthEditor::designWidth() const  { return kDesignW + 2 * kCheek; }

SynthEditor::SynthEditor (ZephyrAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
#if ZEPHYR_SYNTH
      fx (p, "ZEPHYR", "12-VOICE VINTAGE POLYSYNTH + EFFECTS RACK")
#else
      fx (p)
#endif
{
    setLookAndFeel (&laf);
    addAndMakeVisible (content);
    content.addAndMakeVisible (cheekL);
    content.addAndMakeVisible (cheekR);
    content.addAndMakeVisible (fx);
    fx.onThemeChanged = [this]
    {
        if (synthPanel != nullptr) synthPanel->rebuild();
        if (arpPanel != nullptr) arpPanel->rebuild();
        cheekL.themeChanged(); cheekR.themeChanged();
        repaint();
    };
    fx.dialogHost = &content;
#if ZEPHYR_SYNTH
    synthPanel = std::make_unique<SynthPanel> (p, 0);
    content.addAndMakeVisible (*synthPanel);
    arpPanel = std::make_unique<SynthPanel> (p, 1);
    content.addChildComponent (*arpPanel);
    for (auto* b : { &synthTab, &arpTab, &fxTab })
    {
        b->setClickingTogglesState (true);
        b->setRadioGroupId (4242);
        content.addAndMakeVisible (*b);
    }
    synthTab.onClick = [this] { showTab (0); };
    arpTab.onClick   = [this] { showTab (1); };
    fxTab.onClick    = [this] { showTab (2); };
    matrix = std::make_unique<MatrixDrawer> (p.apvts);
    content.addChildComponent (*matrix);
    matrixBtn.setClickingTogglesState (true);
    content.addAndMakeVisible (matrixBtn);
    matrixBtn.onClick = [this] { matrix->setVisible (matrixBtn.getToggleState()); matrix->toFront (false); };
    synthTab.setToggleState (true, juce::dontSendNotification);
    showTab (0);
#endif

    const int dh = designHeight();
    float s = 1.f;                                                  // taille initiale : tient dans l'ecran
    if (auto* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        s = juce::jmin (1.f, (float) (d->userArea.getHeight() - 90) / (float) dh, (float) (d->userArea.getWidth() - 20) / (float) designWidth());
    s = juce::jmax (0.6f, s);
    constrainer.setFixedAspectRatio ((double) designWidth() / (double) dh);
    constrainer.setSizeLimits (designWidth() * 6 / 10, dh * 6 / 10, designWidth() * 3 / 2, dh * 3 / 2);
    setConstrainer (&constrainer);
    setResizable (true, true);
    setSize (juce::roundToInt ((float) designWidth() * s), juce::roundToInt ((float) dh * s));
}

SynthEditor::~SynthEditor() { setLookAndFeel (nullptr); }

void SynthEditor::showTab (int t)
{
    if (synthPanel != nullptr) synthPanel->setVisible (t == 0);
    if (arpPanel != nullptr) arpPanel->setVisible (t == 1);
    fx.repaint();
}

void SynthEditor::paint (juce::Graphics& g) { g.fillAll (juce::Colour (look::T().panelBot)); }

void SynthEditor::resized()
{
    const int dh = designHeight();
    content.setTransform ({});
    content.setBounds (0, 0, designWidth(), dh);
    content.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) designWidth()));
    cheekL.setBounds (0, 0, kCheek, dh);
    cheekR.setBounds (kCheek + kDesignW, 0, kCheek, dh);
#if ZEPHYR_SYNTH
    synthTab.setBounds (kCheek + 10, 3, 150, 24);
    arpTab.setBounds (kCheek + 166, 3, 150, 24);
    fxTab.setBounds (kCheek + 322, 3, 150, 24);
    matrixBtn.setBounds (kCheek + 478, 3, 150, 24);
    fx.setBounds (kCheek, 30, kDesignW, kFxH);
    synthPanel->setBounds (kCheek, 30 + kHeaderH + 4, kDesignW, kFxH - kHeaderH - 4);
    arpPanel->setBounds (kCheek, 30 + kHeaderH + 4, kDesignW, kFxH - kHeaderH - 4);
    matrix->setBounds (kCheek + 478, 32, 640, 42 + kSlots * 32 + 10);
    matrix->toFront (false);
#else
    fx.setBounds (kCheek, 0, kDesignW, kFxH);
#endif
}
