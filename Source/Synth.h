// Zephyr synth engine : 12 voix polyphoniques, 2 VCO (saw / pulse / tri / sine) + sub, derive analogique (DRIFT),
// filtres SEM / LADDER / PROPHET avec saturation dans la boucle, poly-mod facon Prophet,
// 4 LFO parametrables (facon Serum), 8 macros, matrice de modulation. Aucune dependance JUCE.
#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace zephyr
{
constexpr int kVoices = 12, kUni = 8, kLfos = 4, kMacros = 8, kSlots = 12;
enum Assign { AsPoly, AsMono, AsUnison, AsShare, AsChord };
enum ModSrc { MsNone, MsLfo1, MsLfo2, MsLfo3, MsLfo4, MsMac1, MsMac2, MsMac3, MsMac4, MsEnv, MsVel, MsWheel, MsAmpEnv, MsKey, MsRandom, MsChanAT, MsPolyAT, MsMac5, MsMac6, MsMac7, MsMac8, kNumModSrc };
inline int macroSrc (int i) { return i < 4 ? MsMac1 + i : MsMac5 + (i - 4); }
enum ModDst { MdNone, MdPitch, MdPitch2, MdPw1, MdPw2, MdFm, MdRing, MdMix, MdHpf, MdLpf, MdReso, MdLevel, MdPan,
              MdFold, MdFxDrive, MdFxWidth, MdFxPhMix, MdFxHpf, MdFxLpf, MdFxPeak, MdFxPhRate, MdFxEnsDepth, MdFxEnsMix, MdFxMix, MdFxPhFb,
              MdDlyTime, MdDlyFb, MdDlyMix, MdRevMix, MdRevDecay, MdRevDamp,
              MdNoise, MdHpfReso, MdSat, MdMorph, MdFltEnv, MdFoldSym, MdLvl1, MdSub, MdPolyMod, MdDrift, kNumModDst };
constexpr int kFxMod = 17;                // destinations FX : MdFxDrive .. MdRevDamp (les 10 premieres = les entrees CV du moteur d'effets)
inline const char* const kModSrcNames[kNumModSrc] { "-", "LFO 1", "LFO 2", "LFO 3", "LFO 4", "MACRO 1", "MACRO 2", "MACRO 3", "MACRO 4", "ENV FLT", "VELOCITY", "MOD WHEEL", "ENV AMP", "KEY TRACK", "RANDOM", "AFTERTOUCH", "POLY AT", "MACRO 5", "MACRO 6", "MACRO 7", "MACRO 8" };
inline const char* const kModDstNames[kNumModDst] { "-", "PITCH", "PITCH VCO2", "PWM VCO1", "PWM VCO2", "FM", "RING", "LEVEL VCO2", "HPF", "CUTOFF", "RESO", "LEVEL", "PAN",
    "FOLD", "SAT DRIVE", "WIDTH", "PHASER MIX", "FX HPF", "FX LPF", "FX PEAK", "PHASER RATE", "ENS DEPTH", "ENSEMBLE MIX", "FX DRY/WET", "PHASER FDBK",
    "DELAY TIME", "DELAY FDBK", "DELAY MIX", "REV MIX", "REV DECAY", "REV DAMP",
    "NOISE", "HPF PEAK", "VCF SAT", "SEM MORPH", "FLT ENV AMT", "FOLD SYM", "LEVEL VCO1", "SUB LEVEL", "POLY-MOD", "DRIFT" };
inline const char* const kLfoDivNames[14] { "Free", "4 bar", "2 bar", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/4.", "1/8.", "1/16.", "1/4T", "1/8T", "1/16T" };
inline const float kLfoDivBeats[14] { 0.f, 16.f, 8.f, 4.f, 2.f, 1.f, 0.5f, 0.25f, 1.5f, 0.75f, 0.375f, 2.f / 3.f, 1.f / 3.f, 1.f / 6.f };

struct SynthParams
{
    int wave1 = 0, wave2 = 0;                 // 0 saw, 1 pulse, 2 triangle, 3 sine
    int oct1 = 0, oct2 = 0;                   // -2..+2
    float tune2 = 0.f, fine2 = 0.f;           // demi-tons, cents
    float pw1 = 0.5f, pw2 = 0.5f;
    int sync = 0;                             // hard sync : VCO2 recale par VCO1
    float fm = 0.f, ring = 0.f;               // 0..1
    float lvl1 = 0.8f, lvl2 = 0.f, sub = 0.f, noise = 0.f;
    float glide = 0.f, drift = 0.f;
    float hpf = 0.f, hpfRes = 0.f, lpf = 0.7f, lpfRes = 0.2f, fltEnv = 0.3f, keytrack = 0.5f, sat = 0.f;
    float aA = 0.01f, aD = 0.3f, aS = 0.8f, aR = 0.25f;
    float fA = 0.01f, fD = 0.35f, fS = 0.2f, fR = 0.25f;
    int assign = 0, hold = 0;                 // affectation des touches (Poly, Mono, Unison, Share, Chord) et HOLD
    float uniDetune = 0.3f;                   // ecart de desaccord des voix empilees
    int ftype = 0;                            // filtre : 0 SEM, 1 Ladder, 2 Prophet
    float semMorph = 0.f;                     // SEM : LP -> notch -> HP
    float fold = 0.f, foldSym = 0.f;          // wavefolder : quantite, symetrie
    float pmEnv = 0.f, pmOsc = 0.f;           // poly-mod : sources (enveloppe filtre, VCO2)
    int pmFreq = 0, pmPw = 0, pmFilt = 0;     // poly-mod : destinations (frequence VCO1, largeur VCO1, filtre)
    float master = 0.7f;
    struct Lfo { float rate = 0.4f, slew = 0.f, fade = 0.f; int wave = 0, sync = 0, trig = 0; } lfo[kLfos];     // wave : 0 sin, 1 tri, 2 saw, 3 carre, 4 S&H
    float macro[kMacros] {};
    struct Slot { int src = 0, dst = 0; float amt = 0.f; } slot[kSlots];
};

namespace sy
{
inline float blep (float t, float dt)
{
    if (t < dt) { t /= dt; return t + t - t * t - 1.f; }
    if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
    return 0.f;
}
inline float osc (int wave, float ph, float dt, float pw)
{
    switch (wave)
    {
        case 0: return 2.f * ph - 1.f - blep (ph, dt);
        case 1: { float t2 = ph - pw; if (t2 < 0.f) t2 += 1.f; return (ph < pw ? 1.f : -1.f) + blep (ph, dt) - blep (t2, dt); }
        case 2: return 4.f * std::fabs (ph - 0.5f) - 1.f;
        default: return std::sin (6.2831853f * ph);
    }
}
inline float softClip (float x) { return x / std::sqrt (1.f + x * x); }          // saturation douce et bon marche

struct Env
{
    enum { Idle, Att, Dec, Sus, Rel } st = Idle;
    float v = 0.f;
    void on()  { st = Att; }
    void off() { if (st != Idle) st = Rel; }
    float tick (float a, float d, float s, float r, float fs)
    {
        auto coef = [fs] (float sec) { return 1.f - std::exp (-1.f / (mx (sec, 0.0005f) * fs)); };
        switch (st)
        {
            case Att: v += (1.4f - v) * coef (a * 0.45f + 0.0005f); if (v >= 1.f) { v = 1.f; st = Dec; } break;
            case Dec: v += (s - v) * coef (d * 0.5f); if (std::fabs (v - s) < 0.0005f) { v = s; st = Sus; } break;
            case Sus: v = s; break;
            case Rel: v += (0.f - v) * coef (r * 0.5f); if (v < 0.0003f) { v = 0.f; st = Idle; } break;
            default: break;
        }
        return v;
    }
    static float mx (float a, float b) { return a > b ? a : b; }
};
// ---- HPF doux en amont du VCF : allege les pads sans "cogner"
struct Hp
{
    float s1 = 0.f, s2 = 0.f;
    float run (float x, float g, float k)
    {
        const float a1 = 1.f / (1.f + g * (g + k)), a2 = g * a1, a3 = g * a2;
        const float v3 = x - s2, v1 = a1 * s1 + a2 * v3, v2 = s2 + a2 * s1 + a3 * v3;
        s1 = 2.f * v1 - s1;
        s1 = s1 / (1.f + 0.05f * s1 * s1);
        s2 = 2.f * v2 - s2;
        return x - k * v1 - v2;
    }
};
// ---- SEM : SVF 12 dB, resonance douce, morph LP -> notch -> HP ; la saturation pince la bande passante (resonance qui "claque")
struct Sem
{
    float s1 = 0.f, s2 = 0.f;
    float run (float x, float g, float k, float morph, float sat)
    {
        const float a1 = 1.f / (1.f + g * (g + k)), a2 = g * a1, a3 = g * a2;
        const float v3 = x - s2, v1 = a1 * s1 + a2 * v3, v2 = s2 + a2 * s1 + a3 * v3;
        s1 = 2.f * v1 - s1;
        s1 = s1 / (1.f + (0.10f + 0.5f * sat) * s1 * s1);
        s2 = 2.f * v2 - s2;
        if (sat > 0.001f) s2 = s2 / (1.f + 0.08f * sat * s2 * s2);
        const float lp = v2, hp = x - k * v1 - v2;
        const float lg = std::min (1.f, std::max (0.f, 2.f * (1.f - morph))), hg = std::min (1.f, std::max (0.f, 2.f * morph));
        return lp * lg + hp * hg;
    }
};
// ---- Ladder 24 dB (4 poles TPT, retour instantane resolu, saturation a l'entree) : rond, les graves s'amincissent avec la resonance
struct Ladder
{
    float s[4] {};
    float run (float x, float g, float k)
    {
        const float G = g / (1.f + g), H = 1.f - G;
        const float G2 = G * G, G3 = G2 * G, G4 = G2 * G2;
        const float S = G3 * H * s[0] + G2 * H * s[1] + G * H * s[2] + H * s[3];
        float u = (x - k * S) / (1.f + k * G4);
        u = std::tanh (u);
        float in = u;
        for (int i = 0; i < 4; ++i)
        {
            const float y = G * in + H * s[i];
            s[i] = 2.f * y - s[i];
            in = y;
        }
        return in;
    }
};
// ---- Prophet 24 dB (facon CEM3320) : etages OTA qui saturent chacun un peu, bande passante compensee (les graves restent),
// resonance chantante et cremeuse
struct Prophet
{
    float s[4] {};
    float run (float x, float g, float k, float sat)
    {
        const float G = g / (1.f + g), H = 1.f - G;
        const float G2 = G * G, G3 = G2 * G, G4 = G2 * G2;
        const float S = G3 * H * s[0] + G2 * H * s[1] + G * H * s[2] + H * s[3];
        float u = (x * (1.f + 0.5f * k) - k * softClip (S * 1.1f) / 1.1f) / (1.f + k * G4);
        float in = softClip (u * (0.9f + 0.6f * sat));
        for (int i = 0; i < 4; ++i)
        {
            const float y = G * in + H * s[i];
            s[i] = 2.f * y - s[i];
            in = i < 3 ? y + 0.04f * (softClip (y * 1.6f) / 1.6f - y) * (1.f + 4.f * sat) : y;
        }
        return in;
    }
};
// ---- wavefolder (repli sinus) : 0 = transparent
inline float fold (float x, float amt, float sym)
{
    if (amt < 0.001f) return x;
    const float g = 1.f + 9.f * amt * amt;
    const float u = g * x + sym * 0.6f * amt;
    const float y = std::sin (u * 1.5707963f * 1.5f) * 0.9f;
    const float mixA = std::min (1.f, amt * 5.f);
    return x + (y - x) * mixA;
}
} // namespace sy

class Synth
{
public:
    void prepare (float sampleRate)
    {
        fs = sampleRate;
        for (auto& v : voices) v = Voice();
        for (int i = 0; i < kLfos; ++i) { gPhase[i] = 0.f; gSh[i] = 0.f; gCur[i] = 0.f; }
        counter = 0; seed = 12345u;
        for (int i = 0; i < kVoices; ++i)                                   // chaque "carte voix" a ses petites differences, fixes
        {
            vCut[i] = rnd(); vEnv[i] = rnd(); vOff1[i] = rnd(); vOff2[i] = rnd();
            walk1[i] = walk2[i] = 0.f; walkT1[i] = rnd(); walkT2[i] = rnd();
        }
    }
    void setParams (const SynthParams& p)
    {
        const int oldAssign = P.assign; const bool oldHold = P.hold != 0;
        P = p;
        if (P.assign != oldAssign) allNotesOff();
        else if (oldHold && ! P.hold) dropLatched();
    }
    void setChord (const int* iv, int n) { chordN = std::max (0, std::min (6, n)); for (int i = 0; i < chordN; ++i) chord[i] = iv[i]; }
    float lfoHz (int i) const { return hz (i); }
    void setHost (double bpmIn) { bpm = bpmIn > 20.0 ? (float) bpmIn : 120.f; }
    void setWheel (float w) { wheel = w; }
    void setBend (float semis) { bend = semis; }
    void setChanAT (float a) { chanAT = a; }
    void setPolyAT (int note, float a) { for (auto& v : voices) if (v.active && v.note == note) v.pat = a; }

    // ---- touches : l'interpretation depend du mode d'affectation (POLY, MONO, UNISON, SHARE, CHORD MEMORY) et de HOLD
    void noteOn (int note, float vel)
    {
        if (P.hold && physCount() == 0 && nKeys > 0) dropLatched();
        int k = findKey (note);
        if (k < 0 && nKeys < kKeys) k = nKeys++;
        if (k >= 0) keys[k] = { note, vel, true, ++stamp };
        if (P.assign == AsPoly) startVoice (note, vel, 0.f, 0.f, 1.f, -1, false);
        else retarget();
    }
    void noteOff (int note)
    {
        const int k = findKey (note);
        if (k < 0) { if (P.assign == AsPoly) releaseNote (note, false); return; }
        if (P.hold) { keys[k].phys = false; return; }
        for (int j = k; j + 1 < nKeys; ++j) keys[j] = keys[j + 1];
        --nKeys;
        if (P.assign == AsPoly) releaseNote (note, false); else retarget();
    }
    void allNotesOff() { nKeys = 0; for (auto& v : voices) if (v.active && v.gated) release (v); }
    // notes venant de l'arpegiateur : une voix par appel, ou une pile de voix desaccordees (mode UNISON)
    void noteOnRaw (int note, float vel) { startVoice (note, vel, 0.f, 0.f, 1.f, -1, true); }
    void noteOnStack (int note, float vel)
    {
        for (int i = 0; i < kUni; ++i)
        {
            const float sp = 2.f * (float) i / (float) (kUni - 1) - 1.f;
            startVoice (note, vel, sp * maxCents(), sp * 0.5f * std::min (1.f, P.uniDetune * 1.5f), kUniGain, -1, true);
        }
    }
    void noteOffRaw (int note) { releaseNote (note, true); }
    // arpegiateur en POLY : un "VCO" fixe par pas (rotation sur 4). Comme sur le Mono/Poly, chaque VCO reagit differemment a la FM.
    void noteOnSlot (int note, float vel, int slot)
    {
        if (slot < 0 || slot >= kVoices) return;
        static const float kFm[4] { 0.25f, 1.f, 0.65f, 1.4f };
        if (auto* v = startVoice (note, vel, 0.f, 0.f, 1.f, slot, true)) v->fmK = kFm[slot & 3];
    }
    void noteOffSlot (int slot, int note) { if (slot >= 0 && slot < kVoices && voices[slot].active && voices[slot].gated && voices[slot].note == note) release (voices[slot]); }
    const float* fxMod() const { return fxOut; }                  // modulations globales des effets (matrice : destinations FX / delay / reverb)
    int activeVoices() const { int n = 0; for (auto& v : voices) n += v.active ? 1 : 0; return n; }

    void process (float* L, float* R, int n)
    {
        for (int i = 0; i < n; ++i) { L[i] = 0.f; R[i] = 0.f; }
        const float lfoHz[kLfos] = { hz (0), hz (1), hz (2), hz (3) };
        for (int pos = 0; pos < n; pos += kCtl)
        {
            const int len = std::min (kCtl, n - pos);
            const float dt = (float) len / fs;
            for (int i = 0; i < kLfos; ++i)
            {
                gPhase[i] += lfoHz[i] * dt;
                if (gPhase[i] >= 1.f) { gPhase[i] -= std::floor (gPhase[i]); gSh[i] = rnd(); }
                gCur[i] += (gSh[i] - gCur[i]) * slewCoef (P.lfo[i].slew, lfoHz[i], dt);          // S&H : marches lissees par SLEW
            }
            // derive lente de chaque carte voix (marche aleatoire lissee, ~1 s)
            const float wc = 1.f - std::exp (-dt / 0.9f);
            for (int i = 0; i < kVoices; ++i)
            {
                if (rndU() < dt * 0.7f) walkT1[i] = rnd();
                if (rndU() < dt * 0.7f) walkT2[i] = rnd();
                walk1[i] += (walkT1[i] - walk1[i]) * wc;
                walk2[i] += (walkT2[i] - walk2[i]) * wc;
            }
            for (auto& v : voices)
                if (v.active) renderVoice (v, (int) (&v - voices), L + pos, R + pos, len, lfoHz, dt);
        }
        for (int i = 0; i < n; ++i) { L[i] = std::tanh (L[i] * P.master * 0.7f); R[i] = std::tanh (R[i] * P.master * 0.7f); }
        computeFxMod();
    }

private:
    static constexpr int kCtl = 16;
    static constexpr float kUniGain = 0.42f;
    struct Voice
    {
        bool active = false; int note = 60; float vel = 1.f; uint64_t age = 0;
        bool gated = false, raw = false; int slot = -1; float uniCents = 0.f, uniPan = 0.f, gainV = 1.f, fmK = 1.f;
        sy::Env aenv, fenv; float pitch = 60.f, ph1 = 0.f, ph2 = 0.f, prev2 = 0.f; bool subFlip = false;
        sy::Hp hp; sy::Sem sem; sy::Ladder lad; sy::Prophet pro; float pat = 0.f, patS = 0.f, rndv = 0.f; float xPrev = 0.f;
        float lph[kLfos] {}, lfade[kLfos] {}, lsh[kLfos] {}, lcur[kLfos] {};
    };
    static constexpr int kKeys = 16;
    struct Key { int note; float vel; bool phys; uint64_t t; };
    Key keys[kKeys];
    int nKeys = 0, chordN = 4, chord[6] { 0, 4, 7, 11, 0, 0 };
    uint64_t stamp = 0;
    Voice voices[kVoices];
    SynthParams P;
    float fs = 44100.f, bpm = 120.f, wheel = 0.f, bend = 0.f, gPhase[kLfos] {}, gSh[kLfos] {}, gCur[kLfos] {};
    float vCut[kVoices] {}, vEnv[kVoices] {}, vOff1[kVoices] {}, vOff2[kVoices] {}, walk1[kVoices] {}, walk2[kVoices] {}, walkT1[kVoices] {}, walkT2[kVoices] {};
    uint64_t counter = 0; int lastNote = -1; uint32_t seed = 12345u;
    float lastVel = 0.8f, fxOut[kFxMod] {}, chanAT = 0.f, lastRnd = 0.f, lastKey = 0.f;

    int findKey (int note) const { for (int i = 0; i < nKeys; ++i) if (keys[i].note == note) return i; return -1; }
    int physCount() const { int c = 0; for (int i = 0; i < nKeys; ++i) c += keys[i].phys ? 1 : 0; return c; }
    float maxCents() const { return 45.f * std::pow (std::max (0.f, std::min (1.f, P.uniDetune)), 1.2f); }
    static void release (Voice& v) { v.aenv.off(); v.fenv.off(); v.gated = false; }
    void releaseNote (int note, bool raw) { for (auto& v : voices) if (v.active && v.gated && v.note == note && v.raw == raw && v.slot < 0) release (v); }
    void dropLatched()                                                      // touches verrouillees (HOLD) : on les lache
    {
        int w = 0;
        for (int i = 0; i < nKeys; ++i)
        {
            if (keys[i].phys) keys[w++] = keys[i];
            else if (P.assign == AsPoly) releaseNote (keys[i].note, false);
        }
        nKeys = w;
        if (P.assign != AsPoly) retarget();
    }
    Voice* startVoice (int note, float vel, float cents, float pan, float gain, int slot, bool raw)
    {
        Voice* v = nullptr;
        if (slot >= 0) v = &voices[slot];
        else
        {
            for (auto& c : voices) if (! c.active) { v = &c; break; }
            if (v == nullptr)
            {
                float best = 1e9f;
                for (auto& c : voices)
                {
                    const float score = (c.gated ? 1000.f : 0.f) + c.aenv.v * 100.f + (float) (counter - c.age) * -1e-6f;
                    if (score < best) { best = score; v = &c; }
                }
            }
        }
        const bool glide = P.glide > 0.001f && lastNote >= 0;
        *v = Voice();
        v->active = true; v->gated = true; v->raw = raw; v->slot = slot; v->note = note; v->vel = vel; v->age = ++counter;
        v->uniCents = cents; v->uniPan = pan; v->gainV = gain;
        v->pitch = glide ? (float) lastNote : (float) note;
        if (P.drift > 0.001f) { v->ph1 = rndU(); v->ph2 = rndU(); }        // oscillateurs libres : chaque note demarre ailleurs dans le cycle
        v->aenv.on(); v->fenv.on(); lastVel = vel;
        v->rndv = rnd(); lastRnd = v->rndv; lastKey = std::max (-1.f, std::min (1.f, ((float) note - 60.f) / 48.f));
        for (int i = 0; i < kLfos; ++i) { v->lph[i] = 0.f; v->lfade[i] = 0.f; v->lsh[i] = rnd(); v->lcur[i] = v->lsh[i]; }
        lastNote = note;
        return v;
    }
    // MONO / UNISON / SHARE / CHORD : chaque voix a une note cible deduite des touches tenues
    void retarget()
    {
        int tgt[kVoices]; float cents[kVoices] {}, pan[kVoices] {};
        for (auto& t : tgt) t = -1;
        float gain = 1.f;
        int latest = -1; uint64_t lt = 0;
        for (int i = 0; i < nKeys; ++i) if (keys[i].t >= lt) { lt = keys[i].t; latest = i; }
        if (latest >= 0)
        {
            const int ln = keys[latest].note; const float mc = maxCents();
            if (P.assign == AsMono) tgt[0] = ln;
            else if (P.assign == AsUnison)
            {
                gain = kUniGain;
                for (int i = 0; i < kUni; ++i) { const float sp = 2.f * (float) i / (float) (kUni - 1) - 1.f; tgt[i] = ln; cents[i] = sp * mc; pan[i] = sp * 0.5f * std::min (1.f, P.uniDetune * 1.5f); }
            }
            else if (P.assign == AsShare)
            {
                gain = 0.42f;
                Key byTime[kKeys]; for (int i = 0; i < nKeys; ++i) byTime[i] = keys[i];
                std::sort (byTime, byTime + nKeys, [] (const Key& a, const Key& b) { return a.t < b.t; });
                const int k = nKeys;
                for (int i = 0; i < kVoices; ++i)
                {
                    tgt[i] = byTime[i % k].note;
                    const int group = (kVoices + k - 1 - (i % k)) / k;                  // nombre de voix sur cette note
                    const int j = i / k;
                    const float sp = group > 1 ? 2.f * (float) j / (float) (group - 1) - 1.f : 0.f;
                    cents[i] = sp * mc * 0.6f; pan[i] = sp * 0.3f;
                }
            }
            else if (P.assign == AsChord)
            {
                gain = 0.62f;
                const int n = std::max (1, chordN);
                for (int i = 0; i < n; ++i)
                {
                    tgt[i] = ln + (chordN > 0 ? chord[i] : 0);
                    cents[i] = ((i & 1) ? 1.f : -1.f) * mc * 0.15f;
                    pan[i] = n > 1 ? (2.f * (float) i / (float) (n - 1) - 1.f) * 0.35f : 0.f;
                }
            }
        }
        const bool legato = P.assign != AsShare;
        for (int i = 0; i < kVoices; ++i)
        {
            auto& v = voices[i];
            if (tgt[i] < 0) { if (v.active && v.gated && v.slot == i) release (v); continue; }
            if (v.active && v.gated && v.slot == i)
            {
                v.uniCents = cents[i]; v.uniPan = pan[i]; v.gainV = gain;
                if (v.note != tgt[i])
                {
                    if (legato) v.note = tgt[i];                                         // legato : pas de retrig, la voix glisse (portamento)
                    else { startVoice (tgt[i], keys[latest].vel, cents[i], pan[i], gain, i, false); }
                }
            }
            else startVoice (tgt[i], keys[latest].vel, cents[i], pan[i], gain, i, false);
        }
    }

    void computeFxMod()                                                    // sources globales : LFO libres, macros, molette, enveloppe de filtre (voix la plus forte), velocite
    {
        float src[kNumModSrc] {};
        for (int i = 0; i < kLfos; ++i) src[MsLfo1 + i] = lfoWave (gPhase[i], P.lfo[i].wave, gCur[i]);
        for (int i = 0; i < kMacros; ++i) src[macroSrc (i)] = P.macro[i];
        float e = 0.f; for (auto& v : voices) if (v.active) e = std::max (e, v.fenv.v);
        src[MsEnv] = e; src[MsVel] = lastVel; src[MsWheel] = wheel;
        float ae = 0.f, pa = 0.f; for (auto& v : voices) if (v.active) { ae = std::max (ae, v.aenv.v); pa = std::max (pa, v.patS); }
        src[MsAmpEnv] = ae; src[MsKey] = lastKey; src[MsRandom] = lastRnd; src[MsChanAT] = chanAT; src[MsPolyAT] = std::max (pa, chanAT);
        float acc[kFxMod] {};
        for (const auto& sl : P.slot)
            if (sl.src > 0 && sl.dst >= MdFxDrive && sl.dst <= MdRevDamp && sl.amt != 0.f) acc[sl.dst - MdFxDrive] += src[sl.src] * sl.amt;
        for (int i = 0; i < kFxMod; ++i) fxOut[i] = std::min (2.f, std::max (-2.f, acc[i]));
    }

    float rnd()  { seed = seed * 1664525u + 1013904223u; return (float) (seed >> 8) / 8388608.f - 1.f; }
    float rndU() { return 0.5f * (rnd() + 1.f); }

    float hz (int i) const
    {
        const auto& l = P.lfo[i];
        if (l.sync > 0 && l.sync < 14) return bpm / 60.f / kLfoDivBeats[l.sync];
        return 0.05f * std::pow (800.f, l.rate);                         // 0.05 .. 40 Hz
    }
public:
    static float lfoWave (float ph, int wave, float sh)
    {
        switch (wave)
        {
            case 0:  return std::sin (6.2831853f * ph);
            case 1:  return 4.f * std::fabs (ph - 0.5f) - 1.f;
            case 2:  return 2.f * ph - 1.f;
            case 3:  return ph < 0.5f ? 1.f : -1.f;
            default: return sh;
        }
    }
    static float slewCoef (float slew, float hzv, float dt)
    {
        if (slew < 0.001f) return 1.f;
        const float tau = std::max (1e-4f, slew * 0.5f / std::max (hzv, 0.01f));
        return 1.f - std::exp (-dt / tau);
    }

private:
    static float c01 (float x) { return std::min (1.f, std::max (0.f, x)); }

    void renderVoice (Voice& v, int vi, float* L, float* R, int len, const float* lfoHzv, float dtCtl)
    {
        // ---- sources de modulation (une fois par bloc de controle)
        float src[kNumModSrc] {};
        for (int i = 0; i < kLfos; ++i)
        {
            const auto& l = P.lfo[i];
            float ph = gPhase[i], sh = gCur[i], fadeG = 1.f;
            if (l.trig)
            {
                v.lph[i] += lfoHzv[i] * dtCtl;
                if (v.lph[i] >= 1.f) { v.lph[i] -= std::floor (v.lph[i]); v.lsh[i] = rnd(); }
                v.lcur[i] += (v.lsh[i] - v.lcur[i]) * slewCoef (l.slew, lfoHzv[i], dtCtl);
                ph = v.lph[i]; sh = v.lcur[i];
                v.lfade[i] += dtCtl;
                const float ft = l.fade * 4.f;
                fadeG = ft > 0.001f ? std::min (1.f, v.lfade[i] / ft) : 1.f;
            }
            src[MsLfo1 + i] = lfoWave (ph, l.wave, sh) * fadeG;
        }
        for (int i = 0; i < kMacros; ++i) src[macroSrc (i)] = P.macro[i];
        src[MsVel] = v.vel; src[MsWheel] = wheel;
        src[MsAmpEnv] = v.aenv.v; src[MsKey] = std::max (-1.f, std::min (1.f, (v.pitch - 60.f) / 48.f)); src[MsRandom] = v.rndv;
        v.patS += (v.pat - v.patS) * 0.4f;
        src[MsChanAT] = chanAT; src[MsPolyAT] = std::max (v.patS, chanAT);
        src[MsEnv] = v.fenv.v;
        float md[kNumModDst] {};
        for (const auto& s : P.slot)
            if (s.src > 0 && s.dst > 0 && s.amt != 0.f) md[s.dst] += src[s.src] * s.amt;

        // ---- derive analogique : desaccord lent, cutoff et enveloppes un peu differents d'une voix a l'autre
        const float drift = c01 (P.drift + md[MdDrift]);
        const float envK = 1.f + 0.14f * drift * vEnv[vi];
        const float fe = v.fenv.tick (P.fA * envK, P.fD * envK, P.fS, P.fR * envK, fs / (float) kCtl);   // enveloppes cadencees au bloc
        src[MsEnv] = fe;
        const float dc1 = drift * (5.f * vOff1[vi] + 11.f * walk1[vi]), dc2 = drift * (5.f * vOff2[vi] + 11.f * walk2[vi]);   // cents

        const float glideC = P.glide > 0.001f ? 1.f - std::exp (-dtCtl / (P.glide * 0.6f + 0.002f)) : 1.f;
        v.pitch += (((float) v.note) - v.pitch) * glideC;
        const float n1 = v.pitch + 12.f * (float) P.oct1 + bend + md[MdPitch] * 12.f + (v.uniCents + dc1) * 0.01f;
        const float n2 = v.pitch + 12.f * (float) P.oct2 + P.tune2 + P.fine2 * 0.01f + bend + md[MdPitch] * 12.f + md[MdPitch2] * 12.f + (v.uniCents + dc2) * 0.01f;
        const float f1 = 440.f * std::pow (2.f, (n1 - 69.f) / 12.f), f2 = 440.f * std::pow (2.f, (n2 - 69.f) / 12.f);
        const float pw1 = std::min (0.95f, std::max (0.05f, P.pw1 + md[MdPw1] * 0.45f));
        const float pw2 = std::min (0.95f, std::max (0.05f, P.pw2 + md[MdPw2] * 0.45f));
        const float noiseA = c01 (P.noise + md[MdNoise]), lv1 = c01 (P.lvl1 + md[MdLvl1]), subA = c01 (P.sub + md[MdSub]);
        const float fmA = c01 (P.fm + md[MdFm]), ringA = c01 (P.ring + md[MdRing]);
        const float l2 = c01 (P.lvl2 + md[MdMix]);

        // ---- filtres
        const float kt = (v.pitch - 60.f) / 12.f * P.keytrack;
        const float hpfHz = 20.f * std::pow (400.f, c01 (P.hpf + md[MdHpf] * 0.5f));
        const float lpfOct = c01 (P.lpf + md[MdLpf] * 0.5f + drift * 0.025f * vCut[vi]);
        float lpfHz = 25.f * std::pow (720.f, lpfOct) * std::pow (2.f, kt + fe * c01 (P.fltEnv + md[MdFltEnv] * 0.5f) * 5.f);
        lpfHz = std::min (lpfHz, 0.45f * fs);
        const float res = c01 (P.lpfRes + md[MdReso] * 0.5f);
        const float hRes = c01 (P.hpfRes + md[MdHpfReso] * 0.5f);
        const float morph = c01 (P.semMorph + md[MdMorph] * 0.5f);
        const float sat = c01 (P.sat + md[MdSat]);
        const float gL = std::tan (3.14159265f * lpfHz / fs), gH = std::tan (3.14159265f * std::min (hpfHz, 0.45f * fs) / fs);
        const float gMax = std::tan (3.14159265f * 0.45f);
        const float kH = 2.f * std::pow (1.f - 0.8f * hRes, 1.5f) + 0.08f;
        const float kSem = 1.75f * std::pow (1.f - res, 1.3f) + 0.05f, kLad = 4.05f * res, kPro = 3.85f * res;
        const float foldA = c01 (P.fold + md[MdFold]), foldS = std::min (1.f, std::max (-1.f, P.foldSym + md[MdFoldSym]));
        // saturation du VCF : on attaque le filtre plus fort (ses non-linearites travaillent), puis on compense le niveau
        const float satIn = 1.f + 5.f * sat * sat + 1.5f * sat, satOut = 1.f / std::pow (satIn, 0.45f);
        // poly-mod (Prophet) : enveloppe du filtre et VCO2 vers la frequence / la largeur du VCO1 et le filtre
        const float pmE = c01 (P.pmEnv + md[MdPolyMod]) * fe, pmO = c01 (P.pmOsc + md[MdPolyMod]);
        const bool pmOn = (P.pmFreq || P.pmPw || P.pmFilt) && (pmE > 0.0005f || pmO > 0.0005f);

        const float level = std::min (2.f, std::max (0.f, 1.f + md[MdLevel]));
        const float pan = std::min (1.f, std::max (-1.f, md[MdPan] + v.uniPan));
        const float gl = std::sqrt (0.5f * (1.f - pan)), gr = std::sqrt (0.5f * (1.f + pan));
        const float dt1 = f1 / fs, dt2 = f2 / fs;
        const float aA = P.aA * envK, aD = P.aD * envK, aR = P.aR * envK;

        for (int i = 0; i < len; ++i)
        {
            const float pm = pmOn ? pmE * 1.0f + pmO * v.prev2 : 0.f;
            // VCO1 est module en frequence par VCO2 (FM audio), VCO2 est recale par VCO1 (hard sync)
            float inc1 = dt1 * std::exp2 (fmA * v.fmK * 2.5f * v.prev2 + (P.pmFreq ? pm * 3.f : 0.f));
            inc1 = std::min (inc1, 0.45f);
            v.ph1 += inc1;
            if (v.ph1 >= 1.f) { v.ph1 -= 1.f; v.subFlip = ! v.subFlip; if (P.sync) v.ph2 = v.ph1 * dt2 / std::max (inc1, 1e-6f); }
            v.ph2 += dt2; if (v.ph2 >= 1.f) v.ph2 -= 1.f;
            const float pwA = P.pmPw ? std::min (0.95f, std::max (0.05f, pw1 + pm * 0.4f)) : pw1;
            const float o1 = sy::osc (P.wave1, v.ph1, inc1, pwA);
            const float o2 = sy::osc (P.wave2, v.ph2, std::min (0.45f, dt2), pw2);
            float subO = 0.f;
            if (subA > 0.0005f) { const float phs = v.ph1 * 0.5f + (v.subFlip ? 0.5f : 0.f); subO = sy::osc (1, phs, inc1 * 0.5f, 0.5f); }
            v.prev2 = o2;
            float x = o1 * lv1 + o2 * l2 + subO * subA * 0.8f + o1 * o2 * ringA * 1.4f + rnd() * noiseA * 0.7f;
            if (foldA > 0.001f) { const float xm = 0.5f * (x + v.xPrev); v.xPrev = x; x = 0.5f * (sy::fold (xm, foldA, foldS) + sy::fold (x, foldA, foldS)); }   // wavefolder (2 points : un peu moins de repliement)
            else v.xPrev = x;
            const float hp = v.hp.run (x * 0.7f, gH, kH);
            const float dx = hp + (sy::softClip (hp * satIn) - hp) * std::min (1.f, sat * 4.f);   // a zero : propre
            const float g = (P.pmFilt && pmOn) ? std::min (gMax, gL * std::exp2 (pm * 3.f)) : gL;
            float lp;
            if (P.ftype == 1)      lp = v.lad.run (dx * 1.2f, g, kLad) * (1.f + 1.0f * res);
            else if (P.ftype == 2) lp = v.pro.run (dx, g, kPro, sat) * 1.1f;
            else                   lp = v.sem.run (dx, g, kSem, morph, sat) * 1.15f;
            lp *= satOut;
            const float a = v.aenv.tick (aA, aD, P.aS, aR, fs);
            const float y = lp * a * level * v.gainV * (0.35f + 0.65f * v.vel) * 0.78f;
            L[i] += y * gl; R[i] += y * gr;
        }
        if (v.aenv.st == sy::Env::Idle) v.active = false;
    }
};
} // namespace zephyr
