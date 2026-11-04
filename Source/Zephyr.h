// Zephyr - synthe de boucles de feedback (larsens, tuyaux qui sifflent, filtres qui s'auto-oscillent). Aucune dependance JUCE.
// Chaque voix : ligne a retard (le tuyau) + filtre passe-bande (choix de l'harmonique) + amortissement + saturation.
// La pression (aftertouch) pousse le gain de boucle et/ou le "vent" (harmonique ou degre de gamme), avec hysteresis.
#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <vector>

namespace eo
{
//  id        nom                       min    max    def    type(0 float,1 bool,2 choix)  centre
#define EO_PARAMS(X) \
    X (out,     "Output",             -24.f,   6.f,  -8.f,  0, 0.f)    \
    X (mode,    "Resonator",            0.f,   1.f,   0.f,  2, 0.f)    \
    X (lock,    "Lock",                 0.f,   1.f,   0.f,  2, 0.f)    \
    X (scale,   "Scale",                0.f,   6.f,   0.f,  2, 0.f)    \
    X (root,    "Root",                 0.f,  11.f,   0.f,  2, 0.f)    \
    X (fb,      "Feedback",             0.f,   1.f,   0.3f, 0, 0.f)    \
    X (wind,    "Wind",                 0.f,   1.f,   0.f,  0, 0.f)    \
    X (range,   "Range",                0.f,   1.f,   0.5f, 0, 0.f)    \
    X (focus,   "Focus",                0.f,   1.f,   0.5f, 0, 0.f)    \
    X (stick,   "Stick",                0.f,   1.f,   0.4f, 0, 0.f)    \
    X (snap,    "Snap",                 0.f,   1.f,   0.7f, 0, 0.f)    \
    X (glide,   "Glide",                5.f, 800.f, 120.f,  0, 150.f)  \
    X (bright,  "Bright",               0.f,   1.f,   0.4f, 0, 0.f)    \
    X (drive,   "Drive",                0.f,   1.f,   0.3f, 0, 0.f)    \
    X (breath,  "Breath",               0.f,   1.f,   0.4f, 0, 0.f)    \
    X (atk,     "Attack",               5.f, 4000.f, 600.f, 0, 300.f)  \
    X (rel,     "Release",             50.f, 8000.f, 1800.f,0, 800.f)  \
    X (detune,  "Detune",               0.f,  30.f,   6.f,  0, 0.f)    \
    X (spread,  "Spread",               0.f,   1.f,   0.6f, 0, 0.f)    \
    X (fm,      "FM Level",             0.f,   1.f,   0.f,  0, 0.f)    \
    X (fmType,  "FM Sound",             0.f,   5.f,   0.f,  2, 0.f)    \
    X (fmChar,  "FM Character",            0.f,   1.f,   0.4f, 0, 0.f)    \
    X (fmRoute, "FM Route",             0.f,   1.f,   0.5f, 0, 0.f)    \
    X (fmDecay, "FM Decay",             0.05f, 8.f,   0.8f, 0, 0.8f)   \
    X (rvSize,  "Reverb Size",          0.f,   1.f,   0.6f, 0, 0.f)    \
    X (rvDamp,  "Reverb Damp",          0.f,   1.f,   0.4f, 0, 0.f)    \
    X (rvMix,   "Reverb Mix",           0.f,   1.f,   0.3f, 0, 0.f)    \
    X (dlMode,  "Delay Mode",           0.f,   2.f,   0.f,  2, 0.f)    \
    X (dlTime,  "Delay Time",           0.f,   1.f,   0.4f, 0, 0.f)    \
    X (dlFb,    "Delay Feedback",       0.f,   1.f,   0.3f, 0, 0.f)    \
    X (dlMod,   "Delay Mod",            0.f,   1.f,   0.3f, 0, 0.f)    \
    X (dlRate,  "Delay Mod Rate",       0.05f, 10.f,  0.8f, 0, 0.8f)   \
    X (dlTone,  "Delay Tone",           0.f,   1.f,   0.5f, 0, 0.f)    \
    X (dlMix,   "Delay Mix",            0.f,   1.f,   0.f,  0, 0.f)    \
    X (dlLoop,  "Delay To Loop",        0.f,   1.f,   0.f,  0, 0.f)    \
    X (dlRoute, "FX Routing",           0.f,   2.f,   0.f,  2, 0.f)    \
    X (p1t,     "Pressure 1 Target",    0.f,   7.f,   0.f,  2, 0.f)    \
    X (p1a,     "Pressure 1 Amount",   -1.f,   1.f,   0.5f, 0, 0.f)    \
    X (p2t,     "Pressure 2 Target",    0.f,   7.f,   1.f,  2, 0.f)    \
    X (p2a,     "Pressure 2 Amount",   -1.f,   1.f,   0.6f, 0, 0.f)    \
    X (p3t,     "Pressure 3 Target",    0.f,   7.f,   3.f,  2, 0.f)    \
    X (p3a,     "Pressure 3 Amount",   -1.f,   1.f,   0.f,  0, 0.f)    \
    X (pcurve,  "Pressure Curve",       0.4f,  2.5f,  1.f,  0, 1.f)    \
    X (mpe,     "MPE",                  0.f,   1.f,   0.f,  1, 0.f)    \
    X (bend,    "Bend Range",           1.f,  48.f,   2.f,  0, 0.f)    \
    X (hold,    "Hold",                 0.f,   1.f,   0.f,  1, 0.f)

enum P
{
#define EO_ENUM(id, nm, mn, mx, df, kind, ctr) p_##id,
    EO_PARAMS (EO_ENUM)
#undef EO_ENUM
    kNumP
};

struct ParamDef { const char* id; const char* name; float mn, mx, def; int kind; float centre; };
inline const ParamDef* paramDefs()
{
    static const ParamDef defs[] = {
#define EO_DEF(id, nm, mn, mx, df, kind, ctr) { #id, nm, mn, mx, df, kind, ctr },
        EO_PARAMS (EO_DEF)
#undef EO_DEF
    };
    return defs;
}

constexpr float kPi = 3.14159265358979f;
constexpr int kVoices = 8;
constexpr int kTargets = 8;           // FEEDBACK WIND FOCUS BRIGHT DRIVE BREATH PITCH FM
constexpr float kTapGain = 2.5f;

inline float fastTanh (float x)       // rationnelle, bornee a +-1
{
    x = std::max (-4.f, std::min (4.f, x));
    const float x2 = x * x;
    return x * (27.f + x2) / (27.f + 9.f * x2);
}

// ---------------------------------------------------------------- briques
struct Svf          // filtre a variables d'etat (TPT) : passe-bande normalise, gain 1 au centre
{
    float ic1 = 0.f, ic2 = 0.f, a1 = 0.f, a2 = 0.f, a3 = 0.f, k = 1.f;
    void set (float fc, float Q, float fs)
    {
        const float g = std::tan (kPi * std::min (fc, 0.45f * fs) / fs);
        k = 1.f / std::max (0.3f, Q);
        a1 = 1.f / (1.f + g * (g + k)); a2 = g * a1; a3 = g * a2;
    }
    float bp (float v0)
    {
        const float v3 = v0 - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.f * v1 - ic1; ic2 = 2.f * v2 - ic2;
        return k * v1;
    }
    void reset() { ic1 = ic2 = 0.f; }
};

struct DelayLine
{
    std::vector<float> buf; int mask = 0, w = 0;
    void prepare (int minSize) { int n = 16; while (n < minSize) n <<= 1; buf.assign ((size_t) n, 0.f); mask = n - 1; w = 0; }
    void clear() { std::fill (buf.begin(), buf.end(), 0.f); }
    void write (float x) { buf[(size_t) w] = x; w = (w + 1) & mask; }
    float read (float d) const       // interpolation cubique (Hermite) ; d >= 2 echantillons
    {
        const float pos = (float) w - 1.f - d;
        const int i = (int) std::floor (pos); const float f = pos - (float) i;
        const float y0 = buf[(size_t) ((i - 1) & mask)], y1 = buf[(size_t) (i & mask)], y2 = buf[(size_t) ((i + 1) & mask)], y3 = buf[(size_t) ((i + 2) & mask)];
        const float c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.f * y2 - 0.5f * y3, c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * f + c2) * f + c1) * f + y1;
    }
};

