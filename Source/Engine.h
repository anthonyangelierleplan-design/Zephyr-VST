// ZEPHYR - moteur d'effets semi-modulaire (aucune dependance a JUCE, donc testable hors plugin)
// Modules : Stereo Wide, Saturation (tape / console / tube), EQ, Filtre, Phaser, Compresseur (opto / glue),
// Ensemble facon string machine, Delay multimode (tape / ice / ping-pong / digital), Reverb plaque + shimmer.
#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

namespace zephyr
{
constexpr float kPi = 3.14159265358979f;
constexpr int kSub = 32;                      // taille du bloc de controle (modulations)

// ------------------------------------------------------------------ topologie du patch
enum Node { NIn, NWide, NSat, NColor, NFilter, NPhaser, NComp, NEnsemble, NDelay, NVerb, NOut, kNumNodes };
constexpr int kFirstMod = NWide;
enum Src  { SrcLfo1, SrcLfo2, SrcSh, SrcEnv, kNumSrc };
enum Cv   { CvDrive, CvWidth, CvPhMix, CvHpf, CvLpf, CvPeak, CvPhRate, CvEnsDepth, CvEnsMix, CvMix, CvShIn, CvShClk, kNumCv };

constexpr int kMaxRack = 10;                   // nombre de modules audio posables sur le rack

// durees des divisions de la synchro (en noires) ; 0 = libre
inline float divBeats (int idx)
{
    static const float t[16] { 0.f, 16.f, 8.f, 4.f, 2.f, 1.f, 0.5f, 0.25f, 0.125f, 4.f / 3.f, 2.f / 3.f, 1.f / 3.f, 1.f / 6.f, 1.5f, 0.75f, 0.375f };
    return t[idx < 0 ? 0 : (idx > 15 ? 15 : idx)];
}

// ------------------------------------------------------------------ helpers
inline float clamp01 (float x)                { return std::min (1.f, std::max (0.f, x)); }
inline float lerp (float a, float b, float t) { return a + (b - a) * t; }
inline float dbToLin (float db)               { return std::pow (10.f, db * 0.05f); }
inline float linToDb (float x)                { return 20.f * std::log10 (std::max (x, 1.0e-9f)); }
inline float onePoleCoef (float fc, float fs) { return 1.f - std::exp (-2.f * kPi * fc / fs); }
inline float timeCoef (float sec, float fs)   { return 1.f - std::exp (-1.f / (std::max (sec, 1.0e-5f) * fs)); }

inline float safety (float x)                 // transparent sous 0.8, ecretage doux au-dessus
{
    const float a = std::fabs (x);
    if (a < 0.8f) return x;
    const float s = x < 0.f ? -1.f : 1.f;
    return s * (0.8f + 0.2f * std::tanh ((a - 0.8f) / 0.2f));
}

struct Rng
{
    uint32_t s = 0x9E3779B9u;
    void seed (uint32_t v) { s = v ? v : 0x9E3779B9u; }
    uint32_t nextU() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float bi()  { return (float) (nextU() & 0xFFFFFF) * (1.f / 8388608.f) - 1.f; }
    float uni() { return (float) (nextU() & 0xFFFFFF) * (1.f / 16777216.f); }
};

struct Buf { float ch[2][kSub]; };

// ligne a retard circulaire (taille puissance de 2), lecture fractionnaire
struct Line
{
    std::vector<float> b; int w = 0, mask = 0;
    void init (int n) { int sz = 16; while (sz < n + 8) sz <<= 1; b.assign ((size_t) sz, 0.f); mask = sz - 1; w = 0; }
    void push (float x) { b[(size_t) w] = x; w = (w + 1) & mask; }
    float tap (int d) const { return b[(size_t) ((w - d) & mask)]; }
    float tapF (float d) const
    {
        const int i = (int) d; const float fr = d - (float) i;
        const float a = b[(size_t) ((w - i) & mask)], c = b[(size_t) ((w - i - 1) & mask)];
        return a + (c - a) * fr;
    }
};

// ------------------------------------------------------------------ biquad (RBJ)
struct Biquad
{
    float b0 = 1.f, b1 = 0.f, b2 = 0.f, a1 = 0.f, a2 = 0.f;
    float z1[2] { 0.f, 0.f }, z2[2] { 0.f, 0.f };

    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0.f; }
    float process (float x, int c)
    {
        const float y = b0 * x + z1[c];
        z1[c] = b1 * x - a1 * y + z2[c];
        z2[c] = b2 * x - a2 * y;
        return y;
    }
    void setRaw (double B0, double B1, double B2, double A0, double A1, double A2)
    {
        b0 = (float) (B0 / A0); b1 = (float) (B1 / A0); b2 = (float) (B2 / A0);
        a1 = (float) (A1 / A0); a2 = (float) (A2 / A0);
    }
    void makeLowPass (double fs, double f, double q)
    {
        const double w = 2.0 * kPi * f / fs, al = std::sin (w) / (2.0 * q), cs = std::cos (w);
        setRaw ((1 - cs) / 2, 1 - cs, (1 - cs) / 2, 1 + al, -2 * cs, 1 - al);
    }
    void makePeak (double fs, double f, double q, double gainDb)
    {
        const double A = std::pow (10.0, gainDb / 40.0), w = 2.0 * kPi * f / fs;
        const double al = std::sin (w) / (2.0 * q), cs = std::cos (w);
        setRaw (1 + al * A, -2 * cs, 1 - al * A, 1 + al / A, -2 * cs, 1 - al / A);
    }
    void makeLowShelf (double fs, double f, double gainDb)
    {
        const double A = std::pow (10.0, gainDb / 40.0), w = 2.0 * kPi * f / fs;
        const double cs = std::cos (w), al = std::sin (w) / 2.0 * std::sqrt (2.0), t = 2.0 * std::sqrt (A) * al;
        setRaw (A * ((A + 1) - (A - 1) * cs + t), 2 * A * ((A - 1) - (A + 1) * cs), A * ((A + 1) - (A - 1) * cs - t),
                (A + 1) + (A - 1) * cs + t, -2 * ((A - 1) + (A + 1) * cs), (A + 1) + (A - 1) * cs - t);
    }
    void makeHighShelf (double fs, double f, double gainDb)
    {
        const double A = std::pow (10.0, gainDb / 40.0), w = 2.0 * kPi * f / fs;
        const double cs = std::cos (w), al = std::sin (w) / 2.0 * std::sqrt (2.0), t = 2.0 * std::sqrt (A) * al;
        setRaw (A * ((A + 1) + (A - 1) * cs + t), -2 * A * ((A - 1) + (A + 1) * cs), A * ((A + 1) + (A - 1) * cs - t),
                (A + 1) - (A - 1) * cs + t, 2 * ((A - 1) - (A + 1) * cs), (A + 1) - (A - 1) * cs - t);
    }
};

