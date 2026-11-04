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
    X (mode,    "Resonator",            0.f,   9.f,   0.f,  2, 0.f)    \
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
    X (dec,     "Decay",               20.f, 8000.f, 800.f, 0, 600.f)  \
    X (sus,     "Sustain",              0.f,   1.f,   1.f,  0, 0.f)    \
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
    X (m1s,    "Mod 1 Source",        0.f,  15.f,   1.f,  2, 0.f)    \
    X (m1d,    "Mod 1 Target",        0.f,  11.f,   0.f,  2, 0.f)    \
    X (m1a,    "Mod 1 Amount",       -1.f,   1.f,   0.5f, 0, 0.f)    \
    X (m2s,    "Mod 2 Source",        0.f,  15.f,   1.f,  2, 0.f)    \
    X (m2d,    "Mod 2 Target",        0.f,  11.f,   1.f,  2, 0.f)    \
    X (m2a,    "Mod 2 Amount",       -1.f,   1.f,   0.6f, 0, 0.f)    \
    X (m3s,    "Mod 3 Source",        0.f,  15.f,   3.f,  2, 0.f)    \
    X (m3d,    "Mod 3 Target",        0.f,  11.f,   3.f,  2, 0.f)    \
    X (m3a,    "Mod 3 Amount",       -1.f,   1.f,   0.0f, 0, 0.f)    \
    X (m4s,    "Mod 4 Source",        0.f,  15.f,   0.f,  2, 0.f)    \
    X (m4d,    "Mod 4 Target",        0.f,  11.f,   0.f,  2, 0.f)    \
    X (m4a,    "Mod 4 Amount",       -1.f,   1.f,   0.0f, 0, 0.f)    \
    X (m5s,    "Mod 5 Source",        0.f,  15.f,   0.f,  2, 0.f)    \
    X (m5d,    "Mod 5 Target",        0.f,  11.f,   0.f,  2, 0.f)    \
    X (m5a,    "Mod 5 Amount",       -1.f,   1.f,   0.0f, 0, 0.f)    \
    X (m6s,    "Mod 6 Source",        0.f,  15.f,   0.f,  2, 0.f)    \
    X (m6d,    "Mod 6 Target",        0.f,  11.f,   0.f,  2, 0.f)    \
    X (m6a,    "Mod 6 Amount",       -1.f,   1.f,   0.0f, 0, 0.f)    \
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
constexpr int kTargets = 12;           // FEEDBACK WIND FOCUS BRIGHT DRIVE BREATH PITCH FM
constexpr float kTapGain = 2.5f;
constexpr float kBowTune = 0.f, kReedTune = 0.f, kStrTune = 1.0262f, kRoomTune = 1.f;      // corrections d'accord (calibrees par mesure)

inline float fastTanh (float x)       // rationnelle, bornee a +-1
{
    x = std::max (-4.f, std::min (4.f, x));
    const float x2 = x * x;
    return x * (27.f + x2) / (27.f + 9.f * x2);
}

// ---------------------------------------------------------------- briques
struct Svf          // filtre a variables d'etat (TPT) : passe-bande normalise, gain 1 au centre
{
    float ic1 = 0.f, ic2 = 0.f, a1 = 0.f, a2 = 0.f, a3 = 0.f, k = 1.f, lo = 0.f, hi = 0.f;
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
        lo = v2; hi = v0 - k * v1 - v2;
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

// ---------------------------------------------------------------- plaque / cloche : rapports de modes (barre, cloche, plaque, bol) et voyelles (formants)
inline void modeRatios (float morph, float* r)
{
    static const float sets[4][8] {
        { 1.f, 2.76f, 5.40f, 8.93f, 13.34f, 18.64f, 24.8f, 32.0f },      // barre
        { 1.f, 2.00f, 2.40f, 3.00f, 4.00f,  5.00f,  6.40f, 8.00f },      // cloche
        { 1.f, 1.58f, 2.50f, 2.90f, 3.90f,  4.60f,  5.50f, 6.60f },      // plaque
        { 1.f, 2.71f, 5.15f, 8.43f, 12.5f,  17.6f,  23.4f, 30.0f } };    // bol chantant
    const float m = std::max (0.f, std::min (0.9999f, morph)) * 3.f; const int i = (int) m; const float f = m - (float) i;
    for (int k = 0; k < 8; ++k) r[k] = sets[i][k] * (1.f - f) + sets[i + 1][k] * f;
}
inline void vowelFormants (float pos, float* fr)       // pos 0..1 : A E I O U
{
    static const float t[5][3] { { 800.f, 1150.f, 2900.f }, { 400.f, 1700.f, 2600.f }, { 350.f, 2000.f, 2800.f }, { 450.f, 800.f, 2830.f }, { 325.f, 700.f, 2530.f } };
    const float m = std::max (0.f, std::min (0.9999f, pos)) * 4.f; const int i = (int) m; const float f = m - (float) i;
    for (int k = 0; k < 3; ++k) fr[k] = t[i][k] * (1.f - f) + t[i + 1][k] * f;
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
        for (auto& v : voices) { v = Voice(); v.dl.prepare ((int) (fs / 12.f) + 16); v.dl2.prepare ((int) (fs / 12.f) + 16); }
        reverb.prepare (fs); delay.prepare (fs);
        for (int i = 0; i < 64; ++i) dlFed[i] = dlFedNext[i] = 0.f;
        for (int c = 0; c < 16; ++c) chanP[c] = 0.f, bendCh[c] = 0.f;
        mw = 0.f; for (auto& c : ccVal) c = 0.f; for (auto& g : gS) g = 0.f; held = false; sustainPedal = false; pkS = 0.f; stamp = 0;
    }

