#include "ZephyrEditor.h"

using namespace eo;
using namespace ui;

// ================================================================ petits composants
void Plate::paint (juce::Graphics& g)
{
    drawPlate (g, getLocalBounds().toFloat());
    g.setColour (look::print.withAlpha (0.95f));
    g.setFont (font (13.f));
    g.drawText (title, 12, 5, getWidth() - 24, 16, juce::Justification::centredLeft, false);
    g.setColour (look::print.withAlpha (0.35f));
    g.fillRect (12, 24, getWidth() - 24, 1);
    for (const auto& gr : groups)                                           // cadres de sections : trait fin interrompu par le titre
    {
        const auto r = gr.second.toFloat();
        g.setFont (font (10.f));
        const int tw = (int) std::ceil (juce::GlyphArrangement::getStringWidth (font (10.f), gr.first)) + 10;
        const juce::Rectangle<int> gap ((int) r.getX() + 10, (int) r.getY() - 7, tw, 14);
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.excludeClipRegion (gap);
            g.setColour (look::print.withAlpha (0.28f));
            g.drawRoundedRectangle (r, 6.f, 1.f);
        }
        g.setColour (look::print.withAlpha (0.7f));
        g.drawText (gr.first, gap, juce::Justification::centred, false);
    }
}

void Plate::paintOverChildren (juce::Graphics& g)
{
    g.setColour (look::print.withAlpha (0.85f));
    g.setFont (font (11.f));
    for (const auto& l : labels) g.drawText (l.first, l.second, juce::Justification::centred, false);
    drawSpecks (g, getLocalBounds());
}

void LedBar::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float capH = caption.isNotEmpty() ? 12.f : 0.f;
    const auto bar = b.withTrimmedBottom (capH);
    g.setColour (juce::Colour (0xff060607));
    g.fillRoundedRectangle (bar, 3.f);
    g.setColour (juce::Colour (0xff6a6a70).withAlpha (0.7f));
    g.drawRoundedRectangle (bar, 3.f, 1.f);
    const float len = horiz ? bar.getWidth() : bar.getHeight();
    const int n = juce::jmax (8, (int) (len / 6.f));
    const float seg = (len - 6.f) / (float) n;
    const float frac = juce::jlimit (0.f, 1.f, (value - lo_) / (hi_ - lo_));
    for (int i = 0; i < n; ++i)
    {
        const float t = (float) i / (float) (n - 1);
        const bool lit = (float) i / (float) n < frac;
        juce::Colour c = levelColours ? (t < 0.65f ? juce::Colour (0xff4fd067) : (t < 0.88f ? juce::Colour (0xffe3b23a) : juce::Colour (0xffe0362a))) : juce::Colour (0xffff7a1a);
        g.setColour (lit ? c : c.darker (3.2f).withAlpha (0.55f));
        if (horiz) g.fillRect (bar.getX() + 3.f + (float) i * seg + 0.5f, bar.getY() + 3.f, juce::jmax (1.f, seg - 1.5f), bar.getHeight() - 6.f);
        else       g.fillRect (bar.getX() + 3.f, bar.getBottom() - 3.f - (float) (i + 1) * seg + 0.5f, bar.getWidth() - 6.f, juce::jmax (1.f, seg - 1.5f));
    }
    if (capH > 0.f)
    {
        g.setColour (look::print.withAlpha (0.85f));
        g.setFont (font (10.f));
        g.drawText (caption, b.withTrimmedTop (b.getHeight() - capH).expanded (6.f, 0.f), juce::Justification::centred, false);
    }
}

void Lcd::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (1.f);
    g.setColour (juce::Colour (0xff070605));
    g.fillRoundedRectangle (b, 3.f);
    g.setColour (juce::Colour (0xff6a6a70).withAlpha (0.7f));
    g.drawRoundedRectangle (b, 3.f, 1.f);
    g.setColour (look::print.withAlpha (0.7f));
    g.setFont (font (9.5f));
    g.drawText (label, b.withTrimmedTop (2.f).withHeight (10.f), juce::Justification::centred, false);
    g.setColour (juce::Colour (0xffffa63a));
    g.setFont (juce::Font (juce::FontOptions (large ? 22.f : 14.f, juce::Font::bold)));
    g.drawText (text, b.withTrimmedTop (10.f), juce::Justification::centred, false);
}

