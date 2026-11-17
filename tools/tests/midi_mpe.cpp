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
    auto vo = [&] (int note) -> zephyr::Synth::Voice* { for (auto& v : p.synth.voices) if (v.active && v.note == note) return &v; return nullptr; };
    p.mpeOn.store (1); p.mpeBend.store (3);
    { juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::noteOn (2, 60, 0.8f), 0); mb.addEvent (juce::MidiMessage::noteOn (3, 67, 0.8f), 0); run (p, mb); }
    chk (vo (60) && vo (67), "deux notes MPE sur canaux 2 et 3");
    { juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::pitchWheel (2, 8192 + 4096), 0); run (p, mb); }  // +24 demi-tons a 48
    chk (std::fabs (vo (60)->nBend - 24.f) < 0.1f && vo (67)->nBend == 0.f, "bend canal 2 : seulement la note 60 (+24 st)");
    chk (std::fabs (p.synth.bend) < 1e-6f, "bend global intact");
    { juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::channelPressureChange (3, 100), 0); run (p, mb); }
    chk (vo (67)->pat > 0.1f && vo (60)->pat == 0.f, "pression canal 3 : seulement la note 67");
    { juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::controllerEvent (2, 74, 127), 0); run (p, mb); }
    chk (vo (60)->slide > 0.99f && vo (67)->slide == 0.f, "slide CC74 canal 2 : note 60 seule");
    chk (p.apvts.getParameter ("s_lpf")->getValue() < 0.99f, "CC74 membre ne touche pas le cutoff");
    { juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::noteOff (2, 60), 0); run (p, mb); }
    chk (vo (60) == nullptr || ! vo (60)->gated, "note off canal 2 relache la note 60");
    chk (vo (67) && vo (67)->gated, "note 67 toujours tenue");
    { juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::noteOn (2, 64, 0.8f), 0); run (p, mb); }
    chk (vo (64) && std::fabs (vo (64)->nBend - 24.f) < 0.1f, "nouvelle note sur le canal 2 herite de l'etat du canal");
    { juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::pitchWheel (1, 8192 + 2048), 0); run (p, mb); }
    chk (p.synth.bend > 0.1f, "canal maitre : bend global");
    p.mpeOn.store (0); p.synth.allNotesOff();
    { juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::noteOn (2, 50, 0.8f), 0); mb.addEvent (juce::MidiMessage::channelPressureChange (2, 100), 0); run (p, mb); }
    chk (vo (50) != nullptr && p.synth.chanAT >= 0.f, "MPE off : comportement standard (note jouee)");
    auto s = p.midiMapToString(); p.mpeOn.store (0); p.mpeBend.store (0); p.midiMapFromString (s.replace (";XM0", ";XM1"));
    chk (p.mpeOn.load() == 1 && p.mpeBend.load() == 3, "reglages MPE sauvegardes / relus");
    std::printf (fail ? "ECHEC\n" : "TOUT OK\n"); return fail;
}