struct Rng { uint32_t s = 2463534242u; float next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (int32_t) s * (1.f / 2147483648.f); } };

// ---------------------------------------------------------------- reverb (reseau a retards, matrice de Hadamard)
class Reverb
{
public:
    void prepare (float sr)
    {
        fs = sr;
        static const int base[8] { 1117, 1327, 1559, 1783, 2017, 2243, 2473, 2699 };
        for (int i = 0; i < 8; ++i)
        {
            maxLen[i] = (int) ((float) base[i] * sr / 44100.f * 2.1f) + 16;
            line[i].assign ((size_t) maxLen[i], 0.f); pos[i] = 0; lp[i] = 0.f;
            baseLen[i] = (float) base[i] * sr / 44100.f; phase[i] = 0.37f * (float) i;
        }
        for (int i = 0; i < 4; ++i) { ap[i].assign ((size_t) (int) (sr * 0.02f), 0.f); apPos[i] = 0; }
        setParams (0.6f, 0.4f, 0.3f);
    }
    void setParams (float size, float damp, float mix)
    {
        lenScale = 0.7f + 0.9f * size;
        const float rt = 1.5f + 16.f * size * size;
        for (int i = 0; i < 8; ++i) gain[i] = std::pow (10.f, -3.f * baseLen[i] * lenScale / (fs * rt));
        dampC = 1.f - (0.05f + 0.9f * (1.f - damp) * (1.f - damp));
        wet = mix;
    }
    void process (float inL, float inR, float& outL, float& outR)
    {
        float x = 0.5f * (inL + inR);
        static const int apLen[4] { 142, 107, 379, 277 };
        for (int k = 0; k < 4; ++k)                                        // diffusion d'entree
        {
            const int n = std::min ((int) ap[k].size() - 1, (int) ((float) apLen[k] * fs / 44100.f));
            const int r = (apPos[k] + (int) ap[k].size() - n) % (int) ap[k].size();
            const float d = ap[k][(size_t) r], v = x + 0.6f * d;
            ap[k][(size_t) apPos[k]] = v; apPos[k] = (apPos[k] + 1) % (int) ap[k].size();
            x = d - 0.6f * v;
        }
        float y[8];
        for (int i = 0; i < 8; ++i)
        {
            phase[i] += 0.00005f * (1.f + 0.13f * (float) i); if (phase[i] > 1.f) phase[i] -= 1.f;
            const float mod = 2.5f * std::sin (2.f * kPi * phase[i]);
            const float d = std::min ((float) maxLen[i] - 4.f, baseLen[i] * lenScale + mod);
            float p = (float) pos[i] - d; while (p < 0.f) p += (float) maxLen[i];
            const int i0 = (int) p; const float f = p - (float) i0; const int i1 = (i0 + 1) % maxLen[i];
            const float v = line[i][(size_t) i0] * (1.f - f) + line[i][(size_t) i1] * f;
            lp[i] += (v - lp[i]) * dampC; y[i] = lp[i];
        }
        // Hadamard 8x8 (transformee de Walsh), normalisee
        for (int s = 1; s < 8; s <<= 1)
            for (int i = 0; i < 8; i += 2 * s)
                for (int j = i; j < i + s; ++j) { const float a = y[j], b = y[j + s]; y[j] = a + b; y[j + s] = a - b; }
        const float nrm = 0.35355339f;
        float oL = 0.f, oR = 0.f;
        for (int i = 0; i < 8; ++i)
        {
            const float fed = y[i] * nrm * gain[i] + x * 0.25f;
            line[i][(size_t) pos[i]] = fed; pos[i] = (pos[i] + 1) % maxLen[i];
            if (i & 1) oR += fed; else oL += fed;
        }
        outL = oL * 0.5f * wet; outR = oR * 0.5f * wet;
    }
    float dryGain() const { return 1.f - 0.5f * wet; }
private:
    float fs = 44100.f, lenScale = 1.f, dampC = 0.5f, wet = 0.3f;
    std::vector<float> line[8], ap[4]; int maxLen[8] {}, pos[8] {}, apPos[4] {};
    float baseLen[8] {}, gain[8] {}, lp[8] {}, phase[8] {};
};

