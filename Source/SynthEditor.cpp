#include "SynthEditor.h"
#include <cstdlib>
#include "FoudreAccords.h"
#include <complex>

using namespace zephyr;

namespace { juce::Font bold (float h) { return juce::Font (juce::FontOptions (h, juce::Font::bold)); } }

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

float LfoView::shapeAt (float ph, int w, float sl) const
{
    if (w != 4) return zephyr::Synth::lfoWave (ph, w, 0.f);
    static const float steps[4] { 0.55f, -0.7f, 0.25f, -0.25f };                // valeurs types d'un S&H, un cycle = 4 marches
    const float pos = ph * 4.f;
    const int k = juce::jlimit (0, 3, (int) pos);
    const float frac = pos - (float) k, r = juce::jmax (0.001f, sl * 0.9f);
    const float t = juce::jlimit (0.f, 1.f, (frac - (1.f - r)) / r);
    return steps[k] + (steps[(k + 1) % 4] - steps[k]) * t * t * (3.f - 2.f * t);
}

void LfoView::paint (juce::Graphics& g)
{   // v7 : l'onde defile comme sur un oscilloscope : le bord droit = maintenant, elle glisse vers la gauche a la vitesse du LFO
    const auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff050506));
    g.fillRect (b);
    g.setColour (juce::Colour (0xff3a1c10));
    for (int i = 1; i < 4; ++i) g.drawHorizontalLine ((int) (b.getY() + b.getHeight() * (float) i / 4.f), b.getX() + 2.f, b.getRight() - 2.f);
    for (int i = 1; i < 4; ++i) g.drawVerticalLine ((int) (b.getX() + b.getWidth() * (float) i / 4.f), b.getY() + 2.f, b.getBottom() - 2.f);
    const int w = juce::roundToInt (value (wave));
    const float sl = value (slew);
    auto yOf = [&] (float v) { return b.getCentreY() - v * (b.getHeight() * 0.38f); };
    const bool live = synth != nullptr && synth->shownLive[idx].load (std::memory_order_relaxed);
    const float now = live ? synth->shownPh[idx].load (std::memory_order_relaxed) : 1.f;      // RETRIG sans note : onde immobile
    auto phAt = [&] (float x01) { float ph = now - 1.f + x01; ph -= std::floor (ph); return juce::jlimit (0.f, 0.9999f, ph); };
    juce::Path p;
    const int n = (int) b.getWidth();
    for (int i = 0; i <= n; ++i)
    {
        const float px = b.getX() + (float) i, py = yOf (shapeAt (phAt ((float) i / (float) n), w, sl));
        if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
    }
    g.setColour (juce::Colour (0xffff7a1a).withAlpha (0.3f));
    g.strokePath (p, juce::PathStrokeType (4.f));
    g.setColour (juce::Colour (0xffffb05a));
    g.strokePath (p, juce::PathStrokeType (1.6f));
    if (live)
    {   // valeur actuelle au bord droit
        const float y = yOf (shapeAt (phAt (1.f), w, sl)), x = b.getRight() - 3.f;
        g.setColour (juce::Colour (0xffff7a1a).withAlpha (0.45f)); g.fillEllipse (x - 6.f, y - 6.f, 12.f, 12.f);
        g.setColour (juce::Colours::white); g.fillEllipse (x - 3.f, y - 3.f, 6.f, 6.f);
    }
    g.setColour (juce::Colour (0xffe9e5d8).withAlpha (0.6f));
    g.drawRect (b, 1.2f);
}


// ---------------------------------------------------------------- sequenceur : etiquettes, grille de pas, pistes de modulation
void Tag::paint (juce::Graphics& g)
{
    g.setColour (look::print.withAlpha (0.65f));
    g.setFont (bold (size));
    const int lines = juce::jmax (1, (int) ((float) getHeight() / (size + 3.f)));     // assez haut : le texte passe a la ligne au lieu d'etre coupe
    if (lines > 1) g.drawFittedText (text, getLocalBounds(), just, lines, 0.9f);
    else g.drawText (text, getLocalBounds(), just, true);
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
// v7 : 16 pas en 4 groupes de 4 (un ecart entre les groupes, comme les temps d'une mesure) ; meme geometrie pour la grille et les pistes
constexpr float kGrpGap = 7.f, kNumH = 16.f;                                    // ecart entre groupes ; hauteur de la ligne des numeros de pas
float stepW (float W) { return (W - 3.f * kGrpGap) / 16.f; }
float stepX (float W, int k) { return (float) k * stepW (W) + (float) (k / 4) * kGrpGap; }
int stepAt (float W, float x) { for (int k = 15; k > 0; --k) if (x >= stepX (W, k) - ((k % 4) == 0 ? kGrpGap * 0.5f : 0.f)) return k; return 0; }
}

StepGrid::StepGrid (ZephyrAudioProcessor& p) : proc (p) { startTimerHz (20); setWantsKeyboardFocus (true); }

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

// vitesses dans l'ordre musical (pour la molette) : x1/2, x1, x3/2, x2, x4
static const int kSpeedOrder[zephyr::kNumSpeeds] { 3, 0, 4, 1, 2 };
static int speedRank (int code) { for (int i = 0; i < zephyr::kNumSpeeds; ++i) if (kSpeedOrder[i] == code) return i; return 1; }
static juce::String speedText (int code) { return juce::String::fromUTF8 (code == 3 ? "\xc3\x97\xc2\xbd" : code == 4 ? "\xc3\x97" "3/2" : code == 1 ? "\xc3\x97" "2" : code == 2 ? "\xc3\x97" "4" : "\xc3\x97" "1"); }

void StepGrid::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (speedRow && (float) e.y > mainBottom())                                 // rangee STEP SPEED : la molette passe d'une vitesse a l'autre
    {
        const int k = stepAt ((float) getWidth(), (float) e.x);
        pushUndo (200 + k);
        const int r = juce::jlimit (0, zephyr::kNumSpeeds - 1, speedRank (proc.arpPat.speed[k].load()) + (w.deltaY > 0 ? 1 : -1));
        proc.arpPat.speed[k] = kSpeedOrder[r];
        repaint();
        return;
    }
    if (paramInt (proc.apvts, "s_arpType") != 1 || (float) e.y > mainBottom()) return;
    const int k = stepAt ((float) getWidth(), (float) e.x);
    const int d = (w.deltaY > 0 ? 1 : -1) * (e.mods.isShiftDown() ? 12 : 1);
    pushUndo (100 + k);
    proc.arpPat.transposeStep (k, d);                                              // v5 : un accord se transpose en bloc
    repaint();
}

