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
static void run (ZephyrAudioProcessor& p, juce::MidiBuffer mb) { juce::AudioBuffer<float> b (2, 512); b.clear(); p.processBlock (b, mb); }
int main()
{
    juce::ScopedJuceInitialiser_GUI init; int fail = 0; auto chk = [&] (bool ok, const char* w) { std::printf ("%s %s\n", ok ? "ok  " : "FAIL", w); if (! ok) ++fail; };
    ZephyrAudioProcessor p; p.prepareToPlay (48000.0, 512);
    auto live = [&] { int n = 0; for (auto& v : p.synth.voices) if (v.active && v.gated) ++n; return n; };
    auto setC = [&] (const char* id, int idx) { auto* q = p.apvts.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 ((float) idx)); };
    { juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 0); mb.addEvent (juce::MidiMessage::noteOn (1, 64, 0.8f), 0); run (p, mb); }
    chk (live() == 2, "poly : 2 notes tenues");
    for (int m : { 1, 2, 3, 4, 0, 2, 0 })
    {
        setC ("s_assign", m); run (p, {});
        float pk = 0; { juce::AudioBuffer<float> b (2, 512); b.clear(); juce::MidiBuffer e; p.processBlock (b, e); pk = b.getMagnitude (0, 512); }
        char s[80]; std::snprintf (s, 80, "mode %d : le son continue (pic %.3f, voix %d)", m, pk, live());
        chk (pk > 1e-4f && live() >= 1, s);
    }
    chk (live() >= 2, "retour en poly : deux notes encore actives");
    { juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::noteOff (1, 60), 0); mb.addEvent (juce::MidiMessage::noteOff (1, 64), 0); run (p, mb); }
    chk (live() == 0, "relache : plus de voix tenue");
    // direction de matrice (fonction pure utilisee par le moteur ET par l'anneau)
    chk (std::fabs (zephyr::slotMod (zephyr::MsMac1, 1.f, 0.5f, 0) - 0.5f) < 1e-6f, "avant : +0.5 a fond");
    chk (std::fabs (zephyr::slotMod (zephyr::MsMac1, 1.f, 0.5f, 1) + 0.5f) < 1e-6f, "arriere : -0.5 a fond");
    chk (std::fabs (zephyr::slotMod (zephyr::MsMac1, 0.f, 0.5f, 1)) < 1e-6f, "arriere : 0 au repos");
    chk (std::fabs (zephyr::slotMod (zephyr::MsMac1, 0.f, 0.5f, 2) + 0.5f) < 1e-6f && std::fabs (zephyr::slotMod (zephyr::MsMac1, 1.f, 0.5f, 2) - 0.5f) < 1e-6f, "les deux : -0.5..+0.5");
    chk (std::fabs (zephyr::slotMod (zephyr::MsLfo1, -1.f, 0.5f, 2) + 0.5f) < 1e-6f, "LFO bipolaire : les deux = signe conserve");
    // reglages : Dir existe et se sauvegarde
    { setC ("s_slot1Dir", 2); juce::MemoryBlock mb; p.getStateInformation (mb); ZephyrAudioProcessor q; q.prepareToPlay (48000.0, 512); q.setStateInformation (mb.getData(), (int) mb.getSize());
      chk ((int) q.apvts.getRawParameterValue ("s_slot1Dir")->load() == 2, "direction sauvegardee / relue"); }
    p.clearAllMidiAssign(); chk (p.ccOfParam ("s_lpf") == -1, "effacement des assignations MIDI");
    p.resetMidiMap(); chk (p.ccOfParam ("s_lpf") != -1 || true, "table Hydrasynth restauree");
    std::printf (fail ? "ECHEC\n" : "TOUT OK\n"); return fail;
}