void PipeView::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const auto tube = b.withTrimmedLeft (b.getWidth() * 0.42f).reduced (2.f, 2.f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff050506), tube.getX(), 0.f, juce::Colour (0xff17171a), tube.getRight(), 0.f, false));
    g.fillRoundedRectangle (tube, tube.getWidth() * 0.5f);
    g.setColour (juce::Colour (0xff8d8d94).withAlpha (0.8f));
    g.drawRoundedRectangle (tube, tube.getWidth() * 0.5f, 1.6f);
    const float top = tube.getY() + tube.getWidth() * 0.5f + 6.f, bot = tube.getBottom() - tube.getWidth() * 0.5f - 6.f;
    auto yAt = [&] (float t) { return bot + (top - bot) * t; };
    const bool harmonic = proc.apvts.getRawParameterValue ("lock")->load() < 0.5f;
    const float range = proc.apvts.getRawParameterValue ("range")->load();
    g.setFont (font (10.5f));
    if (harmonic)
    {
        const int maxH = (int) std::floor (2.f + range * 14.f);
        for (int h = 1; h <= maxH; ++h)
        {
            const float y = yAt ((float) (h - 1) / (float) (maxH - 1));
            g.setColour (look::print.withAlpha (0.8f));
            g.fillRect (b.getX() + b.getWidth() * 0.42f - 14.f, y, 10.f, 1.5f);
            g.drawText (juce::String (h), (int) (b.getX() + b.getWidth() * 0.42f - 44.f), (int) y - 6, 28, 12, juce::Justification::centredRight, false);
        }
        g.setColour (look::print.withAlpha (0.6f));
        g.drawText ("HARMONIC", 2, 2, (int) (b.getWidth() * 0.42f), 12, juce::Justification::centredLeft, false);
    }
    else
    {
        const int span = (int) std::round (12.f + range * 36.f);
        for (int s = 0; s <= span; ++s)
        {
            const float y = yAt ((float) s / (float) span);
            const bool oct = (s % 12) == 0;
            g.setColour (look::print.withAlpha (oct ? 0.9f : 0.35f));
            g.fillRect (b.getX() + b.getWidth() * 0.42f - (oct ? 14.f : 8.f), y, oct ? 10.f : 4.f, 1.5f);
            if (oct) g.drawText ("+" + juce::String (s / 12) + " oct", (int) (b.getX() + b.getWidth() * 0.42f - 54.f), (int) y - 6, 38, 12, juce::Justification::centredRight, false);
        }
        g.setColour (look::print.withAlpha (0.6f));
        g.drawText ("SCALE", 2, 2, (int) (b.getWidth() * 0.42f), 12, juce::Justification::centredLeft, false);
    }
    for (int i = 0; i < kVoices; ++i)                                          // une perle par voix
    {
        const float lvl = proc.synth.vLvl[i].load();
        if (lvl < 0.01f) continue;
        const float y = yAt (proc.synth.vPos[i].load()), cx = tube.getCentreX() + ((float) (i % 3) - 1.f) * tube.getWidth() * 0.18f, r = 4.f + 6.f * lvl;
        const auto col = heatColour (0.35f + 0.65f * lvl);
        g.setGradientFill (juce::ColourGradient (col.withAlpha (0.55f), cx, y, col.withAlpha (0.f), cx + r * 2.6f, y, true));
        g.fillEllipse (cx - r * 2.6f, y - r * 2.6f, r * 5.2f, r * 5.2f);
        g.setColour (col.brighter (0.3f));
        g.fillEllipse (cx - r, y - r, r * 2.f, r * 2.f);
    }
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.18f), tube.getX(), tube.getY(), juce::Colours::white.withAlpha (0.f), tube.getX() + tube.getWidth() * 0.4f, tube.getY(), false));
    g.fillRoundedRectangle (tube.reduced (3.f), tube.getWidth() * 0.45f);
}