// ------------------------------------------------------------------ parametres
struct Params
{
    bool  on = true;
    float output = -6.f, mix = 1.f;
    float wideWidth = 0.f, wideSpread = 0.f, wideBass = 0.f;
    int   satMode = 0;  float satDrive = 0.f, satTone = 0.5f;
    float low = 0.f, mid = 0.f, high = 0.f;
    float hpf = 0.f, hpfPeak = 0.f, lpf = 1.f, lpfPeak = 0.f;
    float phMix = 0.f, phRate = 0.3f, phDepth = 0.6f, phFb = 0.3f;  int phStages = 1, phSync = 0;
    int   compType = 0;  float compAmt = 0.4f, compThresh = 0.35f, compAtt = 0.3f;
    float ensMix = 0.f, ensDepth = 0.6f, ensSpeed = 0.5f, ensTone = 0.6f;
    float lfo1Rate = 0.35f, lfo1Shape = 0.f,   lfo1Level = 0.5f;
    float lfo2Rate = 0.55f, lfo2Shape = 0.34f, lfo2Level = 0.5f;
    float shRate = 0.45f, shSlew = 0.15f, shLevel = 0.5f;
    float envRel = 0.4f, envLevel = 0.6f;
    int   lfo1Sync = 0, lfo2Sync = 0, shSync = 0;                  // 0 = libre, sinon index de division (divBeats)
    int   dlyMode = 0;  float dlyTime = 0.4f, dlyFb = 0.35f, dlyTone = 0.6f, dlyMod = 0.3f, dlyMix = 0.f;  int dlySync = 0;
    float revMix = 0.f, revSize = 0.5f, revDecay = 0.6f, revPre = 0.1f, revDamp = 0.4f, revMod = 0.3f, revWidth = 1.f, revShimmer = 0.f, revLowCut = 0.1f;  bool revFreeze = false;
};

// ------------------------------------------------------------------ sur-echantillonnage x4 (IIR Butterworth ordre 6)
struct Oversampler4
{
    Biquad up1[3], up2[3], dn1[3], dn2[3];

    static float filt (Biquad* f, float x, int c) { return f[2].process (f[1].process (f[0].process (x, c), c), c); }

    void prepare (float fs)
    {
        static const double Q[3] { 0.5176, 0.7071, 1.9319 };
        for (int i = 0; i < 3; ++i)
        {
            up1[i].makeLowPass (fs * 2.0, fs * 0.45, Q[i]);  dn1[i] = up1[i];
            up2[i].makeLowPass (fs * 4.0, fs * 0.90, Q[i]);  dn2[i] = up2[i];
            up1[i].reset(); up2[i].reset(); dn1[i].reset(); dn2[i].reset();
        }
    }
    void up (float x, int c, float* o4)
    {
        const float a[2] { filt (up1, x * 2.f, c), filt (up1, 0.f, c) };
        for (int j = 0; j < 2; ++j) { o4[2 * j] = filt (up2, a[j] * 2.f, c); o4[2 * j + 1] = filt (up2, 0.f, c); }
    }
    float down (const float* i4, int c)
    {
        float h[2];
        for (int j = 0; j < 2; ++j) { filt (dn2, i4[2 * j], c); h[j] = filt (dn2, i4[2 * j + 1], c); }
        filt (dn1, h[0], c);
        return filt (dn1, h[1], c);
    }
};

// ------------------------------------------------------------------ filtre du rack : SVF 2 poles, resonance douce qui chante
struct Svf
{
    float ic1[2] { 0, 0 }, ic2[2] { 0, 0 }, a1 = 0, a2 = 0, a3 = 0, k = 1.4f;

    void set (float fc, float res, float fs)
    {
        const float g = std::tan (kPi * std::min (fc, 0.45f * fs) / fs);
        k = 1.4f * std::pow (1.f - 0.92f * res, 1.5f) + 0.04f;
        a1 = 1.f / (1.f + g * (g + k)); a2 = g * a1; a3 = g * a2;
    }
    void tick (float x, int c, float& lp, float& hp)
    {
        const float v3 = x - ic2[c];
        const float v1 = a1 * ic1[c] + a2 * v3;
        const float v2 = ic2[c] + a2 * ic1[c] + a3 * v3;
        ic1[c] = 2.f * std::tanh ((2.f * v1 - ic1[c]) / 2.f);
        ic2[c] = 2.f * v2 - ic2[c];
        lp = v2;
        hp = x - k * v1 - v2;
    }
    void reset() { ic1[0] = ic1[1] = ic2[0] = ic2[1] = 0.f; }
};

// ------------------------------------------------------------------ STEREO WIDE : largeur mid/side + diffusion (cree de la stereo a partir d'un son mono) + graves en mono
struct Wider
{
    struct Ap { Line d; int len = 100; float g = 0.6f; float tick (float x) { const float z = d.tap (len); const float v = x - g * z; d.push (v); return z + g * v; } };
    Ap a[3], b[3];
    float hp1 = 0.f, hp2 = 0.f, fs = 44100.f;
    void prepare (float sr)
    {
        fs = sr;
        static const int la[3] { 142, 379, 613 }, lb[3] { 191, 283, 557 };
        const float k = fs / 44100.f;
        for (int i = 0; i < 3; ++i)
        {
            a[i].len = std::max (2, (int) ((float) la[i] * k)); a[i].d.init (a[i].len + 4);
            b[i].len = std::max (2, (int) ((float) lb[i] * k)); b[i].d.init (b[i].len + 4);
        }
        hp1 = hp2 = 0.f;
    }
    // width, spread, bassMono : 0..1 (tout a zero = transparent)
    void process (float& l, float& r, float width, float spread, float bassMono)
    {
        const float m = 0.5f * (l + r);
        float s = 0.5f * (l - r) * (1.f + 1.6f * width);
        if (spread > 0.0005f)
        {
            float da = m, db = m;
            for (auto& x : a) da = x.tick (da);
            for (auto& x : b) db = x.tick (db);
            s += spread * 0.75f * (da - db);
        }
        if (bassMono > 0.0005f)
        {
            const float c = onePoleCoef (40.f + 260.f * bassMono, fs);
            hp1 += c * (s - hp1); s -= hp1;
            hp2 += c * (s - hp2); s -= hp2;
        }
        const float g = 1.f / (1.f + 0.22f * width + 0.15f * spread);
        l = (m + s) * g; r = (m - s) * g;
        if (width < 0.0005f && spread < 0.0005f) { l = m + s; r = m - s; }   // neutre : niveau identique
    }
};

// ------------------------------------------------------------------ saturation chaude (mode 0 bande, 1 console, 2 lampe)
inline float tubeStage (float x) { return x >= 0.f ? std::tanh (x) : -0.95f * (1.f - std::exp (1.3f * x)); }
inline float satShape (int mode, float x)
{
    switch (mode)
    {
        case 1:  return std::tanh (x + 0.18f) - std::tanh (0.18f);                    // console : un peu d'harmoniques paires
        case 2:  return tubeStage (x);                                                 // lampe : asymetrique, rond
        default: return x / std::sqrt (1.f + 0.85f * x * x);                           // bande : compression douce, symetrique
    }
}

// ------------------------------------------------------------------ phaser : 4 a 12 etages passe-tout, LFO stereo decale, reinjection
struct Phaser
{
    static constexpr int kMaxSt = 12;
    float z[2][kMaxSt] {}, last[2] {}, ph = 0.f, coef[2] {};
    void reset() { for (auto& c : z) for (auto& v : c) v = 0.f; last[0] = last[1] = 0.f; ph = 0.f; }
};

