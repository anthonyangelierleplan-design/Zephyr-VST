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

// ---------------------------------------------------------------- apercu de la forme d'onde du LFO
float LfoView::value (const juce::String& id) const
{
    if (auto* p = apvts.getRawParameterValue (id)) return p->load();
    return 0.f;
}

void LfoView::paint (juce::Graphics& g)
{
    const auto& th = look::T();
    const auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (th.screenBg));
    g.fillRoundedRectangle (b, 3.f);
    g.setColour (juce::Colour (th.scope).withAlpha (0.12f));
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
    g.setColour (juce::Colour (th.scope).withAlpha (0.3f));
    g.strokePath (p, juce::PathStrokeType (4.f));
    g.setColour (juce::Colour (th.scope).brighter (0.4f));
    g.strokePath (p, juce::PathStrokeType (1.6f));
    g.setColour (juce::Colour (th.accent).withAlpha (0.6f));
    g.drawRoundedRectangle (b.reduced (0.5f), 3.f, 1.f);
}

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

SynthPanel::SynthPanel (ZephyrAudioProcessor& p) : proc (p)
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
    // potards alignes verticalement au centre de la zone ; la largeur des cellules s'adapte a la place disponible
    auto place = [] (juce::Rectangle<int> r, std::vector<KnobUnit*> ks, int x0, int cellW)
    {
        cellW = juce::jmin (cellW, (r.getWidth() - x0) / juce::jmax (1, (int) ks.size()));
        int x = r.getX() + x0;
        const int kh = juce::jmin (r.getHeight(), cellW + 28), y = r.getY() + (r.getHeight() - kh) / 2;
        for (auto* k : ks) { k->setBounds (x, y, cellW, kh); x += cellW; }
    };
    const juce::StringArray waves { "SAW", "PULSE", "TRI", "SIN" }, octs { "-2", "-1", "0", "+1", "+2" };
    const juce::StringArray lfoWaves { "SIN", "TRI", "SAW", "SQR", "S&H" };

    // ---- rang A : oscillateurs, melangeur, modulation (indices 0..3)
    {
        auto* u = unit ("OSCILLATOR 1");
        auto* w = selector (*u, "s_wave1", waves); auto* o = selector (*u, "s_oct1", octs);
        auto* pw = knob (*u, "s_pw1", "PW");
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            w->setBounds (r.getX(), r.getY() + 6, 196, 46); o->setBounds (r.getX(), r.getY() + 62, 196, 34);
            const int kw = juce::jmin (88, r.getWidth() - 200);
            pw->setBounds (r.getX() + 200, r.getY() + (r.getHeight() - (kw + 28)) / 2, kw, kw + 28);
        };
    }
    {
        auto* u = unit ("OSCILLATOR 2");
        auto* w = selector (*u, "s_wave2", waves); auto* o = selector (*u, "s_oct2", octs);
        auto* tu = knob (*u, "s_tune2", "PITCH", "-12", "+12"); auto* fi = knob (*u, "s_fine2", "FINE", "-", "+"); auto* pw = knob (*u, "s_pw2", "PW");
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            w->setBounds (r.getX(), r.getY() + 6, 196, 46); o->setBounds (r.getX(), r.getY() + 62, 196, 34);
            place (r, { tu, fi, pw }, 204, 88);
        };
    }
    {
        auto* u = unit ("MIXER");
        auto* a = knob (*u, "s_lvl1", "OSC 1"); auto* b = knob (*u, "s_lvl2", "OSC 2"); auto* sb = knob (*u, "s_sub", "SUB");
        auto* c = knob (*u, "s_noise", "NOISE"); auto* d = knob (*u, "s_master", "VOLUME");
        u->layoutFn = [=] (juce::Rectangle<int> r) { place (r, { a, b, sb, c, d }, 0, 80); };
    }
    {
        auto* u = unit ("MODULATION  &  DRIFT");
        auto* sy = toggle (*u, "s_sync", "HARD SYNC");
        auto* fm = knob (*u, "s_fm", "FM"); auto* rg = knob (*u, "s_ring", "RING"); auto* gl = knob (*u, "s_glide", "GLIDE");
        auto* dr = knob (*u, "s_drift", "DRIFT");
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            sy->setBounds (r.getX(), r.getY() + 4, 124, 30);
            place (r.withTrimmedTop (36), { fm, rg, gl, dr }, 0, 84);
        };
    }

    // ---- rang B : filtre, enveloppes, wavefolder (indices 4..7)
    {
        auto* u = unit ("VOLTAGE CONTROLLED FILTER");
        auto* ft = selector (*u, "s_ftype", juce::StringArray { "SEM", "LADDER", "PROPHET" });
        auto* h = knob (*u, "s_hpf", "HPF"); auto* hr = knob (*u, "s_hpfRes", "HPF PEAK");
        auto* a = knob (*u, "s_lpf", "CUTOFF"); auto* b = knob (*u, "s_lpfRes", "RESONANCE"); auto* c = knob (*u, "s_fltEnv", "EG AMOUNT");
        auto* d = knob (*u, "s_key", "KEY TRACK"); auto* e = knob (*u, "s_drive", "SATURATION"); auto* mo = knob (*u, "s_semMorph", "SEM MORPH");
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            ft->setBounds (r.getX(), r.getY() + 2, juce::jmin (330, r.getWidth()), 34);
            place (r.withTrimmedTop (38), { h, hr, a, b, c, d, e, mo }, 0, 84);
        };
    }
    {
        auto* u = unit ("ENVELOPE 1  (FILTER)");
        auto* a = knob (*u, "s_fA", "ATTACK"); auto* d = knob (*u, "s_fD", "DECAY"); auto* s = knob (*u, "s_fS", "SUSTAIN"); auto* r2 = knob (*u, "s_fR", "RELEASE");
        u->layoutFn = [=] (juce::Rectangle<int> r) { place (r, { a, d, s, r2 }, 0, 84); };
    }
    {
        auto* u = unit ("ENVELOPE 2  (AMP)");
        auto* a = knob (*u, "s_aA", "ATTACK"); auto* d = knob (*u, "s_aD", "DECAY"); auto* s = knob (*u, "s_aS", "SUSTAIN"); auto* r2 = knob (*u, "s_aR", "RELEASE");
        u->layoutFn = [=] (juce::Rectangle<int> r) { place (r, { a, d, s, r2 }, 0, 84); };
    }
    {
        auto* u = unit ("WAVEFOLDER");
        auto* a = knob (*u, "s_fold", "FOLD"); auto* b = knob (*u, "s_foldSym", "SYMMETRY", "-", "+");
        u->layoutFn = [=] (juce::Rectangle<int> r) { place (r, { a, b }, 0, 84); };
    }

    // ---- rang C : un LFO (choisi dans un menu), poly-mod, macros, key assign, arpegiateur (indices 8..12)
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
        u->layoutFn = [sets, pk] (juce::Rectangle<int> r)
        {
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
        auto* u = unit ("POLY-MOD");
        auto* a = knob (*u, "s_pmEnv", "FILTER ENV"); auto* b = knob (*u, "s_pmOsc", "OSC 2");
        auto* t1 = toggle (*u, "s_pmFreq", "FREQ OSC 1"); auto* t2 = toggle (*u, "s_pmPw", "PW OSC 1"); auto* t3 = toggle (*u, "s_pmFilt", "FILTER");
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            const int cw = r.getWidth() / 2, kh = juce::jmin (r.getHeight() - 120, cw + 22);
            a->setBounds (r.getX(), r.getY() + 4, cw, kh); b->setBounds (r.getX() + cw, r.getY() + 4, cw, kh);
            int y = r.getY() + kh + 12;
            for (auto* t : { t1, t2, t3 }) { t->setBounds (r.getX() + 4, y, r.getWidth() - 8, 30); y += 34; }
        };
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
        auto* u = unit ("ARPEGGIATOR");
        auto* on = selector (*u, "s_arpOn", juce::StringArray { "OFF", "ON", "LATCH" });
        auto* mode = selector (*u, "s_arpMode", juce::StringArray { "UP", "DOWN", "UP-DN" });
        auto* oct = selector (*u, "s_arpOct", juce::StringArray { "1 OCT", "2 OCT", "FULL" });
        auto* rate = knob (*u, "s_arpRate", "CLOCK"); auto* sync = combo (*u, "s_arpSync");
        auto* shift = knob (*u, "s_arpShift", "SHIFT"); auto* clus = knob (*u, "s_arpCluster", "CLUSTER");     // decalages du mode POLY
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            const int lw = juce::jmin (196, (int) (r.getWidth() * 0.52f));
            on->setBounds (r.getX(), r.getY(), lw, 46); mode->setBounds (r.getX(), r.getY() + 54, lw, 46); oct->setBounds (r.getX(), r.getY() + 108, lw, 46);
            const int x2 = r.getX() + lw + 10, rw = r.getRight() - x2;
            sync->setBounds (x2, r.getY() + 2, rw, 28);
            rate->setBounds (x2 + (rw - juce::jmin (rw, 96)) / 2, r.getY() + 32, juce::jmin (rw, 96), 100);
            shift->setBounds (x2, r.getY() + 134, rw / 2, r.getHeight() - 134);
            clus->setBounds (x2 + rw / 2, r.getY() + 134, rw / 2, r.getHeight() - 134);
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
        if ((int) k == proc.learnMacro.load()) name = "LEARN CC...";
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