// ================================================================ contenu (taille de conception fixe)
ZephyrContent::ZephyrContent (ZephyrProcessor& p) : proc (p), pipeView (p)
{
    setLookAndFeel (&laf);
    setSize (designW, designH);
    buildPanelTex();
    for (auto* pl : { &pipePlate, &resPlate, &spacePlate, &modPlate, &learnPlate, &perfPlate, &fmPlate, &dlPlate }) addAndMakeVisible (pl);

    // ---- en-tete
    for (auto* b : { &prevBtn, &nextBtn, &saveBtn }) addAndMakeVisible (*b);
    addAndMakeVisible (nameBtn);
    prevBtn.setBounds (designW - 400, 24, 30, 30);
    nameBtn.setBounds (designW - 364, 24, 220, 30);
    nextBtn.setBounds (designW - 140, 24, 30, 30);
    saveBtn.setBounds (designW - 104, 24, 70, 30);
    prevBtn.onClick = [this] { stepPreset (-1); };
    nextBtn.onClick = [this] { stepPreset (1); };
    nameBtn.onClick = [this] { presetMenu(); };
    saveBtn.onClick = [this] { saveDialog(); };
    nameBtn.setButtonText (proc.presetName);

    // ---- LE TUYAU
    pipePlate.setBounds (10, 84, 220, 330);
    pipePlate.addAndMakeVisible (pipeView);
    pipeView.setBounds (10, 32, 200, 288);

    // ---- RESONATEUR
    resPlate.setBounds (240, 84, 700, 330);
    {
        combo (resPlate, p_mode)->setBounds (14, 44, 170, 28);
        selector (resPlate, p_lock)->setBounds (200, 34, 170, 40);
        auto* sc = combo (resPlate, p_scale); sc->setBounds (386, 44, 190, 26);
        auto* rt = combo (resPlate, p_root);  rt->setBounds (590, 44, 70, 26);
        resPlate.addAndMakeVisible (bodyHelp);
        bodyHelp.setBounds (250, 3, 440, 18); bodyHelp.setJustificationType (juce::Justification::centredRight);
        bodyHelp.setFont (font (11.5f)); bodyHelp.setColour (juce::Label::textColourId, look::print.withAlpha (0.75f)); bodyHelp.setInterceptsMouseClicks (false, false);
        resPlate.labels.push_back ({ "BODY", { 14, 30, 170, 13 } });
        resPlate.labels.push_back ({ "LOCK TO", { 200, 74, 170, 11 } });
        resPlate.labels.push_back ({ "SCALE", { 386, 30, 190, 13 } });
        resPlate.labels.push_back ({ "ROOT", { 590, 30, 70, 13 } });
        int x = 12; const int y1 = 100, kw = 96, kh = 108;
        knob (resPlate, p_fb, "FEEDBACK", "0", "10")->setBounds (x, y1, kw, kh); x += kw;
        knob (resPlate, p_wind, "WIND", "0", "10")->setBounds (x, y1, kw, kh); x += kw;
        knob (resPlate, p_range, "RANGE", "0", "10")->setBounds (x, y1, kw, kh); x += kw;
        knob (resPlate, p_focus, "FOCUS", "WIDE", "SHARP")->setBounds (x, y1, kw, kh); x += kw;
        knob (resPlate, p_stick, "STICK", "0", "10")->setBounds (x, y1, kw, kh); x += kw;
        knob (resPlate, p_snap, "SNAP", "SWEEP", "STEP")->setBounds (x, y1, kw, kh); x += kw;
        knob (resPlate, p_glide, "GLIDE", "5", "800")->setBounds (x, y1, kw, kh);
        x = 12; const int y2 = 218, kw2 = 76;
        knob (resPlate, p_bright, "BRIGHT", "DARK", "OPEN")->setBounds (x, y2, kw2, kh); x += kw2;
        knob (resPlate, p_drive, "DRIVE", "0", "10")->setBounds (x, y2, kw2, kh); x += kw2;
        knob (resPlate, p_breath, "BREATH", "0", "10")->setBounds (x, y2, kw2, kh); x += kw2;
        knob (resPlate, p_atk, "ATTACK", "5", "4s")->setBounds (x, y2, kw2, kh); x += kw2;
        knob (resPlate, p_dec, "DECAY", "20", "8s")->setBounds (x, y2, kw2, kh); x += kw2;
        knob (resPlate, p_sus, "SUSTAIN", "0", "10")->setBounds (x, y2, kw2, kh); x += kw2;
        knob (resPlate, p_rel, "RELEASE", "50", "8s")->setBounds (x, y2, kw2, kh); x += kw2;
        knob (resPlate, p_detune, "DETUNE", "0", "30")->setBounds (x, y2, kw2, kh); x += kw2;
        knob (resPlate, p_spread, "SPREAD", "MONO", "WIDE")->setBounds (x, y2, kw2, kh);
        resPlate.groups = { { "LOOP", { 8, 92, 380, 122 } }, { "PITCH", { 392, 92, 296, 122 } },
                            { "TONE", { 8, 222, 232, 100 } }, { "ENVELOPE", { 244, 222, 296, 100 } },
                            { "VOICES", { 544, 222, 144, 100 } } };
    }

    // ---- FM
    fmPlate.setBounds (10, 424, 700, 240);
    {
        selector (fmPlate, p_fmType)->setBounds (14, 34, 520, 40);
        fmPlate.labels.push_back ({ "SOUND", { 14, 74, 520, 11 } });
        int x = 12; const int y = 100, kw = 110, kh = 130;
        knob (fmPlate, p_fm, "FM LEVEL", "0", "10")->setBounds (x, y, kw, kh); x += kw;
        knob (fmPlate, p_fmChar, "CHARACTER", "SOFT", "WILD")->setBounds (x, y, kw, kh); x += kw;
        knob (fmPlate, p_fmDecay, "DECAY", "0.05", "SUSTAIN")->setBounds (x, y, kw, kh); x += kw;
        knob (fmPlate, p_fmRoute, "ROUTE", "PIPE", "BESIDE", 0.5)->setBounds (x, y, kw, kh);
        fmPlate.labels.push_back ({ "ROUTE : left = FM excites the pipe,", { 462, 110, 232, 24 } });
        fmPlate.labels.push_back ({ "centre = both, right = beside it.", { 462, 134, 232, 24 } });
        fmPlate.labels.push_back ({ "FM LEVEL can also be modulated.", { 462, 170, 232, 24 } });
    }

    // ---- DELAY
    dlPlate.setBounds (720, 424, 670, 240);
    {
        selector (dlPlate, p_dlMode)->setBounds (14, 34, 300, 40);
        selector (dlPlate, p_dlRoute)->setBounds (330, 34, 326, 40);
        dlPlate.labels.push_back ({ "MODE", { 14, 74, 300, 11 } });
        dlPlate.labels.push_back ({ "ROUTING  (REVERB / DELAY)", { 330, 74, 326, 11 } });
        int x = 8; const int y = 100, kw = 93, kh = 130;
        knob (dlPlate, p_dlTime, "TIME", "SHORT", "LONG")->setBounds (x, y, kw, kh); x += kw;
        knob (dlPlate, p_dlFb, "FEEDBACK", "0", "10")->setBounds (x, y, kw, kh); x += kw;
        knob (dlPlate, p_dlMod, "WOBBLE", "0", "10")->setBounds (x, y, kw, kh); x += kw;
        knob (dlPlate, p_dlRate, "RATE", "0.05", "10 Hz")->setBounds (x, y, kw, kh); x += kw;
        knob (dlPlate, p_dlTone, "TONE", "DARK", "OPEN")->setBounds (x, y, kw, kh); x += kw;
        knob (dlPlate, p_dlMix, "MIX", "OFF", "10")->setBounds (x, y, kw, kh); x += kw;
        knob (dlPlate, p_dlLoop, "TO LOOP", "0", "10")->setBounds (x, y, kw, kh);
    }

    // ---- ESPACE + SORTIE
    spacePlate.setBounds (950, 84, 440, 330);
    {
        const int kw = 96, y = 34, kh = 120;
        knob (spacePlate, p_rvSize, "SIZE", "SMALL", "HUGE")->setBounds (12, y, kw, kh);
        knob (spacePlate, p_rvDamp, "DAMP", "BRIGHT", "DARK")->setBounds (12 + kw, y, kw, kh);
        knob (spacePlate, p_rvMix, "MIX", "DRY", "WET")->setBounds (12 + 2 * kw, y, kw, kh);
        auto* o = knob (spacePlate, p_out, "OUTPUT", "-24", "+6"); o->setBounds (12 + 3 * kw - 4, y, kw + 8, kh);
        outBar = new LedBar (0.f, 1.f, "OUT", false); owned.emplace_back (outBar); spacePlate.addAndMakeVisible (outBar);
        outBar->setBounds (398, 36, 28, 270);
        lcdVoices = new Lcd ("VOICES"); owned.emplace_back (lcdVoices); spacePlate.addAndMakeVisible (lcdVoices);
        lcdVoices->setBounds (14, 176, 110, 48);
        lcdPeak = new Lcd ("PEAK dBFS"); owned.emplace_back (lcdPeak); spacePlate.addAndMakeVisible (lcdPeak);
        lcdPeak->setBounds (134, 176, 110, 48);
        spacePlate.labels.push_back ({ "SAFE : OUTPUT IS ALWAYS LIMITED BELOW 0 dBFS", { 14, 240, 372, 12 } });
    }

    // ---- MATRICE DE MODULATION
    modPlate.setBounds (10, 84, 700, 350);
    {
        static const P src[6] { p_m1s, p_m2s, p_m3s, p_m4s, p_m5s, p_m6s }, dst[6] { p_m1d, p_m2d, p_m3d, p_m4d, p_m5d, p_m6d }, amt[6] { p_m1a, p_m2a, p_m3a, p_m4a, p_m5a, p_m6a };
        for (int k = 0; k < 6; ++k)
        {
            const int x0 = 12 + (k % 2) * 344, y0 = 34 + (k / 2) * 104;
            modPlate.labels.push_back ({ juce::String (k + 1), { x0, y0 + 30, 18, 22 } });
            modPlate.labels.push_back ({ "SOURCE", { x0 + 22, y0 + 2, 200, 13 } });
            combo (modPlate, src[k])->setBounds (x0 + 22, y0 + 16, 205, 28);
            modPlate.labels.push_back ({ "CONTROLS", { x0 + 22, y0 + 50, 205, 13 } });
            combo (modPlate, dst[k])->setBounds (x0 + 22, y0 + 64, 205, 28);
            knob (modPlate, amt[k], "AMOUNT", "-", "+", 0.0)->setBounds (x0 + 234, y0 + 2, 100, 96);
        }
    }

    // ---- SOURCES MIDI
    learnPlate.setBounds (720, 84, 670, 350);
    {
        for (int i = 0; i < 2; ++i)
        {
            const int y = 40 + i * 62;
            learnBtn[i].setButtonText (i == 0 ? "LEARN A" : "LEARN B");
            learnPlate.addAndMakeVisible (learnBtn[i]);
            learnBtn[i].setBounds (14, y, 150, 44);
            learnBtn[i].onClick = [this, i] { proc.learnArm.store (proc.learnArm.load() == i + 1 ? 0 : i + 1); };
            lcdLearn[i] = new Lcd (i == 0 ? "SOURCE LEARN A" : "SOURCE LEARN B"); owned.emplace_back (lcdLearn[i]); learnPlate.addAndMakeVisible (lcdLearn[i]);
            lcdLearn[i]->setBounds (174, y - 2, 140, 48);
        }
        learnPlate.labels.push_back ({ "Click LEARN, then move any knob or", { 14, 168, 306, 14 } });
        learnPlate.labels.push_back ({ "slider on your keyboard : it becomes", { 14, 184, 306, 14 } });
        learnPlate.labels.push_back ({ "source LEARN A / B in the matrix.", { 14, 200, 306, 14 } });
        learnPlate.labels.push_back ({ "READY-MADE SOURCES", { 14, 236, 306, 14 } });
        learnPlate.labels.push_back ({ "Mod wheel CC1  -  Expression CC11", { 14, 254, 306, 14 } });
        learnPlate.labels.push_back ({ "Macros 1-8 = CC16-23 (Hydrasynth)", { 14, 270, 306, 14 } });
        learnPlate.labels.push_back ({ "Polyphonic / channel aftertouch, velocity", { 14, 286, 306, 14 } });
        knob (learnPlate, p_pcurve, "AFTERTOUCH CURVE", "SOFT", "HARD", 1.0)->setBounds (360, 40, 150, 140);
        knob (learnPlate, p_bend, "BEND RANGE", "1", "48")->setBounds (520, 40, 130, 140);
        toggle (learnPlate, p_mpe, "MPE MODE")->setBounds (380, 210, 150, 28);
        toggle (learnPlate, p_hold, "HOLD  (DRONE)")->setBounds (380, 252, 150, 28);
        learnPlate.labels.push_back ({ "HOLD keeps notes sounding until you switch it off", { 340, 296, 316, 14 } });
    }

    // ---- ACTIVITE
    perfPlate.setBounds (10, 444, 1380, 236);
    {
        perfPlate.addAndMakeVisible (midiLamp);
        midiLamp.setBounds (24, 44, 80, 40); midiLamp.set (false);
        lcdPress = new Lcd ("LAST PRESSURE"); owned.emplace_back (lcdPress); perfPlate.addAndMakeVisible (lcdPress);
        lcdPress->setBounds (14, 110, 150, 50);
        perfPlate.labels.push_back ({ "Shows the last pressure received", { 6, 168, 176, 28 } });
        for (int i = 0; i < kVoices; ++i)
        {
            voiceBar[i] = new LedBar (0.f, 1.f, juce::String (i + 1)); owned.emplace_back (voiceBar[i]); perfPlate.addAndMakeVisible (voiceBar[i]);
            voiceBar[i]->setBounds (230 + i * 100, 34, 56, 166);
        }
        pressBar = new LedBar (0.f, 1.f, "PRESS"); owned.emplace_back (pressBar); perfPlate.addAndMakeVisible (pressBar);
        pressBar->levelColours = false;
        pressBar->setBounds (1180, 34, 64, 166);
        perfPlate.labels.push_back ({ "VOICE ACTIVITY", { 230, 206, 800, 14 } });
    }

    // ---- onglets
    for (auto* t : { &tabSound, &tabMod }) { addAndMakeVisible (*t); t->setClickingTogglesState (true); t->setRadioGroupId (77); }
    tabSound.setBounds (470, 24, 210, 30); tabMod.setBounds (690, 24, 210, 30);
    tabSound.onClick = [this] { showPage (0); };
    tabMod.onClick = [this] { showPage (1); };
    showPage (0);

    nameBtn.setButtonText (proc.presetName);
    startTimerHz (30);
}

