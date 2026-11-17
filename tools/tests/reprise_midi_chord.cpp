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

static float sweep (ZephyrAudioProcessor& p, int mode, float start, const std::vector<int>& path, bool smoothedUi)
{
    p.takeoverMode.store (mode);
    auto* q = p.apvts.getParameter ("s_arpGate"); q->setValueNotifyingHost (start);
    run (p, {});
    float mx = 0.f, mn = 1.f;
    for (int v : path) { juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::controllerEvent (1, 107, v), 0); run (p, mb); mx = std::max (mx, q->getValue()); mn = std::min (mn, q->getValue()); }
    std::printf ("  mode %d start %.2f -> fin %.3f (min %.3f max %.3f)\n", mode, start, q->getValue(), mn, mx);
    return q->getValue();
}
int main()
{
    juce::ScopedJuceInitialiser_GUI init; int fail = 0; auto chk = [&] (bool ok, const char* w) { std::printf ("%s %s\n", ok ? "ok  " : "FAIL", w); if (! ok) ++fail; };
    ZephyrAudioProcessor p; p.prepareToPlay (48000.0, 512);
    std::vector<int> up, upDown;
    for (int v = 40; v <= 127; v += 3) up.push_back (v); up.push_back (127);
    upDown = up; for (int v = 127; v >= 0; v -= 4) upDown.push_back (v); for (int v = 0; v <= 127; v += 5) upDown.push_back (v); upDown.push_back (127);
    for (int m = 0; m < 3; ++m)
    {
        std::printf ("mode %d\n", m);
        for (float st : { 0.2f, 0.9f })
        {
            for (int k = 0; k < 2; ++k) { p.hwPos[107] = (k == 0 ? 1.f : 0.f); const float r = sweep (p, m, st, upDown, false); if (m != 1) chk (r > 0.97f, "va a fond apres montee-descente-montee"); }
        }
    }
    { p.takeoverMode.store (2); auto* q = p.apvts.getParameter ("s_arpGate");
      for (int v : { 100, 5 }) { juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::controllerEvent (1, 107, v), 0); run (p, mb); if (v == 100) chk (q->getValue() > 0.999f, "gate : CC 100 -> 100 %"); else chk (q->getValue() < 0.001f, "gate : CC 5 -> minimum"); } }
    // LFO jusqu'a 1 kHz : un LFO sinus a 1000 Hz sur la hauteur cree des bandes laterales a f0 +/- 1000 Hz (vraie modulation audio)
    {
        auto setR = [&] (const char* id, float real) { p.setParamReal (id, real); };
        p.resetModules(); setR ("s_l1Rate", 1.f); setR ("s_slot1Src", 1); setR ("s_slot1Dst", 1); setR ("s_arpOn", 0); setR ("s_assign", 0);
        setR ("s_wave1", 3); setR ("s_lvl2", 0.f); setR ("s_lpf", 1.f); setR ("s_aS", 1.f);
        auto sideEnergy = [&] (bool lfoOn)
        {
            setR ("s_slot1Amt", lfoOn ? 0.08f : 0.f);
            { juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::noteOn (1, 69, 0.9f), 0); run (p, mb); }
            for (int k = 0; k < 20; ++k) run (p, {});
            std::vector<float> x; for (int k = 0; k < 8; ++k) { juce::AudioBuffer<float> b (2, 512); b.clear(); juce::MidiBuffer e; p.processBlock (b, e); for (int i = 0; i < 512; ++i) x.push_back (b.getSample (0, i)); }
            auto bin = [&] (double f) { double re = 0, im = 0; for (size_t i = 0; i < x.size(); ++i) { const double w = 0.5 - 0.5 * std::cos (6.2831853 * i / (double) x.size()); re += w * x[i] * std::cos (6.2831853 * f * i / 48000.0); im += w * x[i] * std::sin (6.2831853 * f * i / 48000.0); } return std::sqrt (re * re + im * im) / x.size(); };
            const double sb = bin (1440.0) + bin (-560.0 < 0 ? 560.0 : 560.0);
            { juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::noteOff (1, 69), 0); run (p, mb); }
            for (int k = 0; k < 400; ++k) run (p, {});
            return sb;
        };
        const double off = sideEnergy (false), on = sideEnergy (true);
        std::printf ("  bandes laterales : sans LFO %.5f, LFO 1 kHz %.5f, kCtl=%d\n", off, on, p.synth.kCtl);
        chk (p.synth.kCtl == 2 || p.synth.kCtl == 16, "bloc de controle raccourci pour LFO rapide");
        chk (on > off * 3.0, "LFO 1 kHz : bandes laterales audibles");
    }
    // CHORD LEARN : les notes sonnent, accord = notes tenues ensemble, auto-fin
    {
        auto setB = [&] (const char* id, float v) { p.apvts.getParameter (id)->setValueNotifyingHost (v); };
        setB ("s_chordLearn", 1.f); run (p, {});
        auto live = [&] { int n = 0; for (auto& v : p.synth.voices) if (v.active && v.gated) ++n; return n; };
        { juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::noteOn (1, 64, 0.8f), 0); mb.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 1); mb.addEvent (juce::MidiMessage::noteOn (1, 67, 0.8f), 2); run (p, mb); }
        chk (live() == 3, "learn : les 3 notes sonnent");
        { juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::noteOff (1, 64), 0); mb.addEvent (juce::MidiMessage::noteOff (1, 60), 1); mb.addEvent (juce::MidiMessage::noteOff (1, 67), 2); run (p, mb); }
        chk (p.chordN == 2 && p.chordIv[0] == 4 && p.chordIv[1] == 7, "accord appris : fondamentale la plus basse, +4 +7");
        chk (p.apvts.getRawParameterValue ("s_chordLearn")->load() < 0.5f, "learn termine tout seul");
    }
    std::printf (fail ? "ECHEC\n" : "TOUT OK\n"); return fail;
}