// ---------------------------------------------------------------- delay (normal / ressort / psychedelique)
class DelayFx
{
public:
    void prepare (float sr)
    {
        fs = sr; const int n = (int) (sr * 2.2f);
        for (int c = 0; c < 2; ++c) { line[c].prepare (n); line[c].clear(); lpS[c] = 0.f; dcX[c] = dcY[c] = 0.f; for (auto& a : ap[c]) a = {}; tS[c] = 0.f; }
        ph = 0.f; ph2 = 0.f;
    }
    void setParams (int m, float time01, float fbv, float mod, float rateHz, float tone)
    {
        mode = std::max (0, std::min (2, m));
        if (mode == 0)      { timeMs = 30.f * std::pow (30.f, time01);   fbG = 0.92f * fbv; }
        else if (mode == 1) { timeMs = 1.5f * std::pow (14.7f, time01);  fbG = 0.985f * fbv; }
        else                { timeMs = 120.f * std::pow (11.7f, time01); fbG = 0.92f * fbv; }
        modAmt = mod; rate = rateHz;
        const float fc = 500.f * std::pow (28.f, tone);                              // 500 Hz .. 14 kHz
        lpC = 1.f - std::exp (-2.f * kPi * std::min (fc, 0.45f * fs) / fs);
        smC = 1.f - std::exp (-1.f / ((mode == 1 ? 0.012f : 0.08f) * fs));
    }
    void process (float inL, float inR, float& outL, float& outR)
    {
        ph += rate / fs; if (ph > 1.f) ph -= 1.f;
        ph2 += rate * 0.37f / fs; if (ph2 > 1.f) ph2 -= 1.f;
        const float in[2] { inL, inR }; float y[2];
        for (int c = 0; c < 2; ++c)
        {
            const float off = (c == 0) ? 0.f : 0.25f;
            float lfo = std::sin (2.f * kPi * (ph + off));
            float tgtMs = timeMs * (c == 0 ? 1.f : (mode == 0 ? 1.5f : (mode == 2 ? 1.19f : 1.07f)));
            float depthMs;
            if (mode == 0)      depthMs = modAmt * 1.5f;
            else if (mode == 1) { depthMs = modAmt * 0.45f * tgtMs; lfo = 0.7f * lfo + 0.3f * std::sin (2.f * kPi * (ph2 * 2.7f + off)); }
            else                { depthMs = modAmt * 0.35f * tgtMs; lfo = 0.65f * lfo + 0.35f * std::sin (2.f * kPi * (ph2 + 0.5f * off)); }
            tS[c] += (tgtMs - tS[c]) * smC;
            const float ms = std::max (0.6f, tS[c] + depthMs * lfo);
            const float d = std::max (2.f, std::min ((float) line[c].mask - 8.f, ms * 0.001f * fs));
            y[c] = line[c].read (d);
        }
        outL = y[0]; outR = y[1];
        for (int c = 0; c < 2; ++c)
        {
            const float src = (mode == 1) ? y[c] : y[1 - c];                           // ressort : boucle propre ; sinon ping-pong
            lpS[c] += (src - lpS[c]) * lpC; float f = lpS[c];
            if (mode == 1) for (auto& a : ap[c]) f = a.run (f, 0.62f);                  // dispersion : c'est elle qui fait le "boing"
            const float hp = f - dcX[c] + 0.995f * dcY[c]; dcX[c] = f; dcY[c] = hp;
            line[c].write (in[c] + fastTanh (fbG * hp));
        }
    }
private:
    struct Ap { float x1 = 0.f, y1 = 0.f; float run (float x, float a) { const float y = a * x + x1 - a * y1; x1 = x; y1 = y; return y; } };
    float fs = 44100.f, timeMs = 100.f, fbG = 0.f, modAmt = 0.f, rate = 1.f, lpC = 0.5f, smC = 0.01f, ph = 0.f, ph2 = 0.f;
    int mode = 0; DelayLine line[2]; float lpS[2] {}, dcX[2] {}, dcY[2] {}, tS[2] {}; Ap ap[2][6];
};