void StepGrid::paint (juce::Graphics& g)
{   // v7 : numeros lisibles au-dessus des cases, groupes de 4 pas, accent = case plus lumineuse (plus de barre blanche), tete de lecture = cadre blanc
    const float W = (float) getWidth(), cw = stepW (W), mainB = mainBottom();
    const int len = paramInt (proc.apvts, "s_arpLen") + 1;
    const int cur = shown >= 0 ? shown % len : -1;
    const bool seqType = paramInt (proc.apvts, "s_arpType") == 1, recOn = seqType && paramInt (proc.apvts, "s_seqRec") != 0;
    const int recAt = proc.arpPat.recPos.load();
    for (int k = 0; k < 16; ++k)
    {
        const float dim = k < len ? 1.f : 0.3f, x = stepX (W, k);
        const bool sel = selA >= 0 && k >= juce::jmin (selA, selB) && k <= juce::jmax (selA, selB);
        if (sel)
        {   // v7 : pas selectionnes : pastille derriere le numero + trait sous la colonne
            g.setColour (ledCol().withAlpha (0.35f)); g.fillRoundedRectangle (x + 1.f, 0.f, cw - 2.f, kNumH - 1.f, 3.f);
            g.setColour (ledCol().brighter (0.4f)); g.fillRoundedRectangle (x + 3.f, (float) getHeight() - 2.f, cw - 6.f, 2.f, 1.f);
        }
        // numero du pas : le premier de chaque groupe (1, 5, 9, 13) a la couleur des LED
        g.setFont (bold (12.f));
        g.setColour (k == cur || sel ? juce::Colours::white : (k % 4 == 0 ? ledCol().brighter (0.3f) : look::print).withAlpha ((k % 4 == 0 ? 0.95f : 0.6f) * dim));
        g.drawText (juce::String (k + 1), juce::Rectangle<float> (x, 0.f, cw, kNumH - 2.f).toNearestInt(), juce::Justification::centred, false);
        auto cell = juce::Rectangle<float> (x + 2.f, kNumH, cw - 4.f, mainB - kNumH - 2.f);
        const int st = proc.arpPat.step[k].load(), rt = proc.arpPat.ratchet[k].load();
        drawWell (g, cell);
        if (st > 0)
        {
            auto lit = cell.reduced (3.f);
            if (st == 2) { g.setColour (ledCol().withAlpha (0.3f * dim)); g.fillRoundedRectangle (cell.expanded (2.f), 4.f); }     // accent : halo + case vive
            g.setGradientFill (juce::ColourGradient (st == 2 ? ledCol().brighter (0.55f).withAlpha (dim) : ledCol().darker (0.3f).withAlpha (dim), lit.getX(), lit.getY(),
                                                     st == 2 ? ledCol().brighter (0.1f).withAlpha (dim) : ledCol().darker (0.8f).withAlpha (dim), lit.getX(), lit.getBottom(), false));
            g.fillRoundedRectangle (lit, 2.f);
        }
        if (seqType && st > 0)
        {
            int nt[zephyr::kSeqChordMax]; const int nn = proc.arpPat.stepNotes (k, nt);
            g.setColour (juce::Colours::black.withAlpha (0.85f * dim));
            if (nn > 1)                                                             // Chord Seq (v5) : nom de l'accord, et sa note la plus grave en petit
            {
                int lo = nt[0]; for (int q = 1; q < nn; ++q) lo = juce::jmin (lo, nt[q]);
                auto nm = juce::String::fromUTF8 (foudre::accords::nomAccord (nt, nn).c_str());
                if (nm.isEmpty()) nm = noteName (lo) + "+" + juce::String (nn - 1);
                auto tr = cell.toNearestInt().reduced (2, 2);
                g.setFont (bold (13.f)); g.drawFittedText (nm, tr.withTrimmedBottom (tr.getHeight() / 3), juce::Justification::centred, 1, 0.6f);
                g.setFont (bold (10.f)); g.setColour (juce::Colours::black.withAlpha (0.65f * dim));
                g.drawText (juce::String::fromUTF8 (foudre::accords::nomNote (lo)) + juce::String (lo / 12 - 2), tr.withTrimmedTop (tr.getHeight() * 2 / 3), juce::Justification::centred, false);   // basse, en bemols comme le nom de l'accord
            }
            else
            {
                g.setFont (bold (14.f));
                g.drawText (noteName (nt[0]), cell.toNearestInt(), juce::Justification::centred, false);
            }
            if (proc.arpPat.tie[k].load() != 0) { g.setColour (juce::Colours::black.withAlpha (0.8f * dim)); g.fillRoundedRectangle (cell.getRight() - 9.f, cell.getCentreY() - 3.f, 12.f, 6.f, 2.f); }
        }
        const int spd = proc.arpPat.speed[k].load();
        if (spd != 0 && ! speedRow)                                                 // pas a vitesse propre : badge en haut de la case (visible aussi en vue RATCHET)
        {
            auto bd = juce::Rectangle<float> (cell.getRight() - 31.f, cell.getY() + 3.f, 28.f, 14.f);
            g.setColour (juce::Colours::black.withAlpha (0.75f * dim)); g.fillRoundedRectangle (bd, 3.f);
            g.setColour (ledCol().brighter (0.4f).withAlpha (dim)); g.setFont (bold (10.5f));
            g.drawText (speedText (spd), bd.toNearestInt(), juce::Justification::centred, false);
        }
        auto rc = juce::Rectangle<float> (x + 2.f, mainB + 2.f, cw - 4.f, (float) getHeight() - mainB - 4.f);
        drawWell (g, rc);
        if (speedRow)                                                               // vitesse du pas : x1/2, x1, x3/2, x2, x4 (x1 discret, les autres allumes)
        {
            g.setColour (spd != 0 ? ledCol().brighter (0.3f).withAlpha (dim) : look::print.withAlpha (0.4f * dim));
            g.setFont (bold (spd != 0 ? 15.f : 12.f));
            g.drawText (speedText (spd), rc.toNearestInt(), juce::Justification::centred, false);
        }
        else                                                                        // ratchet : 1 a 4 points
            for (int d = 0; d < 4; ++d)
            {
                const float px = rc.getX() + rc.getWidth() * (0.2f + 0.2f * (float) d), py = rc.getCentreY();
                g.setColour (d < rt && st > 0 ? ledCol().withAlpha (dim) : look::print.withAlpha (0.12f * dim));
                g.fillEllipse (px - 3.5f, py - 3.5f, 7.f, 7.f);
            }
        const auto col = juce::Rectangle<float> (x + 1.f, kNumH - 1.f, cw - 2.f, (float) getHeight() - kNumH);
        if (k == cur) { g.setColour (juce::Colours::white.withAlpha (0.9f)); g.drawRoundedRectangle (col, 3.f, 2.f); }
        if (recOn && k == recAt) { g.setColour (juce::Colour (0xffff4a3a)); g.drawRoundedRectangle (col, 3.f, 2.4f); }
    }
}

void StepGrid::apply (const juce::MouseEvent& e, bool first)
{
    const int k = stepAt ((float) getWidth(), (float) e.x);
    if (first) ratchetRow = (float) e.y > mainBottom();
    if (ratchetRow && speedRow)                                                     // clic : vitesse suivante (x1 > x2 > x4 > x1/2 > x3/2), clic droit : x1 ; glisser = meme vitesse sur les pas voisins
    {
        if (first) { const int sp = proc.arpPat.speed[k].load(); paintState = e.mods.isPopupMenu() ? 0 : (sp + 1) % zephyr::kNumSpeeds; }
        proc.arpPat.speed[k] = paintState;
    }
    else if (ratchetRow)
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
void StepGrid::mouseDown (const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    const int k = stepAt ((float) getWidth(), (float) e.x);
    if ((float) e.y < kNumH || (e.mods.isShiftDown() && (float) e.y <= mainBottom()))
    {   // v7 : selection = clic (ou glisser) sur les numeros, ou MAJ + clic sur les cases ; reclic sur un pas seul selectionne = deselection
        selecting = true;
        if (e.mods.isShiftDown() && selA >= 0) selB = k;
        else if (selA == k && selB == k) { selA = selB = -1; selecting = false; }
        else selA = selB = k;
        repaint();
        return;
    }
    selecting = false;
    pushUndo();
    apply (e, true);
}
void StepGrid::mouseDrag (const juce::MouseEvent& e)
{
    if (selecting) { const int k = stepAt ((float) getWidth(), (float) e.x); if (k != selB) { selB = k; repaint(); } return; }
    apply (e, false);
}

// ---- v7 : historique, duplication, raccourcis clavier
void StepGrid::pushUndo (int coalesce)
{
    auto& h = proc.arpHistory;
    const uint32_t now = juce::Time::getMillisecondCounter();
    if (coalesce >= 0 && coalesce == h.lastKey && now - h.lastMs < 800) { h.lastMs = now; return; }    // molette : un seul pas d'annulation par geste
    h.lastKey = coalesce; h.lastMs = now;
    zephyr::ArpSnapshot sn;
    for (int i = 0; i < zephyr::kArpSteps; ++i) sn.st[i] = proc.arpPat.get (i);
    sn.len = paramInt (proc.apvts, "s_arpLen") + 1;
    h.push (sn);
}

namespace
{
void setPatternLen (ZephyrAudioProcessor& p, int len)
{
    if (auto* prm = p.apvts.getParameter ("s_arpLen")) prm->setValueNotifyingHost (prm->convertTo0to1 ((float) (juce::jlimit (1, 16, len) - 1)));
}
}

void StepGrid::undoRedo (bool redo)
{
    auto& h = proc.arpHistory;
    auto& from = redo ? h.redo : h.undo;
    auto& to   = redo ? h.undo : h.redo;
    if (from.empty()) return;
    zephyr::ArpSnapshot cur;
    for (int i = 0; i < zephyr::kArpSteps; ++i) cur.st[i] = proc.arpPat.get (i);
    cur.len = paramInt (proc.apvts, "s_arpLen") + 1;
    to.push_back (cur);
    const auto sn = from.back(); from.pop_back();
    for (int i = 0; i < zephyr::kArpSteps; ++i) proc.arpPat.put (i, sn.st[i]);
    setPatternLen (proc, sn.len);
    h.lastKey = -1;
    if (auto* u = getParentComponent()) if (auto* panel = u->getParentComponent()) panel->repaint();     // la grille et les pistes
    repaint();
}

void StepGrid::duplicate()
{   // selection : copiee juste apres elle (la selection passe sur la copie : CTRL+D encore = on continue) ; sans selection : tout le motif, longueur doublee
    const int len = paramInt (proc.apvts, "s_arpLen") + 1;
    int a = 0, b = len - 1;
    if (selA >= 0) { a = juce::jmin (selA, selB); b = juce::jmax (selA, selB); }
    const int n = b - a + 1;
    if (b + 1 > 15) return;                                                     // plus de place apres
    pushUndo();
    zephyr::ArpPattern::StepData src[zephyr::kArpSteps];
    for (int i = a; i <= b; ++i) src[i - a] = proc.arpPat.get (i);
    const int end = juce::jmin (15, b + n);
    for (int i = b + 1; i <= end; ++i) proc.arpPat.put (i, src[i - b - 1]);
    if (end + 1 > len) setPatternLen (proc, end + 1);
    if (selA >= 0) { selA = b + 1; selB = end; }
    if (auto* u = getParentComponent()) if (auto* panel = u->getParentComponent()) panel->repaint();
    repaint();
}

bool StepGrid::keyPressed (const juce::KeyPress& key)
{
    const auto m = key.getModifiers();
    const auto c = juce::CharacterFunctions::toUpperCase ((juce::juce_wchar) key.getKeyCode());
    if (m.isCommandDown() && c == 'Z') { undoRedo (m.isShiftDown()); return true; }
    if (m.isCommandDown() && c == 'Y') { undoRedo (true); return true; }
    if (m.isCommandDown() && c == 'D') { duplicate(); return true; }
    if (key == juce::KeyPress::escapeKey && selA >= 0) { selA = selB = -1; repaint(); return true; }
    return false;
}

void StepGrid::randomize()
{
    pushUndo();
    juce::Random r;
    for (int k = 0; k < 16; ++k)
    {
        const float x = r.nextFloat();
        proc.arpPat.step[k] = x < 0.22f ? 0 : (x < 0.42f ? 2 : 1);
        proc.arpPat.ratchet[k] = r.nextFloat() < 0.18f ? 2 + r.nextInt (3) : 1;
    }
    repaint();
}
void StepGrid::clear() { pushUndo(); for (int k = 0; k < 16; ++k) { proc.arpPat.step[k] = 1; proc.arpPat.ratchet[k] = 1; proc.arpPat.speed[k] = 0; } repaint(); }

LaneEditor::LaneEditor (ZephyrAudioProcessor& p, int l) : proc (p), lane (l) { startTimerHz (20); setWantsKeyboardFocus (true); }

void LaneEditor::paint (juce::Graphics& g)
{
    const float W = (float) getWidth(), cw = stepW (W), H = (float) getHeight();
    const int len = paramInt (proc.apvts, "s_lane" + juce::String (lane + 1) + "Len") + 1;
    const int cur = shown >= 0 ? shown % len : -1;
    for (int k = 0; k < 16; ++k)
    {
        const float dim = k < len ? 1.f : 0.3f, v = proc.arpPat.lane[lane][k].load();
        auto cell = juce::Rectangle<float> (stepX (W, k) + 2.f, 2.f, cw - 4.f, H - 4.f);
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
        if (k == cur) { g.setColour (juce::Colours::white.withAlpha (0.9f)); g.drawRoundedRectangle (juce::Rectangle<float> (stepX (W, k) + 1.f, 1.f, cw - 2.f, H - 2.f), 3.f, 2.f); }
    }
}

void LaneEditor::apply (const juce::MouseEvent& e)
{
    const int k = stepAt ((float) getWidth(), (float) e.x);
    float v = 1.f - (float) (e.y - 5) / (float) (getHeight() - 10);
    v = juce::jlimit (0.f, 1.f, v);
    if (v < 0.04f) v = 0.f;
    proc.arpPat.lane[lane][k] = e.mods.isPopupMenu() ? 0.f : v;
    repaint();
}
void LaneEditor::randomize() { juce::Random r; for (int k = 0; k < 16; ++k) proc.arpPat.lane[lane][k] = r.nextFloat() < 0.15f ? 0.f : r.nextFloat(); repaint(); }
void LaneEditor::clear() { for (int k = 0; k < 16; ++k) proc.arpPat.lane[lane][k] = 0.f; repaint(); }


// ---------------------------------------------------------------- ecrans animes, lunettes, flancs en bois
juce::Rectangle<float> drawBezel (juce::Graphics& g, juce::Rectangle<float> b)
{
    const auto& th = look::T();
    g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (b.expanded (1.f, 1.f).translated (0.f, 1.5f), 5.f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (th.capHi).withAlpha (0.9f), b.getX(), b.getY(), juce::Colour (th.capLo), b.getX(), b.getBottom(), false));
    g.fillRoundedRectangle (b, 5.f);
    const auto in = b.reduced (3.5f);
    g.setColour (juce::Colours::black.withAlpha (0.85f)); g.fillRoundedRectangle (in.expanded (0.8f), 3.5f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0c0b0a), in.getX(), in.getY(), juce::Colour (0xff050505), in.getX(), in.getBottom(), false));
    g.fillRoundedRectangle (in, 3.f);
    return in.reduced (1.f);
}