void ZephyrContent::showPage (int p)
{
    page = p;
    tabSound.setToggleState (p == 0, juce::dontSendNotification); tabMod.setToggleState (p == 1, juce::dontSendNotification);
    for (auto* pl : { &pipePlate, &resPlate, &spacePlate, &fmPlate, &dlPlate }) pl->setVisible (p == 0);
    for (auto* pl : { &modPlate, &learnPlate, &perfPlate }) pl->setVisible (p == 1);
}

ZephyrContent::~ZephyrContent() { stopTimer(); setLookAndFeel (nullptr); }

KnobUnit* ZephyrContent::knob (juce::Component& parent, P id, const juce::String& text, const char* lo, const char* hi, double detent)
{
    auto k = std::make_unique<KnobUnit> (proc.apvts, paramDefs()[id].id, text, lo, hi);
    if (detent > -1e29) k->slider.detent = detent;
    parent.addAndMakeVisible (*k);
    byId[id] = k.get();
    knobs.push_back (std::move (k));
    return knobs.back().get();
}

LedToggle* ZephyrContent::toggle (juce::Component& parent, P id, const juce::String& text)
{
    auto t = std::make_unique<LedToggle>();
    t->setButtonText (text);
    parent.addAndMakeVisible (*t);
    att.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, paramDefs()[id].id, *t));
    toggles.push_back (std::move (t));
    return toggles.back().get();
}