// ---------------------------------------------------------------- gammes
inline const int* scaleMask (int s)         // 12 notes : 1 = dans la gamme (relatif a la tonique)
{
    static const int m[7][12] {
        { 1,0,1,0,1,1,0,1,0,1,0,1 },   // majeure
        { 1,0,1,1,0,1,0,1,1,0,1,0 },   // mineure
        { 1,0,1,1,0,1,0,1,0,1,1,0 },   // dorien
        { 1,0,0,1,0,1,0,1,0,0,1,0 },   // pentatonique mineure
        { 1,0,1,0,1,0,0,1,0,1,0,0 },   // pentatonique majeure
        { 1,0,1,0,1,0,1,0,1,0,1,0 },   // tons entiers
        { 1,1,1,1,1,1,1,1,1,1,1,1 } }; // chromatique
    return m[std::max (0, std::min (6, s))];
}

// ---------------------------------------------------------------- petit synthe FM (2 operateurs) : la matiere qui excite le tuyau
struct FmSound { float cr, mr, idx, fbk, decS, idxDecS, sweep, sweepT, wob; };
inline const FmSound& fmSound (int i)
{
    static const FmSound t[6] {
        // porteuse modulante indice retour  dec  decIdx balayage temps wobble
        { 1.f, 3.5f,   3.0f, 0.0f, 1.0f, 0.5f, 0.0f, 0.01f, 0.f },   // Bell   : rapport inharmonique, brillant puis doux
        { 1.f, 1.4142f,4.0f, 0.6f, 1.2f, 1.0f, 0.0f, 0.01f, 0.f },   // Metal  : sonnaille, retour dans le modulateur
        { 1.f, 2.76f,  2.0f, 0.0f, 0.12f,0.6f, 0.15f,0.01f, 0.f },   // Wood   : claque courte
        { 1.f, 1.0f,   1.5f, 0.0f, 0.8f, 0.5f, 0.6f, 0.05f, 14.f },  // Spring : hauteur qui ondule vers le bas
        { 1.f, 2.0f,   5.0f, 0.2f, 0.5f, 0.4f, 5.0f, 0.07f, 0.f },   // Laser  : chute rapide de hauteur
        { 1.f, 5.01f,  0.8f, 0.0f, 2.0f, 2.0f, 0.0f, 0.01f, 0.f } }; // Glass  : fin, long, scintillant
    return t[std::max (0, std::min (5, i))];
}

// ---------------------------------------------------------------- moteur
class Zephyr
{
public:
    // lecture par l'interface
    std::atomic<float> outPk { 0.f }, vPos[kVoices], vLvl[kVoices], lastPressure { 0.f };
    std::atomic<int> midiCount { 0 }, activeVoices { 0 };

    Zephyr() { for (int i = 0; i < kVoices; ++i) { vPos[i] = 0.f; vLvl[i] = 0.f; } }

    void prepare (float sampleRate)
    {
        fs = sampleRate;
        for (auto& v : voices) { v = Voice(); v.dl.prepare ((int) (fs / 12.f) + 16); }
        reverb.prepare (fs); delay.prepare (fs);
        for (int i = 0; i < 64; ++i) dlFed[i] = dlFedNext[i] = 0.f;
        for (int c = 0; c < 16; ++c) chanP[c] = 0.f, bendCh[c] = 0.f;
        mw = 0.f; held = false; sustainPedal = false; pkS = 0.f; stamp = 0;
    }