void FilterView::paint (juce::Graphics& g)
{
    const auto b = drawBezel (g, getLocalBounds().toFloat());
    const juce::Colour ink = juce::Colour (look::T().scope);
    const int type = juce::roundToInt (v (0)); const float lpf = v (1), res = v (2), hpf = v (3), hres = v (4), morph = v (5);
    g.saveState(); g.reduceClipRegion (b.toNearestInt());
    const float f0 = 20.f, f1 = 20000.f, dbLo = -42.f, dbHi = 24.f;
    auto X = [&] (float hz) { return b.getX() + b.getWidth() * std::log (hz / f0) / std::log (f1 / f0); };
    auto Y = [&] (float db) { return b.getBottom() - b.getHeight() * (juce::jlimit (dbLo, dbHi, db) - dbLo) / (dbHi - dbLo); };
    g.setColour (ink.withAlpha (0.12f));
    for (float hz : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f }) g.drawVerticalLine ((int) X (hz), b.getY(), b.getBottom());
    for (float db : { -36.f, -24.f, -12.f, 0.f, 12.f }) g.drawHorizontalLine ((int) Y (db), b.getX(), b.getRight());
    g.setColour (ink.withAlpha (0.28f)); g.drawHorizontalLine ((int) Y (0.f), b.getX(), b.getRight());
    using C = std::complex<float>;
    const float fc = 25.f * std::pow (720.f, lpf), fh = 25.f * std::pow (600.f, hpf);
    const float qL = type == 0 ? 0.6f + std::pow (res, 1.6f) * 9.f : (type == 2 ? 0.55f + std::pow (res, 1.5f) * 7.f : 0.f);
    const float qH = 0.6f + std::pow (hres, 1.6f) * 7.f;
    juce::Path p; const int n = juce::jmax (8, (int) b.getWidth());
    for (int i = 0; i <= n; ++i)
    {
        const float hz = f0 * std::pow (f1 / f0, (float) i / (float) n);
        C s (0.f, hz / fc), H;
        if (type == 1 || type == 3)                                                      // echelle 4 poles a retour (Ladder, Prophet) : le pic se paie en gain de bande passante
        {
            const float kr = type == 3 ? zephyr::sy::proResLaw (res) : res;                   // Prophet : loi de resonance du Prophet-6
            const C p1 = C (1.f) + s, p2 = p1 * p1, G = C (1.f) / (p2 * p2); H = G / (C (1.f) + C (4.f * kr) * G);
        }
        else
        {
            const C lp = C (1.f) / (s * s + s / qL + C (1.f));
            if (type == 2) { const C hp = (s * s) * lp; H = morph < 0.5f ? lp + C (2.f * morph) * hp : C (2.f - 2.f * morph) * lp + hp; }
            else H = lp;
        }
        { const float dN = juce::jlimit (0.f, 1.f, v (6)); const C sE (0.f, hz / 1200.f); H *= C (1.f) + C (4.5f * dN) * (sE / (C (1.f) + sE));      // DRIVE : accent des aigus avant le filtre
          if (type == 1) H *= 1.f + res * (1.2f + 2.3f * dN);
          if (type == 3) H *= 0.75f + zephyr::sy::proResLaw (res) * (0.4f + 1.6f * dN); }                                                                                   // Ladder : le drive compense la perte de graves due au PEAK
        if (hpf > 0.005f) { const C sh (0.f, hz / fh); H *= (sh * sh) / (sh * sh + sh / qH + C (1.f)); }
        const float db = 20.f * std::log10 (std::abs (H) + 1e-6f);
        const float px = b.getX() + (float) i, py = Y (db);
        if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
    }
    juce::Path fill (p); fill.lineTo (b.getRight(), b.getBottom()); fill.lineTo (b.getX(), b.getBottom()); fill.closeSubPath();
    g.setGradientFill (juce::ColourGradient (ink.withAlpha (0.38f), 0.f, b.getY(), ink.withAlpha (0.03f), 0.f, b.getBottom(), false)); g.fillPath (fill);
    g.setColour (ink.withAlpha (0.25f)); g.strokePath (p, juce::PathStrokeType (4.f, juce::PathStrokeType::curved));
    g.setColour (ink); g.strokePath (p, juce::PathStrokeType (1.7f, juce::PathStrokeType::curved));
    g.setColour (ink.withAlpha (0.6f)); g.drawVerticalLine ((int) X (juce::jlimit (f0, f1, fc)), b.getY(), b.getBottom());
    g.setColour (juce::Colour (look::T().print).withAlpha (0.5f)); g.setFont (juce::Font (juce::FontOptions (9.f)));
    for (float hz : { 100.f, 1000.f, 10000.f }) g.drawText (hz >= 1000.f ? juce::String ((int) (hz / 1000.f)) + "k" : juce::String ((int) hz), (int) X (hz) + 2, (int) b.getBottom() - 11, 26, 10, juce::Justification::centredLeft, false);
    g.restoreState();
}

void EnvView::paint (juce::Graphics& g)
{
    const auto b = drawBezel (g, getLocalBounds().toFloat()).reduced (4.f, 5.f);
    const juce::Colour ink = juce::Colour (look::T().scope);
    const float a = juce::jmax (0.03f, v (0)), d = juce::jmax (0.03f, v (1)), s = v (2), r = juce::jmax (0.03f, v (3)), hold = 0.35f;
    const float tot = a + d + hold + r, wa = b.getWidth() * a / tot, wd = b.getWidth() * d / tot, wh = b.getWidth() * hold / tot, wr = b.getWidth() * r / tot;
    const float yTop = b.getY() + 1.f, yBot = b.getBottom() - 1.f, ys = yBot - (yBot - yTop) * s;
    juce::Path p; p.startNewSubPath (b.getX(), yBot);
    const int N = 24;
    for (int i = 1; i <= N; ++i) { const float t = (float) i / N; p.lineTo (b.getX() + wa * t, yBot - (yBot - yTop) * (1.f - std::exp (-4.f * t)) / (1.f - std::exp (-4.f))); }
    const float x1 = b.getX() + wa;
    for (int i = 1; i <= N; ++i) { const float t = (float) i / N; p.lineTo (x1 + wd * t, ys - (ys - yTop) * (std::exp (-4.f * t) - std::exp (-4.f)) / (1.f - std::exp (-4.f))); }
    const float x2 = x1 + wd; p.lineTo (x2 + wh, ys);
    for (int i = 1; i <= N; ++i) { const float t = (float) i / N; p.lineTo (x2 + wh + wr * t, yBot - (yBot - ys) * (std::exp (-4.f * t) - std::exp (-4.f)) / (1.f - std::exp (-4.f))); }
    g.setColour (ink.withAlpha (0.14f));
    for (int i = 1; i < 4; ++i) g.drawHorizontalLine ((int) (b.getY() + b.getHeight() * (float) i / 4.f), b.getX(), b.getRight());
    juce::Path fill (p); fill.lineTo (x2 + wh + wr, yBot); fill.closeSubPath();
    g.setGradientFill (juce::ColourGradient (ink.withAlpha (0.35f), 0.f, b.getY(), ink.withAlpha (0.03f), 0.f, b.getBottom(), false)); g.fillPath (fill);
    g.setColour (ink.withAlpha (0.25f)); g.strokePath (p, juce::PathStrokeType (4.f));
    g.setColour (ink); g.strokePath (p, juce::PathStrokeType (1.6f));
    g.setColour (ink.withAlpha (0.35f));
    for (float x : { x1, x2, x2 + wh }) g.drawVerticalLine ((int) x, b.getY(), b.getBottom());
}