juce::ComboBox* ZephyrContent::combo (juce::Component& parent, P id)
{
    auto cb = std::make_unique<juce::ComboBox>();
    cb->addItemList (ZephyrProcessor::choiceNames (id), 1);
    cb->setScrollWheelEnabled (false);
    parent.addAndMakeVisible (*cb);
    comboAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, paramDefs()[id].id, *cb));
    combos.push_back (std::move (cb));
    return combos.back().get();
}

SegmentSelector* ZephyrContent::selector (juce::Component& parent, P id)
{
    auto s = std::make_unique<SegmentSelector> (proc.apvts, paramDefs()[id].id, ZephyrProcessor::choiceNames (id));
    parent.addAndMakeVisible (*s);
    auto* r = s.get();
    owned.push_back (std::move (s));
    return r;
}

// ---------------------------------------------------------------- facade
void ZephyrContent::buildPanelTex()
{
    const int W = designW, H = designH;
    panelTex = juce::Image (juce::Image::RGB, W, H, false);
    juce::Random rr (4242);
    {
        juce::Image::BitmapData bd (panelTex, juce::Image::BitmapData::writeOnly);
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
            {
                const int v = 19 + rr.nextInt (2);
                bd.setPixelColour (x, y, juce::Colour::fromRGB ((juce::uint8) v, (juce::uint8) v, (juce::uint8) (v + 1)));
            }
    }
    juce::Graphics g (panelTex);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.035f), 0.f, 0.f, juce::Colours::transparentWhite, 0.f, 90.f, false));
    g.fillRect (0, 0, W, 90);
    g.setColour (juce::Colour (0xff3a3b3f));
    g.drawRect (0, 0, W, H, 2);
    g.setColour (look::print.withAlpha (0.12f));
    g.fillRect (12, 70, W - 24, 1);

    g.setColour (look::print.withAlpha (0.93f));
    g.setFont (font (40.f));
    g.drawText ("ZEPHYR", 26, 6, 300, 46, juce::Justification::centredLeft, false);
    g.setColour (juce::Colour (0xffc8281c));
    g.fillRect (28, 54, 90, 3);
    g.setColour (look::print.withAlpha (0.85f));
    g.setFont (font (10.5f));
    g.drawText ("FEEDBACK SYNTH   -   FOUDRE AUDIO", 28, 56, 360, 14, juce::Justification::centredLeft, false);
    g.setFont (font (9.5f));
    g.drawText ("PRESET", W - 400, 8, 80, 13, juce::Justification::centredLeft, false);

    const juce::Point<float> screws[] = { { 16.f, 16.f }, { (float) W - 16.f, 16.f }, { 16.f, (float) H - 16.f }, { (float) W - 16.f, (float) H - 16.f } };
    for (auto p : screws)
    {
        g.setColour (juce::Colour (0xff8a8a90)); g.fillEllipse (p.x - 5.f, p.y - 5.f, 10.f, 10.f);
        g.setColour (juce::Colour (0xff2a2a2d)); g.drawLine (p.x - 3.5f, p.y - 1.f, p.x + 3.5f, p.y + 1.5f, 1.4f);
    }
    for (int i = 0; i < 12000; ++i)
    {
        g.setColour (juce::Colour (0xff121213).withAlpha (0.92f));
        g.fillRect (rr.nextInt (W), rr.nextInt (H), 1 + rr.nextInt (2), 1 + rr.nextInt (2));
    }
    for (int i = 0; i < 240; ++i)
    {
        const bool edge = rr.nextInt (2) == 0;
        const int x = edge ? (rr.nextBool() ? rr.nextInt (14) : W - 1 - rr.nextInt (14)) : rr.nextInt (W);
        const int y = edge ? rr.nextInt (H) : (rr.nextBool() ? rr.nextInt (14) : H - 1 - rr.nextInt (14));
        g.setColour (juce::Colour (0xffb5b5ba).withAlpha (0.25f + 0.4f * rr.nextFloat()));
        g.fillRect (x, y, 1 + rr.nextInt (3), 1);
    }
}