    void setParams (const float* v)
    {
        for (int i = 0; i < kNumP; ++i) p[i] = v[i];
        reverb.setParams (p[p_rvSize], p[p_rvDamp], p[p_rvMix]);
        delay.setParams ((int) p[p_dlMode], p[p_dlTime], p[p_dlFb], p[p_dlMod], p[p_dlRate], p[p_dlTone]);
        outLin = std::pow (10.f, p[p_out] / 20.f);
        const bool h = p[p_hold] > 0.5f;
        if (held && ! h) releaseDeferred();
        held = h;
    }

    // ---- MIDI
    void noteOn (int ch, int note, float vel)
    {
        ++midiCount;
        Voice* v = nullptr;
        for (auto& x : voices) if (x.active && x.note == note && x.ch == ch) { v = &x; break; }
        if (v == nullptr) for (auto& x : voices) if (! x.active) { v = &x; break; }
        if (v == nullptr)                                                      // vol de voix : la plus discrete d'abord
        {
            float best = 1e9f;
            for (auto& x : voices) { const float sc = (x.keyDown ? 1000.f : 0.f) + x.env * 100.f - (float) (stamp - x.age) * 1e-6f; if (sc < best) { best = sc; v = &x; } }
        }
        const bool fresh = ! v->active;
        if (fresh) { v->dl.clear(); v->bp.reset(); v->lp = 0.f; v->env = 0.f; v->pos = 1.f; v->h = 1.f; v->q = note; v->posSemi = (float) note; v->pressS = 0.f; v->d = 2.f; v->dInit = false; }
        v->active = true; v->keyDown = true; v->deferred = false; v->releasing = false;
        v->note = note; v->ch = ch; v->vel = vel; v->age = ++stamp;
        v->kick = 0.25f * (0.3f + vel); v->polyP = 0.f; v->fmT = 0.f; v->fmPhC = v->fmPhM = v->fmFbS = 0.f;
        v->detuneCents = (rng.next()) * p[p_detune];
        v->panPos = rng.next();
        const int scalePc = (int) p[p_root];
        (void) scalePc;
    }
    void noteOff (int ch, int note)
    {
        ++midiCount;
        for (auto& x : voices)
            if (x.active && x.keyDown && x.note == note && x.ch == ch)
            {
                x.keyDown = false;
                if (held || sustainPedal) x.deferred = true; else x.releasing = true;
            }
    }
    void polyPressure (int ch, int note, float val)
    {
        ++midiCount; lastPressure.store (val);
        for (auto& x : voices) if (x.active && x.note == note && (p[p_mpe] < 0.5f || x.ch == ch)) x.polyP = val;
    }
    void channelPressure (int ch, float val) { ++midiCount; lastPressure.store (val); chanP[ch & 15] = val; }
    void pitchBend (int ch, float v) { ++midiCount; bendCh[ch & 15] = v; }
    void controller (int ch, int num, float val)
    {
        ++midiCount;
        if (num == 1) mw = val;
        else if (num == 64) { const bool on = val >= 0.5f; if (sustainPedal && ! on) { sustainPedal = false; releaseDeferred(); } sustainPedal = on; }
        else if (num == 123 || num == 120) for (auto& x : voices) if (x.active) { x.keyDown = false; x.releasing = true; x.deferred = false; }
        (void) ch;
    }