    void setLearn (int a, int b) { learnA = a; learnB = b; }
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
        if (fresh) { v->dl.clear(); v->dl2.clear(); v->dcx = v->dcy = 0.f; v->envP = 0.f; v->gph = v->gs = v->chLp = 0.f; for (int k = 0; k < 8; ++k) v->my1[k] = v->my2[k] = 0.f; for (auto& q : v->fm3) q.reset(); v->bp.reset(); v->mm.reset(); v->lp = 0.f; v->env = 0.f; v->pos = 1.f; v->h = 1.f; v->q = note; v->posSemi = (float) note; v->pressS = 0.f; for (auto& q : v->slotS) q = 0.f; v->d = 2.f; v->dInit = false; }
        v->active = true; v->keyDown = true; v->deferred = false; v->releasing = false; v->dg = 1.f; v->atkDone = false;
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
        ccVal[num & 127] = val;
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
            {                                                                           // modulation des effets (globale : sources les plus fortes parmi les voix)
                float polyMax = 0.f, chanMax = 0.f, velMax = 0.f;
                for (auto& x : voices) if (x.active && (x.keyDown || x.deferred)) { polyMax = std::max (polyMax, x.polyP); velMax = std::max (velMax, x.vel); }
                for (int c = 0; c < 16; ++c) chanMax = std::max (chanMax, chanP[c]);
                for (int k = 0; k < kTargets; ++k) gm[k] = 0.f;
                const float cf = 1.f - std::exp (-(float) m / fs / 0.025f);
                for (int k = 0; k < 6; ++k)
                {
                    const int sr = (int) p[p_m1s + 3 * k], dst = (int) p[p_m1d + 3 * k];
                    float val = 0.f;
                    if (sr == 1) val = polyMax; else if (sr == 2) val = chanMax; else if (sr == 3) val = ccVal[1]; else if (sr == 4) val = ccVal[11];
                    else if (sr == 5) val = velMax; else if (sr >= 6 && sr <= 13) val = ccVal[16 + sr - 6];
                    else if (sr == 14) val = learnA >= 0 ? ccVal[learnA & 127] : 0.f; else if (sr == 15) val = learnB >= 0 ? ccVal[learnB & 127] : 0.f;
                    if (sr >= 1 && sr <= 2) val = std::pow (std::max (0.f, std::min (1.f, val)), p[p_pcurve]);
                    gS[k] += (val - gS[k]) * cf;
                    if (dst >= 9) gm[dst] += p[p_m1a + 3 * k] * gS[k];
                }
                auto cg = [] (float x) { return std::max (0.f, std::min (1.f, x)); };
                reverb.setParams (p[p_rvSize], p[p_rvDamp], cg (p[p_rvMix] + gm[11]));
                delay.setParams ((int) p[p_dlMode], p[p_dlTime], cg (p[p_dlFb] + gm[10]), p[p_dlMod], p[p_dlRate], p[p_dlTone]);
            }
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
            const int rt = (int) p[p_dlRoute]; const float dm = std::max (0.f, std::min (1.f, p[p_dlMix] + gm[9]));
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
        Svf mm; float morph = 0.5f, dg = 1.f, decC = 0.f; bool atkDone = false; float noiseAmp = 0.f, extAmp = 0.f, atkC = 0.001f, relC = 0.9999f;
        int64_t age = 0; int ctl = 0;
        Svf bp; DelayLine dl, dl2; Rng rng;
        float fbNow = 0.3f, envP = 0.f, wn = 0.f, strG = 0.99f, strPos = 0.2f, chG = 0.95f, gph = 0.f, gs = 0.f, chLp = 0.f; float mb[8] {}, mcf[8] {}, mr2[8] {}, my1[8] {}, my2[8] {}; Svf fm3[3]; float fmw[3] {};
        float outEnv = 1.f, beta = 0.3f, slope = 3.f, bowV = 0.1f, rSlope = -0.3f, rNoise = 0.f, rPress = 0.4f, dcx = 0.f, dcy = 0.f;
        float lpC = 0.5f, lpDelay = 0.f, drv = 1.f, tapNorm = 0.5f; float dStep = 0.f;
        float level = 0.f;
        float slotS[6] {}; float fmChM = 0.f; float fmT = 0.f, fmPhC = 0.f, fmPhM = 0.f, fmFbS = 0.f, fmGate = 0.f, fmAmt = 0.f, fmF = 440.f;
    };
    Voice voices[kVoices];
    Reverb reverb;
    DelayFx delay;
    float dlFed[64] {}, dlFedNext[64] {};
    Rng rng;
    float p[kNumP] {};
    float ccVal[128] {}; int learnA = -1, learnB = -1; float gS[6] {}, gm[kTargets] {};
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
        const int body = (int) p[p_mode];
        const bool pipe = ! (body == 1 || body >= 8), harmonic = p[p_lock] < 0.5f, mpe = p[p_mpe] > 0.5f;
        const float dt = (float) CT / fs;
        const int* mask = scaleMask ((int) p[p_scale]);
        const int root = (int) p[p_root];
        const float stickM = 0.45f * p[p_stick];
        const float maxD = (float) (v.dl.mask - 6);
        const float atkMs = p[p_atk], relMs = p[p_rel];
        v.atkC = 1.f - std::exp (-1.f / (std::max (3.f, atkMs) * 0.001f * fs * 0.45f));
        v.relC = std::exp (-6.9f / (std::max (20.f, relMs) * 0.001f * fs));
        v.decC = std::exp (-6.9f / (std::max (20.f, p[p_dec]) * 0.001f * fs));
        const float susL = p[p_sus];

        for (int i = 0; i < n; ++i)
        {
            if (v.ctl == 0)
            {
                // ---- matrice de modulation : chaque emplacement = une source, une cible, un dosage
                const float chp = mpe ? chanP[v.ch & 15] : *std::max_element (chanP, chanP + 16);
                const bool alive = v.keyDown || v.deferred;
                float m[kTargets] {};
                auto cl = [] (float x) { return std::max (0.f, std::min (1.f, x)); };
                for (int k = 0; k < 6; ++k)
                {
                    const int sr = (int) p[p_m1s + 3 * k], dst = (int) p[p_m1d + 3 * k];
                    if (sr == 0) { v.slotS[k] += (0.f - v.slotS[k]) * 0.5f; continue; }
                    float val = 0.f;
                    if (sr == 1) val = v.keyDown || v.deferred ? v.polyP : 0.f;
                    else if (sr == 2) val = v.keyDown ? chp : 0.f;
                    else if (sr == 3) val = ccVal[1];
                    else if (sr == 4) val = ccVal[11];
                    else if (sr == 5) val = v.vel;
                    else if (sr >= 6 && sr <= 13) val = ccVal[16 + sr - 6];
                    else if (sr == 14) val = learnA >= 0 ? ccVal[learnA & 127] : 0.f;
                    else if (sr == 15) val = learnB >= 0 ? ccVal[learnB & 127] : 0.f;
                    if (sr <= 2) val = std::pow (cl (val), p[p_pcurve]);
                    if (! alive && sr > 2) val = v.slotS[k];                              // une note qui s'eteint garde la modulation qu'elle avait
                    v.slotS[k] += (val - v.slotS[k]) * (1.f - std::exp (-dt / 0.025f));
                    m[std::max (0, std::min (kTargets - 1, dst))] += p[p_m1a + 3 * k] * v.slotS[k];
                }
                v.pressS = v.slotS[0];
                const float fbE = cl (p[p_fb] + m[0]), windE = cl (p[p_wind] + m[1]), focE = cl (p[p_focus] + m[2]);
                const float briE = cl (p[p_bright] + m[3]), drvE = cl (p[p_drive] + m[4]), brE = cl (p[p_breath] + m[5]);
                const float pitchMod = 2.f * m[6], fmE = cl (p[p_fm] + m[7]); v.fmChM = m[8];
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
                const float briL = (body == 9) ? 0.5f : briE;                                // MULTIMODE : BRIGHT sert a choisir le mode du filtre, pas la coupure de la boucle
                const float fcut = std::min (0.45f * fs, std::max (fBP, fLoop) * (1.1f + 14.f * briL * briL));
                const float a = std::exp (-2.f * kPi * fcut / fs);
                v.lpC = 1.f - a; v.lpDelay = a / (1.f - a);
                const float w0 = 2.f * kPi * fBP / fs;                                  // perte de l'amortissement a la frequence qui oscille :
                const float lpGain = (1.f - a) / std::sqrt (std::max (1e-6f, 1.f - 2.f * a * std::cos (w0) + a * a));   // compensee, donc gain 1.0 = seuil exact
                const float lpPh = std::atan2 (a * std::sin (w0), 1.f - a * std::cos (w0)) / std::max (1e-4f, w0);
                v.lpDelay = lpPh;
                float D = pipe ? fs / fLoop - 1.f - v.lpDelay : 1.f;
                if (body == 2) D = 0.5f * fs / fLoop * 1.0359f - 1.f - v.lpDelay + kBowTune;                // archet : une ligne avec inversion, retard = demi-periode
                else if (body == 4) D = fs / fLoop * kStrTune - 1.f - v.lpDelay;                       // corde pincee : boucle simple
                else if (body == 7) D = fs / fLoop - 1.f;                                              // choeur : la hauteur est imposee par la source glottale
                else if (body == 6)                                                                    // larsen de piece : la boucle est longue (k periodes) et la salle bouge
                {
                    const float P = fs / fLoop; const float k = std::max (1.f, std::min (40.f, std::floor (0.02f * fs / P)));
                    v.wn += (v.rng.next() - v.wn) * 0.01f;
                    D = k * P * kRoomTune - 1.f - v.lpDelay + 3.f * v.wn;
                }
                else if (body == 3) D = 0.5f * fs / fLoop * 1.0359f - 1.f - v.lpDelay + kReedTune;           // anche : demi-onde (tube cylindrique, harmoniques impaires)
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
                v.bp.set (fcBP, Q, fs); if (body == 9) v.mm.set (fBP, 0.9f, fs);
                const float comp = std::max (0.3f, lpGain * phiComp);
                float g = (0.88f + 0.30f * fbE) / comp;
                if (v.releasing) g = std::min (g, 0.9f / comp);
                v.gain = g;
                v.drv = 1.f + 7.f * drvE * drvE;
                v.tapNorm = 0.5f / std::sqrt (v.drv); v.morph = briE;
                v.noiseAmp = 0.12f * brE * brE * (0.3f + v.vel);
                v.extAmp = p[p_dlLoop] * 0.6f; v.fmAmt = fmE; v.fmF = std::max (20.f, std::min (0.4f * fs, fLoop));
                {
                    const float bowPress = 0.12f + 0.75f * fbE;
                    v.slope = (5.f - 4.f * bowPress) * (0.7f + 0.6f * focE) * (1.f + 1.5f * windE);
                    v.bowV = 0.04f + 0.26f * brE;
                    v.beta = 0.3f;
                    v.rPress = 0.48f + 0.52f * fbE;
                    v.rSlope = -0.3f - 0.025f * windE;
                    v.rNoise = 0.35f * brE * brE;
                    v.strG = 0.985f + 0.0155f * fbE;
                    v.strPos = 0.05f + 0.45f * (1.f - focE);
                    v.chG = 0.90f + 0.15f * fbE; v.fbNow = fbE;
                    if (body == 5)                                                              // PLAQUE : 8 modes
                    {
                        float rt[8]; modeRatios (windE, rt);
                        const float T0 = 0.15f + 6.f * fbE * fbE + 0.4f * fbE;
                        for (int k = 0; k < 8; ++k)
                        {
                            const float fk = fLoop * rt[k];
                            if (fk > 0.45f * fs) { v.mb[k] = 0.f; v.mcf[k] = 0.f; v.mr2[k] = 0.f; continue; }
                            const float tau = T0 / (1.f + (float) k * (0.6f + 2.f * (1.f - briE)));
                            const float r = std::exp (-1.f / (std::max (0.01f, tau) * fs));
                            v.mcf[k] = 2.f * r * std::cos (2.f * kPi * fk / fs); v.mr2[k] = r * r;
                            v.mb[k] = (1.f - r) * 12.f / (1.f + (float) k * (0.5f + 1.5f * (1.f - briE)));
                        }
                    }
                    if (body == 7)                                                              // CHOEUR : 3 formants (voyelle choisie avec WIND)
                    {
                        float fr[3]; vowelFormants (windE, fr);
                        const float Qf = 6.f + 10.f * focE;
                        for (int k = 0; k < 3; ++k) v.fm3[k].set (fr[k], Qf, fs);
                        v.fmw[0] = 1.f; v.fmw[1] = 0.8f; v.fmw[2] = 0.5f;
                    }
                }
                vPos[vi].store (cl (normPos));
            }
            v.ctl = (v.ctl + 1) % CT;
            v.d += v.dStep;
            // ---- boucle
            const float x = v.dl.read (v.d);
            float y = v.bp.bp (x);
            v.lp += (y - v.lp) * v.lpC; y = v.lp;
            float yd = fastTanh (v.drv * y);
            float yo = yd;                                                          // prise de sortie : le coeur de la boucle ne change pas (hauteur stable), seule la couleur change
            if (body == 8)                                                          // ECHELLE : passe-bas (pente raide) + saturation asymetrique chaude
            {
                v.chLp += (v.bp.k * v.bp.lo - v.chLp) * v.lpC;
                yo = 1.15f * (fastTanh (v.drv * v.chLp + 0.35f) - fastTanh (0.35f));
            }
            else if (body == 9)                                                     // MULTIMODE : BRIGHT balaie passe-bas > passe-bande > passe-haut
            {
                const float bw = v.mm.bp (x), mp = v.morph, lo = v.mm.lo, hi = v.mm.hi;     // filtre large sur le signal de la boucle : le souffle, le bruit et les harmoniques passent selon le mode
                const float t = mp < 0.5f ? lo + (bw - lo) * (mp * 2.f) : bw + (hi - bw) * ((mp - 0.5f) * 2.f);
                yo = fastTanh (v.drv * t);
            }
            const float s = yd / v.drv;
            float inj = v.rng.next () * (v.noiseAmp + 1e-6f) + v.kick * v.rng.next();
            v.kick *= 0.9985f;
            if (extL != nullptr) inj += v.extAmp * 0.5f * (extL[i] + (extR != nullptr ? extR[i] : extL[i]));
            // ---- FM : une part excite la boucle, une part sonne a cote
            float fmOut = 0.f;
            if (v.fmAmt > 1e-4f)
            {
                const FmSound& T = fmSound ((int) p[p_fmType]);
                const float fun = std::max (0.f, std::min (1.f, p[p_fmChar] + v.fmChM)), dec = p[p_fmDecay];
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
            float yb = 0.f;
            const bool newBody = body >= 2 && body != 6 && body < 8;
            if (newBody)
            {
                const float injB = 0.3f * inj;
                if (body == 2)                                                           // ARCHET : corde frottee (une ligne, le frottement colle / glisse a chaque aller-retour)
                {
                    v.lp += (v.dl.read (v.d) - v.lp) * v.lpC;
                    const float vs = -v.lp;
                    const float velDiff = v.bowV * v.env - vs;
                    const float xx = std::fabs (velDiff * v.slope) + 0.75f, x2 = xx * xx, tab = std::min (1.f, 1.f / (x2 * x2));
                    yb = vs * 4.f;
                    v.dl.write (0.998f * vs + velDiff * tab + injB);
                }
                else if (body == 4)                                                      // CORDE PINCEE (Karplus-Strong) + position du pincement
                {
                    const float x = v.dl.read (v.d);
                    v.lp += (x - v.lp) * v.lpC;
                    const float injS = 4.2f * std::max (0.4f, std::min (2.f, std::pow (261.f / v.fmF, 0.47f))) * inj;
                    v.dl2.write (injS);
                    const float injC = injS - v.dl2.read (std::max (2.f, v.strPos * v.d));
                    const float sl = fastTanh (v.drv * v.lp) / v.drv;
                    v.dl.write (v.strG * sl + injC);
                    yb = v.lp * 2.f;
                }
                else if (body == 5)                                                      // PLAQUE / CLOCHE / BOL : 8 resonateurs modaux
                {
                    const float exc = 4.f * 1.8f * std::max (0.5f, std::min (8.f, std::pow (v.fmF / 100.f, 0.75f))) * inj;
                    float sum = 0.f;
                    for (int k = 0; k < 8; ++k)
                    {
                        const float yv = v.mb[k] * exc + v.mcf[k] * v.my1[k] - v.mr2[k] * v.my2[k];
                        v.my2[k] = v.my1[k]; v.my1[k] = yv; sum += yv;
                    }
                    yb = sum;
                }
                else if (body == 7)                                                      // CHOEUR : source glottale -> boucle -> 3 formants
                {
                    v.gph += v.fmF / fs; bool wrap = false; if (v.gph >= 1.f) { v.gph -= 1.f; wrap = true; }
                    v.gs += ((wrap ? 1.f : 0.f) - v.gs) * 0.35f;
                    const float src = 7.f * v.gs * (0.5f + v.fbNow) + 0.5f * inj;
                    const float x = v.dl.read (v.d);
                    const float u = src + v.chG * x;
                    float y = 0.f;
                    for (int k = 0; k < 3; ++k) y += v.fmw[k] * v.fm3[k].bp (u);
                    v.chLp += (u - v.chLp) * 0.06f; y += 0.35f * v.chLp;
                    y = fastTanh (y);
                    v.dl.write (y);
                    yb = y * 1.6f * std::max (0.6f, std::min (3.f, std::sqrt (v.fmF / 261.f)));
                }
                else                                                                     // ANCHE (clarinette) : souffle -> anche -> tube
                {
                    const float dlOut = v.dl.read (v.d);
                    v.lp += (dlOut - v.lp) * v.lpC;
                    const float breath = v.rPress * v.env * (1.f + v.rNoise * v.rng.next());
                    const float pd = -0.95f * v.lp - breath;
                    const float rt = std::max (-1.f, std::min (1.f, 0.7f + v.rSlope * pd));
                    v.dl.write (breath + pd * rt + injB);
                    yb = dlOut;
                }
                const float dcy = yb - v.dcx + 0.995f * v.dcy; v.dcx = yb; v.dcy = dcy; yb = dcy;       // retire la composante continue
                yb = fastTanh (yb);
            }
            else v.dl.write (inj + v.gain * s);
            // ---- enveloppe
            if (v.releasing || (! v.keyDown && ! v.deferred)) v.env *= v.relC;
            else { v.env += (1.f - v.env) * v.atkC; if (v.env > 0.9f) v.atkDone = true; }
            if (v.atkDone && ! (v.releasing || (! v.keyDown && ! v.deferred))) v.dg = susL + (v.dg - susL) * v.decC;
            if (v.releasing || (! v.keyDown && ! v.deferred)) v.envP *= v.relC; else v.envP += (1.f - v.envP) * 0.01f;
            v.outEnv = (body == 4 || body == 5 || body == 7) ? v.envP : v.env;
            const float lv = v.level * 25.f;                                        // sous le seuil d'auto-oscillation (pas de pression) : on remonte le niveau, effet nul des que la note chante
            if (newBody)
            {
                const float tb = yb * (body == 2 ? 1.f : 0.8f) * v.outEnv * v.dg;
                out[i] += tb;
                v.level = std::max (v.level * 0.9995f, std::fabs (tb));
            }
            else
            {
                out[i] += yo * v.tapNorm * v.env * v.dg * kTapGain * (1.f + 8.f / (1.f + lv * lv));
                v.level = std::max (v.level * 0.9995f, std::fabs (yo * v.tapNorm * v.env));
            }
        }
        vLvl[vi].store (std::min (1.f, v.level * 3.f));
        if ((v.releasing || (! v.keyDown && ! v.deferred)) && v.env < 1e-4f && v.fmGate < 1e-4f) { v.active = false; v.level = 0.f; }
    }
};
} // namespace eo