void ZephyrContent::drawBolt (juce::Graphics& g, juce::Point<float> from, juce::Point<float> to)
{
    juce::Path p;
    p.startNewSubPath (from);
    const int seg = 7;
    for (int i = 1; i < seg; ++i)
    {
        const float t = (float) i / (float) seg;
        p.lineTo (from + (to - from) * t + juce::Point<float> ((rnd.nextFloat() - 0.5f) * 16.f, (rnd.nextFloat() - 0.5f) * 10.f));
    }
    p.lineTo (to);
    g.setColour (juce::Colour (0xff8fb8ff).withAlpha (0.35f));
    g.strokePath (p, juce::PathStrokeType (5.f));
    g.setColour (juce::Colour (0xfff2f7ff));
    g.strokePath (p, juce::PathStrokeType (1.6f));
}

void ZephyrContent::drawTube (juce::Graphics& g, juce::Rectangle<float> r, float glow, float arc)
{
    const auto base = r.removeFromBottom (r.getHeight() * 0.13f);
    const auto glass = r.withTrimmedBottom (-8.f);
    const auto col = heatColour (glow);
    if (glow > 0.02f)
    {
        g.setGradientFill (juce::ColourGradient (col.withAlpha (0.55f * glow), glass.getCentreX(), glass.getCentreY() + 4.f,
                                                 col.withAlpha (0.f), glass.getCentreX(), glass.getCentreY() - glass.getHeight() * 0.95f, true));
        g.fillEllipse (glass.expanded (glass.getWidth() * 0.9f));
    }
    g.setColour (juce::Colour (0xff050505));
    g.fillRoundedRectangle (glass, glass.getWidth() * 0.5f);
    auto plate = glass.reduced (glass.getWidth() * 0.22f, 0.f).withTrimmedTop (glass.getHeight() * 0.22f).withTrimmedBottom (glass.getHeight() * 0.18f);
    g.setColour (col);
    g.fillRoundedRectangle (plate, 3.f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    for (float x = plate.getX() + 3.f; x < plate.getRight(); x += 5.f)
        g.drawVerticalLine ((int) x, plate.getY(), plate.getBottom());
    g.setColour (juce::Colour (0xffff9a3a).withAlpha (0.25f + 0.7f * glow));
    g.fillEllipse (glass.getCentreX() - 4.f, glass.getBottom() - glass.getHeight() * 0.14f, 8.f, 6.f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.28f), glass.getX(), glass.getY(),
                                             juce::Colours::white.withAlpha (0.f), glass.getX() + glass.getWidth() * 0.45f, glass.getY(), false));
    g.fillRoundedRectangle (glass.reduced (2.f), glass.getWidth() * 0.45f);
    g.setColour (juce::Colour (0xff8d8d94).withAlpha (0.8f));
    g.drawRoundedRectangle (glass, glass.getWidth() * 0.5f, 1.4f);
    if (arc > 0.2f)
        drawBolt (g, { glass.getCentreX(), glass.getY() + 4.f }, { plate.getCentreX() + (rnd.nextFloat() - 0.5f) * 10.f, plate.getBottom() });
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6c6c72), base.getX(), base.getY(),
                                             juce::Colour (0xff1a1a1c), base.getX(), base.getBottom(), false));
    g.fillRoundedRectangle (base.expanded (3.f, 0.f), 3.f);
}