    void process (float* L, float* R, int n, const float* extL, const float* extR)
    {
        float peak = 0.f;
        float mix[256];
        int done = 0;
        while (done < n)
        {
            const int m = std::min (64, n - done);
            for (int i = 0; i < m; ++i) mix[i] = 0.f;
            float panL[kVoices], panR[kVoices];
            float busL[256], busR[256];
            for (int i = 0; i < m; ++i) busL[i] = busR[i] = 0.f;
            int act = 0;
            for (int vi = 0; vi < kVoices; ++vi)
            {
                auto& v = voices[vi];
                if (! v.active) { vPos[vi].store (0.f); vLvl[vi].store (0.f); continue; }
                ++act;
                for (int i = 0; i < m; ++i) mix[i] = 0.f;
                runVoice (v, vi, mix, m, dlFed, nullptr);                          // le retour du delay entre dans la boucle (avec un bloc de 64 echantillons de retard)
                const float pan = 0.5f + 0.5f * p[p_spread] * v.panPos;
                panL[vi] = std::cos (pan * 0.5f * kPi); panR[vi] = std::sin (pan * 0.5f * kPi);
                for (int i = 0; i < m; ++i) { busL[i] += mix[i] * panL[vi]; busR[i] += mix[i] * panR[vi]; }
            }
            activeVoices.store (act);
            const float dg = reverb.dryGain();
            const int rt = (int) p[p_dlRoute]; const float dm = p[p_dlMix];
            for (int i = 0; i < m; ++i)
            {
                const float bl = busL[i], br = busR[i];
                float wl, wr, dL, dR, oL, oR;
                if (rt == 0)        // reverb puis delay
                {
                    reverb.process (bl, br, wl, wr);
                    const float xl = bl * dg + wl, xr = br * dg + wr;
                    delay.process (xl, xr, dL, dR);
                    oL = xl + dm * dL; oR = xr + dm * dR;
                }
                else if (rt == 1)   // delay puis reverb
                {
                    delay.process (bl, br, dL, dR);
                    const float xl = bl + dm * dL, xr = br + dm * dR;
                    reverb.process (xl, xr, wl, wr);
                    oL = xl * dg + wl; oR = xr * dg + wr;
                }
                else                // en parallele
                {
                    reverb.process (bl, br, wl, wr);
                    delay.process (bl, br, dL, dR);
                    oL = bl * dg + wl + dm * dL; oR = br * dg + wr + dm * dR;
                }
                dlFedNext[i] = fastTanh (0.5f * (dL + dR));
                float l = oL * outLin, r = oR * outLin;
                l = std::max (-1.f, std::min (1.f, fastTanh (l))); r = std::max (-1.f, std::min (1.f, fastTanh (r)));   // securite : jamais au-dela de 0 dBFS
                L[done + i] = l; R[done + i] = r;
                peak = std::max (peak, std::max (std::fabs (l), std::fabs (r)));
            }
            for (int i = 0; i < 64; ++i) dlFed[i] = (i < m) ? dlFedNext[i] : 0.f;
            done += m;
        }
        pkS = std::max (peak, pkS * std::pow (0.9f, (float) n / 256.f));
        outPk.store (pkS);
    }

private:
    struct Voice
    {
        bool active = false, keyDown = false, deferred = false, releasing = false, dInit = false;
        int note = 60, ch = 0, q = 60; float vel = 0.f, polyP = 0.f, pressS = 0.f, env = 0.f, kick = 0.f;
        float pos = 1.f, h = 1.f, posSemi = 60.f, detuneCents = 0.f, panPos = 0.f;
        float lp = 0.f, d = 2.f, dTarget = 2.f, gain = 0.9f, dr = 1.f, bpOut = 0.f;
        float noiseAmp = 0.f, extAmp = 0.f, atkC = 0.001f, relC = 0.9999f;
        int64_t age = 0; int ctl = 0;
        Svf bp; DelayLine dl; Rng rng;
        float lpC = 0.5f, lpDelay = 0.f, drv = 1.f, tapNorm = 0.5f; float dStep = 0.f;
        float level = 0.f;
        float fmT = 0.f, fmPhC = 0.f, fmPhM = 0.f, fmFbS = 0.f, fmGate = 0.f, fmAmt = 0.f, fmF = 440.f;
    };
    Voice voices[kVoices];
    Reverb reverb;
    DelayFx delay;
    float dlFed[64] {}, dlFedNext[64] {};
    Rng rng;
    float p[kNumP] {};
    float fs = 44100.f, outLin = 0.4f, chanP[16] {}, bendCh[16] {}, mw = 0.f, pkS = 0.f;
    bool held = false, sustainPedal = false;
    int64_t stamp = 0;

    void releaseDeferred() { for (auto& x : voices) if (x.active && x.deferred && ! x.keyDown) { x.deferred = false; x.releasing = true; } }

    static int nextScaleNote (int q, int dir, const int* mask, int root)       // voisin dans la gamme (dir = +1 / -1)
    {
        for (int k = 1; k < 13; ++k) { const int c = q + dir * k; if (mask[((c - root) % 12 + 12) % 12]) return c; }
        return q + dir;
    }
    static int nearestScaleNote (float m, const int* mask, int root)
    {
        const int r = (int) std::lround (m);
        for (int k = 0; k < 7; ++k)
        {
            if (mask[((r + k - root) % 12 + 12) % 12] && (! mask[((r - k - root) % 12 + 12) % 12] || m >= (float) r)) return r + k;
            if (mask[((r - k - root) % 12 + 12) % 12]) return r - k;
        }
        return r;
    }