void WoodFrame::paint (juce::Graphics& g)
{
    const int H = getHeight(), W = getWidth();
    juce::Random rr (77);
    auto flank = [&] (int x0, bool left)
    {
        const juce::Rectangle<float> r ((float) x0, 0.f, (float) kFlank, (float) H);
        g.setGradientFill (juce::ColourGradient (juce::Colour (look::T().woodA), r.getX(), 0.f, juce::Colour (look::T().woodB), r.getRight(), 0.f, false));
        g.fillRect (r);
        for (int i = 0; i < 90; ++i)                                                         // veines du bois
        {
            const float x = r.getX() + rr.nextFloat() * r.getWidth(), w = 0.5f + rr.nextFloat() * 1.6f;
            juce::Path v; v.startNewSubPath (x, 0.f);
            for (int y = 0; y <= H; y += 40) v.lineTo (x + std::sin ((float) y * 0.013f + x) * 1.6f, (float) y);
            g.setColour ((rr.nextBool() ? juce::Colours::black : juce::Colour (0xffb67a44)).withAlpha (0.05f + 0.1f * rr.nextFloat()));
            g.strokePath (v, juce::PathStrokeType (w));
        }
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.14f), r.getX(), 0.f, juce::Colours::transparentWhite, r.getX() + 6.f, 0.f, false)); g.fillRect (r);
        g.setColour (juce::Colours::black.withAlpha (0.75f)); g.fillRect (left ? r.getRight() - 2.f : r.getX(), 0.f, 2.f, (float) H);           // joint avec la face
        g.setColour (juce::Colour (0xffd9b27a).withAlpha (0.18f)); g.fillRect (left ? r.getRight() - 3.f : r.getX() + 2.f, 0.f, 1.f, (float) H);
        const float cx = r.getCentreX();
        for (float y : { 22.f, (float) H - 22.f })                                           // vis laiton a tete fendue
        {
            g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillEllipse (cx - 6.f, y - 4.5f, 12.f, 12.f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff0d99a), cx - 4.f, y - 4.f, juce::Colour (0xff6b5320), cx + 4.f, y + 4.f, false));
            g.fillEllipse (cx - 5.f, y - 5.f, 10.f, 10.f);
            g.setColour (juce::Colours::black.withAlpha (0.7f)); g.drawLine (cx - 3.4f, y + 0.8f, cx + 3.4f, y - 0.8f, 1.3f);
        }
    };
    flank (0, true); flank (W - kFlank, false);
    // rail du haut : fente d'aeration en plaque metal (a droite des onglets)
    const juce::Rectangle<float> vent ((float) W - 360.f, 5.f, 330.f, 20.f);
    g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (vent.expanded (1.5f), 4.f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2724), 0.f, vent.getY(), juce::Colour (0xff12100f), 0.f, vent.getBottom(), false)); g.fillRoundedRectangle (vent, 3.f);
    for (float x = vent.getX() + 8.f; x < vent.getRight() - 6.f; x += 8.f)
    {
        g.setColour (juce::Colours::black); g.fillRoundedRectangle (x, vent.getY() + 4.f, 3.f, vent.getHeight() - 8.f, 1.5f);
        g.setColour (juce::Colours::white.withAlpha (0.07f)); g.fillRect (x + 3.f, vent.getY() + 5.f, 1.f, vent.getHeight() - 10.f);
    }
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
    // v5 : un seul choix OFF / ARP / SEQ (avant : OFF-ON-LATCH puis ARP-SEQ). SEQ joue meme "ARP" eteint, LATCH est devenu HOLD.
    auto modeSel = [this] (juce::Component& parent)
    {
        auto* ap = &proc.apvts;
        auto s = std::make_unique<FuncSelector> (juce::StringArray { "OFF", "ARP", "SEQ" },
            [ap] { return paramInt (*ap, "s_arpType") == 1 ? 2 : (paramInt (*ap, "s_arpOn") != 0 ? 1 : 0); },
            [ap] (int m)
            {
                auto put = [ap] (const char* id, float real) { if (auto* q = ap->getParameter (id)) q->setValueNotifyingHost (q->convertTo0to1 (real)); };
                if (m == 0) { put ("s_arpOn", 0.f); put ("s_arpType", 0.f); put ("s_seqRec", 0.f); }
                else if (m == 1) { if (paramInt (*ap, "s_arpOn") == 0) put ("s_arpOn", 1.f); put ("s_arpType", 0.f); put ("s_seqRec", 0.f); }
                else { if (paramInt (*ap, "s_arpOn") == 0) put ("s_arpOn", 1.f); put ("s_arpType", 1.f); }
            });
        auto* r = s.get(); parent.addAndMakeVisible (*r); owned.push_back (std::move (s)); return r;
    };
    auto dimmed = [] (juce::Component* c, bool active) { if (c->isEnabled() != active) { c->setEnabled (active); c->setAlpha (active ? 1.f : 0.38f); } };
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
            auto* on = modeSel (*u);
            auto* trip = toggle (*u, "s_arpTriplet", "TRIPLET");          // v5 : les triolets sont dans DIVISION ; ce bouton ne reste visible que s'il est allume (ancien projet)
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
            auto* dir = knob (*u, "s_arpMode", "MODE");  dir->setChoices ({ "UP", "DOWN", "UP-DN", "PLAYED", "RANDOM", "CONVRG", "CHORD" });
            auto* oct = knob (*u, "s_arpOct", "OCTAVE"); oct->setChoices ({ "1", "2", "3", "4" });
            auto* hold = toggle (*u, "s_hold", "HOLD");                   // v5 : LATCH et HOLD ne font plus qu'un
            auto* free = toggle (*u, "s_arpFree", "RUN"); auto* thru = toggle (*u, "s_arpThru", "THRU");
            hold->setTooltip ("HOLD : the notes (and the arp / sequence) keep playing after you release the keys");
            free->setTooltip ("RUN : the sequence and the modulation lanes play without holding a key");
            thru->setTooltip ("THRU : the keys you play also sound, on top of the arp / sequence");
            auto* shift = knob (*u, "s_arpShift", "SHIFT"); auto* clus = knob (*u, "s_arpCluster", "CLUSTER");     // mode POLY (rotation sur 4 VCO) : decalages
            auto* tp = tag (*u, "POLY : 4 VCO", juce::Justification::centred);
            auto* t1 = tag (*u, "OFF  /  ARP  /  SEQ"); auto* t4 = tag (*u, "PLAY");
            on->setTooltip ("ARP : plays the keys you hold.  SEQ : plays the stored steps (REC to record them), transposed by the key you play.");
            refreshers.push_back ([=]
            {
                const bool seq = paramInt (proc.apvts, "s_arpType") == 1;
                for (juce::Component* c : { (juce::Component*) dir, (juce::Component*) oct, (juce::Component*) shift, (juce::Component*) clus, (juce::Component*) tp }) dimmed (c, ! seq);
                if (trip->isVisible() != (paramInt (proc.apvts, "s_arpTriplet") != 0)) u->resized();
            });
            u->layoutFn = [=] (juce::Rectangle<int> r)
            {
                const int w = r.getWidth(), kw = juce::jmin (112, (int) (w * 0.2f)), sw2 = juce::jmin (66, (int) (w * 0.115f));
                const int dw = kw + 34;                                         // MODE (7 crans nommes) plus large qu'OCTAVE : ses libelles lateraux ont la place
                dir->setBounds (r.getX() - 4, r.getY(), dw, r.getHeight());      oct->setBounds (r.getX() + dw - 2, r.getY(), 2 * kw + 6 - dw, r.getHeight());
                const int sx = r.getX() + 2 * kw + 10;
                tp->setBounds (sx, r.getY(), 2 * sw2, 10);
                shift->setBounds (sx, r.getY() + 12, sw2, r.getHeight() - 12); clus->setBounds (sx + sw2, r.getY() + 12, sw2, r.getHeight() - 12);
                const int rx = sx + 2 * sw2 + 12, rw = r.getRight() - rx, y = r.getY();
                t1->setBounds (rx, y, rw, 10);                                   on->setBounds (rx, y + 12, rw, 34);
                t4->setBounds (rx, y + 52, rw, 10);
                const int bw = rw / 3;
                hold->setBounds (rx, y + 64, bw - 3, 30); free->setBounds (rx + bw, y + 64, bw - 3, 30); thru->setBounds (rx + 2 * bw, y + 64, rw - 2 * bw, 30);
                trip->setVisible (paramInt (proc.apvts, "s_arpTriplet") != 0);     // apres la mise en page du module (qui remontre tout)
                if (trip->isVisible()) { trip->setBounds (rx, y + 102, rw / 2 - 3, 30); tap->setBounds (rx + rw / 2 + 3, y + 102, rw - rw / 2 - 3, 30); }
                else tap->setBounds (rx, y + 102, rw, 30);
            };
        }
        {
            auto* u = unit ("CLOCK  &  FEEL");
            // v5 : DIVISION "Free" = horloge libre, reglee en pas par seconde par le potard HZ (grise quand l'horloge suit le tempo) ;
            // le RATCHET global a rejoint la grille des pas (bouton ALL), les triolets sont dans DIVISION
            auto* rate = knob (*u, "s_arpRate", "HZ"); auto* sync = combo (*u, "s_arpSync");
            auto* chance = knob (*u, "s_arpChance", "CHANCE"); auto* gate = knob (*u, "s_arpGate", "GATE"); auto* swing = knob (*u, "s_arpSwing", "SWING");
            auto* ts = tag (*u, "DIVISION");
            refreshers.push_back ([=] { dimmed (rate, paramInt (proc.apvts, "s_arpSync") == 0); });
            u->layoutFn = [=] (juce::Rectangle<int> r)
            {
                ts->setBounds (r.getX(), r.getY(), 80, 10); sync->setBounds (r.getX(), r.getY() + 12, 150, 28);
                const int cw = juce::jmin (100, (r.getWidth() - 170) / 4), x0 = r.getX() + 170;
                int x = x0; for (auto* k : { rate, chance, gate, swing }) { k->setBounds (x, r.getY(), cw, r.getHeight()); x += cw; }
            };
        }
        // ---- rang 2 : pas des notes
        {
            auto* u = unit ("NOTE STEPS  (click : rest / note / accent  -  click numbers : select  -  ctrl+D : duplicate  -  ctrl+Z : undo)");
            auto* len = combo (*u, "s_arpLen");
            auto* grid = new StepGrid (proc); { std::unique_ptr<juce::Component> o (grid); u->addAndMakeVisible (*grid); owned.push_back (std::move (o)); }
            stepGrid = grid;
            auto* rnd = button (*u, "RANDOM", [grid] { grid->randomize(); }); auto* clr = button (*u, "CLEAR", [grid] { grid->clear(); });
            auto* rec = toggle (*u, "s_seqRec", "REC");
            auto stepLen = [this] { return juce::jlimit (1, 16, juce::roundToInt (proc.apvts.getRawParameterValue ("s_arpLen")->load()) + 1); };
            auto* rest = button (*u, "REST", [this, stepLen, grid] { grid->pushUndo(); proc.recGroupClose = true; const int p = juce::jlimit (0, 15, proc.arpPat.recPos.load()); proc.arpPat.step[p] = 0; proc.arpPat.recPos = (p + 1) % stepLen(); grid->repaint(); });
            auto* tie = button (*u, "TIE", [this, stepLen, grid] { grid->pushUndo(); proc.recGroupClose = true; const int p = (proc.arpPat.recPos.load() + stepLen() - 1) % stepLen(); proc.arpPat.tie[p] = 1 - proc.arpPat.tie[p].load(); grid->repaint(); });
            auto* back = button (*u, "BACK", [this, stepLen, grid] { proc.recGroupClose = true; proc.arpPat.recPos = (proc.arpPat.recPos.load() + stepLen() - 1) % stepLen(); grid->repaint(); });
            auto* chd = toggle (*u, "s_seqChord", "CHORD");                // v5 : CHORD = les notes jouees ensemble vont dans le meme pas ; 1 NOTE = une note par pas
            chd->setTooltip ("CHORD : notes played together (within 60 ms) are stored in the same step, up to 6 notes. 1 NOTE : one note per step.");
            auto* all = button (*u, "ALL RATCHET", [this, grid]
            { grid->pushUndo();   // v5 : remplace le potard RATCHET global : tous les pas passent au ratchet suivant (x1 > x2 > x3 > x4 > x1)
                int mx = 1; for (int k = 0; k < 16; ++k) mx = juce::jmax (mx, proc.arpPat.ratchet[k].load());
                const int nx = mx % 4 + 1;
                for (int k = 0; k < 16; ++k) proc.arpPat.ratchet[k] = nx;
                grid->repaint();
            });
            all->setTooltip ("Every step to the next ratchet value (x1, x2, x3, x4)");
            auto* t1 = tag (*u, "PATTERN LENGTH"); auto* t3 = tag (*u, "STEP RECORD  (SEQ)");
            refreshers.push_back ([=]
            {
                const bool seq = paramInt (proc.apvts, "s_arpType") == 1;
                for (juce::Component* c : { (juce::Component*) rec, (juce::Component*) rest, (juce::Component*) tie, (juce::Component*) back, (juce::Component*) chd }) dimmed (c, seq);
                const juce::String ct = paramInt (proc.apvts, "s_seqChord") != 0 ? "CHORD" : "1 NOTE";
                if (chd->getButtonText() != ct) chd->setButtonText (ct);
            });
            auto* rowR = button (*u, "RATCHET", nullptr); auto* rowS = button (*u, "STEP SPEED", nullptr);       // rangee du bas : ratchet ou vitesse du pas
            for (auto* b : { rowR, rowS }) { b->setClickingTogglesState (true); b->setRadioGroupId (4712); }
            rowR->setToggleState (true, juce::dontSendNotification);
            rowR->onClick = [grid] { grid->setSpeedRow (false); };
            rowS->onClick = [grid] { grid->setSpeedRow (true); };
            auto* t2 = tag (*u, "REC + play : notes played together = one chord step. Wheel = transpose.", juce::Justification::topLeft, 9.f);
            u->layoutFn = [=] (juce::Rectangle<int> r)
            {
                t1->setBounds (r.getX(), r.getY(), 120, 10); len->setBounds (r.getX(), r.getY() + 12, 150, 28);
                rnd->setBounds (r.getX(), r.getY() + 46, 72, 22); clr->setBounds (r.getX() + 78, r.getY() + 46, 72, 22);
                t3->setBounds (r.getX() + 164, r.getY(), 150, 10);
                rec->setBounds (r.getX() + 164, r.getY() + 12, 74, 28); rest->setBounds (r.getX() + 244, r.getY() + 14, 70, 24);
                tie->setBounds (r.getX() + 164, r.getY() + 46, 74, 22); back->setBounds (r.getX() + 244, r.getY() + 46, 70, 22);
                chd->setBounds (r.getX() + 164, r.getY() + 74, 150, 26);
                t2->setBounds (r.getX(), r.getY() + 74, 156, 34);
                grid->setBounds (r.getX() + 330, r.getY(), r.getWidth() - 330, r.getHeight());
                rowR->setBounds (r.getRight() - 186, r.getY() - 25, 80, 18); rowS->setBounds (r.getRight() - 102, r.getY() - 25, 96, 18);
                all->setBounds (r.getRight() - 290, r.getY() - 25, 98, 18);
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
                l.ed->grid = stepGrid;
                auto* ed = l.ed;
                l.rnd = button (*u, "RND", [ed] { if (ed->grid != nullptr) ed->grid->pushUndo(); ed->randomize(); }); l.clr = button (*u, "CLR", [ed] { if (ed->grid != nullptr) ed->grid->pushUndo(); ed->clear(); });
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
        startTimerHz (10);                                                // v5 : la page ARP rafraichit aussi ses reglages grises
        return;
    }

    // ================================================================ page SYNTH : flux du signal  (1) SOURCES  ->  (2) FILTRES  ->  (3) ENVELOPPES, puis bandeau FM / fold / macros / jeu / arp
    // ---- 1. sources (indices 0..2)
    {
        auto* u = unit ("VCO 1");
        auto* w = selector (*u, "s_wave1", waves); auto* o = selector (*u, "s_oct1", octs);
        auto* pw = knob (*u, "s_pw1", "PW");
        auto* vb = knob (*u, "s_vib", "LFO AMT"); auto* vw = knob (*u, "s_vibWheel", "+ WHEEL"); auto* va = knob (*u, "s_vibAT", "+ AFTER");
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            w->setBounds (r.getX(), r.getY() + 12, 148, 40); o->setBounds (r.getX(), r.getY() + 58, 148, 30);
            place (r, { pw, vb, vw, va }, 156, 64);
        };
    }
    {
        auto* u = unit ("VCO 2");
        auto* w = selector (*u, "s_wave2", waves); auto* o = selector (*u, "s_oct2", octs); auto* sy = toggle (*u, "s_sync", "HARD SYNC");
        auto* tu = knob (*u, "s_tune2", "PITCH", "-12", "+12"); auto* fi = knob (*u, "s_fine2", "FINE", "-", "+"); auto* pw = knob (*u, "s_pw2", "PW");
        auto* rg = knob (*u, "s_ring", "RING");
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            w->setBounds (r.getX(), r.getY() + 2, 148, 38); o->setBounds (r.getX(), r.getY() + 44, 148, 28); sy->setBounds (r.getX(), r.getY() + 76, 148, 28);
            place (r, { tu, fi, pw, rg }, 156, 64);
        };
    }
    {
        auto* u = unit ("MIXER  &  DRIFT");
        auto* a = knob (*u, "s_lvl1", "VCO 1"); auto* b = knob (*u, "s_lvl2", "VCO 2"); auto* sb = knob (*u, "s_sub", "SUB"); auto* c = knob (*u, "s_noise", "NOISE");
        auto* dr = knob (*u, "s_drift", "DRIFT"); auto* d = knob (*u, "s_master", "VOLUME");
        u->layoutFn = [=] (juce::Rectangle<int> r) { place (r, { a, b, sb, c, dr, d }, 0, juce::jmin (96, r.getWidth() / 6)); };
    }

    // ---- 2. filtres : l'element central (index 3)
    {
        auto* u = unit ("VC FILTER");
        auto* a = knob (*u, "s_lpf", "CUTOFF"); auto* b = knob (*u, "s_lpfRes", "RESONANCE"); auto* c = knob (*u, "s_fltEnv", "EG AMOUNT"); auto* e = knob (*u, "s_drive", "SATURATION");
        auto* d = knob (*u, "s_key", "KEY TRACK"); auto* lf = knob (*u, "s_fltLfo", "LFO 2"); auto* mo = knob (*u, "s_semMorph", "SEM MORPH");
        auto* hc = knob (*u, "s_hpf", "HP CUTOFF"); auto* hr = knob (*u, "s_hpfRes", "HP PEAK");
        auto* ft = selector (*u, "s_ftype", juce::StringArray { "MS-10", "LADDER CEM3320", "SEM", "PROPHET" });
        auto fv = std::make_unique<FilterView> (proc.apvts); auto* fvp = fv.get(); u->addAndMakeVisible (*fv); owned.push_back (std::move (fv));
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            ft->setBounds (r.getX(), r.getY(), r.getWidth(), 32);
            fvp->setBounds (r.getX(), r.getY() + 38, r.getWidth(), 150);
            auto rest = r.withTrimmedTop (38 + 150 + 6);
            const int hh = juce::jmin (rest.getHeight() * 55 / 100, 150);
            const int cw = r.getWidth() / 4;
            place (rest.withHeight (hh), { a, b, c, e }, 0, cw);
            place (rest.withTrimmedTop (hh), { d, lf, mo, hc, hr }, 0, r.getWidth() / 5);
        };
    }

    // ---- 3. enveloppes et modulation (indices 4..6)
    {
        auto* u = unit ("ENVELOPE  -  FILTER");
        envFltUnit = u;
        auto* a = knob (*u, "s_fA", "ATTACK"); auto* d = knob (*u, "s_fD", "DECAY"); auto* s = knob (*u, "s_fS", "SUSTAIN"); auto* r2 = knob (*u, "s_fR", "RELEASE");
        auto ev = std::make_unique<EnvView> (proc.apvts, 'f'); auto* evp = ev.get(); u->addAndMakeVisible (*ev); owned.push_back (std::move (ev));
        u->layoutFn = [=] (juce::Rectangle<int> r) { evp->setBounds (r.getX(), r.getY() + 2, 124, r.getHeight() - 4); place (r, { a, d, s, r2 }, 130, 60); };
    }
    {
        // LINK : l'enveloppe FILTRE pilote aussi le volume ; ce panneau devient ENV 2, libre (matrice : source ENV AMP, ou raccourci DEST / AMOUNT)
        auto* u = unit ("ENVELOPE  -  AMP");
        auto* a = knob (*u, "s_aA", "ATTACK"); auto* d = knob (*u, "s_aD", "DECAY"); auto* s = knob (*u, "s_aS", "SUSTAIN"); auto* r2 = knob (*u, "s_aR", "RELEASE");
        auto ev = std::make_unique<EnvView> (proc.apvts, 'a'); auto* evp = ev.get(); u->addAndMakeVisible (*ev); owned.push_back (std::move (ev));
        auto* lk = toggle (*u, "s_envLink", "LINK");
        lk->setTooltip ("LINK: the filter envelope also drives the amp - this envelope becomes ENV 2, free for modulation");
        auto* dst = combo (*u, "s_env2Dst");
        auto* amt = knob (*u, "s_env2Amt", "AMOUNT", "-", "+");
        envLinkUnit = u; envLinkView = evp; envLinkDst = dst; envLinkAmt = amt;
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            const bool on = proc.apvts.getRawParameterValue ("s_envLink")->load() > 0.5f;
            lk->setBounds (r.getX(), r.getY() + 2, 124, 22);
            evp->setVisible (! on); dst->setVisible (on); amt->setVisible (on);
            const auto side = juce::Rectangle<int> (r.getX(), r.getY() + 28, 124, r.getHeight() - 30);
            evp->setBounds (side);
            dst->setBounds (side.getX(), side.getY(), side.getWidth(), 22);
            amt->setBounds (side.withTrimmedTop (24));
            place (r, { a, d, s, r2 }, 130, 60);
        };
    }
    {
        auto* u = unit ("LFO  /  S&H");
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
            auto view = std::make_unique<LfoView> (proc.apvts, pre + "Wave", pre + "Slew", &proc.synth, i - 1);
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
            const int lw = 138, rx = r.getX() + lw + 8, rw = r.getWidth() - lw - 8;
            pk->setBounds (r.getX(), r.getY(), lw, 26);
            for (auto& st : *sets)
            {
                st.wave->setBounds (r.getX(), r.getY() + 32, lw, 40);
                st.sync->setBounds (r.getX(), r.getY() + 78, lw, 26);
                st.trig->setBounds (r.getX(), r.getY() + 108, lw, 30);
                st.view->setBounds (rx, r.getY(), rw, 42);
                const int cw = rw / 3, y = r.getY() + 46, h = r.getHeight() - 46;
                st.rate->setBounds (rx, y, cw, h); st.slew->setBounds (rx + cw, y, cw, h); st.fade->setBounds (rx + 2 * cw, y, cw, h);
            }
        };
    }

    // ---- bandeau du bas (indices 7..11) : wavefolder, macros, jeu
    {
        auto* u = unit ("WAVEFOLDER");
        auto* a = knob (*u, "s_fold", "FOLD"); auto* b = knob (*u, "s_foldSym", "SYMMETRY", "-", "+");
        u->layoutFn = [=] (juce::Rectangle<int> r) { const int cw = juce::jmin (96, r.getWidth() / 2 - 2); place (r, { a, b }, (r.getWidth() - 2 * cw) / 2, cw); };
    }
    {
        auto* u = unit ("MACROS");
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
        auto* det = knob (*u, "s_uniDetune", "DETUNE"); auto* gl = knob (*u, "s_glide", "PORTAMENTO");
        auto* bd = combo (*u, "s_bendRange");
        auto tg = std::make_unique<Tag> ("BEND RANGE", juce::Justification::centred); auto* tgp = tg.get(); u->addAndMakeVisible (*tg); owned.push_back (std::move (tg));
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            as->setBounds (r.getX(), r.getY(), r.getWidth(), 38);
            hold->setBounds (r.getX(), r.getY() + 44, r.getWidth() / 2 - 4, 30); learn->setBounds (r.getX() + r.getWidth() / 2, r.getY() + 44, r.getWidth() / 2, 30);
            const auto rest = r.withTrimmedTop (82);
            place (rest, { det, gl }, 0, 72);
            const int bx = r.getX() + 148, bw = r.getRight() - bx;
            tgp->setBounds (bx, rest.getCentreY() - 22, bw, 12); bd->setBounds (bx, rest.getCentreY() - 6, bw, 26);
        };
    }
    {
        auto* u = unit ("FM  (cross modulation)");
        // ---- page PRINCIPALE : caractere, quantite, rapport, enveloppe ; page AVANCEE : sens, modulateur, type, retour, profondeur selon la note...
        auto* depth = knob (*u, "s_fm", "AMOUNT");
        auto* ratio = knob (*u, "s_fmRatio", "RATIO");
        ratio->setChoices ({ "FREE", "1:2", "2:3", "1:1", "3:2", "2:1", "3:1", "4:1", "5:1", "7:1", "7:2", "x1.41", "5:2", "9:2", "6:1", "8:1", "x2.76", "x5.4" });
        auto* envAmt = knob (*u, "s_fmEnvAmt", "ENV AMT"); auto* dec = knob (*u, "s_fmDec", "DECAY");
        auto* vel = knob (*u, "s_fmVel", "VELOCITY"); auto* at = knob (*u, "s_fmAT", "AFTERTOUCH");
        auto* att = knob (*u, "s_fmAtt", "ATTACK"); auto* fine = knob (*u, "s_fmFine", "FINE", "-", "+");
        auto* fb = knob (*u, "s_fmFb", "FEEDBACK"); auto* key = knob (*u, "s_fmKey", "KEY SCALE");
        auto* dir = selector (*u, "s_fmDir", juce::StringArray { "2>1", "1>2", "BOTH" });
        auto* wave = selector (*u, "s_fmWave", juce::StringArray { "OSC", "SIN", "TRI", "SQR" });
        auto* type = selector (*u, "s_fmType", juce::StringArray { "EXP", "LINEAR" });
        auto tag = [this, u] (const juce::String& t) { auto c = std::make_unique<Tag> (t); auto* r = c.get(); u->addAndMakeVisible (*r); owned.push_back (std::move (c)); return r; };
        auto* tPal = tag ("CHARACTER"); auto* tDir = tag ("DIRECTION"); auto* tWav = tag ("MODULATOR"); auto* tTyp = tag ("FM TYPE");
        // caracteres : un clic regle tout (ondes, ratio, profondeur, enveloppes d'amplitude) ; tout reste ensuite ajustable
        struct Ch { const char* name; float fm; int ratio, mwave, dir; float envAmt, att, dec, fine, vel, at; int type; float fb, key; int w1, w2; float lvl2, aA, aD, aS, aR; };
        static const Ch kCh[] {
            { "BELL",     0.30f, 10, 1, 0, 0.60f, 0.00f, 0.70f, 0.f, 0.30f, 0.00f, 1, 0.00f, 0.5f, 3, 3, 0.f, 0.000f, 0.75f, 0.00f, 0.60f },
            { "E-PIANO",  0.22f,  3, 1, 0, 0.50f, 0.00f, 0.40f, 0.f, 0.60f, 0.00f, 1, 0.00f, 0.6f, 3, 3, 0.f, 0.000f, 0.60f, 0.15f, 0.35f },
            { "TINE",     0.20f,  3, 1, 0, 0.75f, 0.00f, 0.22f, 0.f, 0.80f, 0.00f, 1, 0.10f, 0.5f, 3, 3, 0.f, 0.000f, 0.50f, 0.10f, 0.30f },
            { "MARIMBA",  0.22f,  7, 1, 0, 0.65f, 0.00f, 0.18f, 0.f, 0.50f, 0.00f, 1, 0.00f, 0.4f, 3, 3, 0.f, 0.000f, 0.35f, 0.00f, 0.25f },
            { "GLASS",    0.16f,  8, 1, 0, 0.45f, 0.00f, 0.60f, 0.f, 0.40f, 0.20f, 1, 0.00f, 0.5f, 3, 3, 0.f, 0.000f, 0.80f, 0.00f, 0.60f },
            { "BRASS",    0.25f,  3, 1, 0, 0.50f, 0.35f, 0.50f, 0.f, 0.40f, 0.40f, 1, 0.00f, 0.3f, 3, 3, 0.f, 0.080f, 0.40f, 0.80f, 0.20f },
            { "ORGAN",    0.28f,  5, 1, 0, 0.00f, 0.00f, 0.50f, 0.f, 0.00f, 0.00f, 1, 0.15f, 0.0f, 3, 3, 0.f, 0.010f, 0.30f, 1.00f, 0.12f },
            { "GONG",     0.35f, 16, 1, 0, 0.50f, 0.00f, 0.80f, 0.f, 0.20f, 0.00f, 1, 0.10f, 0.3f, 3, 3, 0.f, 0.000f, 0.90f, 0.00f, 0.75f },
            { "WOOD",     0.30f, 12, 1, 0, 0.85f, 0.00f, 0.12f, 0.f, 0.60f, 0.00f, 1, 0.00f, 0.3f, 3, 3, 0.f, 0.000f, 0.20f, 0.00f, 0.14f },
            { "STRING",   0.22f,  3, 1, 0, 0.60f, 0.00f, 0.30f, 0.f, 0.50f, 0.00f, 1, 0.30f, 0.4f, 3, 3, 0.f, 0.000f, 0.55f, 0.10f, 0.30f },
            { "FM BASS",  0.30f,  1, 1, 0, 0.50f, 0.00f, 0.22f, 0.f, 0.60f, 0.00f, 1, 0.25f, 0.2f, 3, 3, 0.f, 0.000f, 0.50f, 0.60f, 0.15f },
            { "CLAV",     0.25f,  6, 1, 0, 0.70f, 0.00f, 0.10f, 0.f, 0.70f, 0.00f, 1, 0.10f, 0.3f, 3, 3, 0.f, 0.000f, 0.22f, 0.00f, 0.12f },
            { "CHOIR",    0.12f,  4, 1, 0, 0.20f, 0.50f, 0.70f, 0.f, 0.00f, 0.50f, 1, 0.00f, 0.2f, 3, 3, 0.f, 0.550f, 0.50f, 0.90f, 0.70f },
            { "SPACE",    0.20f, 17, 1, 0, 0.30f, 0.60f, 0.90f, 0.f, 0.00f, 0.40f, 1, 0.10f, 0.2f, 3, 3, 0.f, 0.650f, 0.60f, 0.90f, 0.80f },
            { "LASER",    0.55f,  9, 1, 0, 1.00f, 0.00f, 0.12f, 0.f, 0.30f, 0.00f, 0, 0.00f, 0.0f, 3, 3, 0.f, 0.000f, 0.30f, 0.00f, 0.20f },
            { "METAL",    0.45f, 11, 2, 2, 0.30f, 0.00f, 0.60f, 0.15f, 0.00f, 0.00f, 0, 0.00f, 0.0f, 2, 2, 0.3f, 0.000f, 0.60f, 0.00f, 0.40f },
            { "GROWL",    0.55f,  1, 3, 0, 0.00f, 0.00f, 0.40f, 0.20f, 0.00f, 0.30f, 0, 0.00f, 0.0f, 0, 0, 0.2f, 0.010f, 0.40f, 0.90f, 0.15f } };
        constexpr int kNCh = (int) (sizeof (kCh) / sizeof (kCh[0]));
        auto pal = std::make_unique<juce::ComboBox>();
        pal->addItem ("choose...", 1); for (int i = 0; i < kNCh; ++i) pal->addItem (kCh[i].name, i + 2);
        pal->setSelectedId (1, juce::dontSendNotification); pal->setScrollWheelEnabled (false);
        u->addAndMakeVisible (*pal);
        auto* palP = pal.get(); owned.push_back (std::move (pal));
        palP->onChange = [this, palP]
        {
            const int id = palP->getSelectedId();
            if (id < 2) return;
            const auto& c = kCh[id - 2];
            auto S = [this] (const char* n, float v) { proc.setParamReal (n, v); };
            S ("s_oct2", 2.f); S ("s_tune2", 0.f); S ("s_fine2", 0.f);                     // le rapport du caractere s'entend tel quel
            S ("s_fm", c.fm); S ("s_fmRatio", (float) c.ratio); S ("s_fmWave", (float) c.mwave); S ("s_fmDir", (float) c.dir);
            S ("s_fmEnvAmt", c.envAmt); S ("s_fmAtt", c.att); S ("s_fmDec", c.dec); S ("s_fmFine", c.fine); S ("s_fmVel", c.vel); S ("s_fmAT", c.at);
            S ("s_fmType", (float) c.type); S ("s_fmFb", c.fb); S ("s_fmKey", c.key);
            S ("s_wave1", (float) c.w1); S ("s_wave2", (float) c.w2); S ("s_lvl1", 0.8f); S ("s_lvl2", c.lvl2);          // le modulateur reste muet : on n'entend que la porteuse
            S ("s_aA", c.aA); S ("s_aD", c.aD); S ("s_aS", c.aS); S ("s_aR", c.aR); S ("s_lpf", 0.9f); S ("s_fltEnv", 0.f);
        };
        // de : tire un son FM musical (rapports harmoniques ou cloche, decay coherent, modulation de phase)
        auto dice = std::make_unique<juce::TextButton> ("DICE");
        u->addAndMakeVisible (*dice); auto* diceP = dice.get(); owned.push_back (std::move (dice));
        diceP->setTooltip ("Random musical FM sound");
        diceP->onClick = [this]
        {
            static juce::Random rnd;
            static const int harmonic[] { 3, 3, 5, 5, 6, 7, 4, 1, 10, 12, 8, 16 };           // surtout 1:1, 2:1, 3:1, 4:1, 3:2, 1:2, cloches
            const int ratioIdx = harmonic[rnd.nextInt ((int) (sizeof (harmonic) / sizeof (harmonic[0])))];
            const float dec = 0.1f + rnd.nextFloat() * 0.7f, sustained = rnd.nextFloat() < 0.25f ? 1.f : 0.f;
            auto S = [this] (const char* n, float v) { proc.setParamReal (n, v); };
            S ("s_oct2", 2.f); S ("s_tune2", 0.f); S ("s_fine2", 0.f);
            S ("s_fmType", 1.f); S ("s_fmDir", 0.f); S ("s_fmWave", 1.f); S ("s_fmRatio", (float) ratioIdx);
            S ("s_fm", 0.12f + rnd.nextFloat() * 0.3f); S ("s_fmEnvAmt", sustained > 0.f ? rnd.nextFloat() * 0.3f : 0.3f + rnd.nextFloat() * 0.6f);
            S ("s_fmDec", dec); S ("s_fmAtt", sustained > 0.f ? rnd.nextFloat() * 0.5f : 0.f); S ("s_fmFb", rnd.nextFloat() < 0.4f ? rnd.nextFloat() * 0.35f : 0.f);
            S ("s_fmKey", 0.3f + rnd.nextFloat() * 0.4f); S ("s_fmVel", 0.2f + rnd.nextFloat() * 0.5f); S ("s_fmFine", 0.f); S ("s_fmAT", rnd.nextFloat() < 0.5f ? 0.3f : 0.f);
            S ("s_wave1", 3.f); S ("s_wave2", 3.f); S ("s_lvl1", 0.8f); S ("s_lvl2", 0.f); S ("s_lpf", 0.9f); S ("s_fltEnv", 0.f);
            S ("s_aA", sustained > 0.f ? rnd.nextFloat() * 0.5f : 0.f); S ("s_aD", juce::jmin (0.95f, dec + 0.1f)); S ("s_aS", sustained > 0.f ? 0.8f : rnd.nextFloat() * 0.2f); S ("s_aR", 0.2f + rnd.nextFloat() * 0.5f);
        };
        auto more = std::make_unique<juce::TextButton> ("MORE");
        more->setClickingTogglesState (true); u->addAndMakeVisible (*more); auto* moreP = more.get(); owned.push_back (std::move (more));
        moreP->setTooltip ("Advanced FM controls");
        moreP->onClick = [u, moreP] { moreP->setButtonText (moreP->getToggleState() ? "BACK" : "MORE"); u->resized(); };
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            const bool adv = moreP->getToggleState();
            const int dw = juce::jmin (110, r.getWidth() / 3), x = r.getX() + dw + 6, w = r.getWidth() - dw - 6;
            for (auto* c : std::vector<juce::Component*> { tPal, palP, diceP, ratio, envAmt, dec, att, vel, at })                           // page principale
                c->setVisible (! adv);
            for (auto* c : std::vector<juce::Component*> { tDir, dir, tWav, wave, tTyp, type, fine, fb, key })                              // page avancee
                c->setVisible (adv);
            depth->setVisible (true); moreP->setVisible (true);
            depth->setBounds (r.getX(), r.getY() + 20, dw, r.getHeight() - 20);
            const int ky = r.getY() + 46, kh = (r.getHeight() - 46) / 2, cw = w / 3;
            if (! adv)
            {
                tPal->setBounds (x, r.getY(), 90, 10);
                palP->setBounds (x, r.getY() + 12, w / 2, 28); diceP->setBounds (x + w / 2 + 4, r.getY() + 12, w / 4 - 4, 28); moreP->setBounds (x + w * 3 / 4 + 4, r.getY() + 12, w / 4 - 4, 28);
                int i = 0; for (auto* k : { ratio, envAmt, dec, att, vel, at }) { k->setBounds (x + (i % 3) * cw, ky + (i / 3) * kh, cw, kh); ++i; }
            }
            else
            {
                tDir->setBounds (x, r.getY(), 90, 10); dir->setBounds (x, r.getY() + 12, w / 3 - 4, 30);
                tWav->setBounds (x + w / 3, r.getY(), 90, 10); wave->setBounds (x + w / 3, r.getY() + 12, w / 3 - 4, 30);
                tTyp->setBounds (x + 2 * w / 3, r.getY(), 90, 10); type->setBounds (x + 2 * w / 3, r.getY() + 12, w / 3, 30);
                moreP->setBounds (r.getX() + 2, r.getY() - 2, dw - 4, 20);
                int i = 0; for (auto* k : { fb, key, fine }) { k->setBounds (x + i * cw, ky, cw, kh); ++i; }
            }
        };
    }
    {
        auto* u = unit ("ARP");
        auto* on = modeSel (*u);
        auto* rt = knob (*u, "s_arpRate", "RATE"); auto* gt = knob (*u, "s_arpGate", "GATE");
        auto tg = std::make_unique<Tag> ("full control : ARP tab", juce::Justification::centred, 8.5f); auto* tgp = tg.get(); u->addAndMakeVisible (*tg); owned.push_back (std::move (tg));
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            on->setBounds (r.getX(), r.getY(), r.getWidth(), 38);
            place (r.withTrimmedTop (46).withTrimmedBottom (14), { rt, gt }, 0, r.getWidth() / 2);
            tgp->setBounds (r.getX(), r.getBottom() - 12, r.getWidth(), 12);
        };
    }
    {   // index 12 : poly-mod facon Prophet-5 (enveloppe de filtre et VCO 2 -> frequence / largeur du VCO 1, filtre)
        auto* u = unit ("POLY-MOD");
        auto* a = knob (*u, "s_pmEnv", "FILTER ENV"); auto* b = knob (*u, "s_pmOsc", "OSC 2");
        auto* t1 = toggle (*u, "s_pmFreq", "FREQ A"); auto* t2 = toggle (*u, "s_pmPw", "PW A"); auto* t3 = toggle (*u, "s_pmFilt", "VCF");
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            const int th = 30, tw = (r.getWidth() - 8) / 3;
            int x = r.getX(); for (auto* t : { t1, t2, t3 }) { t->setBounds (x, r.getY(), tw, th); x += tw + 4; }
            place (r.withTrimmedTop (th + 8), { a, b }, 0, r.getWidth() / 2);
        };
    }
    startTimerHz (10);
}