// ------------------------------------------------------------------ ensemble facon string machine : 3 lignes BBD, chorus lent + vibrato rapide dephases de 120 degres
struct Ensemble
{
    Line line; float fs = 44100.f, phC = 0.f, phV = 0.f, lpL = 0.f, lpR = 0.f;
    void prepare (float sr) { fs = sr; line.init ((int) (0.04f * fs)); phC = phV = 0.f; lpL = lpR = 0.f; }
    void tick (float in, float depth, float speed, float tone, float& wl, float& wr)
    {
        const float mult = 0.5f * std::pow (4.f, speed);
        phC += 0.63f * mult / fs; if (phC >= 1.f) phC -= 1.f;
        phV += 6.3f * mult / fs;  if (phV >= 1.f) phV -= 1.f;
        line.push (in);
        float t[3];
        for (int i = 0; i < 3; ++i)
        {
            const float o = (float) i / 3.f;
            const float ms = 6.5f + depth * (2.4f * std::sin (6.2831853f * (phC + o)) + 0.32f * std::sin (6.2831853f * (phV + o)));
            t[i] = line.tapF (std::max (1.f, ms * 0.001f * fs));
        }
        const float a = onePoleCoef (3500.f + 9000.f * tone, fs);
        const float l = (t[0] + 0.55f * t[1]) * 0.75f, r = (t[2] + 0.55f * t[1]) * 0.75f;
        lpL += a * (l - lpL); lpR += a * (r - lpR);
        wl = lpL; wr = lpR;
    }
};

// ------------------------------------------------------------------ compresseur (1 opto doux, 2 glue de bus) + gain auto
struct Comp
{
    int type = 0;
    float fs = 44100.f, env = 0.f, grDb = 0.f;
    float aC = 0, rFast = 0, rSlow = 0, thr = -3.f, knee = 8.f, rmax = 4.f, makeup = 1.f, warm = 0.f;

    void prepare (float sr) { fs = sr; env = 0.f; grDb = 0.f; }
    float grCurve (float over) const
    {
        if (over <= -knee * 0.5f) return 0.f;
        const float x = over < knee * 0.5f ? (over + knee * 0.5f) * (over + knee * 0.5f) / (2.f * knee) : over;    // genou doux
        return x * (1.f - 1.f / rmax);
    }
    void set (int t, float a, float th, float att)
    {
        type = t;
        if (t == 1) { aC = timeCoef (0.003f + 0.04f * att, fs); rFast = timeCoef (0.06f, fs); rSlow = timeCoef (1.4f, fs); knee = 12.f; rmax = 2.f + 8.f * a; warm = 0.06f; }
        else        { aC = timeCoef (0.0001f * std::pow (300.f, att), fs); rFast = timeCoef (0.1f, fs); rSlow = timeCoef (0.6f, fs); knee = 5.f; rmax = 1.5f + 4.5f * a; warm = 0.f; }
        thr = -2.f - 34.f * th;                                              // seuil : -2 dB ... -36 dB
        makeup = dbToLin (0.75f * grCurve (-8.f - thr));
    }
    void process (float& l, float& r)
    {
        if (type == 0) return;
        const float det = std::max (std::fabs (l), std::fabs (r));
        const float rel = lerp (rFast, rSlow, clamp01 (grDb / 8.f));       // plus il compresse, plus il relache lentement (opto)
        env += (det > env ? aC : rel) * (det - env);
        grDb = grCurve (linToDb (env) - thr);
        const float g = dbToLin (-grDb) * makeup;
        l *= g; r *= g;
        if (warm > 0.f) { l = std::tanh (l * (1.f + warm)) / (1.f + warm); r = std::tanh (r * (1.f + warm)) / (1.f + warm); }
    }
};

// ------------------------------------------------------------------ modulations du rack
struct Lfo
{
    float ph = 0.f;
    static float wave (float p, float shape)
    {
        const float w[4] { std::sin (2.f * kPi * p), 4.f * std::fabs (p - 0.5f) - 1.f, 2.f * p - 1.f, p < 0.5f ? 1.f : -1.f };
        const float s = clamp01 (shape) * 3.f;
        const int i = std::min (2, (int) s);
        return lerp (w[i], w[i + 1], s - (float) i);
    }
    float tick (float hz, float shape, int n, float fs)
    {
        ph += hz * (float) n / fs; ph -= std::floor (ph);
        return wave (ph, shape);
    }
    float tickSync (double ppq, float beats, float shape)          // phase verrouillee sur la position dans la mesure
    {
        const double c = ppq / (double) beats;
        ph = (float) (c - std::floor (c));
        return wave (ph, shape);
    }
};

struct SampleHold
{
    float ph = 0.f, held = 0.f, out = 0.f; bool clkHigh = false;
    // mode : 0 horloge interne, 1 entree CLK, 2 synchro tempo (trigSync)
    float tick (float hz, int mode, bool trigSync, float clk, float in, float slew, int n, float fs)
    {
        bool trig = false;
        if (mode == 1)      { trig = clk > 0.2f && ! clkHigh; clkHigh = clk > 0.2f; }
        else if (mode == 2) trig = trigSync;
        else { ph += hz * (float) n / fs; if (ph >= 1.f) { ph -= std::floor (ph); trig = true; } }
        if (trig) held = in;
        const float tau = slew * slew * 0.4f;
        const float k = tau < 0.0005f ? 1.f : 1.f - std::exp (-((float) n / fs) / tau);
        out += k * (held - out);
        return out;
    }
};