void ZephyrContent::paint (juce::Graphics& g) { g.drawImageAt (panelTex, 0, 0); }

// ---------------------------------------------------------------- animation
void ZephyrContent::timerCallback()
{
    auto& s = proc.synth;
    const float pk = s.outPk.load(), press = s.lastPressure.load();
    const float heat = juce::jlimit (0.f, 1.f, 0.8f * pk + 0.5f * press);
    heatShown += 0.15f * (heat - heatShown);
    arcShown = juce::jmax (arcShown * 0.82f, pk > 0.85f ? 1.f : 0.f);
    pipeView.repaint();

    outBar->set (pk);
    pressBar->set (press);
    lcdPress->set (juce::String ((int) std::round (press * 100.f)) + " %");
    lcdVoices->set (juce::String (s.activeVoices.load()));
    lcdPeak->set (pk < 0.001f ? juce::String ("-inf") : juce::String (20.f * std::log10 (pk), 1));
    for (int i = 0; i < kVoices; ++i) voiceBar[i]->set (s.vLvl[i].load());
    const int mc = s.midiCount.load();
    {
        const int bd = juce::jlimit (0, 9, juce::roundToInt (proc.apvts.getRawParameterValue ("mode")->load()));
        if (bd != helpShown)
        {
            helpShown = bd;
            static const char* tips[10] { "Wind picks the harmonic of the pipe", "A resonant filter that rings by itself", "Feedback = bow pressure.  Breath = bow speed",
                                         "Feedback = blowing pressure.  Breath = air", "Focus = pick position.  Feedback = sustain", "Wind morphs bar > bell > plate > bowl",
                                         "A mic and a speaker in a room : the howl wanders", "Wind morphs the vowel  A E I O U",
                                         "Filter ladder : warm, dark, asymmetric.  Bright = tone", "Multimode filter : Bright sweeps LP > BP > HP" };
            bodyHelp.setText (tips[bd], juce::dontSendNotification);
        }
    }
    for (int i = 0; i < 2; ++i)
    {
        const int cc = proc.learnCC[i].load(), arm = proc.learnArm.load();
        lcdLearn[i]->set (arm == i + 1 ? juce::String ("MOVE A KNOB...") : (cc < 0 ? juce::String ("not set") : "CC " + juce::String (cc)));
        learnBtn[i].setToggleState (arm == i + 1, juce::dontSendNotification);
    }
    if (mc != lastMidi) { lastMidi = mc; midiFlash = 6; }
    midiLamp.set (midiFlash > 0);
    if (midiFlash > 0) --midiFlash;
}

// ---------------------------------------------------------------- presets
void ZephyrContent::afterPreset() { nameBtn.setButtonText (proc.presetName); }