void SynthPanel::timerCallback()
{
    for (auto* v : lfoViews) v->repaint();
    for (auto& f : refreshers) f();
    if (envLinkUnit != nullptr)                                                         // LINK : titre et commandes du panneau AMP / ENV 2
    {
        const bool on = proc.apvts.getRawParameterValue ("s_envLink")->load() > 0.5f;
        const juce::String t = on ? "ENV 2  -  FREE  (AMP = FILTER ENV)" : "ENVELOPE  -  AMP";
        if (envLinkUnit->title != t || envLinkDst->isVisible() != on) { envLinkUnit->title = t; envLinkUnit->resized(); envLinkUnit->repaint(); }
        if (envFltUnit != nullptr) { const juce::String tf = on ? "ENVELOPE  -  FILTER  +  AMP" : "ENVELOPE  -  FILTER"; if (envFltUnit->title != tf) { envFltUnit->title = tf; envFltUnit->repaint(); } }
    }
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
                if (sp != nullptr && dp != nullptr && (int) sp->load() == macroSrc ((int) k) && (int) dp->load() > 0) {
                    name = juce::String (kModDstNames[(int) dp->load()]);                // etiquette courte : "ENSEMBLE MIX" -> "ENSEMBLE"
                    if (name.length() > 9 && (name.endsWith (" MIX") || name.endsWith (" AMT"))) name = name.dropLastCharacters (4);
                    if (name.length() > 9) name = name.replace ("DECAY", "DCY").replace ("CUTOFF", "CUT").replace ("SHIMMER", "SHIMR");
                    break; }
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
    {   // colonnes : sources | filtres | enveloppes ; puis le bandeau du bas
        const int cx = 10, srcW = 420, envW = 392, fltW = W - 20 - srcW - envW - 2 * gap;
        for (int k = 0; k < 3; ++k) { units[(size_t) k]->fullHeight = 160; units[(size_t) k]->setBounds (cx, k * (160 + gap), srcW, 160); }
        units[3]->fullHeight = 496; units[3]->setBounds (cx + srcW + gap, 0, fltW, 496);
        const int eh[3] { 146, 146, 188 }; int y = 0;
        for (int k = 0; k < 3; ++k) { units[(size_t) (4 + k)]->fullHeight = eh[k]; units[(size_t) (4 + k)]->setBounds (W - 10 - envW, y, envW, eh[k]); y += eh[k] + gap; }
        row (504, 224, { { 10, 430.f }, { 12, 210.f }, { 7, 168.f }, { 8, 298.f }, { 9, 250.f }, { 11, 190.f } });
    }
    paintFacade (tex, W, getHeight(), false);
}