// ------------------------------------------------------------------ delay multimode : 0 TAPE (pleurage, saturation, s'assombrit), 1 ICE (chaque repetition
// monte d'une octave et se diffuse : cristallin), 2 PING-PONG, 3 DIGITAL (propre, LP par TONE, HP par MOD)
struct MultiDelay
{
    std::vector<float> buf[2];
    int w = 0, mask = 0; float fs = 44100.f, dly = 0.f, lpFb[2] {}, hpFb[2] {}, lpIn[2] {}, wowPh = 0.f, flPh = 0.f, drift = 0.f;
    Line shL[2]; float shPh[2] {}, shLp[2] {};
    struct Ap { Line d; int len = 100; float tick (float x, float g) { const float z = d.tap (len); const float v = x - g * z; d.push (v); return z + g * v; } };
    Ap diff[2][2];
    Rng rng;
    void prepare (float sr)
    {
        fs = sr;
        int sz = 1; while (sz < (int) (2.3f * fs)) sz <<= 1;
        for (auto& b : buf) b.assign ((size_t) sz, 0.f);
        mask = sz - 1; w = 0; dly = 0.f; wowPh = flPh = drift = 0.f;
        for (int c = 0; c < 2; ++c)
        {
            lpFb[c] = hpFb[c] = lpIn[c] = 0.f; shPh[c] = c * 0.37f; shLp[c] = 0.f; shL[c].init ((int) (1800.f * std::max (1.f, fs / 44100.f)) + 16);
            static const int dl[2][2] { { 113, 337 }, { 149, 293 } };
            for (int k = 0; k < 2; ++k) { diff[c][k].len = std::max (2, (int) ((float) dl[c][k] * fs / 44100.f)); diff[c][k].d.init (diff[c][k].len + 4); }
        }
        rng.seed (0xD31A7u);
    }
    float read (int c, float d) const
    {
        const float rp = (float) w - d;
        int i0 = (int) std::floor (rp); const float fr = rp - (float) i0;
        const float a = buf[c][(size_t) (i0 & mask)], b = buf[c][(size_t) ((i0 + 1) & mask)];
        return a + (b - a) * fr;
    }
    float octaveUp (int c, float x)                                     // +12 demi-tons : deux tetes de lecture a fenetre de Hann
    {
        const float W = 1800.f * std::max (1.f, fs / 44100.f);
        shL[c].push (x);
        shPh[c] -= 1.f / W; if (shPh[c] < 0.f) shPh[c] += 1.f;
        const float p2 = shPh[c] >= 0.5f ? shPh[c] - 0.5f : shPh[c] + 0.5f;
        const float w1 = std::sin (kPi * shPh[c]), w2 = std::sin (kPi * p2);
        return shL[c].tapF (W * shPh[c] + 1.f) * w1 * w1 + shL[c].tapF (W * p2 + 1.f) * w2 * w2;
    }
    void process (const float* inL, const float* inR, float* outL, float* outR, int n, int nc,
                  int mode, float timeSec, float fb, float tone, float mod, float mix)
    {
        const float target = std::max (8.f, std::min ((float) mask - 8.f, timeSec * fs));
        if (dly <= 0.f) dly = target;
        const float glide = mode == 0 ? 0.0004f : 0.004f;                                    // la bande glisse (les changements de temps chantent)
        const float lpHz = mode == 0 ? 1800.f + 9000.f * tone : (mode == 1 ? 4000.f + 14000.f * tone : 1500.f + 18000.f * tone * tone);
        const float aLp = onePoleCoef (std::min (lpHz, 0.45f * fs), fs);
        const float aHp = onePoleCoef (mode == 3 ? 20.f + 900.f * mod * mod : 70.f, fs);
        const float g = mode == 0 ? fb * 1.08f : fb;
        const float wowInc = 0.45f / fs, flInc = 5.7f / fs;
        for (int i = 0; i < n; ++i)
        {
            dly += (target - dly) * glide;
            float dm = dly;
            if (mode == 0)
            {
                wowPh += wowInc; if (wowPh >= 1.f) wowPh -= 1.f;
                flPh += flInc;   if (flPh >= 1.f) flPh -= 1.f;
                drift += (rng.bi() * 0.5f - drift) * 0.0004f;
                dm *= 1.f + mod * (0.0045f * std::sin (6.2831853f * wowPh) + 0.0009f * std::sin (6.2831853f * flPh) + 0.002f * drift);
            }
            else if (mode == 1)
            {
                wowPh += 0.3f / fs; if (wowPh >= 1.f) wowPh -= 1.f;
                dm *= 1.f + 0.0012f * std::sin (6.2831853f * wowPh);
            }
            float x[2] { inL[i], nc > 1 ? inR[i] : inL[i] }, y[2];
            for (int c = 0; c < 2; ++c) y[c] = read (c, std::max (2.f, dm + (mode == 1 && c == 1 ? 0.011f * fs : 0.f)));
            const float mono = 0.5f * (x[0] + x[1]);
            for (int c = 0; c < 2; ++c)
            {
                float f = mode == 2 ? y[1 - c] : y[c];                                           // ping-pong : chaque cote nourrit l'autre
                lpFb[c] += aLp * (f - lpFb[c]); f = lpFb[c];
                hpFb[c] += aHp * (f - hpFb[c]); f -= hpFb[c];
                if (mode == 0) f = std::tanh (1.4f * f * g) / 1.4f;                               // bande : la boucle sature et se compresse
                else if (mode == 1)
                {
                    const float up = octaveUp (c, f);
                    shLp[c] += onePoleCoef (9000.f, fs) * (up - shLp[c]);
                    f = 0.45f * f + 0.75f * shLp[c];
                    const float dg = 0.2f + 0.5f * mod;
                    f = diff[c][1].tick (diff[c][0].tick (f, dg), dg);
                    f = std::tanh (f * g * 1.1f) / 1.1f;
                }
                else f = std::tanh (f * g * 1.05f) / 1.05f;
                float in = mode == 2 ? (c == 0 ? mono : 0.f) : x[c];
                if (mode == 0) { lpIn[c] += onePoleCoef (12000.f, fs) * (in - lpIn[c]); in = lpIn[c]; }
                buf[c][(size_t) w] = std::max (-2.f, std::min (2.f, in + f));
            }
            w = (w + 1) & mask;
            outL[i] = x[0] + mix * y[0];
            if (nc > 1) outR[i] = x[1] + mix * y[1];
        }
    }
};