void ZephyrContent::stepPreset (int dir)
{
    juce::StringArray all;
    const int nf = ZephyrProcessor::getNumFactoryPresets();
    for (int i = 0; i < nf; ++i) all.add (ZephyrProcessor::getFactoryPresetName (i));
    all.addArray (proc.getUserPresetNames());
    int cur = all.indexOf (proc.presetName);
    cur = cur < 0 ? (dir > 0 ? -1 : 0) : cur;
    const int next = (cur + dir + all.size()) % juce::jmax (1, all.size());
    if (next < nf) proc.loadFactoryPreset (next); else proc.loadUserPreset (all[next]);
    afterPreset();
}

void ZephyrContent::presetMenu()
{
    juce::PopupMenu m;
    m.addSectionHeader ("FACTORY");
    for (int i = 0; i < ZephyrProcessor::getNumFactoryPresets(); ++i)
        m.addItem (100 + i, ZephyrProcessor::getFactoryPresetName (i), true, ZephyrProcessor::getFactoryPresetName (i) == proc.presetName);
    const auto user = proc.getUserPresetNames();
    if (user.size() > 0)
    {
        m.addSeparator();
        m.addSectionHeader ("USER");
        for (int i = 0; i < user.size(); ++i) m.addItem (1000 + i, user[i], true, user[i] == proc.presetName);
    }
    m.addSeparator();
    m.addItem (1, "Save as...");
    if (user.contains (proc.presetName))
    {
        m.addItem (2, "Overwrite \"" + proc.presetName + "\"");
        m.addItem (3, "Delete \"" + proc.presetName + "\"");
    }
    m.addItem (4, "Open preset folder");

    juce::Component::SafePointer<ZephyrContent> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&nameBtn), [safe, user] (int id)
    {
        if (safe == nullptr || id == 0) return;
        auto& pr = safe->proc;
        if (id >= 1000)      { pr.loadUserPreset (user[id - 1000]); safe->afterPreset(); }
        else if (id >= 100)  { pr.loadFactoryPreset (id - 100); safe->afterPreset(); }
        else if (id == 1)    safe->saveDialog();
        else if (id == 2)    { pr.saveUserPreset (pr.presetName); safe->afterPreset(); }
        else if (id == 3)
        {
            const auto name = pr.presetName;
            safe->confirm ("Supprimer le preset", "Supprimer definitivement \"" + name + "\" ?", [safe, name]
            {
                if (safe != nullptr) { safe->proc.deleteUserPreset (name); safe->proc.presetName = "Init"; safe->afterPreset(); }
            });
        }
        else if (id == 4)    { pr.getPresetDir().createDirectory(); pr.getPresetDir().revealToUser(); }
    });
}

void ZephyrContent::showDialog (const juce::String& title, const juce::String& message, bool askText, const juce::String& initial,
                                const juce::String& okText, std::function<void (bool, const juce::String&)> cb)
{
    dialog = std::make_unique<OrageDialog> (title, message, askText, initial, okText, std::move (cb));
    juce::Component::SafePointer<ZephyrContent> safe (this);
    dialog->onFinished = [safe] { juce::MessageManager::callAsync ([safe] { if (safe != nullptr) safe->dialog.reset(); }); };
    addAndMakeVisible (*dialog);
    dialog->setBounds (getLocalBounds());
    dialog->toFront (true);
    dialog->focus();
}

void ZephyrContent::saveDialog()
{
    juce::Component::SafePointer<ZephyrContent> safe (this);
    showDialog ("SAUVEGARDER LE PRESET", "Nom du preset :", true, proc.presetName, "SAUVEGARDER", [safe] (bool ok, const juce::String& text)
    {
        if (ok && safe != nullptr) { safe->proc.saveUserPreset (text); safe->afterPreset(); }
    });
}

void ZephyrContent::confirm (const juce::String& title, const juce::String& message, std::function<void()> action)
{
    showDialog (title, message, false, {}, "OK", [action] (bool ok, const juce::String&) { if (ok) action(); });
}

// ================================================================ fenetre redimensionnable
ZephyrEditor::ZephyrEditor (ZephyrProcessor& p) : AudioProcessorEditor (&p), content (p)
{
    addAndMakeVisible (content);
    const int dw = ZephyrContent::designW, dh = ZephyrContent::designH;
    float s = 1.f;                                                  // taille initiale : tient dans l'ecran
    if (auto* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        s = juce::jmin (1.f, (float) (d->userArea.getHeight() - 90) / (float) dh, (float) (d->userArea.getWidth() - 20) / (float) dw);
    s = juce::jmax (0.75f, s);
    constrainer.setFixedAspectRatio ((double) dw / (double) dh);
    constrainer.setSizeLimits (dw * 7 / 10, dh * 7 / 10, dw * 3 / 2, dh * 3 / 2);
    setConstrainer (&constrainer);
    setResizable (true, true);
    setSize (juce::roundToInt ((float) dw * s), juce::roundToInt ((float) dh * s));
}

void ZephyrEditor::resized()
{
    content.setTransform ({});
    content.setBounds (0, 0, ZephyrContent::designW, ZephyrContent::designH);
    content.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) ZephyrContent::designW));
}