MatrixDrawer::MatrixDrawer (juce::AudioProcessorValueTreeState& apvts)
{
    for (int i = 1; i <= kSlots; ++i)
    {
        const juce::String pre ("s_slot" + juce::String (i));
        auto r = std::make_unique<Row>();
        for (auto* cb : { &r->src, &r->dst, &r->dir }) cb->setScrollWheelEnabled (false);
        r->dir.addItemList ({ "FORWARD", "BACKWARD", "BOTH" }, 1);                       // sens : l'anneau du potard suit ce choix
        fillGrouped (r->src, srcFamilies(), kModSrcNames);
        fillGrouped (r->dst, dstFamilies(), kModDstNames);
        r->amt.detent = 0.0; r->amt.showMark = true;
        r->amt.setPopupDisplayEnabled (true, true, nullptr);
        r->amt.setScrollWheelEnabled (false);
        r->amt.setDoubleClickReturnValue (true, 0.0);
        addAndMakeVisible (r->src); addAndMakeVisible (r->dst); addAndMakeVisible (r->dir); addAndMakeVisible (r->amt);
        comboAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, pre + "Dir", r->dir));
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
    g.drawText ("DIRECTION", 358, 30, 100, 8, juce::Justification::centredLeft, false);
    g.drawText ("AMOUNT", 468, 30, 100, 8, juce::Justification::centredLeft, false);
}