    void runVoice (Voice& v, int vi, float* out, int n, const float* extL, const float* extR)
    {
        constexpr int CT = 16;
        const bool pipe = p[p_mode] < 0.5f, harmonic = p[p_lock] < 0.5f, mpe = p[p_mpe] > 0.5f;
        const float dt = (float) CT / fs;
        const int* mask = scaleMask ((int) p[p_scale]);
        const int root = (int) p[p_root];
        const float stickM = 0.45f * p[p_stick];
        const float maxD = (float) (v.dl.mask - 6);
        const float atkMs = p[p_atk], relMs = p[p_rel];
        v.atkC = 1.f - std::exp (-1.f / (std::max (3.f, atkMs) * 0.001f * fs * 0.45f));
        v.relC = std::exp (-6.9f / (std::max (20.f, relMs) * 0.001f * fs));

        for (int i = 0; i < n; ++i)
        {
            if (v.ctl == 0)
            {
                // ---- pression -> sources de modulation
                const float chp = mpe ? chanP[v.ch & 15] : *std::max_element (chanP, chanP + 16);
                float pr = std::max (v.polyP, std::max (chp, mw));
                if (! v.keyDown) pr = v.deferred ? v.polyP : 0.f;                       // une note relachee (ou tenue par HOLD / pedale) ne repart jamais sur la pression d'une AUTRE touche ou d'une molette
                pr = std::pow (std::max (0.f, std::min (1.f, pr)), p[p_pcurve]);
                v.pressS += (pr - v.pressS) * (1.f - std::exp (-dt / 0.025f));
                float m[kTargets] {};
                m[(int) p[p_p1t]] += p[p_p1a] * v.pressS; m[(int) p[p_p2t]] += p[p_p2a] * v.pressS; m[(int) p[p_p3t]] += p[p_p3a] * v.pressS;
                auto cl = [] (float x) { return std::max (0.f, std::min (1.f, x)); };
                const float fbE = cl (p[p_fb] + m[0]), windE = cl (p[p_wind] + m[1]), focE = cl (p[p_focus] + m[2]);
                const float briE = cl (p[p_bright] + m[3]), drvE = cl (p[p_drive] + m[4]), brE = cl (p[p_breath] + m[5]);
                const float pitchMod = 2.f * m[6], fmE = cl (p[p_fm] + m[7]);
                const float bendSemi = (mpe ? bendCh[v.ch & 15] : bendCh[0]) * p[p_bend];
                const float semi = (float) v.note + bendSemi + v.detuneCents * 0.01f + pitchMod;
                const float glideC = 1.f - std::exp (-dt / (std::max (5.f, p[p_glide]) * 0.001f));
                float fBP, fLoop, normPos;
                if (harmonic)
                {
                    const float f0 = 440.f * std::pow (2.f, (semi - 69.f) / 12.f);
                    const float maxH = 2.f + p[p_range] * 14.f, pp = 1.f + windE * (maxH - 1.f);
                    if (std::fabs (pp - v.h) > 0.5f + stickM) v.h = std::max (1.f, std::min (std::floor (maxH), std::round (pp)));
                    const float tgt = pp + p[p_snap] * (v.h - pp);
                    v.pos += (tgt - v.pos) * glideC;
                    fBP = f0 * v.pos; fLoop = f0; normPos = (v.pos - 1.f) / (maxH - 1.f);
                }
                else
                {
                    const float span = 12.f + p[p_range] * 36.f, mAbs = semi + windE * span;
                    int q = v.q;
                    if (mask[((q - root) % 12 + 12) % 12] == 0) q = nearestScaleNote ((float) q, mask, root);
                    for (int guard = 0; guard < 8; ++guard)
                    {
                        const int hi = nextScaleNote (q, 1, mask, root), lo = nextScaleNote (q, -1, mask, root);
                        if (mAbs > (float) q + (float) (hi - q) * (0.5f + stickM)) q = hi;
                        else if (mAbs < (float) q - (float) (q - lo) * (0.5f + stickM)) q = lo;
                        else break;
                    }
                    v.q = q;
                    const float tgt = mAbs + p[p_snap] * ((float) q - mAbs);
                    v.posSemi += (tgt - v.posSemi) * glideC;
                    fBP = fLoop = 440.f * std::pow (2.f, (std::max (0.f, std::min (127.f, v.posSemi)) - 69.f) / 12.f);
                    normPos = (v.posSemi - (float) v.note) / span;
                }
                fBP = std::max (20.f, std::min (0.42f * fs, fBP)); fLoop = std::max (16.f, std::min (0.4f * fs, fLoop));
                const float Q = std::max (0.8f, 0.8f * std::pow (15.f, focE) * (harmonic ? v.pos : 1.f));
                const float fcut = std::min (0.45f * fs, std::max (fBP, fLoop) * (1.1f + 14.f * briE * briE));
                const float a = std::exp (-2.f * kPi * fcut / fs);
                v.lpC = 1.f - a; v.lpDelay = a / (1.f - a);
                const float w0 = 2.f * kPi * fBP / fs;                                  // perte de l'amortissement a la frequence qui oscille :
                const float lpGain = (1.f - a) / std::sqrt (std::max (1e-6f, 1.f - 2.f * a * std::cos (w0) + a * a));   // compensee, donc gain 1.0 = seuil exact
                const float lpPh = std::atan2 (a * std::sin (w0), 1.f - a * std::cos (w0)) / std::max (1e-4f, w0);
                v.lpDelay = lpPh;
                float D = pipe ? fs / fLoop - 1.f - v.lpDelay : 1.f;
                D = std::max (2.f, std::min (maxD, D));
                if (! v.dInit) { v.d = D; v.dInit = true; }
                v.dTarget = D; v.dStep = (D - v.d) / (float) CT;
                float fcBP = fBP, phiComp = 1.f;                                         // mode FILTER : le retard minimal decale la phase de la boucle
                if (! pipe)                                                              // -> on recale le centre du filtre (phase exacte) et on compense la perte
                {
                    const float phi = std::min (1.3f, w0 * (D + 1.f) + std::atan2 (a * std::sin (w0), 1.f - a * std::cos (w0)));
                    const float u = std::tan (phi) / Q, x = 0.5f * (-u + std::sqrt (u * u + 4.f));
                    const float gc = std::tan (kPi * std::min (fBP, 0.45f * fs) / fs) / x;
                    fcBP = fs / kPi * std::atan (gc); phiComp = std::cos (phi);
                }
                v.bp.set (fcBP, Q, fs);
                const float comp = std::max (0.3f, lpGain * phiComp);
                float g = (0.88f + 0.30f * fbE) / comp;
                if (v.releasing) g = std::min (g, 0.9f / comp);
                v.gain = g;
                v.drv = 1.f + 7.f * drvE * drvE;
                v.tapNorm = 0.5f / std::sqrt (v.drv);
                v.noiseAmp = 0.12f * brE * brE * (0.3f + v.vel);
                v.extAmp = p[p_dlLoop] * 0.6f; v.fmAmt = fmE; v.fmF = std::max (20.f, std::min (0.4f * fs, fLoop));
                vPos[vi].store (cl (normPos));
            }
            v.ctl = (v.ctl + 1) % CT;
            v.d += v.dStep;
            // ---- boucle
            const float x = v.dl.read (v.d);
            float y = v.bp.bp (x);
            v.lp += (y - v.lp) * v.lpC; y = v.lp;
            const float yd = fastTanh (v.drv * y);
            const float s = yd / v.drv;
            float inj = v.rng.next () * (v.noiseAmp + 1e-6f) + v.kick * v.rng.next();
            v.kick *= 0.9985f;
            if (extL != nullptr) inj += v.extAmp * 0.5f * (extL[i] + (extR != nullptr ? extR[i] : extL[i]));
            // ---- FM : une part excite la boucle, une part sonne a cote
            float fmOut = 0.f;
            if (v.fmAmt > 1e-4f)
            {
                const FmSound& T = fmSound ((int) p[p_fmType]);
                const float fun = p[p_fmChar], dec = p[p_fmDecay];
                v.fmT += 1.f / fs;
                const float ampEnv = dec >= 7.9f ? 1.f : std::exp (-v.fmT / std::max (0.02f, dec * T.decS));
                float idxEnv = std::exp (-v.fmT / std::max (0.02f, std::min (dec, 4.f) * T.idxDecS));
                if (dec >= 7.9f) idxEnv = 0.25f + 0.75f * idxEnv;
                const float sw = T.sweep * (0.4f + 1.2f * fun) * std::exp (-v.fmT / T.sweepT) * (T.wob > 0.f ? std::cos (2.f * kPi * T.wob * v.fmT) : 1.f);
                const float f = std::min (0.45f * fs, v.fmF * T.cr * std::max (0.2f, 1.f + sw));
                const float idx = T.idx * (0.25f + 2.2f * fun) * idxEnv * std::max (0.2f, std::min (1.f, 1200.f / f));   // moins d'indice dans l'aigu : pas de repliement
                v.fmPhM += f * T.mr / fs; v.fmPhM -= std::floor (v.fmPhM);
                const float mod = std::sin (2.f * kPi * v.fmPhM + T.fbk * v.fmFbS); v.fmFbS = mod;
                v.fmPhC += f / fs; v.fmPhC -= std::floor (v.fmPhC);
                fmOut = std::sin (2.f * kPi * v.fmPhC + idx * mod) * ampEnv;
            }
            if (v.releasing || (! v.keyDown && ! v.deferred)) v.fmGate *= v.relC; else v.fmGate += (1.f - v.fmGate) * 0.02f;
            fmOut *= v.fmGate;
            const float route = p[p_fmRoute];
            inj += fmOut * v.fmAmt * 0.35f * std::min (1.f, 2.f * (1.f - route));
            out[i] += fmOut * v.fmAmt * 0.2f * std::min (1.f, 2.f * route);
            v.dl.write (inj + v.gain * s);
            // ---- enveloppe
            if (v.releasing || (! v.keyDown && ! v.deferred)) v.env *= v.relC;
            else v.env += (1.f - v.env) * v.atkC;
            const float lv = v.level * 25.f;                                        // sous le seuil d'auto-oscillation (pas de pression) : on remonte le niveau, effet nul des que la note chante
            out[i] += yd * v.tapNorm * v.env * kTapGain * (1.f + 8.f / (1.f + lv * lv));
            v.level = std::max (v.level * 0.9995f, std::fabs (yd * v.tapNorm * v.env));
        }
        vLvl[vi].store (std::min (1.f, v.level * 3.f));
        if ((v.releasing || (! v.keyDown && ! v.deferred)) && v.env < 1e-4f && v.fmGate < 1e-4f) { v.active = false; v.level = 0.f; }
    }
};
} // namespace eo