// ------------------------------------------------------------------ reverb plaque (Dattorro) + shimmer + freeze
struct Reverb
{
    using Dl = Line;
    float fs = 44100.f, r = 1.f;
    Dl pre, in1, in2, in3, in4, mAL, dL1, aL2, dL2, mAR, dR1, aR2, dR2, shBuf;
    float bwLp = 0.f, hpS = 0.f, dampL = 0.f, dampR = 0.f, outL = 0.f, outR = 0.f, shLp = 0.f;
    float lfoPh = 0.f, sizeS = 1.f, freezeS = 0.f, shPh = 0.f, shAmt = 0.f, tankL = 0.f, tankR = 0.f, dcL = 0.f, dcR = 0.f;
    void prepare (float sampleRate)
    {
        fs = sampleRate; r = fs / 29761.f;
        auto mx = [this] (int n) { return (int) ((float) n * r * 1.7f) + 64; };
        pre.init ((int) (0.25f * fs));
        in1.init (mx (142)); in2.init (mx (107)); in3.init (mx (379)); in4.init (mx (277));
        mAL.init (mx (672 + 40)); dL1.init (mx (4453)); aL2.init (mx (1800)); dL2.init (mx (3720));
        mAR.init (mx (908 + 40)); dR1.init (mx (4217)); aR2.init (mx (2656)); dR2.init (mx (3163));
        shBuf.init (8192);
        bwLp = hpS = dampL = dampR = outL = outR = shLp = 0.f; lfoPh = 0.f; sizeS = 1.f; freezeS = 0.f; shPh = 0.f; shAmt = 0.f;
        tankL = tankR = dcL = dcR = 0.f;
    }
    static float ap (Dl& d, float len, float g, float x)           // allpass de Schroeder (longueur fractionnaire)
    {
        const float z = d.tapF (len);
        const float v = x - g * z;
        d.push (v);
        return z + g * v;
    }
    void process (const float* inL, const float* inR, float* oL, float* oR, int n, int nc,
                  float mix, float size, float decay, float preT, float damp, float mod, float width, float shim, float lowcut, bool freeze)
    {
        const float sizeT = 0.55f + 1.0f * size;
        const float rt = 0.45f * std::pow (80.f, decay) * (1.f + 0.6f * decay);
        const float loopSec = 2.f * (10645.f / 29761.f) * sizeT;
        const float gDecay = std::min (0.9985f, std::pow (10.f, -1.5f * loopSec / rt));
        const float dampHz = 16000.f * std::pow (1.f - damp, 2.2f) + 1200.f;
        const float aDamp = onePoleCoef (dampHz, fs), aBw = onePoleCoef (13000.f, fs);
        const float aHp = onePoleCoef (20.f * std::pow (25.f, lowcut), fs);
        const float aShLp = onePoleCoef (6500.f, fs);
        const float preS = std::min (0.2f, 0.0f + 0.2f * preT * preT) * fs;
        const float dif1 = 0.75f, dif2 = 0.625f, dd1 = 0.7f, dd2 = std::max (0.25f, std::min (0.5f, gDecay + 0.15f));
        const float dryA = std::cos (mix * 0.5f * kPi), wetA = std::sin (mix * 0.5f * kPi);
        const float excur = (1.f + 14.f * mod) * r;
        const float lfoInc = 0.8f / fs;
        const float inject = shim * 0.9f * std::sqrt (std::max (0.f, 1.f - gDecay * gDecay));
        const float W = 2048.f * std::max (1.f, fs / 44100.f);
        for (int i = 0; i < n; ++i)
        {
            sizeS += (sizeT - sizeS) * 0.0003f;
            freezeS += ((freeze ? 1.f : 0.f) - freezeS) * 0.002f;
            shAmt += (inject - shAmt) * 0.002f;
            const float gT = gDecay + (0.99985f - gDecay) * freezeS;
            const float aDm = aDamp + (onePoleCoef (16000.f, fs) - aDamp) * freezeS;
            const float x = 0.5f * (inL[i] + (nc > 1 ? inR[i] : inL[i]));
            float shifted = 0.f;
            if (shAmt > 1e-4f)
            {
                shBuf.push (tankL + tankR);
                shPh -= 1.f / W; if (shPh < 0.f) shPh += 1.f;
                const float p2 = shPh >= 0.5f ? shPh - 0.5f : shPh + 0.5f;
                const float w1 = std::sin (kPi * shPh), w2 = std::sin (kPi * p2);
                shifted = shBuf.tapF (W * shPh + 1.f) * w1 * w1 + shBuf.tapF (W * p2 + 1.f) * w2 * w2;
                shLp += aShLp * (shifted - shLp); shifted = shLp;
            }
            else shBuf.push (0.f);
            pre.push ((x * (1.f - freezeS)) + std::tanh (shifted) * shAmt * (1.f - freezeS));
            float v = pre.tapF (std::max (1.f, preS));
            hpS += aHp * (v - hpS); v -= hpS;
            bwLp += aBw * (v - bwLp); v = bwLp;
            v = ap (in1, 142.f * r, dif1, v);
            v = ap (in2, 107.f * r, dif1, v);
            v = ap (in3, 379.f * r, dif2, v);
            v = ap (in4, 277.f * r, dif2, v);
            lfoPh += lfoInc; if (lfoPh >= 1.f) lfoPh -= 1.f;
            const float mL = std::sin (6.2831853f * lfoPh), mR = std::sin (6.2831853f * lfoPh + 1.5707963f);
            const float sc = sizeS * r;
            const float lIn = v + gT * tankR, rIn = v + gT * tankL;
            float a = ap (mAL, 672.f * sc + excur * mL, -dd1, lIn);
            dL1.push (a);
            float t = dL1.tapF (4453.f * sc);
            dampL += aDm * (t - dampL); t = dampL * gT;
            t = ap (aL2, 1800.f * sc, dd2, t);
            dL2.push (t);
            const float nL = dL2.tapF (3720.f * sc);
            a = ap (mAR, 908.f * sc + excur * mR, -dd1, rIn);
            dR1.push (a);
            t = dR1.tapF (4217.f * sc);
            dampR += aDm * (t - dampR); t = dampR * gT;
            t = ap (aR2, 2656.f * sc, dd2, t);
            dR2.push (t);
            const float nR = dR2.tapF (3163.f * sc);
            tankL = nL; tankR = nR;
            const float yl = dR1.tapF (266.f * sc) + dR1.tapF (2974.f * sc) - aR2.tapF (1913.f * sc) + dR2.tapF (1996.f * sc)
                           - dL1.tapF (1990.f * sc) - aL2.tapF (187.f * sc) - dL2.tapF (1066.f * sc);
            const float yr = dL1.tapF (353.f * sc) + dL1.tapF (3627.f * sc) - aL2.tapF (1228.f * sc) + dL2.tapF (2673.f * sc)
                           - dR1.tapF (2111.f * sc) + dR2.tapF (335.f * sc) - aR2.tapF (121.f * sc);
            const float m = 0.5f * (yl + yr), sd = 0.5f * (yl - yr) * (width * 1.5f);
            float wl = (m + sd) * 0.5f, wr = (m - sd) * 0.5f;
            dcL += 0.0005f * (wl - dcL); wl -= dcL; dcR += 0.0005f * (wr - dcR); wr -= dcR;
            const float l0 = inL[i], r0 = nc > 1 ? inR[i] : inL[i];
            oL[i] = l0 * dryA + wl * wetA;
            if (nc > 1) oR[i] = r0 * dryA + wr * wetA;
        }
    }
};

// ================================================================== moteur
class Engine
{
public:
    std::atomic<float> glow { 0.f }, bright { 0.f };            // le prisme de l'en-tete : niveau de sortie et brillance (part des aigus)
    std::atomic<float> compGr { 0.f };                          // vumeter du comp (dB de reduction)
    std::atomic<float> modLed[kNumSrc];
    static constexpr int kExt = 17;                              // 10 entrees CV + phaser feedback + delay temps / feedback / mix + reverb mix / decay / damp
    std::atomic<uint32_t> audioIn[kNumNodes], cvIn[kNumCv];     // le patch : masques de bits des sources branchees

    std::atomic<int> rackSlot[kMaxRack];                         // ordre des modules sur le rack (-1 = vide)

    Engine()
    {
        for (auto& a : audioIn) a.store (0);
        for (auto& c : cvIn) c.store (0);
        for (auto& m : modLed) m.store (0.f);
        setRack ({});                                   // au lancement : rack vide (INPUT -> OUTPUT direct)
    }

    // ---- rack
    void setDefaultRack() { setRack ({ NSat, NColor, NFilter, NComp, NPhaser, NEnsemble, NDelay, NVerb, NWide }); }
    void setRack (const std::vector<int>& order)
    {
        int k = 0;
        bool used[kNumNodes] {};
        for (int v : order)
            if (k < kMaxRack && v >= kFirstMod && v < NOut && ! used[v]) { used[v] = true; rackSlot[k++].store (v); }
        for (; k < kMaxRack; ++k) rackSlot[k].store (-1);
    }
    std::vector<int> getRack() const
    {
        std::vector<int> r;
        for (auto& sl : rackSlot) { const int v = sl.load(); if (v >= kFirstMod && v < NOut) r.push_back (v); }
        return r;
    }
    bool inRack (int node) const { for (auto& sl : rackSlot) if (sl.load() == node) return true; return false; }
    void addToRack (int node)    { if (! inRack (node)) { auto r = getRack(); r.push_back (node); setRack (r); } }
    void removeFromRack (int node)
    {
        auto r = getRack();
        r.erase (std::remove (r.begin(), r.end(), node), r.end());
        setRack (r);
    }
    std::string serializeRack() const
    {
        std::string str;
        for (int v : getRack()) str += std::to_string (v) + ",";
        return str;
    }
    void loadRack (const std::string& str)
    {
        std::vector<int> r;
        const char* q = str.c_str();
        while (*q)
        {
            char* end = nullptr;
            const long v = std::strtol (q, &end, 10);
            if (end == q) break;
            r.push_back ((int) v);
            q = (*end == ',') ? end + 1 : end;
        }
        setRack (r);
    }

    // ---- transport de l'hote (appele a chaque bloc)
    void setHost (double bpm, double ppq, bool playing, bool hasPpq)
    {
        hostBpm = bpm > 1.0 ? bpm : 120.0; hostPpq = ppq; hostPlaying = playing; hostHasPpq = hasPpq;
    }

