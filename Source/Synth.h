// Zephyr synth engine : 12 voix (base Orage v20b) : 2 VCO (saw / pulse / tri / sine) + sub, hard sync, FM, ring, derive analogique (DRIFT),
// poly-mod facon Prophet, filtres circuit MS-10 / Ladder CEM3320 / SEM + PROPHET (SSM2040), 4 LFO, 8 macros, matrice. Aucune dependance JUCE.
#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace zephyr
{
constexpr int kVoices = 12, kUni = 8, kLfos = 4, kMacros = 8, kSlots = 16, kLanes = 8;
enum Assign { AsPoly, AsMono, AsUnison, AsShare, AsChord };
enum ModSrc { MsNone, MsLfo1, MsLfo2, MsLfo3, MsLfo4, MsMac1, MsMac2, MsMac3, MsMac4, MsEnv, MsVel, MsWheel, MsAmpEnv, MsKey, MsRandom, MsChanAT, MsPolyAT, MsMac5, MsMac6, MsMac7, MsMac8, MsArp1, MsArp2, MsArp3, MsArp4, MsArp5, MsArp6, MsArp7, MsArp8, kNumModSrc };
inline const float kFmRatios[18] { 1.f, 0.5f, 2.f / 3.f, 1.f, 1.5f, 2.f, 3.f, 4.f, 5.f, 7.f, 3.5f, 1.41421356f, 2.5f, 4.5f, 6.f, 8.f, 2.7568f, 5.4f };
inline int macroSrc (int i) { return i < 4 ? MsMac1 + i : MsMac5 + (i - 4); }
inline int laneSrc (int i)  { return i < 4 ? MsArp1 + i : MsArp5 + (i - 4); }
// ---- matrice : SENS de chaque routage. 0 AVANT (l'anneau part du potard vers le haut), 1 ARRIERE (vers le bas), 2 LES DEUX (anneau symetrique autour du potard).
//      Le son suit exactement l'anneau : la valeur reste dans la zone coloree, quelle que soit la source.
inline bool srcBipolar (int s) { return (s >= MsLfo1 && s <= MsLfo4) || s == MsKey || s == MsRandom; }     // sources qui vont de -1 a +1 (les autres : 0 a 1)
inline float slotMod (int src, float v, float amt, int dir)                                                // contribution signee, en fraction de la course de la destination
{
    const float a = std::fabs (amt);
    const bool bi = srcBipolar (src);
    const float u = bi ? 0.5f * (v + 1.f) : v;                                                              // source ramenee a 0..1
    if (dir == 1) return -a * u;
    if (dir == 2) return a * (bi ? v : 2.f * u - 1.f);
    return a * u;
}
enum ModDst { MdNone, MdPitch, MdPitch2, MdPw1, MdPw2, MdFm, MdRing, MdMix, MdHpf, MdLpf, MdReso, MdLevel, MdPan,
              MdFold, MdFxPower, MdFxClip, MdFxUnc, MdFxHpf, MdFxLpf, MdFxPeak, MdFxDestroy, MdFxRate, MdFxSpring, MdFxMix, MdFxChaos, MdBbdTime, MdBbdFb, MdBbdMix,
              MdRevMix, MdRevDecay, MdRevDamp,
              MdNoise, MdHpfReso, MdDrive, MdMorph, MdFltEnv, MdFoldSym, MdLvl1,
              MdStutOn, MdStutMode, MdStutDiv, MdStutFeel, MdStutLen, MdStutDecay, MdStutPitch, MdStutMix,
              MdAmpA, MdAmpD, MdAmpS, MdAmpR, MdFltA, MdFltD, MdFltS, MdFltR, MdGlide, MdKeyTrack,                              // voix : enveloppes, portamento, key tracking
              MdRevSize, MdRevPre, MdRevShimmer, MdRevWidth, MdBbdTone, MdBbdWobble, MdSprDecay, MdCompAmt, MdGateThresh, MdOctAmt, MdColLow, MdColMid, MdColHigh,   // effets (entrees 25..37 du moteur)
              MdArpRate, MdArpGate, MdArpSwing, MdArpChance, MdLfo1Rate, MdLfo2Rate, MdLfo3Rate, MdLfo4Rate,                // globaux du synthe (entrees 38..45)
              MdSub, MdPolyMod, MdDrift,                                                                                  // Zephyr : sub, poly-mod, derive
              kNumModDst };
// Zephyr : les entrees FX du moteur d'Orage portent d'autres modules (meme numerotation, autres noms)
constexpr int MdSatDrive = MdFxPower, MdWideWidth = MdFxClip, MdPhMix = MdFxUnc, MdPhRate = MdFxDestroy, MdEnsDepth = MdFxRate, MdEnsMix = MdFxSpring,
              MdPhFb = MdFxChaos, MdDlyTime = MdBbdTime, MdDlyFb = MdBbdFb, MdDlyMix = MdBbdMix, MdDlyTone = MdBbdTone, MdDlyMod = MdBbdWobble,
              MdEnsSpeed = MdSprDecay, MdWideSpread = MdGateThresh, MdSatTone = MdOctAmt, MdEqLow = MdColLow, MdEqMid = MdColMid, MdEqAir = MdColHigh;
// noms de Zephyr v1 (presets d'usine)
constexpr int MdFxDrive = MdFxPower, MdFxWidth = MdFxClip, MdFxPhMix = MdFxUnc, MdFxPhRate = MdFxDestroy, MdFxEnsDepth = MdFxRate, MdFxEnsMix = MdFxSpring, MdFxPhFb = MdFxChaos, MdSat = MdDrive;
// course native du potard couverte par une quantite de 100 % dans le moteur (le moteur applique 0,5 / 0,45 / 0,6 sur ces destinations) :
// la matrice divise par ce facteur, donc 100 % = tour COMPLET du potard (l'anneau et le son sont d'accord)
inline float modNative (int d)
{
    if (d == MdHpf || d == MdLpf || d == MdReso || d == MdHpfReso || d == MdMorph || d == MdFltEnv) return 0.5f;
    if (d == MdPw1 || d == MdPw2) return 0.45f;
    if (d == MdColLow || d == MdColMid || d == MdColHigh) return 0.5f;
    if ((d >= MdFxPower && d <= MdRevDamp) || (d >= MdRevSize && d <= MdOctAmt)) return 0.6f;
    return 1.f;
}
constexpr int kFxMod = 46;                // destinations FX : MdFxPower .. MdRevDamp (les 10 premieres = les entrees CV du moteur d'effets)
// destinations du raccourci ENV 2 (LINK allume) : 0 aucune
inline constexpr int kEnv2Dst[] { MdNone, MdPitch2, MdPitch, MdFm, MdPw1, MdFold, MdMix, MdRing };
inline const char* const kEnv2DstNames[] { "OFF", "VCO 2 PITCH", "VCO 1+2 PITCH", "FM", "PWM", "WAVEFOLD", "VCO 2 LEVEL", "RING" };
constexpr int kNumEnv2Dst = (int) (sizeof (kEnv2Dst) / sizeof (kEnv2Dst[0]));
inline const char* const kModSrcNames[kNumModSrc] { "-", "LFO 1", "LFO 2", "LFO 3", "LFO 4", "MACRO 1", "MACRO 2", "MACRO 3", "MACRO 4", "ENV FLT", "VELOCITY", "MOD WHEEL", "ENV AMP", "KEY TRACK", "RANDOM", "AFTERTOUCH", "POLY AT", "MACRO 5", "MACRO 6", "MACRO 7", "MACRO 8", "ARP LANE 1", "ARP LANE 2", "ARP LANE 3", "ARP LANE 4", "ARP LANE 5", "ARP LANE 6", "ARP LANE 7", "ARP LANE 8" };
inline const char* const kModDstNames[kNumModDst] { "-", "PITCH", "PITCH VCO2", "PWM VCO1", "PWM VCO2", "FM", "RING", "LEVEL VCO2", "HPF", "CUTOFF", "RESO", "LEVEL", "PAN",
    "FOLD", "SAT DRIVE", "WIDE WIDTH", "PHASER MIX", "FX HPF", "FX LPF", "FX PEAK", "PHASER RATE", "ENS DEPTH", "ENSEMBLE MIX", "FX DRY/WET", "PHASER FDBK", "DELAY TIME", "DELAY FEEDBK", "DELAY MIX",
    "REV MIX", "REV DECAY", "REV DAMP", "NOISE", "HPF RESO", "VCF SATURATION", "SEM MORPH", "FLT ENV AMT", "FOLD SYM", "LEVEL VCO1",
    "STUT ENGAGE", "STUT MODE", "STUT DIV", "STUT FEEL", "STUT LENGTH", "STUT DECAY", "STUT PITCH", "STUT MIX",
    "AMP ATTACK", "AMP DECAY", "AMP SUSTAIN", "AMP RELEASE", "FLT ATTACK", "FLT DECAY", "FLT SUSTAIN", "FLT RELEASE", "PORTAMENTO", "KEY TRACK",
    "REV SIZE", "REV PRE-DELAY", "REV SHIMMER", "REV WIDTH", "DELAY TONE", "DELAY MOD", "ENS SPEED", "COMP AMOUNT", "WIDE SPREAD", "SAT TONE", "EQ LOW", "EQ MID", "EQ AIR",
    "ARP RATE", "ARP GATE", "ARP SWING", "ARP CHANCE", "LFO 1 RATE", "LFO 2 RATE", "LFO 3 RATE", "LFO 4 RATE", "SUB LEVEL", "POLY-MOD", "DRIFT" };
inline const char* const kLfoDivNames[14] { "Free", "4 bar", "2 bar", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/4.", "1/8.", "1/16.", "1/4T", "1/8T", "1/16T" };
inline const float kLfoDivBeats[14] { 0.f, 16.f, 8.f, 4.f, 2.f, 1.f, 0.5f, 0.25f, 1.5f, 0.75f, 0.375f, 2.f / 3.f, 1.f / 3.f, 1.f / 6.f };

struct SynthParams
{
    int wave1 = 0, wave2 = 0;                 // 0 saw, 1 pulse, 2 triangle, 3 sinus
    int oct1 = 0, oct2 = 0;                   // -2..+2
    float tune2 = 0.f, fine2 = 0.f;           // demi-tons, cents
    float pw1 = 0.5f, pw2 = 0.5f;
    int sync = 0;                             // hard sync : VCO2 recale par VCO1
    float fm = 0.f, ring = 0.f;               // 0..1
    float lvl1 = 0.8f, lvl2 = 0.f, noise = 0.f, sub = 0.f;
    float drift = 0.f;                        // derive analogique : desaccord lent, cutoff et enveloppes un peu differents d'une voix a l'autre
    float pmEnv = 0.f, pmOsc = 0.f;           // poly-mod (Prophet) : sources enveloppe de filtre et VCO 2
    int pmFreq = 0, pmPw = 0, pmFilt = 0;     // poly-mod : destinations frequence VCO 1, largeur VCO 1, filtre
    float glide = 0.f;
    float hpf = 0.f, hpfRes = 0.f, lpf = 0.7f, lpfRes = 0.25f, fltEnv = 0.3f, keytrack = 0.5f, drive = 0.2f;
    float aA = 0.01f, aD = 0.3f, aS = 0.8f, aR = 0.25f;
    float fA = 0.01f, fD = 0.35f, fS = 0.2f, fR = 0.25f;
    int assign = 0, hold = 0;                 // affectation des touches (Poly, Mono, Unison, Share, Chord) et HOLD
    int fmType = 0; float fmFb = 0.f, fmKey = 0.f;
    bool envLink = false; int env2Dst = 0; float env2Amt = 0.f;   // LINK : l'enveloppe FILTRE pilote aussi le volume, l'enveloppe AMP devient ENV 2 libre (raccourci DEST / AMOUNT)   // FM : 0 exponentielle (analogique) / 1 lineaire (modulation de phase, style DX) ; retour du modulateur sur lui-meme ; profondeur reduite dans l'aigu
    int fmDir = 0, fmRatio = 0, fmWave = 0;   // FM : sens (0 VCO2>VCO1, 1 VCO1>VCO2, 2 croisee), rapport de frequence (0 libre), onde du modulateur (0 = celle du VCO)
    float fmFine = 0.f, fmAtt = 0.f, fmDec = 0.4f, fmEnvAmt = 0.f, fmVel = 0.f, fmAT = 0.f;     // FM : desaccord fin, enveloppe de profondeur, vitesse, aftertouch
    float uniDetune = 0.3f;                   // ecart de desaccord des voix empilees
    int ftype = 0;                            // filtre passe-bas : 0 MS-10 (circuit), 1 Ladder CEM3320 (circuit), 2 SEM (circuit), 3 Prophet (SSM2040)
    int quality = 1;                          // 0 performance (filtres x2, folder/saturation x4), 1 qualite (x4/x4), 2 high fidelity (x8/x8)
    float vib = 0.f, vibWheel = 0.f, vibAT = 0.f;   // vibrato VCO1 : quantite de LFO1 sur la hauteur (+ molette, + aftertouch)
    float fltLfo = 0.f;                       // quantite de LFO2 sur le cutoff (LFO 1 est deja sur la hauteur du VCO 1) (potard du bloc filtre, comme sur l'Hydrasynth)
    float semMorph = 0.f;                     // SEM : LP -> notch -> HP
    float fold = 0.f, foldSym = 0.f;           // wavefolder : quantite, symetrie
    float master = 0.7f;
    struct Lfo { float rate = 0.307f, slew = 0.f, fade = 0.f; int wave = 0, sync = 0, trig = 0; } lfo[kLfos];     // wave : 0 sin, 1 tri, 2 saw, 3 carre, 4 S&H
    float macro[kMacros] {};
    struct Slot { int src = 0, dst = 0; float amt = 0.f; int dir = 0; } slot[kSlots];     // amt = quantite (0..1), dir = sens (voir slotMod)
    struct Lane { int dst = 0; float amt = 0.f, glide = 0.f; } lane[kLanes];      // pistes de modulation de l'arpegiateur : destination directe (sans passer par la matrice)
};

namespace sy
{
inline float blep (float t, float dt)
{
    if (t < dt) { t /= dt; return t + t - t * t - 1.f; }
    if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
    return 0.f;
}
inline float blamp (float t, float dt)                                     // integrale du blep : arrondit les angles du triangle (anti-repliement)
{
    if (t < dt) { t = t / dt - 1.f; return -(1.f / 3.f) * t * t * t; }
    if (t > 1.f - dt) { t = (t - 1.f) / dt + 1.f; return (1.f / 3.f) * t * t * t; }
    return 0.f;
}
inline float naive (int wave, float ph, float pw)                          // forme brute (sans correction) : sert a mesurer les sauts du hard sync
{
    switch (wave)
    {
        case 0: return 2.f * ph - 1.f;
        case 1: return ph < pw ? 1.f : -1.f;
        case 2: return 4.f * std::fabs (ph - 0.5f) - 1.f;
        default: return std::sin (6.2831853f * ph);
    }
}
// caractere analogique (SETTINGS > VCO MODEL) : rampe du VCO legerement courbee (condensateur qui se charge), sommets du triangle arrondis
inline float analogShape (int wave, float y, float curve)
{
    if (wave == 0) return y + curve * (1.f - y * y);                   // dent de scie : rampe bombee, extremites inchangees (le saut reste le meme)
    if (wave == 2) return (y - 0.05f * y * y * y) * (1.f / 0.95f);     // triangle : sommets arrondis
    return y;                                                           // carre (affaissement par la liaison capacitive, plus bas), sinus : purs
}
inline float osc (int wave, float ph, float dt, float pw)
{
    switch (wave)
    {
        case 0: return 2.f * ph - 1.f - blep (ph, dt);
        case 3: return std::sin (6.2831853f * ph);                              // sinus (FM propre, style DX)
        case 1: { float t2 = ph - pw; if (t2 < 0.f) t2 += 1.f; return (ph < pw ? 1.f : -1.f) + blep (ph, dt) - blep (t2, dt); }
        case 2: { float t2 = ph + 0.5f; if (t2 >= 1.f) t2 -= 1.f; return 4.f * std::fabs (ph - 0.5f) - 1.f + 4.f * dt * (blamp (t2, dt) - blamp (ph, dt)); }
        default: return std::sin (6.2831853f * ph);                     // sinus : pas d'harmonique, donc pas de repliement
    }
}
struct Env
{
    enum { Idle, Att, Dec, Sus, Rel } st = Idle;
    float v = 0.f;
    void on()  { st = Att; }
    void off() { if (st != Idle) st = Rel; }
    float tick (float a, float d, float s, float r, float fs)
    {
        auto coef = [fs] (float sec) { return 1.f - std::exp (-1.f / (juce_max (sec, 0.0005f) * fs)); };
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
    static float juce_max (float a, float b) { return a > b ? a : b; }
};
struct Svf
{
    float s1 = 0.f, s2 = 0.f;
    // renvoie lp, hp via references
    void run (float x, float g, float k, float& lp, float& hp)
    {
        const float a1 = 1.f / (1.f + g * (g + k)), a2 = g * a1, a3 = g * a2;
        const float v3 = x - s2, v1 = a1 * s1 + a2 * v3, v2 = s2 + a2 * s1 + a3 * v3;
        s1 = 2.f * v1 - s1;
        s1 = s1 / (1.f + 0.25f * s1 * s1 * 0.25f * 4.f);          // saturation douce de la bande passante : l'auto-oscillation se stabilise
        s2 = 2.f * v2 - s2;
        lp = v2; hp = x - k * v1 - v2;
    }
};
// ---- HPF MS20 : le pic de resonance reste libre (peu de saturation), donc il gonfle les basses quand le cutoff est bas
struct Hp
{
    float s1 = 0.f, s2 = 0.f;
    float run (float x, float g, float k)
    {
        const float a1 = 1.f / (1.f + g * (g + k)), a2 = g * a1, a3 = g * a2;
        const float v3 = x - s2, v1 = a1 * s1 + a2 * v3, v2 = s2 + a2 * s1 + a3 * v3;
        s1 = 2.f * v1 - s1;
        s1 = s1 / (1.f + 0.02f * s1 * s1);
        s2 = 2.f * v2 - s2;
        return x - k * v1 - v2;
    }
};
// ---- LPF MS20 : resonance qui crie (saturation asymetrique dans la boucle + emphase de la bande)
struct Lp
{
    float s1 = 0.f, s2 = 0.f, bp = 0.f;
    float run (float x, float g, float k, float res)
    {
        const float a1 = 1.f / (1.f + g * (g + k)), a2 = g * a1, a3 = g * a2;
        const float v3 = x - s2, v1 = a1 * s1 + a2 * v3, v2 = s2 + a2 * s1 + a3 * v3;
        s1 = 2.f * v1 - s1;
        const float wild = 0.10f + 0.9f * res * res;
        s1 = s1 > 0.f ? s1 / (1.f + 0.10f * s1 * s1) : s1 / (1.f + wild * 0.6f * s1 * s1);   // diode : les deux alternances ne saturent pas pareil
        s2 = 2.f * v2 - s2;
        // la bande passante est ajoutee a la sortie (le pic devient criard) ; elle est refiltree a ~2x le cutoff : les harmoniques de la saturation ne fuient plus dans les aigus quand le filtre est ferme
        const float g2 = std::min (0.9f, 2.f * g), G2 = g2 / (1.f + g2);
        bp += G2 * (v1 - bp);
        return v2 + res * res * 1.1f * bp;
    }
};
// ---- SEM : SVF 12 dB, resonance douce, morph LP -> notch -> HP
struct Sem
{
    float s1 = 0.f, s2 = 0.f;
    float run (float x, float g, float k, float morph)
    {
        const float a1 = 1.f / (1.f + g * (g + k)), a2 = g * a1, a3 = g * a2;
        const float v3 = x - s2, v1 = a1 * s1 + a2 * v3, v2 = s2 + a2 * s1 + a3 * v3;
        s1 = 2.f * v1 - s1;
        s1 = s1 / (1.f + 0.12f * s1 * s1);
        s2 = 2.f * v2 - s2;
        const float lp = v2, hp = x - k * v1 - v2;
        const float lg = std::min (1.f, std::max (0.f, 2.f * (1.f - morph))), hg = std::min (1.f, std::max (0.f, 2.f * morph));
        return lp * lg + hp * hg;
    }
};
// ---- Ladder 24 dB (4 poles TPT, retour instantane resolu, saturation a l'entree)
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
// tanh rapide (Pade 7/6) : erreur < 1e-4 sur +/-4.5, borne au-dela ; utilisee dans les etages surechantillonnes
inline float tanhFast (float x)
{
    if (x > 4.5f) return 1.f; if (x < -4.5f) return -1.f;
    const float x2 = x * x;
    return x * (135135.f + x2 * (17325.f + x2 * (378.f + x2))) / (135135.f + x2 * (62370.f + x2 * (3150.f + x2 * 28.f)));
}
// ---- MS-10 : modele du CIRCUIT du VCF Korg35 (schemas Korg MS-10 / Haible), valide sur des enregistrements d'un MS-10 de 1978
// deux etages RC a transconductance (OTA, tanh) 3,3 nF / 1 nF, attenuation d'entree 1/101, ampli de sortie 270k/4,7k (gain 58) limite par des diodes 1N4148,
// retour PEAK par 2,2k + 5,6k + 1 nF vers le noeud intermediaire. Integration trapezoidale + Newton 4x4 : le retour est implicite (pas de retard d'un echantillon).
inline double tanhSmall (double x)                                  // tanh : serie pour |x| < 0.5 (signal faible devant l'echelle de l'OTA), Pade rapide sinon
{
    if (std::fabs (x) < 0.5) { const double x2 = x * x; return x * (1.0 + x2 * (-1.0 / 3 + x2 * (2.0 / 15 + x2 * (-17.0 / 315 + x2 * (62.0 / 2835 - x2 * (1382.0 / 155925)))))); }
    return (double) tanhFast ((float) x);
}
inline double atanhFast (double z)                                  // atanh : serie pour |z| < 0.3 (le cas courant : signal faible devant l'echelle de l'OTA), logarithme sinon
{
    const double a = std::fabs (z);
    if (a < 0.15) { const double z2 = z * z; return z * (1.0 + z2 * (1.0 / 3 + z2 * (1.0 / 5 + z2 * (1.0 / 7)))); }
    if (a < 0.4) { const double z2 = z * z; return z * (1.0 + z2 * (1.0 / 3 + z2 * (1.0 / 5 + z2 * (1.0 / 7 + z2 * (1.0 / 9 + z2 * (1.0 / 11 + z2 * (1.0 / 13 + z2 * (1.0 / 15 + z2 * (1.0 / 17))))))))); }
    return 0.5 * std::log ((1.0 + z) / (1.0 - z));
}
struct Ms10Cst
{
    double Gc6 = 0, Gc3 = 0, Gc5 = 0, kdc = 0;
    void prepare (double fs) { const double T = 1.0 / fs; Gc6 = 2 * 3.3e-9 / T; Gc3 = 2 * 1e-9 / T; Gc5 = 2 * 1e-9 / T; kdc = 1.0 - std::exp (-6.2831853 * 3.4 / fs); }
};
struct Ms10Tab
{
    static constexpr int N = 34;
    double lRp[N], lOm[N], lRa[N], kF[N];
    Ms10Tab()
    {
        static const double kRp[N] = {1000,1258.93,1584.89,1995.26,2511.89,3162.28,3981.07,5011.87,6309.57,7943.28,10000,12589.3,15848.9,19952.6,25118.9,31622.8,39810.7,50118.7,63095.7,79432.8,100000,125893,158489,199526,251189,316228,398107,501187,630957,794328,1e+06,1.25893e+06,1.58489e+06,1.99526e+06};
        static const double kOm[N] = {124095,118186,102951,96000,80963.9,70034.1,60589.5,52329.5,44846.7,38201.2,32343,27211.2,22748.4,18899.1,15608.1,12819.6,10477,8525.03,6910.58,5583.98,4500.37,3619.51,2906.44,2331.28,1868.67,1497.49,1200.23,962.555,772.729,621.259,500.481,404.213,327.482,266.319};
        static const double kFt[N] = {1.9000,1.9000,1.7491,1.6235,1.5077,1.4054,1.3183,1.2463,1.1883,1.1424,1.1067,1.0794,1.0586,1.0431,1.0316,1.0231,1.0169,1.0124,1.0091,1.0068,1.0050,1.0037,1.0028,1.0021,1.0016,1.0012,1.0009,1.0006,1.0004,1.0003,1.0002,1.0001,1.0001,1.0000};
        for (int i = 0; i < N; ++i) { lRp[i] = std::log (kRp[N - 1 - i]); lOm[i] = std::log (kOm[N - 1 - i]); lRa[i] = std::log (kRp[i]); kF[i] = kFt[i]; }
    }
    static const Ms10Tab& get() { static const Ms10Tab t; return t; }
    double rpFor (double om) const                                  // resistance des etages pour une pulsation analogique (la frequence d'auto-oscillation = le cutoff)
    {
        const double l = std::log (std::max (om, 1.0));
        if (l <= lOm[0]) return std::exp (lRp[0] + (lOm[0] - l));
        if (l >= lOm[N - 1]) return std::exp (lRp[N - 1]);
        int i = 0; while (i < N - 2 && lOm[i + 1] < l) ++i;
        const double t = (l - lOm[i]) / (lOm[i + 1] - lOm[i]); return std::exp (lRp[i] + t * (lRp[i + 1] - lRp[i]));
    }
    double kFor (double rp) const                                   // compense la perte de gain de boucle dans les aigus
    {
        const double l = std::log (rp); if (l <= lRa[0]) return kF[0]; if (l >= lRa[N - 1]) return kF[N - 1];
        int i = 0; while (i < N - 2 && lRa[i + 1] < l) ++i;
        const double t = (l - lRa[i]) / (lRa[i + 1] - lRa[i]); return kF[i] + t * (kF[i + 1] - kF[i]);
    }
    static double potOf (double res)                                // reglage de resonance (0..1) -> position electrique du potard PEAK, calee sur les mesures (30 % : +2,5 dB, 60 % : +6 dB, 100 % : oscille)
    {
        static const double x[] = { 0.0, 0.15, 0.30, 0.45, 0.60, 0.75, 0.88, 1.0 }, y[] = { 0.0, 0.36, 0.47, 0.52, 0.57, 0.60, 0.64, 0.75 };
        if (res <= 0.0) return 0.0; if (res >= 1.0) return y[7];
        int i = 0; while (i < 6 && x[i + 1] < res) ++i;
        return y[i] + (res - x[i]) / (x[i + 1] - x[i]) * (y[i + 1] - y[i]);
    }
    void controls (double om, double res, const Ms10Cst& c, float& Gp, float& pk, float& Ge) const
    {
        const double Rp = rpFor (om), p = potOf (res), Rs = 2200.0 + 5600.0 + 10000.0 * p * (1.0 - p);
        Gp = (float) (1.0 / Rp); pk = (float) (0.2 * p * kFor (Rp)); Ge = (float) (1.0 / (Rs + 1.0 / c.Gc5));
    }
};
#ifdef MS10_COUNT
inline long g_ms10It = 0;
#endif
struct Ms10
{
    double Vm = 0, Vo = 0, Vf = 0, Vd = 0, Vd1 = 0, h6 = 0, h3 = 0, h5 = 0, vdc = 0; uint32_t rng = 2463534242u;
    void reset() { Vm = Vo = Vf = Vd = Vd1 = h6 = h3 = h5 = vdc = 0; }
    // Le systeme (4 inconnues : noeud intermediaire m, sortie o, sortie de l'ampli f, tension de diode d) se ramene a UNE inconnue d :
    //   f = d + Rd.Id(d) ; o = vdc + R63 (f/R64 + Id) ; m = o + Vs.atanh ((Gc3 o - h3) / (Gp Vs)) ; il reste l'equation du noeud m, resolue par Newton 1D.
    float run (float xin, float Gpf, float pkf, float Gef, const Ms10Cst& C)
    {
        constexpr double kSc = 0.4, att = 1.0 / 101.0, R63 = 4700.0, R64 = 270000.0, Is = 2e-8, nVt = 0.07, da = 0.3, Rd = 2200.0, Vs = 0.1, makeup = 1.15;
        const double Gp = Gpf, pk = pkf, Ge = Gef;
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;                                   // bruit minuscule : amorce l'auto-oscillation, comme le bruit du vrai circuit
        const double vi = xin * kSc * att + ((double) rng * (1.0 / 4294967296.0) - 0.5) * 2e-7;
        const double g5 = h5 / C.Gc5, GpVs = Gp * Vs, iGpVs = 1.0 / GpVs, kG = C.Gc3 / Gp, iVs = 1.0 / Vs, iNv = 1.0 / nVt;
        double d = 2.0 * Vd - Vd1, m = Vm, o = Vo, f = Vf;      // depart : extrapolation lineaire de la tension de diode
        for (int it = 0; it < 10; ++it)
        {
            const double ex = (double) expf ((float) std::min (d * iNv, 40.0)), en = 1.0 / ex;
            const double Id = Is * ((1 + da) * ex - (1 - da) * en), dId = Is * iNv * ((1 + da) * ex + (1 - da) * en);
            f = d + Rd * Id;                         const double f1 = 1.0 + Rd * dId;
            o = vdc + R63 * (f / R64 + Id);          const double o1 = R63 * (f1 / R64 + dId);
            const double I2 = C.Gc3 * o - h3;
            const double z = std::max (-0.999999, std::min (0.999999, I2 * iGpVs)), w = 1.0 - z * z;
            m = o + Vs * atanhFast (z);
            const double m1 = o1 * (1.0 + kG / w);
            const double t1 = tanhSmall ((vi - m) * iVs), I1 = GpVs * t1, dI1 = Gp * (1.0 - t1 * t1);
            const double F = I1 - I2 - C.Gc6 * m + h6 + Ge * (pk * (o + f) - m - g5);
            const double dF = -dI1 * m1 - C.Gc3 * o1 - C.Gc6 * m1 + Ge * (pk * (o1 + f1) - m1);
            double step = -F / dF; const double as = std::fabs (step); if (as > 0.1) step *= 0.1 / as;
            d += step;
#ifdef MS10_COUNT
            ++g_ms10It;
#endif
            if (as < 4e-7) break;
        }
        const double Va = o + f;
        if (! std::isfinite (Va)) { reset(); return 0.f; }
        const double iF = Ge * (pk * Va - m - g5);
        h6 = 2 * C.Gc6 * m - h6; h3 = 2 * C.Gc3 * o - h3; h5 += 2 * iF;
        Vm = m; Vo = o; Vf = f; Vd1 = Vd; Vd = d; vdc += (o - vdc) * C.kdc;
        return (float) (Va * (makeup / (att * (1.0 + R64 / R63) * kSc)));
    }
};
// ---- LADDER "CEM3320" : modele du circuit du filtre 4 poles CEM3320 (Sequential Pro-One) : 4 etages OTA (tanh) + condensateur, retour de la sortie par le VCA de resonance.
// Chaque etage : C dv/dt = Gm*Vs*tanh((entree - v)/Vs), Vs = 2*Vt = 52 mV ; le signal est ramene au niveau du mV par les resistances d'entree.
// Integration trapezoidale + Newton 4x4 (jacobienne bidiagonale + 1 terme de retour, resolue en O(n)) : le retour est implicite, la frequence d'auto-oscillation = le cutoff.
struct Cem3320
{
    float v[4] {}, t[4] {}; unsigned rs = 12345u;
    void reset() { for (int i = 0; i < 4; ++i) v[i] = t[i] = 0.f; }
    float run (float x, float g, float k)                       // g = tan(pi f/fs), k = gain du retour (auto-oscillation vers 4)
    {
        constexpr float sc = 1.0f;                              // niveau moteur -> unites de Vs (0,052 V par unite)
        rs = rs * 1664525u + 1013904223u;
        const float u = x * sc + (float) (int) (rs >> 8) * 1e-9f;     // bruit thermique infime : amorce l'auto-oscillation

        float w[4] = { v[0], v[1], v[2], v[3] }, s[4];
        for (int i = 0; i < 4; ++i) s[i] = v[i] + g * t[i];
        for (int it = 0; it < 12; ++it)
        {
            float a = u - k * w[3], al = 0.f, be = 0.f, A[3], B[3];
            for (int i = 0; i < 4; ++i)
            {
                const float tn = tanhFast (a - w[i]), d = 1.f - tn * tn, F = w[i] - g * tn - s[i], Jd = 1.f + g * d;
                if (i == 0) { al = -F / Jd; be = -g * d * k / Jd; }
                else        { al = (-F + g * d * al) / Jd; be = g * d * be / Jd; }
                if (i < 3) { A[i] = al; B[i] = be; }
                a = w[i];
            }
            float dl[4]; dl[3] = al / (1.f - be);
            float step = std::fabs (dl[3]);
            for (int i = 0; i < 3; ++i) { dl[i] = A[i] + B[i] * dl[3]; step = std::max (step, std::fabs (dl[i])); }
            const float lim = step > 1.f ? 1.f / step : 1.f;                // Newton amorti : jamais plus d'une unite de Vs par iteration (la tanh sature au-dela)
            for (int i = 0; i < 4; ++i) w[i] += dl[i] * lim;
            if (step < 2e-6f) break;
        }
        float a = u - k * w[3];
        for (int i = 0; i < 4; ++i) { t[i] = tanhFast (a - w[i]); v[i] = w[i]; a = w[i]; }
        if (! (std::fabs (v[3]) < 1e3f)) reset();
        return v[3] * (1.f / sc);
    }
};
// ---- SEM "circuit" : filtre a variables d'etat Oberheim SEM : melangeur A12 (entree - LP - amortissement*BP), deux integrateurs CA3080 (tanh) A13/A15 precedes de diviseurs,
// suiveurs Q39/Q40, BP en A14, LP en A16, amortissement fixe par le potard RES (50k) avec ecretage par les diodes D11/D12. Trapezoide + Newton 2x2 (retour implicite).
struct SemCkt
{
    float bp = 0.f, lp = 0.f, t1 = 0.f, t2 = 0.f;
    void reset() { bp = lp = t1 = t2 = 0.f; }
    // x entree, g = tan(pi f/fs) (a la cadence d'appel), d = amortissement petit signal (2 = sans resonance), morph LP -> notch -> HP
    // Le melangeur applique un amortissement FIXE (d0 = 2) ; le potard RES renvoie la bande passante en phase pour l'annuler en partie, a travers les diodes D11/D12 :
    // a fort niveau la regeneration s'ecrete, l'amortissement remonte vers d0 -> la resonance ne peut pas s'emballer.
    float run (float x, float g, float d, float morph)
    {
        constexpr float sc = 1.0f / 1.2f;                       // niveau moteur -> echelle des OTA
        constexpr float il = 1.f / 1.6f;                        // seuil des diodes D11/D12 (en echelle OTA)
        constexpr float d0 = 2.f;
        const float q = d0 - d;                                 // regeneration (0 = pas de resonance)
        const float xn = x * sc;
        const float s1 = bp + g * t1, s2 = lp + g * t2;
        float b = bp, l = lp, hp = 0.f;
        for (int it = 0; it < 12; ++it)
        {
            const float tl = tanhFast (b * il), Lb = tl / il, dL = 1.f - tl * tl;
            hp = xn - l - d0 * b + q * Lb;
            const float ta = tanhFast (hp), tb = tanhFast (b), ea = 1.f - ta * ta, eb = 1.f - tb * tb;
            const float F1 = b - g * ta - s1, F2 = l - g * tb - s2;
            const float a11 = 1.f + g * ea * (d0 - q * dL), a12 = g * ea, a21 = -g * eb, a22 = 1.f;
            const float det = a11 * a22 - a12 * a21;
            float db = (-F1 * a22 + F2 * a12) / det, dl = (-a11 * F2 + a21 * F1) / det;
            const float step = std::max (std::fabs (db), std::fabs (dl)), lim = step > 1.f ? 1.f / step : 1.f;      // Newton amorti
            b += db * lim; l += dl * lim;
            if (step < 2e-6f) break;
        }
        const float tl = tanhFast (b * il);
        hp = xn - l - d0 * b + q * (tl / il);
        t1 = tanhFast (hp); t2 = tanhFast (b); bp = b; lp = l;
        if (! (std::fabs (lp) < 1e3f && std::fabs (bp) < 1e3f)) reset();
        const float lg = std::min (1.f, std::max (0.f, 2.f * (1.f - morph))), hg = std::min (1.f, std::max (0.f, 2.f * morph));
        return (l * lg + hp * hg) * (1.f / sc);
    }
};
// ---- PROPHET "SSM2040" : modele du filtre 4 poles des Prophet-5 rev 1 / rev 2 (SSM2040). Meme famille que le CEM3320 (4 cellules a transconductance
// chargeant chacune un condensateur, tension de sortie suivie), mais : le signal attaque les cellules plus fort (elles saturent plus tot : rond, cremeux),
// le retour de resonance passe par un VCA qui s'ecrete lui-meme (la resonance chante sans crier) et une partie du signal d'entree est remontee avec
// la resonance (les graves restent). Trapeze + Newton 4x4 amorti (retour implicite, ecretage du retour inclus dans la jacobienne). Fait d'apres le schema et
// la theorie du circuit, PAS calibre sur un enregistrement (aucun disponible).
// ecretage de securite du bus : lineaire jusqu'a 0,9, puis arrondi vers 1,15 (n'agit que sur les depassements du limiteur)
inline float busClip (float x)
{
    const float a = std::fabs (x);
    if (a < 0.9f) return x;
    return (x < 0.f ? -1.f : 1.f) * (0.9f + 0.25f * std::tanh ((a - 0.9f) / 0.25f));
}
// loi du potard RESONANCE du filtre Prophet, calee sur un Prophet-6 (LPF) enregistre : presque pas de bosse jusqu'a 50 % (+2 dB),
// montee rapide entre 50 et 75 % (+21 dB), auto-oscillation tout en haut. Valeur = part du gain de retour maximal (4,04).
inline constexpr float kProResLaw[17] { 0.0000f, 0.0235f, 0.0499f, 0.0788f, 0.1100f, 0.1407f, 0.1726f, 0.2120f, 0.2650f, 0.3865f, 0.5783f, 0.7673f, 0.8800f, 0.9270f, 0.9651f, 0.9907f, 1.0000f };
inline float proResLaw (float r)
{
    const float x = std::min (1.f, std::max (0.f, r)) * 16.f; const int i = std::min (15, (int) x);
    return kProResLaw[i] + (kProResLaw[i + 1] - kProResLaw[i]) * (x - (float) i);
}
inline float proResInv (float m)                                  // inverse : reglage du potard qui donne la meme resonance qu'avant la v3 (presets, anciens projets)
{
    m = std::min (1.f, std::max (0.f, m));
    int i = 0; while (i < 15 && kProResLaw[i + 1] < m) ++i;
    const float d = kProResLaw[i + 1] - kProResLaw[i];
    return ((float) i + (d > 0.f ? (m - kProResLaw[i]) / d : 0.f)) / 16.f;
}
struct ProphetSsm
{
    float v[4] {}, t[4] {}; unsigned rs = 777u;
    void reset() { for (int i = 0; i < 4; ++i) v[i] = t[i] = 0.f; }
    float run (float x, float g, float k)                       // g = tan(pi f/fs) a la cadence d'appel, k = gain du retour (auto-oscillation vers 4)
    {
        constexpr float sc = 1.0f, fbS = 0.55f;                 // niveau d'attaque des cellules ; seuil d'ecretage du VCA de resonance
        rs = rs * 1664525u + 1013904223u;
        const float u = x * sc * (1.f + 0.12f * k) + (float) (int) (rs >> 8) * 1e-9f;     // compensation des graves + bruit infime (amorce l'auto-oscillation)
        float w[4] = { v[0], v[1], v[2], v[3] }, s[4];
        for (int i = 0; i < 4; ++i) s[i] = v[i] + g * t[i];
        for (int it = 0; it < 12; ++it)
        {
            const float tf = tanhFast (w[3] * fbS), fb = tf / fbS, fbd = 1.f - tf * tf;
            float a = u - k * fb, al = 0.f, be = 0.f, A[3], B[3];
            for (int i = 0; i < 4; ++i)
            {
                const float tn = tanhFast (a - w[i]), d = 1.f - tn * tn, F = w[i] - g * tn - s[i], Jd = 1.f + g * d;
                if (i == 0) { al = -F / Jd; be = -g * d * k * fbd / Jd; }
                else        { al = (-F + g * d * al) / Jd; be = g * d * be / Jd; }
                if (i < 3) { A[i] = al; B[i] = be; }
                a = w[i];
            }
            float dl[4]; dl[3] = al / (1.f - be);
            float step = std::fabs (dl[3]);
            for (int i = 0; i < 3; ++i) { dl[i] = A[i] + B[i] * dl[3]; step = std::max (step, std::fabs (dl[i])); }
            const float lim = step > 1.f ? 1.f / step : 1.f;
            for (int i = 0; i < 4; ++i) w[i] += dl[i] * lim;
            if (step < 2e-6f) break;
        }
        const float tf = tanhFast (w[3] * fbS);
        float a = u - k * tf / fbS;
        for (int i = 0; i < 4; ++i) { t[i] = tanhFast (a - w[i]); v[i] = w[i]; a = w[i]; }
        if (! (std::fabs (v[3]) < 1e3f)) reset();
        return v[3] * (1.f / sc);
    }
};
// ---- surechantillonnage x4 autour d'une non-linearite (2 etages demi-bande) : les harmoniques creees au-dessus de Nyquist sont filtrees au lieu de se replier
template <int L> struct HalfBand                                    // L impair avec (L-1)/2 impair (11, 15, 23...) : un tap sur deux est nul, on ne calcule que les autres
{
    float h[L] {}, xh[L] {}, zh[L] {};
    HalfBand()
    {
        const int c = (L - 1) / 2; double sum = 0;
        for (int k = 0; k < L; ++k)
        {
            const double t = (k - c) * 0.5, sinc = (k == c) ? 1.0 : std::sin (3.14159265358979 * t) / (3.14159265358979 * t);
            const double w = 0.42 - 0.5 * std::cos (6.283185307179586 * k / (L - 1)) + 0.08 * std::cos (2.0 * 6.283185307179586 * k / (L - 1));   // Blackman
            h[k] = (float) (0.5 * sinc * w); sum += h[k];
        }
        for (auto& v : h) v = (float) (v / sum);
    }
    void up (float x, float& y0, float& y1)                             // 1 echantillon -> 2
    {
        const int n = (L + 1) / 2;
        for (int k = n - 1; k > 0; --k) xh[k] = xh[k - 1];
        xh[0] = x;
        float a = 0.f;
        for (int j = 0; j < n; ++j) a += h[2 * j] * xh[j];
        y0 = 2.f * a; y1 = 2.f * h[(L - 1) / 2] * xh[(L - 3) / 4];
    }
    float down (float z0, float z1)                                     // 2 echantillons -> 1
    {
        for (int k = L - 1; k > 1; --k) zh[k] = zh[k - 2];
        zh[1] = z0; zh[0] = z1;
        float y = h[(L - 1) / 2] * zh[(L - 1) / 2];
        for (int k = 0; k < L; k += 2) y += h[k] * zh[k];
        return y;
    }
};
struct Over4
{
    HalfBand<23> a; HalfBand<11> b;
    template <typename F> float run (float x, F&& nl)
    {
        float u0, u1; a.up (x, u0, u1);
        float w0, w1, w2, w3; b.up (u0, w0, w1); b.up (u1, w2, w3);
        w0 = nl (w0); w1 = nl (w1); w2 = nl (w2); w3 = nl (w3);
        const float d0 = b.down (w0, w1), d1 = b.down (w2, w3);
        return a.down (d0, d1);
    }
};
// ---- surechantillonnage x2 d'un bloc A MEMOIRE (filtres circuit) : le pas d'integration est divise par 2, donc g reste < 1 meme cutoff ouvert a fond
// (le trapeze + Newton ne peut plus sonner a Nyquist ni diverger), et les harmoniques creees par les tanh au-dessus de Nyquist sont filtrees.
struct Over2
{
    HalfBand<23> a;
    template <typename F> float run (float x, F&& f)
    {
        float u0, u1; a.up (x, u0, u1);
        u0 = f (u0); u1 = f (u1);
        return a.down (u0, u1);
    }
};
// ---- surechantillonnage x2 / x4 / x8 au choix (mode qualite) : demi-bandes en cascade 23 -> 11 -> 11 taps
struct OverVar
{
    HalfBand<23> a; HalfBand<11> b, c;
    template <typename F> float run (float x, int fac, F&& nl)
    {
        float u0, u1; a.up (x, u0, u1);
        if (fac <= 2) { u0 = nl (u0); u1 = nl (u1); return a.down (u0, u1); }
        float w[4]; b.up (u0, w[0], w[1]); b.up (u1, w[2], w[3]);
        if (fac <= 4)
        {
            for (float& v : w) v = nl (v);
            const float d0 = b.down (w[0], w[1]), d1 = b.down (w[2], w[3]);
            return a.down (d0, d1);
        }
        float z[8]; for (int i = 0; i < 4; ++i) c.up (w[i], z[2 * i], z[2 * i + 1]);
        for (float& v : z) v = nl (v);
        for (int i = 0; i < 4; ++i) w[i] = c.down (z[2 * i], z[2 * i + 1]);
        const float d0 = b.down (w[0], w[1]), d1 = b.down (w[2], w[3]);
        return a.down (d0, d1);
    }
};
inline float gHalf (float g) { return g / (1.f + std::sqrt (1.f + g * g)); }      // tan(a) -> tan(a/2) : le meme cutoff a cadence double
inline float gHalfN (float g, int fac) { g = gHalf (g); if (fac >= 4) g = gHalf (g); if (fac >= 8) g = gHalf (g); return g; }
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
        fs = sampleRate; msC.prepare (fs); sy::Ms10Tab::get();
        for (auto& v : voices) v = Voice();
        for (int i = 0; i < kLfos; ++i) { gPhase[i] = 0.f; gSh[i] = 0.f; gCur[i] = 0.f; }
        counter = 0; seed = 12345u; busEnv = 0.f; dcL = dcR = 0.f;
        for (int i = 0; i < kVoices; ++i)                                   // chaque "carte voix" a ses petites differences, fixes (derive analogique)
        {
            vCut[i] = rnd(); vEnv[i] = rnd(); vOff1[i] = rnd(); vOff2[i] = rnd();
            vCrv1[i] = 0.06f * (1.f + 0.35f * rnd()); vCrv2[i] = 0.06f * (1.f + 0.35f * rnd());     // tolerances de chaque carte voix (VCO MODEL analogique)
            vPw1[i] = 0.008f * rnd(); vPw2[i] = 0.008f * rnd(); vLv1[i] = 1.f + 0.03f * rnd(); vLv2[i] = 1.f + 0.03f * rnd();
            walk1[i] = walk2[i] = 0.f; walkT1[i] = rnd(); walkT2[i] = rnd();
        }
    }
    void setParams (const SynthParams& p)
    {
        const int oldAssign = P.assign; const bool oldHold = P.hold != 0;
        P = p; Tg = p;
        if (! smInit) { for (int i = 0; i < kSmN; ++i) sm[i] = sm1[i] = p.*kSm[i]; smInit = true; }
        if (P.assign != oldAssign) remapAssign (oldAssign);                 // changement de mode note tenue : le son continue dans le nouveau mode (plus de coupure)
        else if (oldHold && ! P.hold) dropLatched();
    }
    void setChord (const int* iv, int n) { chordN = std::max (0, std::min (6, n)); for (int i = 0; i < chordN; ++i) chord[i] = iv[i]; }
    float lfoLive (int i) const { return lfoWave (gPhase[i], P.lfo[i].wave, gCur[i]); }   // valeur courante d'un LFO global (affichage des anneaux)
    float lfoHz (int i) const { return hz (i); }                       // cadence de l'arpegiateur = LFO 2
    void setHost (double bpmIn) { bpm = bpmIn > 20.0 ? (float) bpmIn : 120.f; }
    void setWheel (float w) { wheelT = w; }
    void setBend (float semis) { bend = semis; }
    void setChanAT (float a) { chanAT = a; }
    void setAtResponse (float tauSec) { atTau = std::max (0.0005f, tauSec); }  // reactivite de l'aftertouch : constante de temps de chacun des 2 lisseurs
    void setNoteSlide (int note, float a) { for (auto& v : voices) if (v.active && v.note == note) v.slide = a; }        // MPE : slide (CC74) -> agit comme une molette par note
    void setPolyAT (int note, float a) { for (auto& v : voices) if (v.active && v.note == note) v.pat = a; }
    // MPE (Hydrasynth en mode MPE) : chaque note a sa pression et son pitch bend ; la source POLY AT suit alors la pression de SA note
    void setPerNoteAT (bool on) { perNoteAT = on; }
    void setPhaseFree (bool on) { phaseFree = on; }
    void setVcoAnalog (bool on) { vcoAnalog = on; }          // SETTINGS > VCO MODEL : analogique (rampe courbee, liaison capacitive, tolerances) ou numerique pur          // SETTINGS > VCO PHASE : oscillateurs libres (comme en analogique) ou recales a chaque note
    void setNoteBend (int note, float semis) { for (auto& v : voices) if (v.active && v.note == note) v.nBend = semis; }

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
    // arpegiateur en POLY (facon Mono/Poly) : un "VCO" fixe par pas (rotation sur 4) ; chaque VCO reagit differemment a la FM
    void noteOnSlot (int note, float vel, int slot)
    {
        if (slot < 0 || slot >= kVoices) return;
        static const float kFm[4] { 0.25f, 1.f, 0.65f, 1.4f };
        if (auto* v = startVoice (note, vel, 0.f, 0.f, 1.f, slot, true)) v->fmK = kFm[slot & 3];
    }
    void noteOffSlot (int slot, int note) { if (slot >= 0 && slot < kVoices && voices[slot].active && voices[slot].gated && voices[slot].note == note) release (voices[slot]); }
    void setLane (int i, float v) { laneT[i] = v; }
    const float* fxMod() const { return fxOut; }                  // modulations globales des effets (matrice : destinations FX/BBD)
    int activeVoices() const { int n = 0; for (auto& v : voices) n += v.active ? 1 : 0; return n; }

    void process (float* L, float* R, int n)
    {
        for (int i = 0; i < n; ++i) { L[i] = 0.f; R[i] = 0.f; }
        // LFO globaux (voix sans retrig)
        const float lfoHz[kLfos] = { hz (0) * std::pow (2.f, 3.f * fxOut[42]), hz (1) * std::pow (2.f, 3.f * fxOut[43]), hz (2) * std::pow (2.f, 3.f * fxOut[44]), hz (3) * std::pow (2.f, 3.f * fxOut[45]) };   // vitesse des LFO modulable (matrice, +/-3 octaves)
        {   // LFO rapides : la modulation est echantillonnee plus finement (16 -> 2 echantillons) pour qu'un LFO a quelques centaines de Hz reste une vraie onde et non un escalier
            float fastest = 0.f;
            for (int i = 0; i < kLfos; ++i)
            {
                bool used = (i == 0 && P.vib + P.vibWheel + P.vibAT > 0.f) || (i == 1 && P.fltLfo != 0.f);
                for (const auto& sl : P.slot) if (sl.src == MsLfo1 + i && sl.dst > 0 && sl.amt != 0.f) used = true;
                if (used) fastest = std::max (fastest, lfoHz[i]);
            }
            kCtl = fastest > 700.f ? 2 : (fastest > 350.f ? 4 : (fastest > 160.f ? 8 : 16));
        }
        for (int pos = 0; pos < n; pos += kCtl)
        {
            const int len = std::min (kCtl, n - pos);
            const float dt = (float) len / fs;
            {   // reglages continus lisses (~20 ms) : un reglage qui saute d'un bloc hote a l'autre devient une rampe, plus d'escalier audible
                const float sc = 1.f - std::exp (-dt / 0.0055f);                                   // deux lisseurs en cascade : une marche (souris, CC MIDI a 7 bits) devient une courbe en S sans cassure
                for (int i = 0; i < kSmN; ++i) { sm1[i] += (Tg.*kSm[i] - sm1[i]) * sc; sm[i] += (sm1[i] - sm[i]) * sc; P.*kSm[i] = sm[i]; }
            }
            {   // aftertouch de canal et molette : lisses a la cadence de controle (avant : une marche par bloc hote -> gresillement des filtres)
                atSc = 1.f - std::exp (-dt / atTau);
                chanAT1 += (chanAT - chanAT1) * atSc; chanATs += (chanAT1 - chanATs) * atSc;
                const float wc = 1.f - std::exp (-dt / 0.006f);
                wheel1 += (wheelT - wheel1) * wc; wheel += (wheel1 - wheel) * wc;
            }
            for (int i = 0; i < kLfos; ++i)
            {
                gPhase[i] += lfoHz[i] * dt;
                if (gPhase[i] >= 1.f) { gPhase[i] -= std::floor (gPhase[i]); gSh[i] = rnd(); }
                gCur[i] += (gSh[i] - gCur[i]) * slewCoef (P.lfo[i].slew, lfoHz[i], dt);          // S&H : marches lissees par SLEW
            }
            {   // derive lente de chaque carte voix (marche aleatoire lissee, ~1 s)
                const float wc = 1.f - std::exp (-dt / 0.9f);
                for (int i = 0; i < kVoices; ++i)
                {
                    if (rndU() < dt * 0.7f) walkT1[i] = rnd();
                    if (rndU() < dt * 0.7f) walkT2[i] = rnd();
                    walk1[i] += (walkT1[i] - walk1[i]) * wc;
                    walk2[i] += (walkT2[i] - walk2[i]) * wc;
                }
            }
            for (int i = 0; i < kLanes; ++i)                                      // pistes de l'arpegiateur : marches ou glissando selon GLIDE
            {
                const float sec = P.lane[i].glide * 0.6f;
                laneC[i] += (laneT[i] - laneC[i]) * (sec < 0.002f ? 1.f : 1.f - std::exp (-dt / (sec * 0.35f)));
            }
            for (auto& v : voices)
                if (v.active) renderVoice (v, L + pos, R + pos, len, lfoHz, dt);
        }
        const float dcC = 1.f - std::exp (-6.2831853f * 6.f / fs);                              // coupe-continu a 6 Hz : la saturation asymetrique ne laisse plus de composante continue en sortie
        // bus de sortie : limiteur de crete (gain lisse) au lieu d'une saturation instantanee. Une tanh sur la SOMME des voix
        // fait intermoduler les notes d'un accord entre elles (les filtres "s'entrechoquent") ; un gain qui varie lentement, non.
        const float aAtt = 1.f - std::exp (-1.f / (0.0008f * fs)), aRel = 1.f - std::exp (-1.f / (0.18f * fs));
        for (int i = 0; i < n; ++i)
        {
            float l = L[i] * P.master * kBusGain, r = R[i] * P.master * kBusGain;
            const float pk = std::max (std::fabs (l), std::fabs (r));
            busEnv += (pk > busEnv ? aAtt : aRel) * (pk - busEnv);
            const float gr = busEnv > kBusThr ? kBusThr / busEnv : 1.f;
            l = sy::busClip (l * gr); r = sy::busClip (r * gr);
            dcL += dcC * (l - dcL); dcR += dcC * (r - dcR);
            L[i] = l - dcL; R[i] = r - dcR;
        }
        computeFxMod();
    }

private:
    int kCtl = 16;                                                       // bloc de controle : 16 echantillons, raccourci a 8/4/2 quand un LFO utilise est rapide (LFO audio, jusqu'a 1 kHz)
    static constexpr float SynthParams::* kSm[] = { &SynthParams::lpf, &SynthParams::lpfRes, &SynthParams::hpf, &SynthParams::hpfRes, &SynthParams::drive, &SynthParams::semMorph, &SynthParams::fold, &SynthParams::foldSym,
        &SynthParams::fm, &SynthParams::ring, &SynthParams::lvl1, &SynthParams::lvl2, &SynthParams::noise, &SynthParams::pw1, &SynthParams::pw2, &SynthParams::fltEnv, &SynthParams::keytrack, &SynthParams::master,
        &SynthParams::sub, &SynthParams::drift, &SynthParams::pmEnv, &SynthParams::pmOsc };
    static constexpr float kUniGain = 0.42f;
    static constexpr int kSmN = (int) (sizeof (kSm) / sizeof (kSm[0]));
    sy::Ms10Cst msC;
    static constexpr float kCemTune = 0.075f;                  // l'OTA sature fait baisser la frequence d'auto-oscillation : compensation
    static constexpr float kProTune = 0.075f;
    // (Prophet : sortie compensee pour rester au niveau des autres filtres)
    SynthParams Tg; float sm[kSmN] {}, sm1[kSmN] {}; float dcL = 0.f, dcR = 0.f, busEnv = 0.f; bool smInit = false;
    static constexpr float kBusThr = 0.85f;                   // seuil du limiteur de bus
    static constexpr float kBusGain = 0.35f;                  // reserve pour 12 voix : un accord dense reste lineaire (avant : 0,7 dans une tanh)
    struct Voice
    {
        bool active = false; int note = 60; float vel = 1.f; uint64_t age = 0;
        bool gated = false, raw = false; int slot = -1; float uniCents = 0.f, uniPan = 0.f, gainV = 1.f;
        sy::Env aenv, fenv, e2; float pitch = 60.f, ph1 = 0.f, ph2 = 0.f, prev1 = 0.f, prev2 = 0.f, fmT = 0.f;
        sy::Hp hp; sy::Ms10 ms; sy::Cem3320 cem; sy::SemCkt sck; sy::ProphetSsm pro; bool subFlip = false; float fmK = 1.f; float pat = 0.f, pat1 = 0.f, patS = 0.f, nBend = 0.f, nBendS = 0.f, slide = 0.f, rndv = 0.f; float xPrev = 0.f, drvLp = 0.f; sy::OverVar osF, osD, osV; float dl1 = 0.f, dl2 = 0.f, dls = 0.f, ac1x = 0.f, ac1y = 0.f, ac2x = 0.f, ac2y = 0.f; sy::HalfBand<23> dsA; sy::HalfBand<15> dsB; bool cInit = false; float c0[20] {}; float lph[kLfos] {}, lfade[kLfos] {}, lsh[kLfos] {}, lcur[kLfos] {}; float envM[9] {};     // envM : modulation des enveloppes (bloc precedent)
    };
    static constexpr int kKeys = 16;
    struct Key { int note; float vel; bool phys; uint64_t t; };
    Key keys[kKeys];
    int nKeys = 0, chordN = 4, chord[6] { 0, 4, 7, 11, 0, 0 };
    uint64_t stamp = 0;
    Voice voices[kVoices];
    SynthParams P;
    bool perNoteAT = false, phaseFree = true, vcoAnalog = true;
    float fs = 44100.f, bpm = 120.f, wheel = 0.f, bend = 0.f, gPhase[kLfos] {}, gSh[kLfos] {}, gCur[kLfos] {};
    float laneT[kLanes] {}, laneC[kLanes] {};
    float vCrv1[kVoices] {}, vCrv2[kVoices] {}, vPw1[kVoices] {}, vPw2[kVoices] {}, vLv1[kVoices] {}, vLv2[kVoices] {};
    float vCut[kVoices] {}, vEnv[kVoices] {}, vOff1[kVoices] {}, vOff2[kVoices] {}, walk1[kVoices] {}, walk2[kVoices] {}, walkT1[kVoices] {}, walkT2[kVoices] {};
    uint64_t counter = 0; int lastNote = -1; uint32_t seed = 12345u;
    float atTau = 0.012f, atSc = 1.f, chanAT1 = 0.f, chanATs = 0.f, wheelT = 0.f, wheel1 = 0.f;
    float lastVel = 0.8f, fxOut[kFxMod] {}, chanAT = 0.f, lastRnd = 0.f, lastKey = 0.f;

    int findKey (int note) const { for (int i = 0; i < nKeys; ++i) if (keys[i].note == note) return i; return -1; }
    int physCount() const { int c = 0; for (int i = 0; i < nKeys; ++i) c += keys[i].phys ? 1 : 0; return c; }
    float maxCents() const { return 45.f * std::pow (std::max (0.f, std::min (1.f, P.uniDetune)), 1.2f); }
    static void release (Voice& v) { v.aenv.off(); v.fenv.off(); v.e2.off(); v.gated = false; }
    // Bascule de mode (POLY / MONO / UNISON / SHARE / CHORD) sans couper les notes tenues : les voix en cours sont reprises par le nouveau mode,
    // celles qui deviennent inutiles partent en release naturelle (jamais de coupure sechement).
    void remapAssign (int oldA)
    {
        if (P.assign == AsPoly)
        {
            bool used[kKeys] {};
            for (auto& v : voices)
            {
                if (! (v.active && v.gated) || v.raw) continue;
                int m = -1; for (int i = 0; i < nKeys; ++i) if (! used[i] && keys[i].note == v.note) { m = i; break; }
                if (m >= 0) { used[m] = true; v.slot = -1; v.uniCents = 0.f; v.uniPan = 0.f; v.gainV = 1.f; }
                else release (v);
            }
            for (int i = 0; i < nKeys; ++i) if (! used[i]) startVoice (keys[i].note, keys[i].vel, 0.f, 0.f, 1.f, -1, false);
        }
        else
        {
            if (oldA == AsPoly) for (int i = 0; i < kVoices; ++i) { auto& v = voices[i]; if (v.active && v.gated && ! v.raw) v.slot = i; }   // les voix poly deviennent les voix de pile
            retarget();
        }
    }
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
        // voix reutilisee alors qu'elle sonne encore (vol de voix, MONO pendant le release) : comme sur un vrai synthe, elle garde son etat
        // (enveloppes repartant de leur niveau actuel, filtres, oscillateurs qui continuent) -> plus de clic ; sinon voix neuve
        const bool reuse = v->active;
        if (! reuse) *v = Voice();
        else { v->pat = v->pat1 = v->patS = 0.f; v->nBend = v->nBendS = 0.f; v->slide = 0.f; }
        v->active = true; v->gated = true; v->raw = raw; v->slot = slot; v->note = note; v->vel = vel; v->age = ++counter;
        v->uniCents = cents; v->uniPan = pan; v->gainV = gain;
        v->pitch = glide ? (float) lastNote : (float) note;
        if (phaseFree && ! reuse) { v->ph1 = rndU(); v->ph2 = rndU(); v->subFlip = rndU() < 0.5f; }   // oscillateurs libres : chaque note demarre ailleurs dans le cycle (sinon : toujours au meme point)
        v->aenv.on(); v->fenv.on(); v->e2.on(); lastVel = vel;
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

    static int fxIndex (int dst)                                           // destination de matrice -> entree du moteur d'effets (-1 : pas une destination FX)
    {
        if (dst >= MdFxPower && dst <= MdRevDamp) return dst - MdFxPower;
        if (dst >= MdStutOn && dst <= MdStutMix) return 17 + (dst - MdStutOn);
        if (dst >= MdRevSize && dst <= MdColHigh) return 25 + (dst - MdRevSize);
        if (dst >= MdArpRate && dst <= MdLfo4Rate) return 38 + (dst - MdArpRate);
        return -1;
    }
    void computeFxMod()                                                    // sources globales : LFO libres, macros, molette, enveloppe de filtre (voix la plus forte), velocite
    {
        float src[kNumModSrc] {};
        for (int i = 0; i < kLfos; ++i) src[MsLfo1 + i] = lfoWave (gPhase[i], P.lfo[i].wave, gCur[i]);
        for (int i = 0; i < kMacros; ++i) src[macroSrc (i)] = P.macro[i];
        for (int i = 0; i < kLanes; ++i) src[laneSrc (i)] = laneC[i];
        float e = 0.f; for (auto& v : voices) if (v.active) e = std::max (e, v.fenv.v);
        src[MsEnv] = e; src[MsVel] = lastVel; src[MsWheel] = wheel;
        float ae = 0.f, pa = 0.f; for (auto& v : voices) if (v.active) { ae = std::max (ae, P.envLink ? v.e2.v : v.aenv.v); pa = std::max (pa, v.patS); }
        src[MsAmpEnv] = ae; src[MsKey] = lastKey; src[MsRandom] = lastRnd; 
        src[MsChanAT] = chanATs; src[MsPolyAT] = std::max (pa, chanATs);
        float acc[kFxMod] {};
        for (const auto& sl : P.slot)
            if (sl.src > 0 && sl.amt != 0.f && fxIndex (sl.dst) >= 0) acc[fxIndex (sl.dst)] += slotMod (sl.src, src[sl.src], sl.amt, sl.dir) / modNative (sl.dst);
        for (int i = 0; i < kLanes; ++i)
            if (P.lane[i].amt != 0.f && fxIndex (P.lane[i].dst) >= 0) acc[fxIndex (P.lane[i].dst)] += laneC[i] * P.lane[i].amt / modNative (P.lane[i].dst);
        for (int i = 0; i < kFxMod; ++i) fxOut[i] = std::min (2.f, std::max (-2.f, acc[i]));
    }

    float rnd() { seed = seed * 1664525u + 1013904223u; return (float) (seed >> 8) / 8388608.f - 1.f; }
    float rndU() { return 0.5f * (rnd() + 1.f); }

    float hz (int i) const
    {
        const auto& l = P.lfo[i];
        if (l.sync > 0 && l.sync < 14) return bpm / 60.f / kLfoDivBeats[l.sync];
        return 0.05f * std::pow (20000.f, l.rate);                       // 0.05 .. 1000 Hz (v22c ; avant 300 Hz)
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
    void renderVoice (Voice& v, float* L, float* R, int len, const float* lfoHzv, float dtCtl)
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
        for (int i = 0; i < kLanes; ++i) src[laneSrc (i)] = laneC[i];
        const int vi = (int) (&v - voices);
        const float driftA = std::min (1.f, std::max (0.f, P.drift + v.envM[8]));          // derive (modulation du bloc precedent)
        const float envK = 1.f + 0.14f * driftA * vEnv[vi];                                 // chaque voix a des temps d'enveloppe un peu a elle
        auto tm = [envK] (float t, float m) { return t * envK * std::pow (4.f, m); };      // temps d'enveloppe : x4 / :4 a pleine modulation
        const float fA2 = tm (P.fA, v.envM[4]), fD2 = tm (P.fD, v.envM[5]), fS2 = std::min (1.f, std::max (0.f, P.fS + v.envM[6])), fR2 = tm (P.fR, v.envM[7]);
        const float fe = v.fenv.tick (fA2, fD2, fS2, fR2, fs / (float) kCtl);   // enveloppes cadencees au bloc
        // LINK : ENV 2 libre = l'enveloppe AMP (ses reglages et modulations), le volume suit l'enveloppe FILTRE
        const float e2v = P.envLink ? v.e2.tick (tm (P.aA, v.envM[0]), tm (P.aD, v.envM[1]), std::min (1.f, std::max (0.f, P.aS + v.envM[2])), tm (P.aR, v.envM[3]), fs / (float) kCtl) : v.aenv.v;
        const float wheelV = std::max (wheel, v.slide);
        src[MsEnv] = fe; src[MsVel] = v.vel; src[MsWheel] = wheelV;
        src[MsAmpEnv] = e2v; src[MsKey] = std::max (-1.f, std::min (1.f, (v.pitch - 60.f) / 48.f)); src[MsRandom] = v.rndv;
        v.pat1 += (v.pat - v.pat1) * atSc; v.patS += (v.pat1 - v.patS) * atSc;      // aftertouch polyphonique : 2 lisseurs (les pas de 1/127 ne craquent plus dans un filtre resonant)
        src[MsChanAT] = chanATs; src[MsPolyAT] = perNoteAT ? v.patS : std::max (v.patS, chanATs);
        v.nBendS += (v.nBend - v.nBendS) * atSc;
        float md[kNumModDst] {};
        for (const auto& s : P.slot)
            if (s.src > 0 && s.dst > 0 && s.amt != 0.f) md[s.dst] += slotMod (s.src, src[s.src], s.amt, s.dir) / modNative (s.dst);
        for (int i = 0; i < kLanes; ++i)
            if (P.lane[i].dst > 0 && P.lane[i].amt != 0.f) md[P.lane[i].dst] += laneC[i] * P.lane[i].amt / modNative (P.lane[i].dst);

        if (P.fltLfo != 0.f) md[MdLpf] += src[MsLfo2] * P.fltLfo / modNative (MdLpf);
        if (P.envLink && P.env2Dst > 0 && P.env2Dst < kNumEnv2Dst && P.env2Amt != 0.f)        // raccourci ENV 2 -> destination
        {
            const int d = kEnv2Dst[P.env2Dst];
            md[d] += e2v * P.env2Amt / modNative (d);
            if (d == MdPw1) md[MdPw2] += e2v * P.env2Amt / modNative (MdPw2);                    // PWM : les deux VCO
        }
        for (int k = 0; k < 8; ++k) v.envM[k] = md[MdAmpA + k];
        v.envM[8] = md[MdDrift];
        const float dc1 = driftA * (5.f * vOff1[vi] + 11.f * walk1[vi]), dc2 = driftA * (5.f * vOff2[vi] + 11.f * walk2[vi]);     // derive en cents
        const float aA2 = tm (P.aA, md[MdAmpA]), aD2 = tm (P.aD, md[MdAmpD]), aS2 = std::min (1.f, std::max (0.f, P.aS + md[MdAmpS])), aR2 = tm (P.aR, md[MdAmpR]);
        const float glideV = std::min (1.f, std::max (0.f, P.glide + md[MdGlide]));
        const float glideC = glideV > 0.001f ? 1.f - std::exp (-dtCtl / (glideV * 0.6f + 0.002f)) : 1.f;
        v.pitch += (((float) v.note) - v.pitch) * glideC;
        float vibD = P.vib + P.vibWheel * wheelV + P.vibAT * src[MsPolyAT];                // vibrato VCO1 : LFO 1, profondeur = potard + molette + aftertouch
        vibD = std::min (1.f, std::max (0.f, vibD));
        const float vibSt = vibD * vibD * 3.f * src[MsLfo1];                              // jusqu'a +/-3 demi-tons, courbe quadratique (reglage fin des petites profondeurs)
        const float n1 = v.pitch + 12.f * (float) P.oct1 + bend + v.nBendS + md[MdPitch] * 12.f + (v.uniCents + dc1) * 0.01f + vibSt;
        const float n2 = v.pitch + 12.f * (float) P.oct2 + P.tune2 + P.fine2 * 0.01f + bend + v.nBendS + md[MdPitch] * 12.f + md[MdPitch2] * 12.f + (v.uniCents + dc2) * 0.01f;
        // rapport de frequence entre les deux VCO (FM) : VCO2 suit VCO1 x rapport, la hauteur propre de VCO2 est ignoree
        // rapport FM : la hauteur du VCO 2 suit le VCO 1 selon le rapport, et OCTAVE / TUNE / FINE du VCO 2 se reglent toujours par-dessus
        const float n2r = P.fmRatio > 0 ? n1 + 12.f * std::log2 (kFmRatios[std::min (17, P.fmRatio)]) + 12.f * (float) P.oct2 + P.tune2 + P.fine2 * 0.01f + P.fmFine * 0.5f + md[MdPitch2] * 12.f : n2 + P.fmFine * 0.5f;
        const float f1 = 440.f * std::pow (2.f, (n1 - 69.f) / 12.f), f2 = 440.f * std::pow (2.f, (n2r - 69.f) / 12.f);
        const float pw1 = std::min (0.95f, std::max (0.05f, P.pw1 + md[MdPw1] * 0.45f));
        const float pw2 = std::min (0.95f, std::max (0.05f, P.pw2 + md[MdPw2] * 0.45f));
        const float noiseA = std::min (1.f, std::max (0.f, P.noise + md[MdNoise])), lv1 = std::min (1.f, std::max (0.f, P.lvl1 + md[MdLvl1]));
        // enveloppe de profondeur FM : montee (ATT) puis descente exponentielle (DEC) depuis le debut de la note
        v.fmT += dtCtl;
        const float fAtt = 0.001f * std::pow (1000.f, P.fmAtt), fDec = 0.03f * std::pow (100.f, P.fmDec);
        const float fEnv = v.fmT < fAtt ? v.fmT / fAtt : std::exp (-(v.fmT - fAtt) / fDec * 3.f);
        const float fmKeyS = 1.f / (1.f + P.fmKey * std::max (0.f, (v.pitch - 60.f) / 12.f) * 0.8f);   // profondeur FM reduite dans l'aigu (evite le grincement)
        const float fmA = v.fmK * std::min (1.f, std::max (0.f, P.fm + md[MdFm] + P.fmEnvAmt * fEnv + P.fmVel * v.vel + P.fmAT * v.patS)) * fmKeyS, ringA = std::min (1.f, std::max (0.f, P.ring + md[MdRing]));
        const float l2 = std::min (1.f, std::max (0.f, P.lvl2 + md[MdMix]));
        const float subA = std::min (1.f, std::max (0.f, P.sub + md[MdSub]));
        // poly-mod (Prophet) : enveloppe de filtre et VCO 2 vers la frequence / la largeur du VCO 1 et le filtre
        const float pmE = std::min (1.f, std::max (0.f, P.pmEnv + md[MdPolyMod])) * fe, pmO = std::min (1.f, std::max (0.f, P.pmOsc + md[MdPolyMod]));
        const bool pmOn = (P.pmFreq || P.pmPw || P.pmFilt) && (pmE > 0.0005f || pmO > 0.0005f);

        // ---- filtres
        const float kt = (v.pitch - 60.f) / 12.f * std::min (1.5f, std::max (0.f, P.keytrack + md[MdKeyTrack]));
        const float hpfHz = 25.f * std::pow (600.f, std::min (1.f, std::max (0.f, P.hpf + md[MdHpf] * 0.5f)));
        const float lpfOct = std::min (1.f, std::max (0.f, P.lpf + md[MdLpf] * 0.5f + driftA * 0.025f * vCut[vi]));
        float lpfHz = 25.f * std::pow (720.f, lpfOct) * std::pow (2.f, kt + fe * std::min (1.f, std::max (0.f, P.fltEnv + md[MdFltEnv] * 0.5f)) * 5.f);
        lpfHz = std::min (lpfHz, 0.45f * fs);
        const float res = std::min (1.f, std::max (0.f, P.lpfRes + md[MdReso] * 0.5f));
        const float hRes = std::min (1.f, std::max (0.f, P.hpfRes + md[MdHpfReso] * 0.5f));
        const float morph = std::min (1.f, std::max (0.f, P.semMorph + md[MdMorph] * 0.5f));
        const float gMaxL = std::tan (3.14159265f * 0.45f);
        const float gL = std::tan (3.14159265f * lpfHz / fs), gH = std::tan (3.14159265f * std::min (hpfHz, 0.45f * fs) / fs);
        const float kL = 2.f * std::pow (1.f - res, 1.35f) - 0.12f * res, kH = 2.f * std::pow (1.f - hRes, 1.5f) + 0.02f;
        const float kSem = 1.75f * std::pow (1.f - res, 1.3f) + 0.06f, kLad = 4.05f * res;
        const float foldA = std::min (1.f, std::max (0.f, P.fold + md[MdFold])), foldS = std::min (1.f, std::max (-1.f, P.foldSym + md[MdFoldSym]));
        const float drive = 1.f + std::min (1.f, std::max (0.f, P.drive + md[MdDrive])) * 5.f;
        const float drvC = 1.f - std::exp (-6.2831853f * 1200.f / fs), drvEmph = 4.5f * (drive - 1.f) / 5.f;      // emphase au-dessus de ~1,2 kHz, proportionnelle au drive

        const float level = std::min (2.f, std::max (0.f, 1.f + md[MdLevel]));
        const float pan = std::min (1.f, std::max (-1.f, md[MdPan] + v.uniPan));
        const float gl = std::sqrt (0.5f * (1.f - pan)), gr = std::sqrt (0.5f * (1.f + pan));
        const float dt1 = f1 / fs, dt2 = f2 / fs;
        const int facOsc = P.quality >= 2 ? 4 : 2;                                          // surechantillonnage des VCO
        const float acA = std::exp (-6.2831853f * 5.f / (fs * (float) facOsc));             // liaison capacitive des VCO analogiques (~5 Hz)
        const float dt1s = dt1 / (float) facOsc, dt2s = dt2 / (float) facOsc, noiseK = 0.7f * std::sqrt ((float) facOsc);   // le bruit garde son niveau apres decimation
        // valeurs de controle : calculees une fois par bloc de 16 echantillons, puis INTERPOLEES echantillon par echantillon (pas de marches)
        const int facFlt = P.quality <= 0 ? 2 : (P.quality == 1 ? 4 : 8), facFx = P.quality >= 2 ? 8 : 4;
        float c1[20] = { gL, gH, kL, kH, kSem, kLad, res, morph, drive, drvEmph, foldA, foldS, level, lv1, l2, noiseA, 0.f, 0.f, 0.f, 0.f };
        if (P.ftype == 0) sy::Ms10Tab::get().controls (2.0 * fs * (double) gL, (double) res, msC, c1[16], c1[17], c1[18]);       // MS-10 : resistance des etages, retour PEAK
        if (P.ftype == 1) c1[16] = 4.04f * res;
        else if (P.ftype == 3)                                                                                                    // Prophet : loi du Prophet-6
        {
            const float m = sy::proResLaw (res), xf = std::min (1.3f, lpfHz / 13000.f);
            c1[16] = 4.04f * m * (1.f - 0.09f * xf * xf * std::min (1.f, std::max (0.f, (1.f - m) / 0.12f)));   // le retour perd un peu de gain dans l'aigu (pic moins haut au-dessus de 7 kHz, comme le vrai) ; auto-oscillation intacte a fond
        }                                                                  // CEM3320 / SSM2040 : gain de retour (auto-oscillation au-dessus de 4)
        else if (P.ftype == 2) c1[16] = 1.9f * std::pow (1.f - res, 1.3f) + 0.03f;                                               // SEM : amortissement (potard RES)
        if (! v.cInit) { for (int q = 0; q < 20; ++q) v.c0[q] = c1[q]; v.cInit = true; }
        float cd[20]; for (int q = 0; q < 20; ++q) cd[q] = (c1[q] - v.c0[q]) / (float) len;
        float cc[20]; for (int q = 0; q < 20; ++q) cc[q] = v.c0[q];

        for (int i = 0; i < len; ++i)
        {
            for (int q = 0; q < 20; ++q) cc[q] += cd[q];
            const float gL = cc[0], gH = cc[1], kL = cc[2], kH = cc[3], kSem = cc[4], kLad = cc[5], res = cc[6], morph = cc[7], drive = cc[8], drvEmph = cc[9];
            const float foldA = cc[10], foldS = cc[11], level = cc[12], lv1 = cc[13], l2 = cc[14], noiseA = cc[15];
            // FM audio (sens au choix) ; VCO2 est recale par VCO1 si le hard sync est actif
            // increments de phase bornes sous Nyquist (une FM extreme sur une note aigue pouvait les faire exploser -> phase hors [0,1[ -> NaN)
            // section VCO surechantillonnee (x2, x4 en High Fidelity) : dent de scie, carre, sync, FM, retour et poly-mod restent propres jusque dans l'aigu
            float xs[4], pmLast = 0.f;
            for (int ko = 0; ko < facOsc; ++ko)
            {
                const float pm = pmOn ? pmE + pmO * v.prev2 : 0.f;                              // poly-mod (valeur du VCO 2 de l'echantillon precedent)
                pmLast = pm;
                const float pmF = (pmOn && P.pmFreq) ? std::exp2 (pm * 3.f) : 1.f;
                float inc1, inc2, pm1 = 0.f, pm2 = 0.f;
                if (P.fmType == 0)                                                    // FM exponentielle (analogique) : la profondeur module la frequence en octaves
                {
                    const float fb = P.fmFb * 1.2f;
                    inc1 = std::min (0.49f, std::max (0.f, ((P.fmDir != 1 && fmA > 1e-5f) ? dt1s * std::pow (2.f, fmA * 2.5f * v.prev2) : dt1s) * pmF));
                    inc2 = std::min (0.49f, std::max (0.f, (P.fmDir != 0 && fmA > 1e-5f) ? dt2s * std::pow (2.f, fmA * 2.5f * v.prev1) : dt2s));
                    if (fb > 1e-4f) { if (P.fmDir != 1) inc2 = std::min (0.49f, inc2 * std::pow (2.f, fb * v.prev2)); if (P.fmDir != 0) inc1 = std::min (0.49f, inc1 * std::pow (2.f, fb * v.prev1)); }
                }
                else                                                                  // FM lineaire (modulation de phase) : la hauteur reste stable, comme sur un DX
                {
                    inc1 = std::min (0.49f, dt1s * pmF); inc2 = std::min (0.49f, dt2s);
                    const float beta = fmA * fmA * 12.f / 6.2831853f;                 // indice en cycles : 0..~1,9 (12 rad) ; courbe quadratique = reglage fin des petits indices
                    const float fbc = P.fmFb * P.fmFb * 7.f / 6.2831853f;
                    if (fmA > 1e-5f) { if (P.fmDir != 1) pm1 = beta * v.prev2; if (P.fmDir != 0) pm2 = beta * v.prev1; }
                    if (fbc > 1e-5f) { if (P.fmDir != 1) pm2 += fbc * v.prev2; if (P.fmDir != 0) pm1 += fbc * v.prev1; }
                }
                v.ph1 += inc1;
                float syncJ = 0.f, syncX = 0.f; bool synced = false;
                if (v.ph1 >= 1.f)
                {
                    v.ph1 -= std::floor (v.ph1); v.subFlip = ! v.subFlip;
                    if (P.sync)
                    {
                        // hard sync anti-repliement : instant exact du recalage (fraction d'echantillon), saut de la forme du VCO 2 a cet instant,
                        // puis correction polyBLEP de part et d'autre du saut (l'echantillon precedent est retenu un echantillon pour ca)
                        syncX = std::min (1.f, v.ph1 / std::max (inc1, 1e-6f));                     // temps ecoule depuis le recalage, en echantillons
                        float pw = v.ph2 + inc2 * (1.f - syncX); pw -= std::floor (pw);            // phase du VCO 2 juste avant le recalage
                        syncJ = sy::naive (P.wave2, 0.f, pw2) - sy::naive (P.wave2, pw, pw2);
                        synced = pm2 == 0.f;
                        v.ph2 = syncX * inc2 - inc2;                                                 // apres l'increment ci-dessous : syncX * inc2 (avant : un echantillon d'avance)
                    }
                }
                v.ph2 += inc2; if (v.ph2 >= 1.f) v.ph2 -= std::floor (v.ph2);
                auto wrapP = [] (float x) { return x - std::floor (x); };
                const float pwA = (pmOn && P.pmPw) ? std::min (0.95f, std::max (0.05f, pw1 + pm * 0.4f)) : pw1;
                const float pwT1 = vcoAnalog ? std::min (0.95f, std::max (0.05f, pwA + vPw1[vi])) : pwA, pwT2 = vcoAnalog ? std::min (0.95f, std::max (0.05f, pw2 + vPw2[vi])) : pw2;
                const float o1 = sy::osc (P.wave1, pm1 != 0.f ? wrapP (v.ph1 + pm1) : v.ph1, std::min (0.45f, inc1), pwT1);
                const float o2 = sy::osc (P.wave2, pm2 != 0.f ? wrapP (v.ph2 + pm2) : v.ph2, std::min (0.45f, inc2), pwT2);
                if (P.fmWave == 0) { v.prev1 = o1; v.prev2 = o2; }
                else                                                                  // modulateur dedie : sinus, triangle ou carre
                {
                    auto m = [this] (float ph, float inc) { return P.fmWave == 1 ? std::sin (6.2831853f * ph) : sy::osc (P.fmWave == 2 ? 2 : 1, ph, std::min (0.45f, inc), 0.5f); };
                    v.prev1 = m (v.ph1, inc1); v.prev2 = m (v.ph2, inc2);
                }
                // sortie des VCO retardee d'un echantillon : laisse le temps de corriger le saut du hard sync des deux cotes
                float o2c = o2;
                if (synced)
                {
                    const float x1 = syncX, jA = P.wave2 == 0 ? -2.f : (P.wave2 == 1 ? 2.f : 0.f);   // saut deja corrige par osc() en debut de cycle (dent de scie / carre)
                    v.dl2 += syncJ * 0.5f * (x1 * x1);                                       // avant le saut : l'echantillon retenu monte vers lui
                    o2c += (syncJ - jA) * -0.5f * (1.f - x1) * (1.f - x1);                   // apres le saut : complement de la correction d'osc()
                }
                float o1n = o1;
                if (vcoAnalog)                                                         // VCO analogique : forme, liaison capacitive (~5 Hz : le carre s'affaisse dans le grave), tolerances
                {
                    o1n = sy::analogShape (P.wave1, o1, vCrv1[vi]) * vLv1[vi]; o2c = sy::analogShape (P.wave2, o2c, vCrv2[vi]) * vLv2[vi];
                    const float y1 = acA * (v.ac1y + o1n - v.ac1x); v.ac1x = o1n; v.ac1y = y1; o1n = y1;
                    const float y2 = acA * (v.ac2y + o2c - v.ac2x); v.ac2x = o2c; v.ac2y = y2; o2c = y2;
                }
                const float o1e = v.dl1, o2e = v.dl2; v.dl1 = o1n; v.dl2 = o2c;
                float xk = o1e * lv1 + o2e * l2 + o1e * o2e * ringA * 1.4f + rnd() * noiseA * noiseK;
                if (subA > 0.0005f) { const float phs = v.ph1 * 0.5f + (v.subFlip ? 0.5f : 0.f); const float so = v.dls; v.dls = sy::osc (1, phs, std::min (0.45f, inc1 * 0.5f), 0.5f); xk += so * subA * 0.8f; }   // sub : carre une octave sous le VCO 1
                xs[ko] = xk;
            }
            float x;
            if (facOsc == 2) x = v.dsA.down (xs[0], xs[1]);
            else { const float h0 = v.dsB.down (xs[0], xs[1]); const float h1 = v.dsB.down (xs[2], xs[3]); x = v.dsA.down (h0, h1); }   // dans l'ordre (l'ordre d'evaluation des arguments n'est pas garanti)
            x = v.osF.run (x, facFx, [foldA, foldS] (float u) { return sy::fold (u, foldA, foldS); });      // wavefolder surechantillonne x4 (plus de repliement de spectre)
            const float hp = v.hp.run (x * 0.7f, gH, kH);                  // HPF puis LPF, comme sur le MS20
            // DRIVE "shiny" : on accentue le haut du spectre AVANT la saturation (les graves gonflent moins, les harmoniques brillent), puis on rend le niveau
            v.drvLp += drvC * (hp - v.drvLp);
            const float hiPart = hp - v.drvLp;
            const float dIn = hp + drvEmph * hiPart;
            const float dBias = 0.06f * (drive - 1.f), dOff = std::tanh (dBias), dNorm = 1.f / (1.f + 0.12f * (drive - 1.f));
            const float dx = v.osD.run (dIn, facFx, [drive, dBias, dOff, dNorm] (float u) { return (sy::tanhFast (u * drive + dBias) - dOff) * dNorm; });   // saturation surechantillonnee x4
            float lp;
            const float gLm = (pmOn && P.pmFilt) ? std::min (gMaxL, gL * std::exp2 (pmLast * 3.f)) : gL;     // poly-mod vers le filtre (circuits Ladder / SEM / Prophet)
            if (P.ftype == 0)      lp = v.ms.run (dx, cc[16], cc[17], cc[18], msC);
            else if (P.ftype == 1)
            {
                const float kq = cc[16], g2 = sy::gHalfN (gLm, facFlt) * (1.f + kCemTune * std::min (1.f, std::max (0.f, (kq - 3.8f) * 4.2f)));
                lp = v.osV.run (dx, facFlt, [&v, g2, kq] (float u) { return v.cem.run (u, g2, kq); }) * (1.f + res * (1.2f + 2.3f * (drive - 1.f) / 5.f));   // le DRIVE rend les graves que la resonance retire (compensation, comme le contour d'un Moog)
            }
            else if (P.ftype == 3)
            {
                const float kq = cc[16], g2 = sy::gHalfN (gLm, facFlt) * (1.f + kProTune * std::min (1.f, std::max (0.f, (kq - 3.8f) * 4.2f)));
                lp = v.osV.run (dx, facFlt, [&v, g2, kq] (float u) { return v.pro.run (u, g2, kq); }) * (0.75f + sy::proResLaw (res) * (0.4f + 1.6f * (drive - 1.f) / 5.f));   // meme compensation des graves par le DRIVE
            }
            else
            {
                const float dq = cc[16], g2 = sy::gHalfN (gLm, facFlt);
                lp = v.osV.run (dx, facFlt, [&v, g2, dq, morph] (float u) { return v.sck.run (u, g2, dq, morph); });
            }
            const float a = P.envLink ? v.aenv.tick (fA2, fD2, fS2, fR2, fs) : v.aenv.tick (aA2, aD2, aS2, aR2, fs);   // LINK : le volume suit l'enveloppe FILTRE (au rythme audio)
            const float y = lp * a * level * v.gainV * (0.35f + 0.65f * v.vel);
            if (! (std::fabs (y) < 8.f))                                   // garde-fou : NaN, infini ou valeur folle -> on remet les etats a zero (jamais de bruit ni de blocage)
            {
                v.hp = sy::Hp(); v.ms.reset(); v.cem.reset(); v.sck.reset(); v.pro.reset(); v.drvLp = 0.f; v.xPrev = 0.f; v.prev1 = v.prev2 = 0.f; v.osF = sy::OverVar(); v.osD = sy::OverVar(); v.osV = sy::OverVar(); v.dsA = sy::HalfBand<23>(); v.dsB = sy::HalfBand<15>(); v.dl1 = v.dl2 = v.dls = 0.f;
                if (! std::isfinite (v.ph1)) v.ph1 = 0.f;
                if (! std::isfinite (v.ph2)) v.ph2 = 0.f;
                continue;
            }
            L[i] += y * gl; R[i] += y * gr;
        }
        for (int q = 0; q < 20; ++q) v.c0[q] = c1[q];
        if (v.aenv.st == sy::Env::Idle) v.active = false;
    }
};
} // namespace zephyr
