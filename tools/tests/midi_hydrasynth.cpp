#include <juce_audio_utils/juce_audio_utils.h>
#include <sstream>
#include <set>
#include "Arp.h"
#include <map>
#include <array>
#include <atomic>
#include <functional>
#include <algorithm>
#include <cmath>
#include <random>
#include <thread>
#include <mutex>

#define private public
#include "PluginProcessor.h"
#undef private
// MIDI : carte Hydrasynth par defaut, Program Change, sustain, pitch bend
static double rms (ZephyrAudioProcessor& p, juce::MidiBuffer mb, int blocks = 1)
{
    juce::AudioBuffer<float> b (2, 512); double s = 0;
    for (int k = 0; k < blocks; ++k) { b.clear(); p.processBlock (b, mb); mb.clear(); for (int i = 0; i < 512; ++i) s += b.getSample (0, i) * b.getSample (0, i); }
    return std::sqrt (s / (512.0 * blocks));
}
int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    int fail = 0; auto chk = [&] (bool ok, const char* w) { std::printf ("%s %s\n", ok ? "ok  " : "FAIL", w); if (! ok) ++fail; };
    ZephyrAudioProcessor p; p.prepareToPlay (48000.0, 512);
    // 1. aucune collision de CC par defaut
    std::map<int, juce::String> seen; bool dup = false; int n = 0;
    for (int i = 0; i < p.mapIds.size(); ++i) { const int c = p.defaultCcOf (p.mapIds[i]); if (c < 0) continue; ++n; if (seen.count (c)) { dup = true; std::printf ("  collision CC%d: %s / %s\n", c, seen[c].toRawUTF8(), p.mapIds[i].toRawUTF8()); } seen[c] = p.mapIds[i]; }
    chk (! dup, "pas de collision de CC"); std::printf ("  %d parametres sur CC par defaut\n", n);
    chk (p.defaultCcOf ("s_lvl2") == 46 && p.defaultCcOf ("s_arpOct") == 120 && p.defaultCcOf ("s_ring") == 43, "OSC2 vol=46, arp oct=120, ring=43");
    for (auto id : { "s_lpf", "s_lpfRes", "s_aA", "s_fR", "revMix", "dlyFb", "s_l4Rate", "s_uniDetune" }) chk (p.defaultCcOf (id) >= 0, id);
    // 2. CC74 -> cutoff (mode direct)
    p.takeoverMode.store (2);
    { juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::controllerEvent (1, 74, 127), 0); rms (p, mb); }
    auto* lp = p.apvts.getParameter ("s_lpf"); chk (lp->getValue() > 0.99f, "CC74=127 -> cutoff 100 %");
    { juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::controllerEvent (1, 74, 0), 0); rms (p, mb); } chk (lp->getValue() < 0.01f, "CC74=0 -> cutoff 0 %");
    // 3. Program Change
    { juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::programChange (1, 5), 0); rms (p, mb); }
    for (int k = 0; k < 50; ++k) juce::MessageManager::getInstance()->runDispatchLoopUntil (10);
    chk (p.presetName == ZephyrAudioProcessor::getFactoryPresetName (5), "Program Change 5 -> preset 5");
    // 4. sustain : note relachee pedale enfoncee continue de sonner, puis s'arrete au relachement
    p.loadFactoryPreset (0); p.setParamReal ("s_arpOn", 0.f); p.setParamReal ("s_hold", 0.f); p.setParamReal ("s_aR", 0.1f);
    { juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::controllerEvent (1, 64, 127), 0); mb.addEvent (juce::MidiMessage::noteOn (1, 60, 0.9f), 1); rms (p, mb, 20); }
    { juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::noteOff (1, 60), 0); rms (p, mb, 1); }
    const double heldA = rms (p, {}, 20);
    { juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::controllerEvent (1, 64, 0), 0); rms (p, mb, 1); }
    const double after = rms (p, {}, 400);
    std::printf ("  rms note tenue %.4f, apres relachement pedale %.6f\n", heldA, after);
    chk (heldA > 0.002 && after < heldA * 0.2, "pedale sustain");
    // 5. pitch bend : etendue
    p.setParamReal ("s_bendRange", 11.f);
    juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::pitchWheel (1, 16383), 0); rms (p, mb);
    chk (std::fabs (p.bendRange - 12.f) < 0.5f, "bend range lu");
    std::printf ("  bendRange = %.1f\n", p.bendRange);
    // 6. LFO1 -> filtre : le potard LFO change le son (variation du niveau d'un bloc a l'autre) et est sur le CC 52
    chk (p.defaultCcOf ("s_fltLfo") == 52, "s_fltLfo sur CC52");
    auto var = [&] (float amt)
    {
        p.loadFactoryPreset (0); p.setParamReal ("s_lpf", 0.35f); p.setParamReal ("s_lpfRes", 0.5f); p.setParamReal ("s_fltEnv", 0.f); p.setParamReal ("s_aS", 1.f);
        p.setParamReal ("s_l1Rate", 0.55f); p.setParamReal ("s_fltLfo", amt);
        juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::noteOn (1, 48, 0.9f), 0); rms (p, mb, 40);
        double m = 0, m2 = 0; const int N = 300; for (int k = 0; k < N; ++k) { const double r = rms (p, {}, 1); m += r; m2 += r * r; }
        m /= N; return std::sqrt (std::max (0.0, m2 / N - m * m)) / (m + 1e-9);
    };
    const double v0 = var (0.f), v1 = var (1.f); std::printf ("  variation relative : LFO 0 = %.4f, LFO 1 = %.4f\n", v0, v1);
    chk (v1 > 3 * v0 + 0.01, "le potard LFO module le cutoff");
    // 7. Zephyr : reverb sur les CC reverb de l'Hydrasynth (91/65/67), delay 92/15/14/63, Pre-FX -> ensemble 93, Post-FX -> phaser 94
    chk (p.defaultCcOf ("revMix") == 91 && p.defaultCcOf ("revDecay") == 65 && p.defaultCcOf ("revDamp") == 67 && p.defaultCcOf ("dlyMix") == 92 && p.defaultCcOf ("dlyTime") == 15
         && p.defaultCcOf ("dlyFb") == 14 && p.defaultCcOf ("dlyTone") == 63 && p.defaultCcOf ("ensMix") == 93 && p.defaultCcOf ("phMix") == 94 && p.defaultCcOf ("s_arpRatchet") == 109, "CC FX Zephyr");
    for (int k = 0; k < 8; ++k) chk (p.defaultCcOf ("s_m" + juce::String (k + 1)) == 16 + k, "macro sur CC 16..23");
    { ZephyrAudioProcessor q; const auto rk = q.engine.getRack(); chk (rk.size() == 3 && rk[0] == zephyr::NEnsemble && rk[1] == zephyr::NDelay && rk[2] == zephyr::NVerb, "nouvelle instance : rack = ensemble + delay + reverb"); }
    { p.loadFactoryPreset (0); const auto rk = p.engine.getRack(); chk (rk.size() == 3, "preset Init : ensemble + delay + reverb");
      chk (p.apvts.getRawParameterValue ("ensMix")->load() == 0.f && p.apvts.getRawParameterValue ("dlyMix")->load() == 0.f && p.apvts.getRawParameterValue ("revMix")->load() == 0.f, "mix des effets a zero (depart neutre)"); }
    // 8. arpegiateur Zephyr : MONO arpege l'accord, POLY tourne sur 4 VCO, un changement de mode ne coupe pas l'arpege, pas de note bloquee
    {
        zephyr::Arp a; a.prepare (48000.f); zephyr::ArpPattern pat; a.setPattern (&pat);
        zephyr::ArpParams P; P.on = 1; P.len = 8; P.assign = 1; a.setParams (P);
        for (int nt : { 60, 64, 67 }) a.noteOn (nt, 0.8f);
        zephyr::Arp::Ev ev[256]; std::set<int> monoNotes, slots; int bal = 0, onAfterSwitch = 0, slotBal[4] {};
        for (int k = 0; k < 300; ++k)
        {
            if (k == 150) { P.assign = 0; P.shift = 0.4f; P.cluster = 0.3f; a.setParams (P); a.resetVoices(); }
            const int c = a.process (512, 6.f, ev, 256);
            for (int i = 0; i < c; ++i)
            {
                if (ev[i].lane >= 0) continue;
                bal += ev[i].on ? 1 : -1;
                if (ev[i].slot >= 0) { slots.insert (ev[i].slot); slotBal[ev[i].slot] += ev[i].on ? 1 : -1; }
                if (ev[i].on && k < 150) monoNotes.insert (ev[i].note);
                if (ev[i].on && k >= 160) ++onAfterSwitch;
            }
        }
        for (int nt : { 60, 64, 67 }) a.noteOff (nt);
        for (int k = 0; k < 50; ++k) { const int c = a.process (512, 6.f, ev, 256); for (int i = 0; i < c; ++i) if (ev[i].lane < 0) { bal += ev[i].on ? 1 : -1; if (ev[i].slot >= 0) slotBal[ev[i].slot] += ev[i].on ? 1 : -1; } }
        std::printf ("  arp : MONO %d notes distinctes, POLY %d VCO utilises, %d notes apres le changement de mode, bilan on/off %d\n", (int) monoNotes.size(), (int) slots.size(), onAfterSwitch, bal);
        chk (monoNotes.size() == 3, "MONO arpege l'accord tenu");
        chk (slots.size() == 4, "POLY : rotation sur les 4 VCO");
        chk (onAfterSwitch >= 7, "l'arpege continue apres un changement de mode");
        chk (bal == 0 && slotBal[0] == 0 && slotBal[1] == 0 && slotBal[2] == 0 && slotBal[3] == 0, "aucune note bloquee");
    }
    // 9. projet Zephyr v1 : filtres SEM / Ladder / Prophet -> circuits, destinations sub / poly-mod / derive renumerotees
    {
        auto st = p.apvts.copyState(); st.removeProperty ("zv2", nullptr); st.removeProperty ("lfoV2", nullptr);
        for (auto c : st) if (c.hasProperty ("id"))
        {
            if (c.getProperty ("id").toString() == "s_ftype") c.setProperty ("value", 2, nullptr);          // v1 : Prophet
            if (c.getProperty ("id").toString() == "s_slot1Dst") c.setProperty ("value", 40, nullptr);      // v1 : DRIFT
        }
        juce::MemoryBlock mbk; if (auto xml = st.createXml()) p.copyXmlToBinary (*xml, mbk);
        p.setStateInformation (mbk.getData(), (int) mbk.getSize());
        chk (juce::roundToInt (p.apvts.getRawParameterValue ("s_ftype")->load()) == 3, "v1 Prophet -> Prophet circuit");
        chk (juce::roundToInt (p.apvts.getRawParameterValue ("s_slot1Dst")->load()) == zephyr::MdDrift, "v1 DRIFT -> nouvelle destination DRIFT");
    }
    auto notes = [] (int ratchet, int range, float stepHz)
    {
        zephyr::Arp a; a.prepare (48000.f); zephyr::ArpPattern pat; a.setPattern (&pat);
        zephyr::ArpParams P; P.on = 1; P.ratchet = ratchet; P.range = range; P.len = 8; a.setParams (P);
        for (int nt : { 60, 64, 67 }) a.noteOn (nt, 0.8f);
        zephyr::Arp::Ev ev[256]; int on = 0, hi = 0; std::set<int> distinct;
        for (int k = 0; k < 940; ++k) { const int c = a.process (512, stepHz, ev, 256); for (int i = 0; i < c; ++i) if (ev[i].lane < 0 && ev[i].on) { ++on; hi = std::max (hi, ev[i].note); distinct.insert (ev[i].note); } }
        return std::array<int, 3> { on, hi, (int) distinct.size() };
    };
    const auto r1 = notes (1, 0, 4.f), r3 = notes (3, 0, 4.f), rT = notes (1, 0, 6.f), r4o = notes (1, 3, 4.f);
    std::printf ("  arp 10 s : ratchet 1 = %d notes, ratchet 3 = %d, triolet(x1.5) = %d, 4 oct : %d notes distinctes (max %d)\n", r1[0], r3[0], rT[0], r4o[2], r4o[1]);
    chk (r3[0] >= 2.7 * r1[0] && r3[0] <= 3.3 * r1[0], "ratchet global 3x = 3 fois plus de notes");
    chk (rT[0] >= 1.4 * r1[0] && rT[0] <= 1.6 * r1[0], "triolets : 1,5 fois plus de pas");
    chk (r4o[2] == 12 && r4o[1] == 67 + 36, "4 octaves : 12 notes distinctes jusqu'a +3 octaves");
    std::printf ("%s\n", fail ? "ECHEC" : "TOUT OK"); return fail;
}