    // ---- patch
    void setAudioCable (int src, int dst, bool on)
    {
        if (src < 0 || src >= NOut || dst < kFirstMod || dst >= kNumNodes) return;
        if (on) audioIn[dst].fetch_or (1u << src); else audioIn[dst].fetch_and (~(1u << src));
    }
    void setCvCable (int src, int dst, bool on)
    {
        if (src < 0 || src >= kNumSrc || dst < 0 || dst >= kNumCv) return;
        if (on) cvIn[dst].fetch_or (1u << src); else cvIn[dst].fetch_and (~(1u << src));
    }
    void clearPatch() { for (auto& a : audioIn) a.store (0); for (auto& c : cvIn) c.store (0); }
    std::string serializePatch() const
    {
        std::string s = "z1:";
        for (auto& a : audioIn) s += std::to_string (a.load()) + ",";
        for (auto& c : cvIn)    s += std::to_string (c.load()) + ",";
        return s;
    }
    void loadPatch (const std::string& s)
    {
        clearPatch();
        if (s.compare (0, 3, "z1:") != 0) return;
        const char* p = s.c_str() + 3;
        for (int i = 0; i < (int) kNumNodes + (int) kNumCv; ++i)
        {
            char* end = nullptr;
            const unsigned long v = std::strtoul (p, &end, 10);
            if (end == p) break;
            p = (*end == ',') ? end + 1 : end;
            if (i < kNumNodes) audioIn[i].store ((uint32_t) v & 0x7FFu);
            else               cvIn[i - kNumNodes].store ((uint32_t) v & 0xFu);
        }
    }

    void prepare (float sampleRate)
    {
        fs = sampleRate;
        os.prepare (fs); comp.prepare (fs); wider.prepare (fs); ens.prepare (fs); dly.prepare (fs); verb.prepare (fs); phaser.reset();
        for (auto& e : extS) e = 0.f;
        dcC = onePoleCoef (12.f, fs);
        smoothC = timeCoef (0.02f, fs);
        for (int c = 0; c < 2; ++c) { dcLp[c] = 0.f; tiltLp[c] = 0.f; tapeLp[c] = 0.f; }
        lowB.reset(); midB.reset(); hiB.reset(); hpfSvf.reset(); lpfSvf.reset();
        lowDbLast = midDbLast = hiDbLast = -999.f;
        for (auto& b : out) for (auto& ch : b.ch) for (auto& v : ch) v = 0.f;
        mixCur = outCur = -1.f;
        lfo1 = Lfo(); lfo2 = Lfo(); sh = SampleHold(); envV = 0.f; ppqNow = 0.0; shLastIdx = -1; grPeak = 0.f;
        eSum = hSum = 0.f; eN = 0; hfLp = 0.f; glowS = brightS = 0.f;
        glow.store (0.f); bright.store (0.f);
        shRng.seed (0x5EEDu);
    }

    void setParams (const Params& in) { p = in; }
    void setExtMod (const float* v) { for (int i = 0; i < kExt; ++i) extT[i] = v[i]; }      // modulations venues de la matrice du synthe (kExt valeurs)

    void process (float** io, int n, int nc)
    {
        if (hostPlaying && hostHasPpq) ppqNow = hostPpq;           // en lecture : on se cale sur la position d'Ableton
        for (int pos = 0; pos < n; pos += kSub)
        {
            const int len = std::min (kSub, n - pos);
            float* sub[2] { io[0] + pos, nc > 1 ? io[1] + pos : nullptr };
            processSub (sub, len, nc);
        }
        finishBlock (n);
    }

private:
    // ------------------------------------------------ un bloc de controle
    void processSub (float** io, int n, int nc)
    {
        Buf& in0 = out[NIn];
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < kSub; ++i) in0.ch[c][i] = (c < nc && i < n) ? io[c][i] : 0.f;

        curBpm = hostBpm;
        curPpqStart = ppqNow;
        curPpqInc = curBpm / (60.0 * (double) fs);
        ppqNow += (double) n * curPpqInc;

        for (int i = 0; i < kExt; ++i) extS[i] += (extT[i] - extS[i]) * 0.25f;          // modulations de la matrice du synthe, lissees
        updateModulation (n, in0);
        computeEffective();

        // modules presents sur le rack, dans l'ordre : le cablage par defaut (normalise) suit cet ordre
        int rackOrder[kMaxRack], rn = 0, defSrc[kNumNodes] {};
        bool active[kNumNodes] {};
        active[NIn] = true; active[NOut] = true;
        for (auto& sl : rackSlot)
        {
            const int v = sl.load();
            if (v >= kFirstMod && v < NOut && ! active[v]) { active[v] = true; rackOrder[rn++] = v; }
        }
        uint32_t activeSrc = 1u << NIn;
        int prev = NIn;
        for (int k = 0; k < rn; ++k) { defSrc[rackOrder[k]] = prev; prev = rackOrder[k]; activeSrc |= 1u << rackOrder[k]; }
        defSrc[NOut] = prev;

        uint32_t srcMask[kNumNodes] {};
        for (int k = 0; k <= rn; ++k)
        {
            const int nd = k < rn ? rackOrder[k] : (int) NOut;
            const uint32_t m = audioIn[nd].load() & activeSrc;
            srcMask[nd] = m ? m : (1u << defSrc[nd]);          // sinon : cablage par defaut
        }
        int order[kNumNodes], cnt = 0, state[kNumNodes] {};
        state[NIn] = 2;
        for (int k = 0; k <= rn; ++k) visit (k < rn ? rackOrder[k] : (int) NOut, srcMask, state, order, cnt);