void SynthPanel::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu()) return;
    for (size_t k = 0; k < macroKnobs.size(); ++k)
        if (e.eventComponent == &macroKnobs[k]->slider)
        {
            juce::PopupMenu m;
            m.addItem (1, "MIDI learn : bouge un potard de ton clavier (CC actuel " + juce::String (proc.macroCC[k].load()) + ")");
            m.addItem (2, "CC par defaut (" + juce::String (21 + (int) k) + ")");
            m.showMenuAsync (juce::PopupMenu::Options(), [this, k] (int r)
            {
                if (r == 1) proc.learnMacro.store ((int) k);
                else if (r == 2) { proc.macroCC[k].store (21 + (int) k); proc.learnMacro.store (-1); }
            });
            return;
        }
}

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
    row (0, 212, { { 0, 316.f }, { 1, 490.f }, { 2, 432.f }, { 3, 380.f } });
    row (220, 212, { { 4, 730.f }, { 5, 360.f }, { 6, 360.f }, { 7, 200.f } });
    row (440, 276, { { 8, 320.f }, { 9, 220.f }, { 10, 340.f }, { 11, 270.f }, { 12, 390.f } });

    paintFacade (tex, W, getHeight(), false);
}

// ---------------------------------------------------------------- volet de la matrice
namespace
{
struct Fam { const char* title; std::vector<int> items; };
void fillGrouped (juce::ComboBox& cb, const std::vector<Fam>& fams, const char* const* names)
{
    cb.addItem ("-", 1);
    for (const auto& f : fams)
    {
        cb.addSectionHeading (f.title);
        for (int i : f.items) cb.addItem (names[i], i + 1);                       // id = index du parametre + 1
    }
}
const std::vector<Fam>& srcFamilies()
{
    static const std::vector<Fam> f { { "LFO", { MsLfo1, MsLfo2, MsLfo3, MsLfo4 } }, { "MACROS", { MsMac1, MsMac2, MsMac3, MsMac4, MsMac5, MsMac6, MsMac7, MsMac8 } },
                                      { "ENVELOPES", { MsEnv, MsAmpEnv } }, { "PERFORMANCE", { MsVel, MsWheel, MsKey, MsChanAT, MsPolyAT } }, { "RANDOM", { MsRandom } } };
    return f;
}
const std::vector<Fam>& dstFamilies()
{
    static const std::vector<Fam> f {
        { "PITCH", { MdPitch, MdPitch2, MdDrift } },
        { "OSCILLATORS", { MdPw1, MdPw2, MdFm, MdRing, MdPolyMod, MdLvl1, MdMix, MdSub, MdNoise } },
        { "FILTER", { MdHpf, MdHpfReso, MdLpf, MdReso, MdSat, MdMorph, MdFltEnv } },
        { "WAVEFOLDER", { MdFold, MdFoldSym } },
        { "AMP / SPACE", { MdLevel, MdPan } },
        { "FX RACK", { MdFxDrive, MdFxWidth, MdFxHpf, MdFxLpf, MdFxPeak, MdFxPhMix, MdFxPhRate, MdFxPhFb, MdFxEnsMix, MdFxEnsDepth, MdFxMix } },
        { "DELAY", { MdDlyTime, MdDlyFb, MdDlyMix } },
        { "REVERB", { MdRevMix, MdRevDecay, MdRevDamp } } };
    return f;
}
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
        comboAtt.push_back (std::make_unique<IdComboAttachment> (*apvts.getParameter (pre + "Src"), r->src));
        comboAtt.push_back (std::make_unique<IdComboAttachment> (*apvts.getParameter (pre + "Dst"), r->dst));
        sliderAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, pre + "Amt", r->amt));
        rows.push_back (std::move (r));
    }
}