void MatrixDrawer::resized()
{
    const int rowH = 32;
    for (size_t i = 0; i < rows.size(); ++i)
    {
        const int y = 42 + (int) i * rowH;
        rows[i]->src.setBounds (14, y, 150, 26);
        rows[i]->dst.setBounds (172, y, 176, 26);
        rows[i]->dir.setBounds (358, y, 100, 26);
        rows[i]->amt.setBounds (468, y, getWidth() - 468 - 16, 26);
    }
}

// ---------------------------------------------------------------- fenetre : onglets (synthe) + mise a l'echelle

int SynthEditor::designHeight() const { return ZEPHYR_SYNTH ? 30 + kFxH : kFxH; }
int SynthEditor::designWidth() const  { return kDesignW + 2 * WoodFrame::kFlank; }

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
    content.addAndMakeVisible (wood);                                   // flancs en bois aussi sur Zephyr FX
    content.addAndMakeVisible (fx);
    fx.onThemeChanged = [this] { if (synthPanel != nullptr) synthPanel->rebuild(); if (arpPanel != nullptr) arpPanel->rebuild(); repaint(); };
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
    if (const char* ong = std::getenv ("FOUDRE_ONGLET")) { const int t = juce::jlimit (0, 2, std::atoi (ong)); showTab (t); (t == 0 ? synthTab : t == 1 ? arpTab : fxTab).setToggleState (true, juce::dontSendNotification); }   // banc d'essai (foudre-banc capture --onglet) ; jamais defini dans un DAW
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
    fx.setDrawerTogglesVisible (t == 2);
    look::fxPageShown = t == 2;
    fx.repaint();
}