        bool done[kNumNodes] {};
        done[NIn] = true;
        for (int k = 0; k < cnt; ++k)
        {
            const int nd = order[k];
            Buf in {}; bool fed = false;
            for (int s = 0; s < NOut; ++s)
                if (srcMask[nd] & (1u << s))
                {
                    if (! done[s]) fed = true;              // boucle de retour : on lit le bloc precedent
                    for (int c = 0; c < nc; ++c) for (int i = 0; i < n; ++i) in.ch[c][i] += out[s].ch[c][i];
                }
            if (fed) for (int c = 0; c < nc; ++c) for (int i = 0; i < n; ++i) in.ch[c][i] = 1.25f * std::tanh (0.8f * in.ch[c][i]);

            switch (nd)
            {
                case NWide:     modWide (in, out[nd], n, nc); break;
                case NSat:      modSat (in, out[nd], n, nc); break;
                case NColor:    modColor (in, out[nd], n, nc); break;
                case NFilter:   modFilter (in, out[nd], n, nc); break;
                case NPhaser:   modPhaser (in, out[nd], n, nc); break;
                case NComp:     modComp (in, out[nd], n, nc); break;
                case NEnsemble: modEnsemble (in, out[nd], n, nc); break;
                case NDelay:    modDelay (in, out[nd], n, nc); break;
                case NVerb:     modVerb (in, out[nd], n, nc); break;
                default:        modOut (in, io, n, nc); break;
            }
            done[nd] = true;
        }
    }

    void visit (int nd, const uint32_t* srcMask, int* state, int* order, int& cnt)
    {
        if (state[nd] != 0) return;                           // deja traite, ou en cours (boucle de retour)
        state[nd] = 1;
        for (int s = kFirstMod; s < NOut; ++s)
            if (srcMask[nd] & (1u << s)) visit (s, srcMask, state, order, cnt);
        state[nd] = 2;
        order[cnt++] = nd;
    }

    // ------------------------------------------------ LFO, S&H, enveloppe -> tensions de commande
    void updateModulation (int n, const Buf& in)
    {
        const float b1 = divBeats (p.lfo1Sync), b2 = divBeats (p.lfo2Sync), bs = divBeats (p.shSync);
        const float v1 = (b1 > 0.f ? lfo1.tickSync (ppqNow, b1, p.lfo1Shape)
                                   : lfo1.tick (0.05f * std::pow (800.f, p.lfo1Rate), p.lfo1Shape, n, fs)) * p.lfo1Level;
        const float v2 = (b2 > 0.f ? lfo2.tickSync (ppqNow, b2, p.lfo2Shape)
                                   : lfo2.tick (0.05f * std::pow (800.f, p.lfo2Rate), p.lfo2Shape, n, fs)) * p.lfo2Level;
        float pk = 0.f;
        for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) pk = std::max (pk, std::fabs (in.ch[c][i]));
        const float dt = (float) n / fs;
        const float rel = 0.02f * std::pow (75.f, p.envRel);
        envV += (1.f - std::exp (-dt / (pk > envV ? 0.003f : rel))) * (pk - envV);
        const float ve = clamp01 (envV * 2.2f) * p.envLevel;

        const float base[3] { v1, v2, ve };
        const uint32_t inMask  = cvIn[CvShIn].load()  & ~(1u << SrcSh);
        const uint32_t clkMask = cvIn[CvShClk].load() & ~(1u << SrcSh);
        float inVal = 0.f, clkVal = 0.f;
        for (int s = 0; s < 3; ++s)
        {
            const int srcId = s == 2 ? SrcEnv : s;
            if (inMask & (1u << srcId))  inVal += base[s];
            if (clkMask & (1u << srcId)) clkVal += base[s];
        }
        if (! inMask) inVal = shRng.bi();
        bool syncTrig = false;
        if (bs > 0.f)
        {
            const long long idx = (long long) std::floor (ppqNow / (double) bs);
            syncTrig = idx != shLastIdx; shLastIdx = idx;
        }
        const int shMode = bs > 0.f ? 2 : (clkMask != 0 ? 1 : 0);
        const float vs = sh.tick (0.3f * std::pow (100.f, p.shRate), shMode, syncTrig, clkVal, inVal, p.shSlew, n, fs) * p.shLevel * 2.f;

        srcVal[SrcLfo1] = v1; srcVal[SrcLfo2] = v2; srcVal[SrcSh] = vs; srcVal[SrcEnv] = ve;
        for (int d = 0; d < kNumCv; ++d)
        {
            const uint32_t m = cvIn[d].load();
            float sum = 0.f;
            for (int s = 0; s < kNumSrc; ++s) if (m & (1u << s)) sum += srcVal[s];
            cv[d] = std::max (-2.f, std::min (2.f, sum + (d < 10 ? extS[d] : 0.f)));
        }
        for (int s = 0; s < kNumSrc; ++s) modLed[s].store (std::min (1.f, std::fabs (srcVal[s])));
    }

    float U (float base, int cvi) const { return clamp01 (base + 0.6f * cv[cvi]); }

    void computeEffective()
    {
        if (std::fabs (p.low - lowDbLast) > 0.01f)  { lowB.makeLowShelf (fs, 120.0, p.low);    lowDbLast = p.low; }
        if (std::fabs (p.mid - midDbLast) > 0.01f)  { midB.makePeak (fs, 1100.0, 0.7, p.mid);  midDbLast = p.mid; }
        if (std::fabs (p.high - hiDbLast) > 0.01f)  { hiB.makeHighShelf (fs, 9000.0, p.high); hiDbLast = p.high; }

        eSatDrive = U (p.satDrive, CvDrive);
        eSatGain = dbToLin (26.f * std::pow (eSatDrive, 1.2f));
        eWidth = U (p.wideWidth, CvWidth);

        const float hU = U (p.hpf, CvHpf), lU = U (p.lpf, CvLpf);
        const float hRes = clamp01 (p.hpfPeak + 0.6f * cv[CvPeak]), lRes = clamp01 (p.lpfPeak + 0.6f * cv[CvPeak]);
        filterActive = hU > 0.003f || lU < 0.997f;
        hpfSvf.set (20.f * std::pow (250.f, hU), hRes, fs);
        lpfSvf.set (40.f * std::pow (450.f, lU), lRes, fs);

        ePhMix = U (p.phMix, CvPhMix);
        ePhFb = clamp01 (p.phFb + 0.6f * extS[10]);
        const float phBeats = divBeats (p.phSync);
        ePhHz = phBeats > 0.f ? (float) curBpm / 60.f / phBeats : 0.03f * std::pow (266.f, U (p.phRate, CvPhRate));

        comp.set (p.compType, p.compAmt, p.compThresh, p.compAtt);
        eEnsMix = U (p.ensMix, CvEnsMix);
        eEnsDepth = U (p.ensDepth, CvEnsDepth);
        outTarget = dbToLin (p.output);
        mixTarget = p.on ? U (p.mix, CvMix) : 0.f;
        if (mixCur < 0.f) { mixCur = mixTarget; outCur = outTarget; }
    }

    // ------------------------------------------------ modules
    void modWide (const Buf& in, Buf& o, int n, int nc)
    {
        for (int i = 0; i < n; ++i)
        {
            float l = in.ch[0][i], r = nc > 1 ? in.ch[1][i] : l;
            if (nc > 1) wider.process (l, r, eWidth, p.wideSpread, p.wideBass);
            o.ch[0][i] = l; if (nc > 1) o.ch[1][i] = r;
        }
    }

    void modSat (const Buf& in, Buf& o, int n, int nc)
    {
        const bool tiltOn = std::fabs (p.satTone - 0.5f) > 0.002f;
        const float t = (p.satTone - 0.5f) * 1.3f, aT = onePoleCoef (900.f, fs);
        const float aTape = onePoleCoef (19000.f - 9000.f * eSatDrive, fs * 4.f);
        const float makeup = 1.f / std::sqrt (eSatGain);
        const int mode = std::max (0, std::min (2, p.satMode));
        for (int i = 0; i < n; ++i)
            for (int c = 0; c < nc; ++c)
            {
                float x = in.ch[c][i];
                if (eSatDrive > 0.001f)
                {
                    float u4[4];
                    os.up (x, c, u4);
                    for (int k = 0; k < 4; ++k)
                    {
                        float y = satShape (mode, u4[k] * eSatGain) * makeup;
                        if (mode == 0) { tapeLp[c] += aTape * (y - tapeLp[c]); y = tapeLp[c]; }
                        u4[k] = y;
                    }
                    x = os.down (u4, c);
                    dcLp[c] += dcC * (x - dcLp[c]); x -= dcLp[c];
                }
                if (tiltOn) { tiltLp[c] += aT * (x - tiltLp[c]); const float hi = x - tiltLp[c]; x = tiltLp[c] * (1.f - t * 0.6f) + hi * (1.f + t); }
                o.ch[c][i] = x;
            }
    }

    void modColor (const Buf& in, Buf& o, int n, int nc)
    {
        for (int i = 0; i < n; ++i)
            for (int c = 0; c < nc; ++c)
                o.ch[c][i] = hiB.process (midB.process (lowB.process (in.ch[c][i], c), c), c);
    }

    void modFilter (const Buf& in, Buf& o, int n, int nc)
    {
        for (int i = 0; i < n; ++i)
            for (int c = 0; c < nc; ++c)
            {
                if (! filterActive) { o.ch[c][i] = in.ch[c][i]; continue; }
                float lp, hp, lp2, hp2;
                hpfSvf.tick (in.ch[c][i], c, lp, hp);
                lpfSvf.tick (hp, c, lp2, hp2);
                o.ch[c][i] = lp2;
            }
    }

    void modPhaser (const Buf& in, Buf& o, int n, int nc)
    {
        static const int stagesTab[4] { 4, 6, 8, 12 };
        const int st = stagesTab[std::max (0, std::min (3, p.phStages))];
        const float span = 0.8f + 4.8f * p.phDepth;                                    // octaves balayees
        auto coefFor = [this, span] (float ph)
        {
            const float l01 = 0.5f + 0.5f * std::sin (6.2831853f * ph);
            const float fc = std::min (0.42f * fs, 140.f * std::pow (2.f, l01 * span));
            const float tn = std::tan (kPi * fc / fs);
            return (tn - 1.f) / (tn + 1.f);
        };
        if (divBeats (p.phSync) > 0.f) { const double c = ppqNow / (double) divBeats (p.phSync); phaser.ph = (float) (c - std::floor (c)); }
        else { phaser.ph += ePhHz * (float) n / fs; phaser.ph -= std::floor (phaser.ph); }
        const float a0 = coefFor (phaser.ph), a1 = coefFor (phaser.ph + 0.25f);
        const float fb = 0.82f * ePhFb;
        const float dryG = 1.f - 0.5f * ePhMix, wetG = 0.5f * ePhMix;
        for (int i = 0; i < n; ++i)
            for (int c = 0; c < nc; ++c)
            {
                const float x = in.ch[c][i];
                if (ePhMix < 0.0005f) { o.ch[c][i] = x; continue; }
                const float a = c == 0 ? a0 : a1;
                float v = x + fb * phaser.last[c];
                for (int k = 0; k < st; ++k)
                {
                    const float y = a * v + phaser.z[c][k];
                    phaser.z[c][k] = v - a * y;
                    v = y;
                }
                phaser.last[c] = std::tanh (v);
                o.ch[c][i] = x * dryG + v * wetG;
            }
    }

    void modComp (const Buf& in, Buf& o, int n, int nc)
    {
        for (int i = 0; i < n; ++i)
        {
            float l = in.ch[0][i], r = nc > 1 ? in.ch[1][i] : l;
            comp.process (l, r);
            grPeak = std::max (grPeak, comp.grDb);
            o.ch[0][i] = l; if (nc > 1) o.ch[1][i] = r;
        }
    }

    void modEnsemble (const Buf& in, Buf& o, int n, int nc)
    {
        const float a = eEnsMix * 0.5f * kPi, dryG = std::cos (a), wetG = std::sin (a) * 1.08f;
        for (int i = 0; i < n; ++i)
        {
            const float l0 = in.ch[0][i], r0 = nc > 1 ? in.ch[1][i] : l0;
            float wl = 0.f, wr = 0.f;
            if (eEnsMix > 0.0005f) ens.tick (0.5f * (l0 + r0), eEnsDepth, p.ensSpeed, p.ensTone, wl, wr);
            else ens.line.push (0.5f * (l0 + r0));
            o.ch[0][i] = l0 * dryG + wl * wetG;
            if (nc > 1) o.ch[1][i] = r0 * dryG + wr * wetG;
        }
    }

    void modDelay (const Buf& in, Buf& o, int n, int nc)
    {
        float t;
        if (p.dlySync > 0) t = divBeats (p.dlySync) * 60.f / (float) curBpm;
        else               t = 0.02f * std::pow (90.f, clamp01 (p.dlyTime + 0.6f * extS[11]));         // 20 ms .. 1,8 s
        const float fb = clamp01 (p.dlyFb + 0.6f * extS[12]), mix = clamp01 (p.dlyMix + 0.6f * extS[13]);
        const float* inL = in.ch[0]; const float* inR = in.ch[nc > 1 ? 1 : 0];
        float dummyR[kSub];
        dly.process (inL, inR, o.ch[0], nc > 1 ? o.ch[1] : dummyR, n, nc, p.dlyMode, std::min (2.1f, t), fb * 0.97f, p.dlyTone, p.dlyMod, mix);
    }

    void modVerb (const Buf& in, Buf& o, int n, int nc)
    {
        const float mix = clamp01 (p.revMix + 0.6f * extS[14]), dec = clamp01 (p.revDecay + 0.6f * extS[15]), dmp = clamp01 (p.revDamp + 0.6f * extS[16]);
        const float* inL = in.ch[0]; const float* inR = in.ch[nc > 1 ? 1 : 0];
        float dummyR[kSub];
        verb.process (inL, inR, o.ch[0], nc > 1 ? o.ch[1] : dummyR, n, nc, mix, p.revSize, dec, p.revPre, dmp, p.revMod, p.revWidth, p.revShimmer, p.revLowCut, p.revFreeze);
    }

    void modOut (const Buf& wetIn, float** io, int n, int nc)
    {
        const Buf& dry = out[NIn];
        const float aHf = onePoleCoef (2500.f, fs);
        for (int i = 0; i < n; ++i)
        {
            mixCur += smoothC * (mixTarget - mixCur);
            outCur += smoothC * (outTarget - outCur);
            float mono = 0.f;
            for (int c = 0; c < nc; ++c)
            {
                const float wet = safety (wetIn.ch[c][i] * outCur);
                const float y = dry.ch[c][i] * (1.f - mixCur) + wet * mixCur;
                io[c][i] = y;
                mono += y;
            }
            mono /= (float) nc;
            hfLp += aHf * (mono - hfLp);                                   // part des aigus pour le prisme
            const float hf = mono - hfLp;
            eSum += mono * mono; hSum += hf * hf; ++eN;
        }
    }

    // ------------------------------------------------ lumiere du prisme et vumeter
    void finishBlock (int n)
    {
        const float dt = (float) n / fs;
        if (eN > 0)
        {
            const float rms = std::sqrt (eSum / (float) eN);
            const float lvl = clamp01 ((linToDb (rms) + 54.f) / 48.f);
            const float br = eSum > 1e-9f ? clamp01 (std::sqrt (hSum / eSum) * 1.6f) : 0.f;
            glowS += (1.f - std::exp (-dt / (lvl > glowS ? 0.05f : 0.6f))) * (lvl - glowS);
            brightS += (1.f - std::exp (-dt / 0.25f)) * ((lvl > 0.02f ? br : 0.f) - brightS);
            glow.store (glowS); bright.store (brightS);
        }
        eSum = hSum = 0.f; eN = 0;
        compGr.store (std::max (grPeak, compGr.load() * 0.8f));
        grPeak = 0.f;
    }

    // ------------------------------------------------ etat
    Params p;
    float fs = 44100.f;
    Buf out[kNumNodes];
    Oversampler4 os; Comp comp; Wider wider; Phaser phaser; Ensemble ens; MultiDelay dly; Reverb verb;
    Biquad lowB, midB, hiB; Svf hpfSvf, lpfSvf;
    Lfo lfo1, lfo2; SampleHold sh; Rng shRng; float envV = 0.f;
    float srcVal[kNumSrc] {}, cv[kNumCv] {}, extS[kExt] {}, extT[kExt] {};
    float dcC = 0, smoothC = 0, dcLp[2] {}, tiltLp[2] {}, tapeLp[2] {};
    float eSatDrive = 0, eSatGain = 1, eWidth = 0, ePhMix = 0, ePhFb = 0, ePhHz = 0.5f, eEnsMix = 0, eEnsDepth = 0.6f;
    float outTarget = 1, mixTarget = 1, outCur = -1, mixCur = -1;
    float grPeak = 0.f, eSum = 0.f, hSum = 0.f, hfLp = 0.f, glowS = 0.f, brightS = 0.f; int eN = 0;
    double hostBpm = 120.0, hostPpq = 0.0, ppqNow = 0.0, curBpm = 120.0, curPpqStart = 0.0, curPpqInc = 0.0;
    bool hostPlaying = false, hostHasPpq = false;
    long long shLastIdx = -1;
    float lowDbLast = -999.f, midDbLast = -999.f, hiDbLast = -999.f;
    bool filterActive = false;
};
} // namespace zephyr
