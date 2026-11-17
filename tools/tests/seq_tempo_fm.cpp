// Zephyr / Orage : OCTAVE et TUNE du VCO 2 avec un rapport FM, sequenceur sans ARP, sequenceur qui reste en tempo
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "Arp.h"
#include <cstdio>
using namespace zephyr;
static int bad = 0;
static void check (bool ok, const char* what) { std::printf ("%s %s\n", ok ? "ok  " : "ECHEC", what); if (! ok) ++bad; }

static double pitchOf (const std::vector<float>& x, double fs)       // frequence par passages a zero
{
    int z = 0; double first = -1, last = -1;
    for (size_t i = 1; i < x.size(); ++i) if (x[i - 1] <= 0.f && x[i] > 0.f) { const double t = (double) i - x[i] / (x[i] - x[i - 1]); if (first < 0) first = t; last = t; ++z; }
    return z > 1 ? (z - 1) * fs / (last - first) : 0.0;
}
static double vco2Hz (int ratio, int oct2, float tune2)
{
    Synth s; s.prepare (48000.f); SynthParams p; p.wave1 = 3; p.wave2 = 3; p.lvl1 = 0.f; p.lvl2 = 0.8f; p.fm = 0.f; p.fmRatio = ratio; p.oct2 = oct2; p.tune2 = tune2;
    p.lpf = 1.f; p.lpfRes = 0.f; p.drift = 0.f; p.aA = 0.f; p.aS = 1.f; p.ftype = 1; s.setParams (p); s.noteOn (57, 1.f);
    std::vector<float> L (512), R (512), o;
    for (int b = 0; b < 60; ++b) { s.process (L.data(), R.data(), 512); if (b > 20) o.insert (o.end(), L.begin(), L.end()); }
    return pitchOf (o, 48000.0);
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    // 1) FM : rapport 2:1 (index 5), OCTAVE et TUNE du VCO 2 agissent toujours
    {
        const double base = vco2Hz (5, 0, 0.f), up = vco2Hz (5, 1, 0.f), dn = vco2Hz (5, -1, 0.f), t7 = vco2Hz (5, 0, 7.f), free0 = vco2Hz (0, 1, 0.f);
        std::printf ("  rapport 2:1 sur La 220 : %.1f Hz, octave +1 %.1f, -1 %.1f, +7 demi-tons %.1f (sans rapport, oct +1 : %.1f)\n", base, up, dn, t7, free0);
        check (std::abs (base - 440.0) < 3.0, "rapport 2:1 = 440 Hz");
        check (std::abs (up / base - 2.0) < 0.02 && std::abs (base / dn - 2.0) < 0.02, "OCTAVE du VCO 2 actif avec un rapport FM");
        check (std::abs (t7 / base - std::pow (2.0, 7.0 / 12.0)) < 0.01, "TUNE du VCO 2 actif avec un rapport FM");
    }
    // 2) sequenceur en marche avec ARP eteint
    {
        ZephyrAudioProcessor pr; pr.prepareToPlay (48000, 512);
        pr.setParamReal ("s_arpOn", 0.f); pr.setParamReal ("s_arpType", 1.f);
        juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer mid; mid.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
        int changes = 0; float prev = 0.f; bool was = false;
        for (int b = 0; b < 200; ++b)
        {
            buf.clear(); pr.processBlock (buf, mid); mid.clear();
            const float m = buf.getMagnitude (0, 0, 512); const bool on = m > 0.01f; if (on != was) ++changes; was = on; prev = m;
        }
        juce::ignoreUnused (prev);
        std::printf ("  SEQ sans ARP : %d alternances son / silence en 2 s\n", changes);
        check (changes >= 6, "le sequenceur joue sans allumer l'ARP");
        pr.setParamReal ("s_arpType", 0.f);
    }
    // 3) en tempo : nouvelle note apres relachement -> les pas restent sur la grille ; et calage sur le DAW
    {
        ArpPattern pat; Arp a; a.prepare (48000.f); a.setPattern (&pat);
        ArpParams P; P.on = 1; P.seq = true; P.len = 8; P.gate = 0.5f; a.setParams (P);
        const float stepHz = 8.f; const double stepLen = 48000.0 / stepHz;   // 6000 echantillons par pas
        Arp::Ev ev[256]; std::vector<double> ons; double t = 0;
        auto run = [&] (int blocks) { for (int b = 0; b < blocks; ++b) { const int c = a.process (500, stepHz, ev, 256); for (int k = 0; k < c; ++k) if (ev[k].on && ev[k].lane < 0) ons.push_back (t + ev[k].pos); t += 500; } };
        a.noteOn (60, 0.8f); run (100);               // 50000 ech.
        a.noteOff (60); run (13);                     // 6500 ech. de trou
        a.noteOn (62, 0.8f); run (60);                // reprend
        double worst = 0; for (double o : ons) { const double r = std::fmod (o - ons[0], stepLen); if (r > 0.03 * 48000.0) worst = std::max (worst, std::min (r, stepLen - r)); }   // rattrapage permis : <= 30 ms de retard
        std::printf ("  SEQ libre : %zu pas joues, ecart maxi a la grille %.0f echantillons (pas = %.0f)\n", ons.size(), worst, stepLen);
        check (ons.size() > 12 && worst <= 1.0, "SEQ : changer de note ne relance pas l'horloge");

        Arp b; b.prepare (48000.f); b.setPattern (&pat); b.setParams (P); ons.clear(); t = 0;
        const double bpm = 120.0, spb = 4.0;          // doubles croches : 4 pas par temps -> 8 pas/s
        double ppq = 3.37;                            // le morceau part d'une position quelconque
        b.noteOn (60, 0.8f);
        for (int k = 0; k < 120; ++k)
        {
            if (k == 60) b.noteOff (60);
            if (k == 70) b.noteOn (65, 0.8f);
            b.setHost (true, ppq * spb); const int c = b.process (500, (float) (bpm / 60.0 * spb), ev, 256);
            for (int e = 0; e < c; ++e) if (ev[e].on && ev[e].lane < 0) ons.push_back (ppq * spb + ev[e].pos * (bpm / 60.0 * spb) / 48000.0);
            ppq += 500.0 / 48000.0 * bpm / 60.0;
        }
        double w2 = 0; for (size_t i = 0; i < ons.size(); ++i) { const double r = ons[i] - std::floor (ons[i]); if (r > 0.03 * 8.0) w2 = std::max (w2, std::min (r, 1.0 - r)); }   // pas = 1/8 s : 30 ms = 0.24 pas
        std::printf ("  SEQ cale sur le DAW : %zu pas, ecart maxi a la grille du morceau %.4f pas\n", ons.size(), w2);
        check (ons.size() >= 8 && w2 < 0.01, "SEQ : pas alignes sur la mesure du DAW");
    }
    std::printf (bad ? "%d ECHEC(S)\n" : "TOUT OK\n", bad);
    return bad;
}