void MatrixDrawer::paint (juce::Graphics& g)
{
    const auto& th = look::T();
    const auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (b.translated (0.f, 4.f), 7.f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (th.plateTop), 0.f, 0.f, juce::Colour (th.plateBot), 0.f, b.getBottom(), false));
    g.fillRoundedRectangle (b, 7.f);
    g.setColour (juce::Colour (th.accent).withAlpha (0.8f));
    g.drawRoundedRectangle (b.reduced (1.f), 7.f, 1.3f);
    g.setColour (look::print.withAlpha (0.95f));
    g.setFont (bold (13.f));
    g.drawText ("MODULATION MATRIX   (reaches the filter, the FX rack, the delay and the reverb)", 14, 6, getWidth() - 28, 18, juce::Justification::centredLeft, false);
    g.setColour (juce::Colour (th.accent));
    g.fillRect (14, 27, 60, 2);
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

// ---------------------------------------------------------------- fenetre : flancs en bois, onglets (synthe) + mise a l'echelle

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
        cheekL.themeChanged(); cheekR.themeChanged();
        repaint();
    };
    fx.dialogHost = &content;
#if ZEPHYR_SYNTH
    synthPanel = std::make_unique<SynthPanel> (p);
    content.addAndMakeVisible (*synthPanel);
    for (auto* b : { &synthTab, &fxTab })
    {
        b->setClickingTogglesState (true);
        b->setRadioGroupId (4242);
        content.addAndMakeVisible (*b);
    }
    synthTab.onClick = [this] { showTab (0); };
    fxTab.onClick    = [this] { showTab (1); };
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
    s = juce::jmax (0.5f, s);
    constrainer.setFixedAspectRatio ((double) designWidth() / (double) dh);
    constrainer.setSizeLimits (designWidth() / 2, dh / 2, designWidth() * 3 / 2, dh * 3 / 2);
    setConstrainer (&constrainer);
    setResizable (true, true);
    setSize (juce::roundToInt ((float) designWidth() * s), juce::roundToInt ((float) dh * s));
}

SynthEditor::~SynthEditor() { setLookAndFeel (nullptr); }

void SynthEditor::showTab (int t)
{
    if (synthPanel != nullptr) synthPanel->setVisible (t == 0);
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
    fxTab.setBounds (kCheek + 166, 3, 150, 24);
    matrixBtn.setBounds (kCheek + 322, 3, 150, 24);
    fx.setBounds (kCheek, 30, kDesignW, kFxH);
    synthPanel->setBounds (kCheek, 30 + kHeaderH + 4, kDesignW, kFxH - kHeaderH - 4);
    matrix->setBounds (kCheek + 322, 32, 640, 42 + kSlots * 32 + 10);
    matrix->toFront (false);
#else
    fx.setBounds (kCheek, 0, kDesignW, kFxH);
#endif
}