void SynthEditor::paint (juce::Graphics& g) { g.fillAll (juce::Colour (look::T().panelBot)); }

void SynthEditor::resized()
{
    const int dh = designHeight();
    content.setTransform ({});
    content.setBounds (0, 0, designWidth(), dh);
    content.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) designWidth()));
#if ZEPHYR_SYNTH
    synthTab.setBounds (34, 3, 170, 24);
    arpTab.setBounds (210, 3, 170, 24);
    fxTab.setBounds (386, 3, 170, 24);
    matrixBtn.setBounds (562, 3, 170, 24);
    wood.setBounds (0, 0, designWidth(), dh);
    const int fl = WoodFrame::kFlank, iw = designWidth() - 2 * fl;
    fx.setBounds (fl, 30, iw, kFxH);
    synthPanel->setBounds (fl, 30 + kHeaderH + 4, iw, kFxH - kHeaderH - 4);
    arpPanel->setBounds (fl, 30 + kHeaderH + 4, iw, kFxH - kHeaderH - 4);
    matrix->setBounds (562, 32, 640, 42 + kSlots * 32 + 10);
    matrix->toFront (false);
#else
    wood.setBounds (0, 0, designWidth(), dh);
    fx.setBounds (WoodFrame::kFlank, 0, designWidth() - 2 * WoodFrame::kFlank, kFxH);
#endif
}
