// Arpegiateur facon Korg Monopoly : ON / LATCH, UP / DOWN / UP-DOWN, portee 1 OCT / 2 OCT / FULL (4 octaves).
// Les pas sont cadences par le LFO 2 (le "MG2" du Monopoly : 1 pas par cycle, note ouverte la moitie du cycle).
// Le motif depend du mode d'affectation des touches : POLY arpege les notes tenues, MONO / UNISON une seule note a travers les octaves,
// CHORD MEMORY les notes de l'accord memorise, SHARE le partage des 6 voix entre les touches (d'ou des motifs etranges : A B A B A B...).
// Aucune dependance JUCE.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace zephyr
{
struct ArpParams
{
    int on = 0;            // 0 off, 1 on, 2 latch
    int dir = 0;           // 0 up, 1 down, 2 up-down
    int range = 0;         // 0 : 1 octave, 1 : 2 octaves, 2 : full (4 octaves)
    int assign = 0;        // mode d'affectation des touches (voir Synth : poly, mono, unison, share, chord)
    bool hold = false;     // HOLD du synthe : meme effet que LATCH
};

class Arp
{
public:
    struct Ev { int pos; bool on; int note; float vel; bool stack; };

    void prepare (float sampleRate) { fs = sampleRate; reset(); }
    void reset() { nKeys = 0; sounding = -1; soundingStack = false; idx = 0; phase = 0.999999f; stamp = 0; }
    void setParams (const ArpParams& p) { P = p; }
    void setChord (const int* iv, int n) { chordN = std::min (6, std::max (0, n)); for (int i = 0; i < chordN; ++i) chord[i] = iv[i]; }
    void allOff() { nKeys = 0; }

    void noteOn (int note, float vel)
    {
        const bool latch = P.on == 2 || P.hold;
        if (latch && physCount() == 0 && nKeys > 0) nKeys = 0;           // nouvelle prise apres relachement complet : remplace les notes verrouillees
        for (int i = 0; i < nKeys; ++i) if (keys[i].note == note) { keys[i].vel = vel; keys[i].phys = true; keys[i].t = ++stamp; return; }
        if (nKeys == 0) { phase = 0.999999f; idx = 0; }                  // le premier pas part tout de suite
        if (nKeys < kKeys) keys[nKeys++] = { note, vel, true, ++stamp };
    }
    void noteOff (int note)
    {
        for (int i = 0; i < nKeys; ++i)
            if (keys[i].note == note)
            {
                if (P.on == 2 || P.hold) keys[i].phys = false;
                else { for (int j = i; j + 1 < nKeys; ++j) keys[j] = keys[j + 1]; --nKeys; }
                return;
            }
    }

    // stepHz : cadence des pas (frequence du LFO 2) ; renvoie le nombre d'evenements ecrits
    int process (int n, float stepHz, Ev* out, int maxOut)
    {
        int cnt = 0;
        auto off = [&] (int pos) { if (sounding >= 0 && cnt < maxOut) { out[cnt++] = { pos, false, sounding, 0.f, soundingStack }; } sounding = -1; };
        if (P.on == 0) { off (0); return cnt; }
        const float inc = std::max (0.f, stepHz) / fs;
        for (int i = 0; i < n; ++i)
        {
            if (nKeys == 0) { off (i); phase = 0.999999f; idx = 0; continue; }
            phase += inc;
            if (phase >= 1.f)
            {
                phase -= std::floor (phase);
                off (i);
                int seq[128]; const int len = buildSeq (seq);
                if (len > 0 && cnt < maxOut)
                {
                    idx %= len;
                    const int note = seq[idx++];
                    const bool stack = P.assign == 2;
                    out[cnt++] = { i, true, note, lastVel(), stack };
                    sounding = note; soundingStack = stack;
                }
            }
            else if (sounding >= 0 && phase >= 0.5f) off (i);
        }
        return cnt;
    }

private:
    static constexpr int kKeys = 16;
    struct Key { int note; float vel; bool phys; uint64_t t; };
    Key keys[kKeys];
    int nKeys = 0, sounding = -1, idx = 0, chord[6] { 0, 4, 7, 11, 0, 0 }, chordN = 4;
    bool soundingStack = false;
    float fs = 44100.f, phase = 0.999999f;
    uint64_t stamp = 0;
    ArpParams P;

    int physCount() const { int c = 0; for (int i = 0; i < nKeys; ++i) c += keys[i].phys ? 1 : 0; return c; }
    float lastVel() const { float v = 0.8f; uint64_t t = 0; for (int i = 0; i < nKeys; ++i) if (keys[i].t >= t) { t = keys[i].t; v = keys[i].vel; } return v; }
    int lastNote() const { int nt = 60; uint64_t t = 0; for (int i = 0; i < nKeys; ++i) if (keys[i].t >= t) { t = keys[i].t; nt = keys[i].note; } return nt; }

    int buildSeq (int* seq) const
    {
        int base[16], nb = 0;
        switch (P.assign)
        {
            case 1: case 2: base[nb++] = lastNote(); break;                                   // mono / unison : une seule note
            case 4:                                                                           // chord memory
                base[nb++] = lastNote();
                for (int i = 0; i < chordN && nb < 12; ++i) base[nb++] = lastNote() + chord[i];
                break;
            case 3:                                                                           // share : 6 voix reparties dans l'ordre de jeu
            {
                Key byTime[kKeys]; for (int i = 0; i < nKeys; ++i) byTime[i] = keys[i];
                std::sort (byTime, byTime + nKeys, [] (const Key& a, const Key& b) { return a.t < b.t; });
                for (int v = 0; v < 6; ++v) base[nb++] = byTime[v % nKeys].note;
                break;
            }
            default:                                                                          // poly : notes tenues, de bas en haut
            {
                for (int i = 0; i < nKeys; ++i) base[nb++] = keys[i].note;
                std::sort (base, base + nb);
                break;
            }
        }
        const int oct = P.range == 0 ? 1 : (P.range == 1 ? 2 : 4);
        int up[64], nu = 0;
        for (int o = 0; o < oct; ++o)
            for (int i = 0; i < nb && nu < 64; ++i) if (base[i] + 12 * o <= 127) up[nu++] = base[i] + 12 * o;
        int len = 0;
        if (P.dir == 1)       { for (int i = nu - 1; i >= 0; --i) seq[len++] = up[i]; }
        else if (P.dir == 2)
        {
            for (int i = 0; i < nu; ++i) seq[len++] = up[i];
            for (int i = nu - 2; i >= 1; --i) seq[len++] = up[i];
        }
        else                  { for (int i = 0; i < nu; ++i) seq[len++] = up[i]; }
        return len;
    }
};
} // namespace zephyr
