#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "SynthEditor.h"

namespace
{
enum Kind { kFloat, kBool, kChoice };
struct PDef { const char* id; const char* name; float mn, mx, def; Kind kind; const char* choices; };

// destinations des sources de modulation du rack (Zephyr FX) : index du menu -> entree CV du moteur
const char* const kModDstChoices = "Off|Sat Drive|Wide Width|Phaser Mix|Phaser Rate|Ensemble Depth|Ensemble Mix|Filter HPF|Filter LPF|Filter Peak|Delay Time|Delay Feedback|Delay Mix|Reverb Mix|Reverb Decay|Dry/Wet";
const int kModDstCv[16] { -1, zephyr::CvDrive, zephyr::CvWidth, zephyr::CvPhMix, zephyr::CvPhRate, zephyr::CvEnsDepth, zephyr::CvEnsMix, zephyr::CvHpf, zephyr::CvLpf, zephyr::CvPeak,
                          zephyr::CvDlyTime, zephyr::CvDlyFb, zephyr::CvDlyMix, zephyr::CvRevMix, zephyr::CvRevDecay, zephyr::CvMix };
const char* const kSyncChoices = "Free|4 bar|2 bar|1 bar|1/2|1/4|1/8|1/16|1/32|1/2T|1/4T|1/8T|1/16T|1/4.|1/8.|1/16.";

// l'ordre ici = l'ordre de lecture dans readParams()
// l'ordre ici = l'ordre de lecture dans readParams(). Tous les modules demarrent neutres (mix / drive a zero).
const PDef kDefs[] = {
    { "on",        "Power",            0,   1,  1,    kBool,   nullptr },
    { "output",    "Output",         -24,  12, -6,    kFloat,  nullptr },
    { "mix",       "Dry/Wet",          0,   1,  1,    kFloat,  nullptr },
    { "wideWidth", "Wide Width",       0,   1,  0,    kFloat,  nullptr },
    { "wideSpread","Wide Spread",      0,   1,  0,    kFloat,  nullptr },
    { "wideBass",  "Wide Bass Mono",   0,   1,  0,    kFloat,  nullptr },
    { "satMode",   "Sat Mode",         0,   2,  0,    kChoice, "Tape|Console|Tube" },
    { "satDrive",  "Sat Drive",        0,   1,  0,    kFloat,  nullptr },
    { "satTone",   "Sat Tone",         0,   1,  0.5f, kFloat,  nullptr },
    { "low",       "EQ Low",         -12,  12,  0,    kFloat,  nullptr },
    { "mid",       "EQ Mid",         -12,  12,  0,    kFloat,  nullptr },
    { "high",      "EQ Air",         -12,  12,  0,    kFloat,  nullptr },
    { "hpf",       "HPF Cutoff",       0,   1,  0,    kFloat,  nullptr },
    { "hpfPeak",   "HPF Peak",         0,   1,  0,    kFloat,  nullptr },
    { "lpf",       "LPF Cutoff",       0,   1,  1,    kFloat,  nullptr },
    { "lpfPeak",   "LPF Peak",         0,   1,  0,    kFloat,  nullptr },
    { "phMix",     "Phaser Mix",       0,   1,  0,    kFloat,  nullptr },
    { "phRate",    "Phaser Rate",      0,   1,  0.3f, kFloat,  nullptr },
    { "phDepth",   "Phaser Depth",     0,   1,  0.6f, kFloat,  nullptr },
    { "phFb",      "Phaser Feedback",  0,   1,  0.3f, kFloat,  nullptr },
    { "phStages",  "Phaser Stages",    0,   3,  1,    kChoice, "4|6|8|12" },
    { "phSync",    "Phaser Sync",      0,  15,  0,    kChoice, kSyncChoices },
    { "compType",  "Comp Type",        0,   2,  0,    kChoice, "Off|Opto|Glue" },
    { "compAmt",   "Comp Ratio",       0,   1,  0.4f, kFloat,  nullptr },
    { "compThresh","Comp Threshold",   0,   1,  0.35f,kFloat,  nullptr },
    { "compAtt",   "Comp Attack",      0,   1,  0.3f, kFloat,  nullptr },
    { "ensMix",    "Ensemble Mix",     0,   1,  0,    kFloat,  nullptr },
    { "ensDepth",  "Ensemble Depth",   0,   1,  0.6f, kFloat,  nullptr },
    { "ensSpeed",  "Ensemble Speed",   0,   1,  0.5f, kFloat,  nullptr },
    { "ensTone",   "Ensemble Tone",    0,   1,  0.6f, kFloat,  nullptr },
    { "lfo1Rate",  "LFO1 Rate",        0,   1,  0.35f,kFloat,  nullptr },
    { "lfo1Shape", "LFO1 Shape",       0,   1,  0,    kFloat,  nullptr },
    { "lfo1Level", "LFO1 Level",       0,   1,  0.5f, kFloat,  nullptr },
    { "lfo1Sync",  "LFO1 Sync",        0,  15,  0,    kChoice, kSyncChoices },
    { "lfo2Rate",  "LFO2 Rate",        0,   1,  0.55f,kFloat,  nullptr },
    { "lfo2Shape", "LFO2 Shape",       0,   1,  0.34f,kFloat,  nullptr },
    { "lfo2Level", "LFO2 Level",       0,   1,  0.5f, kFloat,  nullptr },
    { "lfo2Sync",  "LFO2 Sync",        0,  15,  0,    kChoice, kSyncChoices },
    { "shRate",    "S&H Rate",         0,   1,  0.45f,kFloat,  nullptr },
    { "shSlew",    "S&H Slew",         0,   1,  0.15f,kFloat,  nullptr },
    { "shLevel",   "S&H Level",        0,   1,  0.5f, kFloat,  nullptr },
    { "shSync",    "S&H Sync",         0,  15,  0,    kChoice, kSyncChoices },
    { "envRel",    "Env Release",      0,   1,  0.4f, kFloat,  nullptr },
    { "envLevel",  "Env Level",        0,   1,  0.6f, kFloat,  nullptr },
    { "dlyMode",   "Delay Mode",       0,   3,  0,    kChoice, "Tape|Ice|Ping-Pong|Digital" },
    { "dlyTime",   "Delay Time",       0,   1,  0.45f,kFloat,  nullptr },
    { "dlyFb",     "Delay Feedback",   0,   1,  0.35f,kFloat,  nullptr },
    { "dlyTone",   "Delay Tone",       0,   1,  0.6f, kFloat,  nullptr },
    { "dlyMod",    "Delay Mod",        0,   1,  0.3f, kFloat,  nullptr },
    { "dlyMix",    "Delay Mix",        0,   1,  0.f,  kFloat,  nullptr },
    { "dlySync",   "Delay Sync",       0,  15,  0,    kChoice, kSyncChoices },
    { "revMix",    "Reverb Mix",       0,   1,  0,    kFloat,  nullptr },
    { "revSize",   "Reverb Size",      0,   1,  0.5f, kFloat,  nullptr },
    { "revDecay",  "Reverb Decay",     0,   1,  0.6f, kFloat,  nullptr },
    { "revPre",    "Reverb Pre-Delay", 0,   1,  0.1f, kFloat,  nullptr },
    { "revDamp",   "Reverb Damping",   0,   1,  0.4f, kFloat,  nullptr },
    { "revMod",    "Reverb Mod",       0,   1,  0.3f, kFloat,  nullptr },
    { "revWidth",  "Reverb Width",     0,   1,  1,    kFloat,  nullptr },
    { "revShimmer","Reverb Shimmer",   0,   1,  0,    kFloat,  nullptr },
    { "revLowCut", "Reverb Low Cut",   0,   1,  0.1f, kFloat,  nullptr },
    { "revFreeze", "Reverb Freeze",    0,   1,  0,    kBool,   nullptr },
    // ajoutes en v2 (toujours en fin de liste : les projets et l'automation de la v1 restent valides)
    { "stutOn",    "Stutter Engage",   0,   1,  0,    kBool,   nullptr },
    { "stutMode",  "Stutter Mode",     0,   3,  0,    kChoice, "Beat Repeat|Reverse|Gate|Offbeat" },
    { "stutDiv",   "Stutter Division", 0,   6,  4,    kChoice, "1 bar|1/2|1/4|1/8|1/16|1/32|1/64" },
    { "stutFeel",  "Stutter Feel",     0,   2,  0,    kChoice, "Straight|Triplet|Dotted" },
    { "stutLen",   "Stutter Length",   0,   1,  0.6f, kFloat,  nullptr },
    { "stutDecay", "Stutter Decay",    0,   1,  0,    kFloat,  nullptr },
    { "stutPitch", "Stutter Pitch",    0,   1,  0.5f, kFloat,  nullptr },
    { "stutMix",   "Stutter Mix",      0,   1,  1,    kFloat,  nullptr },
    // v2c : routage delay / reverb, et destinations des sources de modulation du rack (remplacent le patchbay)
    { "space",     "Delay/Reverb Routing", 0, 1, 0,   kChoice, "Serial|Parallel" },
    { "lfo1Dst",   "LFO1 Destination", 0,  15,  0,    kChoice, kModDstChoices },
    { "lfo2Dst",   "LFO2 Destination", 0,  15,  0,    kChoice, kModDstChoices },
    { "shDst",     "S&H Destination",  0,  15,  0,    kChoice, kModDstChoices },
    { "envDst",    "Env Destination",  0,  15,  0,    kChoice, kModDstChoices },
};
constexpr int kNumDefs = (int) (sizeof (kDefs) / sizeof (kDefs[0]));

// ---- presets d'usine
#include <map>
#include <set>
#if ZEPHYR_SYNTH
constexpr float kSynthMakeupDb = 4.f;     // rattrapage partiel de la reserve du bus (accords denses propres, notes seules ~2 dB sous la v1)
#endif
using namespace zephyr;
#if ZEPHYR_SYNTH
static_assert (zephyr::Engine::kExt <= zephyr::kFxMod && zephyr::kLanes == zephyr::kArpLanes, "entrees de modulation des effets et pistes : moteur, synthe et arpegiateur doivent correspondre");

// ---------------------------------------------------------------- parametres du synthe (version instrument)
struct SynDef { juce::String id, name; float mn, mx, def; Kind kind; juce::String choices; };
const std::vector<SynDef>& synDefs()
{
    static const std::vector<SynDef> defs = []
    {
        std::vector<SynDef> v;
        auto F = [&v] (const juce::String& id, const juce::String& nm, float mn, float mx, float d) { v.push_back ({ id, nm, mn, mx, d, kFloat, {} }); };
        auto B = [&v] (const juce::String& id, const juce::String& nm, bool d) { v.push_back ({ id, nm, 0, 1, d ? 1.f : 0.f, kBool, {} }); };
        auto C = [&v] (const juce::String& id, const juce::String& nm, const juce::String& ch, int d) { v.push_back ({ id, nm, 0, 0, (float) d, kChoice, ch }); };
        const juce::String waves ("Saw|Pulse|Triangle|Sine"), octs ("-2|-1|0|+1|+2");
        C ("s_wave1", "VCO1 Wave", waves, 0);  C ("s_oct1", "VCO1 Octave", octs, 2);
        C ("s_wave2", "VCO2 Wave", waves, 0);  C ("s_oct2", "VCO2 Octave", octs, 2);
        F ("s_tune2", "VCO2 Tune", -12, 12, 0);  F ("s_fine2", "VCO2 Fine", -100, 100, 0);
        F ("s_pw1", "VCO1 PW", 0, 1, 0.5f);  F ("s_pw2", "VCO2 PW", 0, 1, 0.5f);
        B ("s_sync", "Hard Sync", false);  F ("s_fm", "FM", 0, 1, 0);  F ("s_ring", "Ring Mod", 0, 1, 0);
        F ("s_lvl1", "VCO1 Level", 0, 1, 0.8f);  F ("s_lvl2", "VCO2 Level", 0, 1, 0);  F ("s_noise", "Noise", 0, 1, 0);
        F ("s_glide", "Glide", 0, 1, 0);
        F ("s_hpf", "HPF Cutoff", 0, 1, 0);  F ("s_hpfRes", "HPF Peak", 0, 1, 0);
        F ("s_lpf", "VCF Cutoff", 0, 1, 0.7f);  F ("s_lpfRes", "VCF Resonance", 0, 1, 0.2f);
        F ("s_fltEnv", "Filter Env Amt", 0, 1, 0.3f);  F ("s_key", "Key Track", 0, 1, 0.5f);  F ("s_drive", "VCF Saturation", 0, 1, 0);
        F ("s_aA", "Amp A", 0, 1, 0.1f);  F ("s_aD", "Amp D", 0, 1, 0.4f);  F ("s_aS", "Amp S", 0, 1, 0.8f);  F ("s_aR", "Amp R", 0, 1, 0.35f);
        F ("s_fA", "Flt A", 0, 1, 0.1f);  F ("s_fD", "Flt D", 0, 1, 0.45f);  F ("s_fS", "Flt S", 0, 1, 0.2f);  F ("s_fR", "Flt R", 0, 1, 0.35f);
        F ("s_master", "Synth Master", 0, 1, 0.7f);
        juce::StringArray divs; for (auto* d : kLfoDivNames) divs.add (d);
        for (int i = 1; i <= kLfos; ++i)
        {
            const juce::String p ("s_l" + juce::String (i));
            F (p + "Rate", "LFO" + juce::String (i) + " Rate", 0, 1, 0.307f);  C (p + "Wave", "LFO" + juce::String (i) + " Wave", "Sine|Triangle|Saw|Square|S&H", 0);
            F (p + "Slew", "LFO" + juce::String (i) + " Slew", 0, 1, 0);  F (p + "Fade", "LFO" + juce::String (i) + " Fade", 0, 1, 0);
            C (p + "Sync", "LFO" + juce::String (i) + " Sync", divs.joinIntoString ("|"), 0);  B (p + "Trig", "LFO" + juce::String (i) + " Retrig", false);
        }
        for (int i = 1; i <= 4; ++i) F ("s_m" + juce::String (i), "Macro " + juce::String (i), 0, 1, 0);
        juce::StringArray srcs, dsts; for (auto* s : kModSrcNames) srcs.add (s); for (auto* s : kModDstNames) dsts.add (s);
        for (int i = 1; i <= kSlots; ++i)
        {
            const juce::String p ("s_slot" + juce::String (i)), n ("Mod " + juce::String (i));
            C (p + "Src", n + " Source", srcs.joinIntoString ("|"), 0);  C (p + "Dst", n + " Dest", dsts.joinIntoString ("|"), 0);
            F (p + "Amt", n + " Amount", -1, 1, 0);
        }
        F ("s_fold", "Wavefolder", 0, 1, 0);  F ("s_foldSym", "Fold Symmetry", -1, 1, 0);
        C ("s_arpOn", "Arp", "Off|On|Latch", 0);
        C ("s_arpMode", "Arp Direction", "Up|Down|Up-Down|Played|Random|Converge", 0);
        C ("s_arpOct", "Arp Range", "1 Oct|2 Oct|3 Oct|4 Oct", 0);
        C ("s_ftype", "Filter Type", "MS-10 (circuit)|Ladder CEM3320 (circuit)|SEM (circuit)|Prophet SSM2040 (circuit)", 2);  F ("s_semMorph", "SEM Morph", 0, 1, 0);
        C ("s_assign", "Key Assign", "Poly|Mono|Unison|Share|Chord", 0);  B ("s_hold", "Hold", false);
        F ("s_uniDetune", "Unison Detune", 0, 1, 0.3f);  B ("s_chordLearn", "Chord Learn", false);
        for (int i = 5; i <= kMacros; ++i) F ("s_m" + juce::String (i), "Macro " + juce::String (i), 0, 1, 0);
        F ("s_arpRate", "Arp Rate", 0, 1, 0.55f);  C ("s_arpSync", "Arp Sync", divs.joinIntoString ("|"), 0);
        // Zephyr v1 (meme position que dans la v1 : l'automation des projets reste valide)
        F ("s_sub", "Sub Level", 0, 1, 0);  F ("s_drift", "Drift", 0, 1, 0);
        F ("s_pmEnv", "Poly-Mod Filter Env", 0, 1, 0);  F ("s_pmOsc", "Poly-Mod VCO2", 0, 1, 0);
        B ("s_pmFreq", "Poly-Mod > Freq VCO1", false);  B ("s_pmPw", "Poly-Mod > PW VCO1", false);  B ("s_pmFilt", "Poly-Mod > Filter", false);
        F ("s_arpShift", "Arp Shift", 0, 1, 0);  F ("s_arpCluster", "Arp Cluster", 0, 1, 0);
        F ("s_arpGate", "Arp Gate", 0.05f, 1, 0.5f);  F ("s_arpSwing", "Arp Swing", 0, 1, 0);  F ("s_arpChance", "Arp Chance", 0, 1, 1);
        B ("s_arpFree", "Arp Free Run", false);
        { juce::StringArray lens; for (int k = 1; k <= 16; ++k) lens.add (juce::String (k) + (k == 1 ? " STEP" : " STEPS")); C ("s_arpLen", "Arp Steps", lens.joinIntoString ("|"), 7);
          for (int k = 1; k <= kLanes; ++k)
          {
              const juce::String q ("s_lane" + juce::String (k));
              C (q + "Len", "Lane " + juce::String (k) + " Steps", lens.joinIntoString ("|"), 7);
              C (q + "Dst", "Lane " + juce::String (k) + " Dest", dsts.joinIntoString ("|"), 0);
              F (q + "Amt", "Lane " + juce::String (k) + " Amount", -1, 1, 0.5f);
              F (q + "Glide", "Lane " + juce::String (k) + " Glide", 0, 1, 0);
          } }
        C ("s_fmDir", "FM Direction", "2>1|1>2|Both", 0);  C ("s_fmRatio", "FM Ratio", "Free|1:2|2:3|1:1|3:2|2:1|3:1|4:1|5:1|7:1|7:2|x1.41", 0);  C ("s_fmWave", "FM Wave", "Osc|Sine|Tri|Square", 0);
        F ("s_fmFine", "FM Fine", -1, 1, 0);  F ("s_fmAtt", "FM Attack", 0, 1, 0);  F ("s_fmDec", "FM Decay", 0, 1, 0.4f);
        F ("s_fmEnvAmt", "FM Env Amount", 0, 1, 0);  F ("s_fmVel", "FM Velocity", 0, 1, 0);  F ("s_fmAT", "FM Aftertouch", 0, 1, 0);
        C ("s_arpType", "Arp Type", "Arp|Seq", 0);  B ("s_seqRec", "Step Record", false);
        { juce::StringArray br; for (int k = 1; k <= 24; ++k) br.add ("Bend +/-" + juce::String (k) + " st"); C ("s_bendRange", "Pitch Bend Range", br.joinIntoString ("|"), 1); }
        F ("s_fltLfo", "Filter LFO1 Amount", 0, 1, 0);
        C ("s_arpRatchet", "Arp Ratchet", "1x|2x|3x|4x", 0);  B ("s_arpTriplet", "Arp Triplet", false);
        F ("s_vib", "VCO1 LFO Amount", 0, 1, 0);  F ("s_vibWheel", "VCO1 LFO + Wheel", 0, 1, 0);  F ("s_vibAT", "VCO1 LFO + Aftertouch", 0, 1, 0);
        return v;
    }();
    return defs;
}
void addSynthParams (juce::AudioProcessorValueTreeState::ParameterLayout& l)
{
    for (const auto& d : synDefs())
    {
        const juce::ParameterID pid { d.id, 1 };
        if (d.kind == kBool)        l.add (std::make_unique<juce::AudioParameterBool> (pid, d.name, d.def > 0.5f));
        else if (d.kind == kChoice) l.add (std::make_unique<juce::AudioParameterChoice> (pid, d.name, juce::StringArray::fromTokens (d.choices, "|", ""), (int) d.def));
        else                        l.add (std::make_unique<juce::AudioParameterFloat> (pid, d.name, juce::NormalisableRange<float> (d.mn, d.mx, 0.001f), d.def));
    }
}
#endif

// ---------------------------------------------------------------- presets d'usine
struct Factory { const char* name; const char* category; void (*apply) (ZephyrAudioProcessor&); };
const char* const kCategories[] = { "Pad", "Strings", "Keys / Bells", "Brass / Lead", "Bass", "Arp / Seq", "Texture", "Rack FX" };

using KV = std::pair<const char*, float>;
// Les presets d'usine sont ecrits dans les unites de la v1 : filtres 0 SEM / 1 Ladder / 2 Prophet (-> circuits 2 / 1 / 3, ou 10 = MS-10),
// vitesse des LFO sur l'ancienne plage 0.05..40 Hz (convertie pour la nouvelle plage 0.05..300 Hz).
void S (ZephyrAudioProcessor& p, std::initializer_list<KV> l)
{
    for (auto& kv : l)
    {
        const juce::String id (kv.first);
        float v = kv.second;
        if (id == "s_ftype") { const int f = juce::roundToInt (v); v = f == 10 ? 0.f : (f == 1 ? 1.f : (f == 2 ? 3.f : 2.f)); }
        if (id.startsWith ("s_l") && id.endsWith ("Rate") && id.length() == 8) v *= 0.7677f;
        p.setParamReal (kv.first, v);
    }
}
void M (ZephyrAudioProcessor& p, int slot, int src, int dst, float amt)
{
    const juce::String b ("s_slot" + juce::String (slot));
    p.setParamReal ((b + "Src").toRawUTF8(), (float) src); p.setParamReal ((b + "Dst").toRawUTF8(), (float) dst); p.setParamReal ((b + "Amt").toRawUTF8(), amt);
}
void fInit (ZephyrAudioProcessor&) {}

// ---- RACK FX (aussi disponibles dans Zephyr FX)
void fStringMachine (ZephyrAudioProcessor& p) { S (p, { {"ensMix",0.65f},{"ensDepth",0.7f},{"ensSpeed",0.5f},{"phMix",0.2f},{"phRate",0.15f},{"revMix",0.3f},{"revDecay",0.7f} }); }
void fTapeGold (ZephyrAudioProcessor& p)      { S (p, { {"satMode",0},{"satDrive",0.3f},{"satTone",0.45f},{"dlyMode",0},{"dlyMix",0.35f},{"dlyFb",0.5f},{"dlySync",14},{"dlyMod",0.5f},{"dlyTone",0.45f},{"revMix",0.2f} }); }
void fIceHall (ZephyrAudioProcessor& p)       { S (p, { {"dlyMode",1},{"dlyMix",0.35f},{"dlyFb",0.55f},{"dlyTime",0.62f},{"dlyMod",0.5f},{"revMix",0.55f},{"revDecay",0.85f},{"revShimmer",0.5f},{"revSize",0.9f} }); }
void fWideBright (ZephyrAudioProcessor& p)    { S (p, { {"high",4.f},{"low",1.f},{"satMode",1},{"satDrive",0.2f},{"wideWidth",0.5f},{"wideSpread",0.4f},{"wideBass",0.5f},{"compType",2},{"compAmt",0.3f},{"compThresh",0.4f} }); }
void fPhaseShimmer (ZephyrAudioProcessor& p)  { S (p, { {"phMix",0.6f},{"phRate",0.2f},{"phFb",0.55f},{"phStages",2},{"ensMix",0.3f},{"revMix",0.4f},{"revShimmer",0.3f},{"revDecay",0.7f} }); }
void fOptoGlow (ZephyrAudioProcessor& p)      { S (p, { {"compType",1},{"compAmt",0.5f},{"compThresh",0.5f},{"satMode",2},{"satDrive",0.25f},{"mid",-1.f},{"high",3.f} }); }

#if ZEPHYR_SYNTH
// ---- PADS
void pAurora (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-8},{"s_wave1",0},{"s_wave2",0},{"s_fine2",7},{"s_lvl1",0.6f},{"s_lvl2",0.55f},{"s_ftype",0},{"s_lpf",0.58f},{"s_lpfRes",0.2f},{"s_fltEnv",0.15f},
            {"s_aA",0.75f},{"s_aD",0.6f},{"s_aS",0.9f},{"s_aR",0.85f},{"s_drift",0.35f},{"s_l1Rate",0.12f},
            {"ensMix",0.45f},{"revMix",0.4f},{"revDecay",0.75f},{"revSize",0.7f},{"revShimmer",0.35f},{"wideSpread",0.4f} });
    M (p, 1, MsLfo1, MdLpf, 0.15f); M (p, 2, MsPolyAT, MdLpf, 0.4f); M (p, 3, MsWheel, MdRevMix, 0.3f);
}
void pGlassCathedral (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-9},{"s_wave1",2},{"s_wave2",3},{"s_oct2",3},{"s_lvl1",0.7f},{"s_lvl2",0.4f},{"s_ftype",2},{"s_lpf",0.7f},{"s_lpfRes",0.15f},{"s_fltEnv",0.1f},
            {"s_aA",0.7f},{"s_aS",0.95f},{"s_aR",0.9f},{"s_drift",0.25f},
            {"dlyMode",1},{"dlyMix",0.25f},{"dlyTime",0.6f},{"dlyFb",0.45f},{"revMix",0.55f},{"revDecay",0.9f},{"revSize",0.9f},{"revShimmer",0.5f} });
    M (p, 1, MsPolyAT, MdLpf, 0.35f); M (p, 2, MsWheel, MdRevMix, 0.3f);
}
void pSilverLining (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-8},{"s_wave1",1},{"s_pw1",0.35f},{"s_wave2",0},{"s_fine2",8},{"s_lvl2",0.5f},{"s_ftype",2},{"s_lpf",0.6f},{"s_lpfRes",0.25f},{"s_drive",0.2f},
            {"s_aA",0.65f},{"s_aS",0.9f},{"s_aR",0.8f},{"s_l2Rate",0.15f},{"s_drift",0.3f},
            {"ensMix",0.35f},{"phMix",0.3f},{"phRate",0.15f},{"wideWidth",0.3f},{"revMix",0.35f},{"revDecay",0.7f} });
    M (p, 1, MsLfo2, MdPw1, 0.4f); M (p, 2, MsPolyAT, MdLpf, 0.4f);
}
void pVelvetHorizon (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-8},{"s_wave1",0},{"s_wave2",0},{"s_oct2",1},{"s_fine2",-6},{"s_lvl2",0.45f},{"s_sub",0.35f},{"s_ftype",1},{"s_lpf",0.45f},{"s_lpfRes",0.15f},
            {"s_fltEnv",0.25f},{"s_fA",0.8f},{"s_fD",0.85f},{"s_fS",0.5f},{"s_aA",0.7f},{"s_aS",0.9f},{"s_aR",0.85f},{"s_drift",0.4f},
            {"compType",1},{"compAmt",0.3f},{"revMix",0.35f},{"revDecay",0.75f},{"ensMix",0.25f} });
    M (p, 1, MsPolyAT, MdLpf, 0.45f); M (p, 2, MsLfo1, MdPitch2, 0.004f); S (p, { {"s_l1Rate",0.1f} });
}
void pMorningDew (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-8},{"s_wave1",3},{"s_wave2",2},{"s_oct2",3},{"s_fm",0.15f},{"s_lvl2",0.35f},{"s_ftype",0},{"s_semMorph",0.2f},{"s_lpf",0.7f},
            {"s_aA",0.6f},{"s_aS",0.9f},{"s_aR",0.8f},{"s_drift",0.2f},
            {"dlyMode",0},{"dlyMix",0.2f},{"dlySync",13},{"revMix",0.45f},{"revShimmer",0.2f},{"revDecay",0.7f} });
    M (p, 1, MsLfo1, MdFm, 0.12f); M (p, 2, MsPolyAT, MdLpf, 0.3f); S (p, { {"s_l1Rate",0.15f} });
}
void pLumiere (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-9},{"s_wave1",0},{"s_wave2",0},{"s_fine2",11},{"s_lvl2",0.6f},{"s_hpf",0.15f},{"s_ftype",2},{"s_lpf",0.65f},{"s_lpfRes",0.3f},{"s_drive",0.3f},
            {"s_aA",0.6f},{"s_aS",0.9f},{"s_aR",0.8f},{"s_drift",0.3f},
            {"ensMix",0.4f},{"wideWidth",0.4f},{"wideSpread",0.3f},{"revMix",0.4f},{"revDecay",0.75f},{"high",2.f} });
    M (p, 1, MsPolyAT, MdSat, 0.4f); M (p, 2, MsWheel, MdLpf, 0.3f);
}
void pPolaris (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-10},{"s_wave1",0},{"s_wave2",1},{"s_fine2",6},{"s_lvl2",0.4f},{"s_ftype",2},{"s_lpf",0.55f},{"s_lpfRes",0.2f},{"s_fltEnv",0.2f},
            {"s_assign",4},{"s_uniDetune",0.4f},{"s_aA",0.5f},{"s_aS",0.9f},{"s_aR",0.8f},{"s_drift",0.3f},
            {"dlyMode",2},{"dlyMix",0.25f},{"dlySync",14},{"revMix",0.4f},{"revDecay",0.75f} });
    M (p, 1, MsLfo1, MdLpf, 0.12f); S (p, { {"s_l1Rate",0.1f} });
}
// ---- STRINGS
void sSolina (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-8},{"s_wave1",0},{"s_wave2",0},{"s_oct2",3},{"s_lvl1",0.7f},{"s_lvl2",0.4f},{"s_ftype",0},{"s_lpf",0.66f},{"s_lpfRes",0.1f},{"s_fltEnv",0.f},
            {"s_aA",0.55f},{"s_aS",1.f},{"s_aR",0.72f},{"s_key",0.7f},
            {"ensMix",0.65f},{"ensDepth",0.7f},{"revMix",0.3f},{"revDecay",0.65f} });
    M (p, 1, MsVel, MdLevel, 0.2f); M (p, 2, MsPolyAT, MdFxEnsDepth, 0.3f);
}
void sVelvetOrch (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-8},{"s_wave1",1},{"s_pw1",0.4f},{"s_wave2",0},{"s_fine2",5},{"s_lvl2",0.6f},{"s_ftype",1},{"s_lpf",0.55f},{"s_lpfRes",0.1f},{"s_fltEnv",0.2f},{"s_fA",0.6f},{"s_fS",0.6f},
            {"s_aA",0.6f},{"s_aS",0.95f},{"s_aR",0.75f},{"s_l1Rate",0.45f},{"s_l1Trig",1},{"s_l1Fade",0.6f},{"s_drift",0.25f},
            {"ensMix",0.5f},{"revMix",0.4f},{"revDecay",0.75f} });
    M (p, 1, MsLfo1, MdPitch, 0.008f); M (p, 2, MsPolyAT, MdLpf, 0.35f);
}
void sCelloLight (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-8},{"s_wave1",0},{"s_oct1",1},{"s_wave2",0},{"s_oct2",1},{"s_fine2",4},{"s_lvl2",0.5f},{"s_ftype",2},{"s_lpf",0.5f},{"s_lpfRes",0.2f},{"s_fltEnv",0.25f},{"s_fA",0.6f},{"s_fS",0.5f},
            {"s_aA",0.45f},{"s_aS",0.95f},{"s_aR",0.6f},{"s_l1Rate",0.42f},{"s_l1Trig",1},{"s_l1Fade",0.5f},{"s_drift",0.3f},{"revMix",0.3f} });
    M (p, 1, MsLfo1, MdPitch, 0.01f); M (p, 2, MsVel, MdFltEnv, 0.3f);
}
void sEnsembleRoyal (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-9},{"s_wave1",0},{"s_wave2",1},{"s_oct2",3},{"s_lvl2",0.35f},{"s_ftype",0},{"s_lpf",0.68f},{"s_aA",0.5f},{"s_aS",1.f},{"s_aR",0.75f},
            {"ensMix",0.8f},{"ensSpeed",0.6f},{"phMix",0.25f},{"phRate",0.08f},{"revMix",0.45f},{"revDecay",0.8f} });
    M (p, 1, MsWheel, MdFxEnsMix, -0.5f);
}
void sSilk (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-8},{"s_wave1",0},{"s_wave2",0},{"s_fine2",10},{"s_lvl2",0.6f},{"s_hpf",0.2f},{"s_ftype",0},{"s_lpf",0.7f},{"s_aA",0.65f},{"s_aS",0.95f},{"s_aR",0.8f},{"s_drift",0.3f},
            {"ensMix",0.5f},{"wideSpread",0.5f},{"revMix",0.35f},{"revDecay",0.7f} });
    M (p, 1, MsPolyAT, MdLpf, 0.3f);
}
// ---- KEYS / BELLS : claviers qui tiennent la note (piano electrique, Wurli, orgue, vibraphone) et cloches longues
void kTinePiano (ZephyrAudioProcessor& p)                     // piano a lames : sinus + harmonique de lame, "bark" FM a l'attaque, autopan
{
    S (p, { {"output",-8},{"s_wave1",3},{"s_wave2",3},{"s_oct2",4},{"s_lvl1",0.85f},{"s_lvl2",0.16f},{"s_fm",0.06f},{"s_ftype",0},{"s_lpf",0.72f},{"s_fltEnv",0.15f},{"s_fD",0.7f},{"s_fS",0.3f},
            {"s_aA",0.f},{"s_aD",0.93f},{"s_aS",0.22f},{"s_aR",0.62f},{"s_drift",0.12f},{"s_l1Rate",0.56f},
            {"phMix",0.25f},{"phRate",0.18f},{"revMix",0.22f},{"revDecay",0.6f} });
    M (p, 1, MsAmpEnv, MdFm, 0.22f); M (p, 2, MsVel, MdFm, 0.18f); M (p, 3, MsLfo1, MdPan, 0.35f); M (p, 4, MsVel, MdLevel, 0.25f);
}
void kWurliGold (ZephyrAudioProcessor& p)                     // piano a anches : pulse etroite, saturation, tremolo
{
    S (p, { {"output",-8},{"s_wave1",1},{"s_pw1",0.42f},{"s_wave2",2},{"s_oct2",3},{"s_lvl1",0.7f},{"s_lvl2",0.3f},{"s_ftype",2},{"s_lpf",0.52f},{"s_lpfRes",0.1f},{"s_fltEnv",0.3f},
            {"s_fD",0.85f},{"s_fS",0.35f},{"s_drive",0.35f},{"s_aA",0.f},{"s_aD",0.9f},{"s_aS",0.4f},{"s_aR",0.55f},{"s_l1Rate",0.68f},
            {"satMode",2},{"satDrive",0.2f},{"revMix",0.2f} });
    M (p, 1, MsLfo1, MdLevel, -0.16f); M (p, 2, MsVel, MdFltEnv, 0.35f); M (p, 3, MsWheel, MdLevel, -0.3f);
}
void kElectricGrand (ZephyrAudioProcessor& p)                 // piano a cordes amplifie : saw + tri, filtre qui s'ouvre avec la velocite
{
    S (p, { {"output",-9},{"s_wave1",0},{"s_wave2",2},{"s_oct2",3},{"s_fine2",4},{"s_lvl1",0.6f},{"s_lvl2",0.4f},{"s_ftype",2},{"s_lpf",0.5f},{"s_lpfRes",0.12f},{"s_fltEnv",0.4f},
            {"s_fD",0.88f},{"s_fS",0.3f},{"s_key",0.8f},{"s_aA",0.f},{"s_aD",0.92f},{"s_aS",0.28f},{"s_aR",0.6f},{"s_drift",0.15f},
            {"ensMix",0.22f},{"revMix",0.25f},{"revDecay",0.65f} });
    M (p, 1, MsVel, MdFltEnv, 0.4f); M (p, 2, MsVel, MdLevel, 0.25f);
}
void kSilkOrgan (ZephyrAudioProcessor& p)                     // orgue doux : tirettes 16' 8' 4', cabine tournante (phaser + ensemble)
{
    S (p, { {"output",-10},{"s_wave1",3},{"s_wave2",3},{"s_oct2",3},{"s_lvl1",0.7f},{"s_lvl2",0.55f},{"s_sub",0.4f},{"s_ftype",0},{"s_lpf",0.8f},{"s_fltEnv",0.f},
            {"s_aA",0.08f},{"s_aD",0.5f},{"s_aS",1.f},{"s_aR",0.32f},{"s_drift",0.1f},
            {"phMix",0.3f},{"phRate",0.5f},{"phDepth",0.4f},{"ensMix",0.3f},{"ensSpeed",0.75f},{"revMix",0.25f} });
    M (p, 1, MsWheel, MdFxPhRate, 0.5f); M (p, 2, MsPolyAT, MdSat, 0.4f);
}
void kVibraphone (ZephyrAudioProcessor& p)                    // vibraphone : sinus + partiel aigu, longue decroissance, moteur de tremolo
{
    S (p, { {"output",-9},{"s_wave1",3},{"s_wave2",3},{"s_oct2",4},{"s_tune2",0.f},{"s_lvl1",0.85f},{"s_lvl2",0.22f},{"s_ring",0.08f},{"s_ftype",0},{"s_lpf",0.78f},
            {"s_aA",0.f},{"s_aD",0.95f},{"s_aS",0.12f},{"s_aR",0.8f},{"s_l1Rate",0.62f},{"revMix",0.38f},{"revDecay",0.75f},{"wideSpread",0.3f} });
    M (p, 1, MsLfo1, MdLevel, -0.22f); M (p, 2, MsVel, MdLevel, 0.3f);
}
void kCrystalBell (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-9},{"s_wave1",3},{"s_wave2",3},{"s_oct2",3},{"s_tune2",7},{"s_fm",0.3f},{"s_lvl2",0.2f},{"s_ftype",2},{"s_lpf",0.85f},{"s_fltEnv",0.f},
            {"s_aA",0.f},{"s_aD",0.9f},{"s_aS",0.15f},{"s_aR",0.78f},{"revMix",0.4f},{"revShimmer",0.3f},{"revDecay",0.75f} });
    M (p, 1, MsAmpEnv, MdFm, 0.3f); M (p, 2, MsVel, MdFm, 0.25f);
}
void kPolyModBell (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-9},{"s_wave1",2},{"s_wave2",3},{"s_tune2",7},{"s_fine2",14},{"s_lvl2",0.f},{"s_pmOsc",0.4f},{"s_pmEnv",0.2f},{"s_pmFreq",1},{"s_ftype",2},{"s_lpf",0.75f},
            {"s_fltEnv",0.3f},{"s_fD",0.85f},{"s_fS",0.2f},{"s_aA",0.f},{"s_aD",0.9f},{"s_aS",0.2f},{"s_aR",0.75f},{"dlyMode",2},{"dlyMix",0.2f},{"dlySync",14},{"revMix",0.35f} });
    M (p, 1, MsVel, MdPolyMod, 0.25f);
}
// ---- BRASS / LEAD
void lGoldenBrass (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-8},{"s_wave1",0},{"s_wave2",0},{"s_fine2",6},{"s_lvl2",0.6f},{"s_ftype",2},{"s_lpf",0.35f},{"s_lpfRes",0.15f},{"s_fltEnv",0.55f},{"s_fA",0.38f},{"s_fD",0.55f},{"s_fS",0.5f},
            {"s_aA",0.22f},{"s_aS",0.9f},{"s_aR",0.45f},{"s_drive",0.35f},{"s_drift",0.25f},{"revMix",0.25f},{"ensMix",0.2f} });
    M (p, 1, MsVel, MdFltEnv, 0.35f); M (p, 2, MsPolyAT, MdLpf, 0.3f);
}
void lProphecySync (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-9},{"s_wave1",0},{"s_wave2",0},{"s_sync",1},{"s_tune2",5},{"s_lvl1",0.3f},{"s_lvl2",0.7f},{"s_ftype",2},{"s_lpf",0.65f},{"s_lpfRes",0.25f},{"s_fltEnv",0.3f},
            {"s_assign",1},{"s_glide",0.12f},{"s_aS",0.9f},{"s_aR",0.35f},{"dlyMode",0},{"dlyMix",0.25f},{"dlySync",14},{"revMix",0.2f} });
    M (p, 1, MsEnv, MdPitch2, 0.35f); M (p, 2, MsPolyAT, MdPitch2, 0.2f);
}
void lSilverFlute (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-8},{"s_wave1",3},{"s_noise",0.08f},{"s_ftype",0},{"s_lpf",0.6f},{"s_lpfRes",0.15f},{"s_aA",0.32f},{"s_aS",0.9f},{"s_aR",0.4f},
            {"s_l1Rate",0.47f},{"s_l1Trig",1},{"s_l1Fade",0.6f},{"dlyMode",2},{"dlyMix",0.2f},{"dlySync",14},{"revMix",0.3f} });
    M (p, 1, MsLfo1, MdPitch, 0.008f); M (p, 2, MsPolyAT, MdNoise, 0.15f); M (p, 3, MsVel, MdLpf, 0.2f);
}
void lLumenLead (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-10},{"s_wave1",0},{"s_wave2",0},{"s_fine2",5},{"s_lvl2",0.6f},{"s_assign",2},{"s_uniDetune",0.25f},{"s_ftype",1},{"s_lpf",0.6f},{"s_lpfRes",0.3f},
            {"s_glide",0.1f},{"s_aS",0.9f},{"s_aR",0.35f},{"dlyMode",2},{"dlyMix",0.25f},{"dlySync",14},{"revMix",0.2f} });
    M (p, 1, MsPolyAT, MdLpf, 0.4f); M (p, 2, MsWheel, MdPitch, 0.01f);
}
void lVelvetHorn (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-8},{"s_wave1",1},{"s_pw1",0.4f},{"s_wave2",0},{"s_fine2",4},{"s_lvl2",0.5f},{"s_ftype",1},{"s_lpf",0.4f},{"s_lpfRes",0.1f},{"s_fltEnv",0.45f},{"s_fA",0.35f},{"s_fS",0.4f},
            {"s_aA",0.25f},{"s_aS",0.9f},{"s_aR",0.45f},{"ensMix",0.3f},{"revMix",0.25f} });
    M (p, 1, MsVel, MdFltEnv, 0.3f);
}
// ---- BASS
void bSatin (ZephyrAudioProcessor& p)
{
    S (p, { {"s_wave1",0},{"s_oct1",1},{"s_sub",0.6f},{"s_ftype",1},{"s_lpf",0.35f},{"s_lpfRes",0.2f},{"s_fltEnv",0.4f},{"s_fD",0.35f},{"s_fS",0.2f},
            {"s_aS",0.9f},{"s_aR",0.22f},{"s_drive",0.15f},{"compType",2},{"compAmt",0.4f} });
}
void bMoon (ZephyrAudioProcessor& p)
{
    S (p, { {"s_wave1",1},{"s_oct1",1},{"s_pw1",0.45f},{"s_ftype",1},{"s_lpf",0.3f},{"s_lpfRes",0.4f},{"s_fltEnv",0.55f},{"s_fD",0.3f},{"s_fS",0.1f},
            {"s_assign",1},{"s_glide",0.08f},{"s_aS",0.9f},{"s_aR",0.2f},{"s_drive",0.25f},{"s_sub",0.3f} });
    M (p, 1, MsVel, MdFltEnv, 0.4f);
}
void bDeepProphet (ZephyrAudioProcessor& p)
{
    S (p, { {"s_wave1",0},{"s_oct1",1},{"s_wave2",1},{"s_oct2",1},{"s_fine2",7},{"s_lvl2",0.5f},{"s_sub",0.3f},{"s_ftype",2},{"s_lpf",0.32f},{"s_lpfRes",0.35f},{"s_fltEnv",0.5f},
            {"s_fD",0.35f},{"s_fS",0.15f},{"s_aS",0.9f},{"s_aR",0.25f},{"s_drive",0.2f} });
}
void bRoundSub (ZephyrAudioProcessor& p)
{
    S (p, { {"s_wave1",3},{"s_oct1",1},{"s_wave2",2},{"s_oct2",1},{"s_lvl2",0.3f},{"s_sub",0.5f},{"s_ftype",0},{"s_lpf",0.5f},{"s_aS",1.f},{"s_aR",0.25f} });
}
// ---- ARP / SEQ
void aStarlight (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-9},{"s_wave1",2},{"s_wave2",0},{"s_oct2",3},{"s_lvl2",0.25f},{"s_ftype",0},{"s_lpf",0.5f},{"s_lpfRes",0.2f},{"s_fltEnv",0.5f},{"s_fD",0.2f},{"s_fS",0.f},
            {"s_aD",0.28f},{"s_aS",0.f},{"s_aR",0.4f},{"s_arpOn",1},{"s_arpMode",0},{"s_arpOct",1},{"s_arpSync",7},
            {"dlyMode",2},{"dlyMix",0.35f},{"dlySync",6},{"dlyFb",0.45f},{"revMix",0.35f},{"revShimmer",0.2f} });
}
void aCrystalCascade (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-9},{"s_wave1",3},{"s_wave2",3},{"s_oct2",3},{"s_fm",0.3f},{"s_lvl2",0.15f},{"s_ftype",2},{"s_lpf",0.8f},{"s_aD",0.3f},{"s_aS",0.f},{"s_aR",0.45f},
            {"s_arpOn",1},{"s_arpMode",2},{"s_arpOct",1},{"s_arpSync",7},{"dlyMode",1},{"dlyMix",0.35f},{"dlySync",14},{"dlyFb",0.5f},{"revMix",0.45f},{"revDecay",0.8f} });
    M (p, 1, MsRandom, MdFm, 0.2f);
}
void aPrismSeq (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-9},{"s_wave1",1},{"s_pw1",0.3f},{"s_ftype",2},{"s_lpf",0.4f},{"s_lpfRes",0.45f},{"s_fltEnv",0.55f},{"s_fD",0.18f},{"s_fS",0.f},{"s_aD",0.25f},{"s_aS",0.3f},{"s_aR",0.25f},
            {"s_arpOn",1},{"s_arpMode",0},{"s_arpOct",2},{"s_arpSync",7},{"phMix",0.4f},{"phSync",3},{"dlyMode",0},{"dlyMix",0.25f},{"dlySync",14},{"revMix",0.2f} });
    M (p, 1, MsLfo1, MdLpf, 0.2f); S (p, { {"s_l1Sync",3} });
}
void aMonopolyLights (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-9},{"s_wave1",0},{"s_wave2",0},{"s_fine2",8},{"s_lvl2",0.6f},{"s_ftype",1},{"s_lpf",0.45f},{"s_lpfRes",0.4f},{"s_fltEnv",0.5f},{"s_fD",0.2f},{"s_fS",0.1f},
            {"s_aD",0.25f},{"s_aS",0.5f},{"s_aR",0.2f},{"s_assign",3},{"s_arpOn",1},{"s_arpMode",2},{"s_arpOct",1},{"s_arpSync",6},{"dlyMode",2},{"dlyMix",0.3f},{"dlySync",5},{"revMix",0.25f} });
}
// ---- TEXTURE
void xIceCrystals (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-10},{"s_wave1",3},{"s_oct1",4},{"s_lvl1",0.4f},{"s_noise",0.3f},{"s_ring",0.4f},{"s_ftype",0},{"s_semMorph",0.5f},{"s_lpf",0.65f},{"s_lpfRes",0.6f},
            {"s_aA",0.6f},{"s_aS",1.f},{"s_aR",0.8f},{"s_l3Wave",4},{"s_l3Rate",0.45f},{"s_l3Slew",0.4f},
            {"dlyMode",1},{"dlyMix",0.5f},{"dlyFb",0.6f},{"revMix",0.6f},{"revShimmer",0.6f},{"revDecay",0.85f} });
    M (p, 1, MsLfo3, MdLpf, 0.3f);
}
void xSolarWind (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-9},{"s_lvl1",0.f},{"s_noise",0.6f},{"s_ftype",0},{"s_semMorph",0.35f},{"s_lpf",0.5f},{"s_lpfRes",0.55f},{"s_l4Rate",0.3f},{"s_l4Wave",4},{"s_l4Slew",0.6f},
            {"s_aA",0.8f},{"s_aS",1.f},{"s_aR",0.8f},{"phMix",0.4f},{"phRate",0.1f},{"revMix",0.55f},{"revDecay",0.8f},{"wideSpread",0.5f} });
    M (p, 1, MsLfo4, MdLpf, 0.5f); M (p, 2, MsPolyAT, MdLpf, 0.3f);
}
void xFrozenTime (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-10},{"s_wave1",2},{"s_wave2",3},{"s_oct2",3},{"s_fine2",5},{"s_lvl2",0.4f},{"s_ftype",2},{"s_lpf",0.6f},{"s_aA",0.8f},{"s_aS",1.f},{"s_aR",0.92f},{"s_drift",0.5f},
            {"dlyMode",1},{"dlyMix",0.3f},{"revMix",0.6f},{"revDecay",1.f},{"revSize",1.f},{"revShimmer",0.7f} });
    M (p, 1, MsWheel, MdRevDecay, 0.4f);
}
void xGossamer (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-9},{"s_wave1",0},{"s_wave2",0},{"s_fine2",9},{"s_lvl2",0.5f},{"s_hpf",0.45f},{"s_ftype",0},{"s_lpf",0.8f},{"s_aA",0.7f},{"s_aS",1.f},{"s_aR",0.85f},
            {"ensMix",0.7f},{"phMix",0.5f},{"phRate",0.08f},{"dlyMode",0},{"dlyMix",0.3f},{"wideSpread",0.6f},{"revMix",0.45f} });
}
void xHaloChoir (ZephyrAudioProcessor& p)
{
    S (p, { {"output",-9},{"s_wave1",1},{"s_pw1",0.3f},{"s_wave2",0},{"s_fine2",6},{"s_lvl2",0.5f},{"s_ftype",0},{"s_semMorph",0.3f},{"s_lpf",0.55f},{"s_lpfRes",0.6f},
            {"s_aA",0.65f},{"s_aS",1.f},{"s_aR",0.8f},{"s_l1Rate",0.18f},{"ensMix",0.5f},{"revMix",0.5f},{"revDecay",0.8f} });
    M (p, 1, MsLfo1, MdMorph, 0.5f); M (p, 2, MsPolyAT, MdLpf, 0.3f);
}

// ---- selection de macros preparee par categorie (slots 5..12 = macros 1..8). Les macros demarrent a 0 : le son du preset ne change pas tant qu'on n'y touche pas.
struct MacroDef { int dst; float amt; };
void applyMacroSet (ZephyrAudioProcessor& p, const juce::String& cat)
{
    static const MacroDef pad[8]   { { MdLpf, 0.5f }, { MdRevMix, 0.4f }, { MdRevDecay, 0.3f }, { MdFxEnsMix, 0.5f }, { MdDrift, 0.5f }, { MdFxPhMix, 0.5f }, { MdFltEnv, 0.4f }, { MdDlyMix, 0.4f } };
    static const MacroDef str[8]   { { MdLpf, 0.5f }, { MdFxEnsMix, 0.5f }, { MdFxEnsDepth, 0.5f }, { MdRevMix, 0.4f }, { MdDrift, 0.4f }, { MdSat, 0.4f }, { MdFltEnv, 0.4f }, { MdFxWidth, 0.5f } };
    static const MacroDef keys[8]  { { MdLpf, 0.5f }, { MdFltEnv, 0.5f }, { MdFm, 0.4f }, { MdRing, 0.4f }, { MdRevMix, 0.4f }, { MdSat, 0.4f }, { MdPolyMod, 0.4f }, { MdDlyMix, 0.4f } };
    static const MacroDef lead[8]  { { MdLpf, 0.5f }, { MdReso, 0.4f }, { MdSat, 0.5f }, { MdFltEnv, 0.5f }, { MdDrift, 0.4f }, { MdRevMix, 0.4f }, { MdDlyMix, 0.4f }, { MdPw1, 0.5f } };
    static const MacroDef bass[8]  { { MdLpf, 0.5f }, { MdReso, 0.4f }, { MdSat, 0.5f }, { MdFltEnv, 0.5f }, { MdSub, 0.5f }, { MdFold, 0.4f }, { MdMix, 0.4f }, { MdFxDrive, 0.4f } };
    static const MacroDef arp[8]   { { MdLpf, 0.5f }, { MdReso, 0.4f }, { MdDlyMix, 0.5f }, { MdDlyFb, 0.4f }, { MdRevMix, 0.4f }, { MdFltEnv, 0.5f }, { MdSat, 0.4f }, { MdFxPhMix, 0.4f } };
    static const MacroDef tex[8]   { { MdLpf, 0.5f }, { MdNoise, 0.5f }, { MdFm, 0.5f }, { MdRing, 0.5f }, { MdRevMix, 0.5f }, { MdDlyMix, 0.5f }, { MdFxPhMix, 0.5f }, { MdDrift, 0.5f } };
    static const MacroDef rack[8]  { { MdLpf, 0.5f }, { MdRevMix, 0.4f }, { MdDlyMix, 0.4f }, { MdFxDrive, 0.4f }, { MdFxEnsMix, 0.4f }, { MdFxPhMix, 0.4f }, { MdFxWidth, 0.4f }, { MdRevDecay, 0.3f } };
    const MacroDef* d = cat == "Pad" ? pad : cat == "Strings" ? str : cat == "Keys / Bells" ? keys : cat == "Brass / Lead" ? lead
                      : cat == "Bass" ? bass : cat == "Arp / Seq" ? arp : cat == "Texture" ? tex : rack;
    for (int k = 0; k < 8; ++k) M (p, 5 + k, macroSrc (k), d[k].dst, d[k].amt);
}
#endif

const Factory kFactory[] = {
    { "Init", "", fInit },
#if ZEPHYR_SYNTH
    { "Aurora Borealis", "Pad", pAurora }, { "Glass Cathedral", "Pad", pGlassCathedral }, { "Silver Lining", "Pad", pSilverLining }, { "Velvet Horizon", "Pad", pVelvetHorizon },
    { "Morning Dew", "Pad", pMorningDew }, { "Lumiere", "Pad", pLumiere }, { "Polaris Chords", "Pad", pPolaris },
    { "Solina Dreams", "Strings", sSolina }, { "Velvet Orchestra", "Strings", sVelvetOrch }, { "Cello Light", "Strings", sCelloLight }, { "Ensemble Royal", "Strings", sEnsembleRoyal }, { "Silk Strings", "Strings", sSilk },
    { "Tine Piano", "Keys / Bells", kTinePiano }, { "Wurli Gold", "Keys / Bells", kWurliGold }, { "Electric Grand", "Keys / Bells", kElectricGrand },
    { "Silk Organ", "Keys / Bells", kSilkOrgan }, { "Vibraphone Dream", "Keys / Bells", kVibraphone }, { "Crystal Bell", "Keys / Bells", kCrystalBell }, { "Poly-Mod Bell", "Keys / Bells", kPolyModBell },
    { "Golden Brass", "Brass / Lead", lGoldenBrass }, { "Prophecy Sync", "Brass / Lead", lProphecySync }, { "Silver Flute", "Brass / Lead", lSilverFlute }, { "Lumen Lead", "Brass / Lead", lLumenLead }, { "Velvet Horn", "Brass / Lead", lVelvetHorn },
    { "Satin Bass", "Bass", bSatin }, { "Moon Ladder", "Bass", bMoon }, { "Deep Prophet", "Bass", bDeepProphet }, { "Round Sub", "Bass", bRoundSub },
    { "Starlight Arp", "Arp / Seq", aStarlight }, { "Crystal Cascade", "Arp / Seq", aCrystalCascade }, { "Prism Sequence", "Arp / Seq", aPrismSeq }, { "Monopoly Lights", "Arp / Seq", aMonopolyLights },
    { "Ice Crystals", "Texture", xIceCrystals }, { "Solar Wind", "Texture", xSolarWind }, { "Frozen Time", "Texture", xFrozenTime }, { "Gossamer", "Texture", xGossamer }, { "Halo Choir", "Texture", xHaloChoir },
#endif
    { "String Machine", "Rack FX", fStringMachine }, { "Tape Echo Gold", "Rack FX", fTapeGold }, { "Ice Hall", "Rack FX", fIceHall },
    { "Wide & Bright", "Rack FX", fWideBright }, { "Phase Shimmer", "Rack FX", fPhaseShimmer }, { "Opto Glow", "Rack FX", fOptoGlow },
};
constexpr int kNumFactory = (int) (sizeof (kFactory) / sizeof (kFactory[0]));
}

juce::AudioProcessorValueTreeState::ParameterLayout ZephyrAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    for (const auto& d : kDefs)
    {
        const juce::ParameterID pid { d.id, 1 };
        if (d.kind == kBool)
            l.add (std::make_unique<juce::AudioParameterBool> (pid, d.name, d.def > 0.5f));
        else if (d.kind == kChoice)
            l.add (std::make_unique<juce::AudioParameterChoice> (pid, d.name, juce::StringArray::fromTokens (d.choices, "|", ""), (int) d.def));
        else
            l.add (std::make_unique<juce::AudioParameterFloat> (pid, d.name, juce::NormalisableRange<float> (d.mn, d.mx, 0.001f), d.def));
    }
#if ZEPHYR_SYNTH
    addSynthParams (l);
#endif
    return l;
}

ZephyrAudioProcessor::ZephyrAudioProcessor()
    : AudioProcessor (BusesProperties()
#if ! ZEPHYR_SYNTH
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
#endif
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, ZEPHYR_SYNTH ? "ZEPHYRSYNTH" : "ZEPHYRFX", createLayout())
{
    for (const auto& d : kDefs) raw.push_back (apvts.getRawParameterValue (d.id));
#if ZEPHYR_SYNTH
    for (const auto& d : synDefs()) rawSyn.push_back (apvts.getRawParameterValue (d.id));
#endif
    for (auto* prm : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
            if (mapIds.size() < kMaxMap) { mapIds.add (rp->paramID); mapParams.push_back (rp); }
    for (auto& c : mapCc) c.store (-1);
    for (auto& v : hwPos) v = -1.f;
    for (int i = 0; i < kMaxMap; ++i) { lastSetV[i] = -1.f; aCur[i] = aHw[i] = lastIn[i] = 0.f; caught[i] = anchored[i] = false; }
    resetMidiMap();
    if (const auto f = midiMapFile(); f.existsAsFile()) midiMapFromString (f.loadFileAsString());
    mapDirty.store (false);
#if ZEPHYR_SYNTH
    engine.setRack ({ NEnsemble, NDelay, NVerb });                       // nouvelle instance = comme le preset Init : ensemble + delay + reverb (mix a zero)
    setParamReal ("output", -6.f + kSynthMakeupDb);
#endif
}

// ---------------------------------------------------------------- MIDI learn universel
juce::File ZephyrAudioProcessor::midiMapFile() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Foudre Audio").getChildFile (ZEPHYR_SYNTH ? "zephyr-midimap-synth.txt" : "zephyr-midimap-fx.txt");
}
int ZephyrAudioProcessor::defaultCcOf (const juce::String& id) const
{
    for (int k = 0; k < 8; ++k) if (id == "s_m" + juce::String (k + 1)) return 16 + k;      // macros 1..8 = CC 16..23 (macros de l'Hydrasynth)
    // Carte par defaut = table "Sorted by CC Number" de l'Hydrasynth (un CC = un seul potard ; l'apprentissage MIDI peut tout rediriger)
    static const std::map<juce::String, int> hydra = {
        { "s_noise", 3 }, { "s_glide", 5 }, { "s_master", 7 },
        { "dlyFb", 14 }, { "dlyTime", 15 },
        { "s_pw1", 24 }, { "s_pw2", 26 }, { "s_fm", 30 },                   // wavscan OSC1/OSC2 -> largeur d'impulsion ; Mutator1 Depth -> FM
        { "s_ring", 43 }, { "s_lvl1", 44 }, { "s_lvl2", 46 },               // RM12 Depth ; OSC1 Vol ; OSC2 Vol
        { "s_drive", 50 }, { "s_key", 51 }, { "s_fltEnv", 54 }, { "s_fltLfo", 52 },             // Filter1 Drive / Keytrack / LFO1amt / ENV1amt
        { "s_hpf", 55 }, { "s_hpfRes", 56 },                                // Filter2 Cutoff / Res -> passe-haut
        { "dlyTone", 63 }, { "revDecay", 65 }, { "revDamp", 67 },          // delay tone ; reverb time / tone
        { "s_lpfRes", 71 }, { "s_l1Rate", 72 }, { "s_l2Rate", 73 }, { "s_lpf", 74 }, { "s_l3Rate", 76 }, { "s_l4Rate", 78 },
        { "s_fA", 81 }, { "s_fD", 82 }, { "s_fS", 83 }, { "s_fR", 84 },     // ENV1 = enveloppe du filtre (Filter1 ENV1amt)
        { "s_aA", 85 }, { "s_aD", 86 }, { "s_aS", 87 }, { "s_aR", 88 },     // ENV2 = enveloppe d'ampli
        { "revMix", 91 }, { "dlyMix", 92 }, { "ensMix", 93 }, { "phMix", 94 }, { "s_uniDetune", 95 },      // reverb, delay ; Pre-FX -> Ensemble, Post-FX -> Phaser
        { "s_arpSync", 106 }, { "s_arpRatchet", 109 }, { "s_arpGate", 107 }, { "s_arpMode", 108 }, { "s_arpChance", 110 }, { "s_arpOct", 120 }, { "s_arpLen", 122 },
        { "s_fine2", 112 } };                                               // OSC2 Cent
    const auto it = hydra.find (id);
    return it != hydra.end() ? it->second : -1;
}
void ZephyrAudioProcessor::resetMidiMap()
{
    for (auto& c : mapCc) c.store (-1);
    for (int i = 0; i < mapIds.size(); ++i) mapCc[(size_t) i].store (defaultCcOf (mapIds[i]));
    for (int i = 0; i < kMaxMap; ++i) anchored[i] = caught[i] = false;
    learnIdx.store (-1); mapDirty.store (true);
}
void ZephyrAudioProcessor::clearMidiAssign (const juce::String& id)
{
    const int i = mapIds.indexOf (id); if (i < 0) return;
    mapCc[(size_t) i].store (-1); anchored[i] = caught[i] = false; learnIdx.store (-1); mapDirty.store (true);
}
void ZephyrAudioProcessor::defaultMidiAssign (const juce::String& id)
{
    const int i = mapIds.indexOf (id); if (i < 0) return;
    mapCc[(size_t) i].store (defaultCcOf (id)); anchored[i] = caught[i] = false; learnIdx.store (-1); mapDirty.store (true);
}
float ZephyrAudioProcessor::atShape (float x) const
{
    x = juce::jlimit (0.f, 1.f, x);
    switch (atCurve.load())
    {
        case 1:  return x * x;                                              // exponentielle : depart doux
        case 2:  return std::sqrt (x);                                      // logarithmique : depart sensible
        case 3:  return x * x * (3.f - 2.f * x);                            // courbe en S
        default: return x;
    }
}
juce::String ZephyrAudioProcessor::midiMapToString() const
{
    juce::String s ("M" + juce::String (takeoverMode.load()));
    s << ";Q" << quality.load() << ";S" << ccSmooth.load() << ";AC" << atCurve.load() << ";AS" << atSpeed.load();
    for (int i = 0; i < mapIds.size(); ++i)
    {
        const int cc = mapCc[(size_t) i].load();
        if (cc != defaultCcOf (mapIds[i])) s << ";" << mapIds[i] << "=" << cc;           // seulement les ecarts par rapport aux CC d'origine
    }
    return s;
}
void ZephyrAudioProcessor::midiMapFromString (const juce::String& str)
{
    resetMidiMap();
    juce::StringArray t; t.addTokens (str.trim(), ";", "");
    for (auto& tok : t)
    {
        if (tok.startsWith ("M") && ! tok.contains ("=")) { takeoverMode.store (juce::jlimit (0, 2, tok.substring (1).getIntValue())); continue; }
        if (! tok.contains ("="))                                               // reglages globaux : qualite, lissage, aftertouch
        {
            if (tok.startsWith ("AC")) atCurve.store (juce::jlimit (0, 3, tok.substring (2).getIntValue()));
            else if (tok.startsWith ("AS")) atSpeed.store (juce::jlimit (0, 5, tok.substring (2).getIntValue()));
            else if (tok.startsWith ("Q")) quality.store (juce::jlimit (0, 2, tok.substring (1).getIntValue()));
            else if (tok.startsWith ("S")) ccSmooth.store (juce::jlimit (0, 5, tok.substring (1).getIntValue()));
            continue;
        }
        const int eq = tok.indexOfChar ('=');
        if (eq <= 0) continue;
        const int i = mapIds.indexOf (tok.substring (0, eq));
        if (i >= 0) mapCc[(size_t) i].store (juce::jlimit (-1, 127, tok.substring (eq + 1).getIntValue()));
    }
    mapDirty.store (false);
}
void ZephyrAudioProcessor::flushMidiMapIfDirty()
{
    if (! mapDirty.exchange (false)) return;
    const auto f = midiMapFile();
    f.getParentDirectory().createDirectory();
    f.replaceWithText (midiMapToString());
}

bool ZephyrAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
#if ZEPHYR_SYNTH
    return out == juce::AudioChannelSet::stereo();
#endif
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

void ZephyrAudioProcessor::prepareToPlay (double sampleRate, int)
{
    setLatencySamples (0);
    engine.prepare ((float) sampleRate);
#if ZEPHYR_SYNTH
    synth.prepare ((float) sampleRate);
    arp.prepare ((float) sampleRate); arp.setPattern (&arpPat);
    evs.clear(); evs.reserve (2048);
#endif
}

zephyr::Params ZephyrAudioProcessor::readParams() const
{
    int i = 0;
    auto f = [this, &i] { return raw[(size_t) i++]->load(); };
    auto b = [&f] { return f() > 0.5f; };
    auto n = [&f] { return juce::roundToInt (f()); };

    zephyr::Params p;
    p.on = b();  p.output = f();  p.mix = f();
    p.wideWidth = f();  p.wideSpread = f();  p.wideBass = f();
    p.satMode = n();  p.satDrive = f();  p.satTone = f();
    p.low = f();  p.mid = f();  p.high = f();
    p.hpf = f();  p.hpfPeak = f();  p.lpf = f();  p.lpfPeak = f();
    p.phMix = f();  p.phRate = f();  p.phDepth = f();  p.phFb = f();  p.phStages = n();  p.phSync = n();
    p.compType = n();  p.compAmt = f();  p.compThresh = f();  p.compAtt = f();
    p.ensMix = f();  p.ensDepth = f();  p.ensSpeed = f();  p.ensTone = f();
    p.lfo1Rate = f();  p.lfo1Shape = f();  p.lfo1Level = f();  p.lfo1Sync = n();
    p.lfo2Rate = f();  p.lfo2Shape = f();  p.lfo2Level = f();  p.lfo2Sync = n();
    p.shRate = f();  p.shSlew = f();  p.shLevel = f();  p.shSync = n();
    p.envRel = f();  p.envLevel = f();
    p.dlyMode = n();  p.dlyTime = f();  p.dlyFb = f();  p.dlyTone = f();  p.dlyMod = f();  p.dlyMix = f();  p.dlySync = n();
    p.revMix = f();  p.revSize = f();  p.revDecay = f();  p.revPre = f();  p.revDamp = f();  p.revMod = f();  p.revWidth = f();  p.revShimmer = f();  p.revLowCut = f();  p.revFreeze = b();
    p.stutOn = b();  p.stutMode = n();  p.stutDiv = n();  p.stutFeel = n();  p.stutLen = f();  p.stutDecay = f();  p.stutPitch = f();  p.stutMix = f();
    p.spaceMode = n();
    i += 4;                                                                 // destinations des sources : appliquees dans applyModRoutes()
    jassert (i == kNumDefs);
    return p;
}


#if ZEPHYR_SYNTH
zephyr::SynthParams ZephyrAudioProcessor::readSynth()
{
    size_t i = 0;
    auto f = [this, &i] { return rawSyn[i++]->load(); };
    auto b = [&f] { return f() > 0.5f; };
    auto n = [&f] { return juce::roundToInt (f()); };
    auto sec = [&f] { return 0.001f * std::pow (5000.f, f()); };      // 1 ms .. 5 s
    SynthParams s;
    s.wave1 = n(); s.oct1 = n() - 2; s.wave2 = n(); s.oct2 = n() - 2;
    s.tune2 = f(); s.fine2 = f(); s.pw1 = f(); s.pw2 = f();
    s.sync = b() ? 1 : 0; s.fm = f(); s.ring = f();
    s.lvl1 = f(); s.lvl2 = f(); s.noise = f(); s.glide = f();
    s.hpf = f(); s.hpfRes = f(); s.lpf = f(); s.lpfRes = f(); s.fltEnv = f(); s.keytrack = f(); s.drive = f();
    s.aA = sec(); s.aD = sec(); s.aS = f(); s.aR = sec();
    s.fA = sec(); s.fD = sec(); s.fS = f(); s.fR = sec();
    s.master = f();
    for (auto& l : s.lfo) { l.rate = f(); l.wave = n(); l.slew = f(); l.fade = f(); l.sync = n(); l.trig = b() ? 1 : 0; }
    for (int k = 0; k < 4; ++k) s.macro[k] = f();
    for (auto& sl : s.slot) { sl.src = n(); sl.dst = n(); sl.amt = f(); }
    s.fold = f(); s.foldSym = f();
    arpParams.on = n(); arpParams.dir = n(); arpParams.range = n();
    s.ftype = n(); s.semMorph = f();
    s.assign = n(); s.hold = b() ? 1 : 0; s.uniDetune = f(); chordLearn = b();
    arpParams.assign = s.assign; arpParams.hold = s.hold != 0;
    for (int k = 4; k < kMacros; ++k) s.macro[k] = f();
    arpRateKnob = f(); arpSyncDiv = n();
    s.sub = f(); s.drift = f(); s.pmEnv = f(); s.pmOsc = f(); s.pmFreq = b(); s.pmPw = b(); s.pmFilt = b();
    arpParams.shift = f(); arpParams.cluster = f();
    arpParams.gate = f(); arpParams.swing = f(); arpParams.chance = f(); arpParams.free = b();
    arpParams.len = n() + 1;
    for (int k = 0; k < kLanes; ++k) { arpParams.laneLen[k] = n() + 1; s.lane[k].dst = n(); s.lane[k].amt = f(); s.lane[k].glide = f(); }
    s.fmDir = n(); s.fmRatio = n(); s.fmWave = n(); s.fmFine = f(); s.fmAtt = f(); s.fmDec = f(); s.fmEnvAmt = f(); s.fmVel = f(); s.fmAT = f();
    arpParams.seq = n() == 1; seqRec = b();
    bendRange = (float) (n() + 1);
    s.fltLfo = f();
    arpParams.ratchet = n() + 1; arpTriplet = b();
    s.vib = f(); s.vibWheel = f(); s.vibAT = f();
    jassert (i == rawSyn.size());
    s.quality = quality.load();
    {                                                                       // lissage des reglages (pas de crenelage quand on tourne un potard MIDI 7 bits)
        static const float tauMs[6] { 0.f, 5.f, 10.f, 20.f, 40.f, 80.f };
        const float tau = tauMs[juce::jlimit (0, 5, ccSmooth.load())];
        const float a = (tau <= 0.f || ! smInit) ? 1.f : 1.f - std::exp (-blockMs / tau);
        float* v[] { &s.lpf, &s.lpfRes, &s.hpf, &s.hpfRes, &s.drive, &s.fltEnv, &s.fm, &s.ring, &s.fold, &s.lvl1, &s.lvl2, &s.noise, &s.master, &s.fltLfo, &s.keytrack };
        for (int k = 0; k < 15; ++k) { smV[k] = (a >= 1.f) ? *v[k] : smV[k] + a * (*v[k] - smV[k]); *v[k] = smV[k]; }
        smInit = true;
    }
    return s;
}
#endif

void ZephyrAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int nc = juce::jmin (2, buffer.getNumChannels());
    const int total = buffer.getNumSamples();
    blockMs = 1000.f * (float) total / (float) juce::jmax (8000.0, getSampleRate());
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, total);
    if (nc == 0 || total == 0) return;

    // tempo et position d'Ableton (synchro des LFO, du S&H et de la destroy machine)
    double bpm = 120.0, ppq = 0.0;
    bool playing = false, hasPpq = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto bp = pos->getBpm()) bpm = *bp;
            if (auto q = pos->getPpqPosition()) { ppq = *q; hasPpq = true; }
            playing = pos->getIsPlaying();
        }
    engine.setHost (bpm, ppq, playing, hasPpq);

#if ZEPHYR_SYNTH
    {
        buffer.clear();
        keyboardState.processNextMidiBuffer (midi, 0, total, true);     // clavier a l'ecran -> notes MIDI
        synth.setHost (bpm);
        {   // controleurs MIDI : appliques AVANT la lecture des reglages, et une seule fois par bloc (derniere valeur) -> pas de bloc de retard, moins de messages vers l'hote
            int lastCcV[128]; bool seen[128] = {};
            {   // MPE : notes tenues sur plusieurs canaux, ou canal qui tourne d'une note a l'autre (dans les 30 dernieres secondes)
                int c0 = -1; bool multi = false;
                for (int nt = 0; nt < 128 && ! multi; ++nt) if (keyDown[nt] || pedalHeld[nt]) { if (c0 < 0) c0 = noteChan[nt]; else if (noteChan[nt] != c0) multi = true; }
                for (const auto meta : midi)                                                       // notes de ce bloc (les CC74 MPE arrivent avec leur note)
                {
                    const auto m = meta.getMessage();
                    if (! m.isNoteOn()) continue;
                    const int ch = m.getChannel();
                    if (c0 < 0) c0 = ch; else if (ch != c0) multi = true;
                    if (lastNoteChan > 0 && ch != lastNoteChan && ch != 1) multi = true;
                }
                if (multi) lastMultiChan = sampleClock;
                mpeActive = sampleClock - lastMultiChan < (juce::int64) (30.0 * (getSampleRate() > 0.0 ? getSampleRate() : 48000.0));
            }
            for (const auto meta : midi)
            {
                const auto m = meta.getMessage();
                if (mpeActive && m.getChannel() != 1 && m.isController() && m.getControllerNumber() != 64) continue;    // MPE : les CC des canaux de notes (74 = timbre par note) ne touchent pas aux potards (seul le canal maitre 1 pilote)
                if (m.isController()) { const int c = m.getControllerNumber(); lastCcV[c] = m.getControllerValue(); seen[c] = true; }
            }
            for (int c = 0; c < 128; ++c) if (seen[c]) handleCC (c, lastCcV[c]);
        }
        { static const float atTau[6] { 0.0015f, 0.005f, 0.012f, 0.03f, 0.07f, 0.16f }; synth.setAtResponse (atTau[juce::jlimit (0, 5, atSpeed.load())]); }   // secondes, par lisseur (2 en cascade)
        synth.setParams (readSynth());
        {   // modulation globale (matrice, pistes) de l'arpegiateur : valeurs du bloc precedent
            const float* gm = synth.fxMod();
            arpParams.gate   = juce::jlimit (0.05f, 1.f, arpParams.gate + 0.5f * gm[39]);
            arpParams.swing  = juce::jlimit (0.f, 1.f, arpParams.swing + 0.5f * gm[40]);
            arpParams.chance = juce::jlimit (0.f, 1.f, arpParams.chance + 0.5f * gm[41]);
        }
        float* sl = buffer.getWritePointer (0);
        float* sr = nc > 1 ? buffer.getWritePointer (1) : buffer.getWritePointer (0);

        // ---- evenements du bloc : MIDI direct, ou arpegiateur (qui genere ses propres notes)
        evs.clear();
        const bool recActive = arpParams.seq && seqRec;                    // enregistrement pas a pas : les touches jouent normalement et remplissent les pas
        zephyr::ArpParams ap = arpParams; if (recActive) ap.on = 0;
        arp.setParams (ap);
        const bool arpOn = ap.on != 0;
        if (recActive != lastRec) { lastRec = recActive; synth.allNotesOff(); arp.allOff(); if (recActive) arpPat.recPos = 0; }
        if (ap.on != lastArpOn) { lastArpOn = ap.on; lastAssign = ap.assign; synth.allNotesOff(); arp.allOff(); }
        if (ap.assign != lastAssign) { lastAssign = ap.assign; arp.resetVoices(); }     // changer de mode ne coupe pas l'arpege : les touches restent, le motif continue
        if (chordLearn != lastLearn)
        {
            if (chordLearn) { learnN = 0; }
            else if (learnN > 0)
            {
                int iv[6], n = 0; const int root = learnNotes[0];
                for (int k = 1; k < learnN && n < 6; ++k) iv[n++] = learnNotes[k] - root;
                if (n == 0) { iv[0] = 0; n = 1; }
                for (int k = 0; k < n; ++k) chordIv[k] = iv[k];
                chordN = n;
            }
            lastLearn = chordLearn;
        }
        synth.setChord (chordIv, chordN); arp.setChord (chordIv, chordN);
        for (const auto meta : midi)
        {
            const auto m = meta.getMessage();
            const int pos = juce::jlimit (0, total - 1, meta.samplePosition);
            if (m.isProgramChange()) { pendingProgram.store (m.getProgramChangeNumber()); triggerAsyncUpdate(); continue; }
            if (m.isController() && m.getControllerNumber() == 64)           // pedale de sustain : les fins de note sont retenues tant qu'elle est enfoncee
            {
                const bool dn = m.getControllerValue() >= 64;
                if (pedal && ! dn)
                    for (int nt = 0; nt < 128; ++nt)
                        if (pedalHeld[nt]) { pedalHeld[nt] = false; keyDown[nt] = false; if (arpOn) arp.noteOff (nt); else evs.push_back ({ pos, 1, nt, 0.f }); }
                pedal = dn;
            }
            if (m.isNoteOn())
            {
                const int nn = m.getNoteNumber(), ch = m.getChannel();
                for (int nt = 0; nt < 128; ++nt)
                    if ((keyDown[nt] || pedalHeld[nt]) && nt != nn && noteChan[nt] != ch) { lastMultiChan = sampleClock + pos; mpeActive = true; break; }
                if (lastNoteChan > 0 && ch != lastNoteChan && ch != 1) { lastMultiChan = sampleClock + pos; mpeActive = true; }   // canal qui change a chaque note : controleur MPE
                lastNoteChan = ch;
                noteChan[nn] = ch; chPress[ch] = 0.f;
                keyDown[nn] = true; pedalHeld[nn] = false;
            }
            else if (m.isNoteOff() && pedal && keyDown[m.getNoteNumber()]) { pedalHeld[m.getNoteNumber()] = true; continue; }
            else if (m.isNoteOff()) keyDown[m.getNoteNumber()] = false;
            if (m.isController()) { lastMidiMsg.store ((2 << 16) | (m.getControllerNumber() << 8) | m.getControllerValue()); if (m.getControllerNumber() == 1) wheelShown.store (m.getControllerValue()); }
            else if (m.isNoteOn()) lastMidiMsg.store ((1 << 16) | (m.getNoteNumber() << 8) | m.getVelocity());
            else if (m.isPitchWheel()) lastMidiMsg.store ((3 << 16) | ((m.getPitchWheelValue() >> 7) << 8) | (m.getPitchWheelValue() & 127));
            else if (m.isChannelPressure() || m.isAftertouch()) lastMidiMsg.store ((4 << 16) | (m.getChannelPressureValue() & 127));
            if (m.isNoteOn())
            {
                if (chordLearn && learnN < 6) learnNotes[learnN++] = m.getNoteNumber();
                if (recActive)
                {
                    const int rp = juce::jlimit (0, zephyr::kArpSteps - 1, arpPat.recPos.load());
                    arpPat.seqNote[rp] = m.getNoteNumber(); arpPat.step[rp] = m.getFloatVelocity() >= 0.85f ? 2 : 1; arpPat.tie[rp] = 0;
                    arpPat.recPos = (rp + 1) % juce::jlimit (1, zephyr::kArpSteps, arpParams.len);
                }
                if (chordLearn) continue;                                    // en apprentissage, les touches ne jouent pas
                if (arpOn) arp.noteOn (m.getNoteNumber(), m.getFloatVelocity()); else evs.push_back ({ pos, 0, m.getNoteNumber(), m.getFloatVelocity() });
            }
            else if (m.isNoteOff())      { if (chordLearn) continue; if (arpOn) arp.noteOff (m.getNoteNumber()); else evs.push_back ({ pos, 1, m.getNoteNumber(), 0.f }); }
            else if (m.isAllNotesOff() || m.isAllSoundOff()) { pedal = false; for (int nt = 0; nt < 128; ++nt) keyDown[nt] = pedalHeld[nt] = false; arp.allOff(); evs.push_back ({ pos, 2, 0, 0.f }); }
            else if (m.isPitchWheel())
            {
                const float b = (float) (m.getPitchWheelValue() - 8192) / 8192.f;
                if (mpeActive && m.getChannel() != 1)                                                  // MPE : bend de la note de ce canal (etendue MPE standard 48 demi-tons)
                {
                    for (int nt = 0; nt < 128; ++nt) if ((keyDown[nt] || pedalHeld[nt]) && noteChan[nt] == m.getChannel()) evs.push_back ({ pos, 15, nt, b * 48.f });
                }
                else evs.push_back ({ pos, 3, 0, b * bendRange });
            }
            else if (m.isAftertouch())   evs.push_back ({ pos, 5, m.getNoteNumber(), atShape ((float) m.getAfterTouchValue() / 127.f) });
            else if (m.isChannelPressure())
            {
                const float v = atShape ((float) m.getChannelPressureValue() / 127.f);
                const int ch = m.getChannel();
                chPress[ch] = v;
                if (mpeActive && ! arpOn)                                                                 // MPE : chaque note suit SA pression (avant : une seule pression qui sautait d'une note a l'autre -> tous les filtres tremblaient)
                {
                    float mx = 0.f;
                    for (int nt = 0; nt < 128; ++nt)
                        if (keyDown[nt] || pedalHeld[nt])
                        {
                            if (noteChan[nt] == ch) evs.push_back ({ pos, 5, nt, v });
                            mx = std::max (mx, chPress[noteChan[nt]]);
                        }
                    evs.push_back ({ pos, 6, 0, mx });                                                  // pression globale (source AFTERTOUCH, effets) : la plus forte, stable
                }
                else evs.push_back ({ pos, 6, 0, v });
            }
            else if (m.isController() && m.getControllerNumber() != 1) {}                // deja traite en debut de bloc
            else if (m.isController() && m.getControllerNumber() == 1 && ! (mpeActive && m.getChannel() != 1)) { evs.push_back ({ pos, 4, 0, (float) m.getControllerValue() / 127.f }); }
        }
        {
            zephyr::Arp::Ev ae[256];
            const int ac = arp.process (total, arpStepHz (bpm) * std::pow (2.f, 2.f * synth.fxMod()[38]), ae, 256);
            arpStepShown.store (arp.currentStep());
            for (int k = 0; k < ac; ++k)
            {
                const int ps = juce::jlimit (0, total - 1, ae[k].pos);
                if (ae[k].lane >= 0) evs.push_back ({ ps, 10, ae[k].lane, ae[k].val });
                else if (ae[k].slot >= 0) evs.push_back ({ ps, ae[k].on ? 12 : 13, ae[k].note | (ae[k].slot << 8), ae[k].vel });
                else evs.push_back ({ ps, ae[k].on ? (ae[k].stack ? 8 : 7) : 9, ae[k].note, ae[k].vel });
            }
        }
        std::stable_sort (evs.begin(), evs.end(), [] (const MidiEv& a, const MidiEv& b) { return a.pos < b.pos; });

        int pos = 0;
        auto render = [&] (int upTo)
        {
            if (upTo > pos) { synth.process (sl + pos, sr + pos, upTo - pos); pos = upTo; }
        };
        synth.setPerNoteAT (mpeActive && ! arpOn);
        for (const auto& e : evs)
        {
            const bool cont = e.type == 3 || e.type == 4 || e.type == 5 || e.type == 6 || e.type == 15;     // controles continus (lisses) : pas de decoupe a l'echantillon pres
            render (cont ? e.pos - (e.pos % 32) : e.pos);
            switch (e.type)
            {
                case 0: synth.noteOn (e.a, e.v); break;
                case 1: synth.noteOff (e.a); break;
                case 2: synth.allNotesOff(); break;
                case 3: synth.setBend (e.v); break;
                case 10: synth.setLane (e.a, e.v); break;
                case 12: synth.noteOnSlot (e.a & 255, e.v, e.a >> 8); break;
                case 13: synth.noteOffSlot (e.a >> 8, e.a & 255); break;
                case 7: synth.noteOnRaw (e.a, e.v); break;
                case 8: synth.noteOnStack (e.a, e.v); break;
                case 9: synth.noteOffRaw (e.a); break;
                case 5: synth.setPolyAT (e.a, e.v); break;
                case 6: synth.setChanAT (e.v); break;
                case 15: synth.setNoteBend (e.a, e.v); break;
                default: synth.setWheel (e.v); break;
            }
        }
        render (total);
        sampleClock += total;
        engine.setExtMod (synth.fxMod());
    }
#endif
    engine.setParams (readParams());
    applyModRoutes();
    float* io[2] = { buffer.getWritePointer (0), nc > 1 ? buffer.getWritePointer (1) : nullptr };
    engine.process (io, total, nc);
    {
        int w = scopeW.load (std::memory_order_relaxed);
        for (int i = 0; i < total; ++i)
        {
            const float l = io[0][i], r = io[1] != nullptr ? io[1][i] : l;
            scopeBuf[(w + i) & (kScope - 1)] = 0.5f * (l + r);
        }
        scopeW.store ((w + total) & (kScope - 1), std::memory_order_release);
    }
}

juce::AudioProcessorEditor* ZephyrAudioProcessor::createEditor()
{
    return new SynthEditor (*this);
}

// ---------------------------------------------------------------- etat (projet Ableton) et presets
juce::ValueTree ZephyrAudioProcessor::captureState()
{
    auto state = apvts.copyState();
    state.setProperty ("lfoV2", 1, nullptr);                               // plage de vitesse des LFO elargie (0.05..300 Hz)
    state.setProperty ("zv2", 1, nullptr); state.setProperty ("zv2c", 1, nullptr);                                 // Zephyr v2 : filtres circuit (0 MS-10, 1 CEM3320, 2 SEM, 3 Prophet), nouvelles destinations
    state.setProperty ("patch", juce::String (engine.serializePatch()), nullptr);
    state.setProperty ("rack", juce::String (engine.serializeRack()), nullptr);
    state.setProperty ("bypass", (int) engine.bypassMask.load(), nullptr);
    state.setProperty ("presetName", presetName, nullptr);
    state.setProperty ("category", presetCategory, nullptr);
    state.setProperty ("hdr", headerScope.load(), nullptr);
    { juce::String c; for (int k = 0; k < chordN; ++k) c << chordIv[k] << ","; state.setProperty ("chord", c, nullptr); }
#if ZEPHYR_SYNTH
    {
        juce::String c;
        for (int k = 0; k < zephyr::kArpSteps; ++k) c << arpPat.step[k].load() << ":" << arpPat.ratchet[k].load() << ":" << arpPat.seqNote[k].load() << ":" << arpPat.tie[k].load() << ",";
        for (int l = 0; l < zephyr::kArpLanes; ++l) { c << "|"; for (int k = 0; k < zephyr::kArpSteps; ++k) c << juce::String (arpPat.lane[l][k].load(), 3) << ","; }
        state.setProperty ("arpPat", c, nullptr);
    }
#endif
    return state;
}

void ZephyrAudioProcessor::applyState (juce::ValueTree state)
{
    engine.loadPatch (state.getProperty ("patch", "").toString().toStdString());
#if ! ZEPHYR_SYNTH
    if (! state.hasProperty ("zv2c"))                                      // ancien patchbay : chaque source garde sa premiere destination
    {
        static const char* ids[4] { "lfo1Dst", "lfo2Dst", "shDst", "envDst" };
        static const int src[4] { zephyr::SrcLfo1, zephyr::SrcLfo2, zephyr::SrcSh, zephyr::SrcEnv };
        for (int k = 0; k < 4; ++k)
            for (int m = 1; m < 16; ++m)
                if (engine.cvIn[kModDstCv[m]].load() & (1u << src[k]))
                {
                    for (auto c : state) if (c.hasProperty ("id") && c.getProperty ("id").toString() == ids[k]) c.setProperty ("value", m, nullptr);
                    break;
                }
    }
#endif
    const auto rk = state.getProperty ("rack", "").toString();
    if (rk.isNotEmpty()) engine.loadRack (rk.toStdString()); else engine.setDefaultRack();
    engine.bypassMask.store ((uint32_t) (int) state.getProperty ("bypass", 0));
    presetName = state.getProperty ("presetName", "Init").toString();
    presetCategory = state.getProperty ("category", "").toString();
    headerScope.store ((int) state.getProperty ("hdr", 0));
    {
        const auto cs = state.getProperty ("chord", "").toString();
        if (cs.isNotEmpty()) { juce::StringArray t; t.addTokens (cs, ",", ""); t.removeEmptyStrings(); chordN = 0; for (auto& x : t) if (chordN < 6) chordIv[chordN++] = x.getIntValue(); }
        else { const int d[4] { 4, 7, 11, 0 }; chordN = 3; for (int k = 0; k < 3; ++k) chordIv[k] = d[k]; }
    }
#if ZEPHYR_SYNTH
    {
        arpPat.reset();
        juce::StringArray parts; parts.addTokens (state.getProperty ("arpPat", "").toString(), "|", "");
        if (parts.size() > 0)
        {
            juce::StringArray t; t.addTokens (parts[0], ",", ""); t.removeEmptyStrings();
            for (int k = 0; k < zephyr::kArpSteps && k < t.size(); ++k)
            {
                juce::StringArray f; f.addTokens (t[k], ":", "");
                arpPat.step[k] = juce::jlimit (0, 2, f[0].getIntValue()); arpPat.ratchet[k] = juce::jlimit (1, 4, f[1].getIntValue());
                arpPat.seqNote[k] = f.size() > 2 ? juce::jlimit (0, 127, f[2].getIntValue()) : 60; arpPat.tie[k] = f.size() > 3 ? juce::jlimit (0, 1, f[3].getIntValue()) : 0;
            }
            for (int l = 0; l < zephyr::kArpLanes && l + 1 < parts.size(); ++l)
            {
                juce::StringArray v; v.addTokens (parts[l + 1], ",", ""); v.removeEmptyStrings();
                for (int k = 0; k < zephyr::kArpSteps && k < v.size(); ++k) arpPat.lane[l][k] = juce::jlimit (0.f, 1.f, v[k].getFloatValue());
            }
        }
    }
#endif
    if (! state.hasProperty ("zv2"))                                       // projet / preset Zephyr v1
        for (auto c : state)
            if (c.hasProperty ("id"))
            {
                const auto id = c.getProperty ("id").toString();
                const int v = juce::roundToInt ((float) c.getProperty ("value", 0.0));
                if (id == "s_ftype") { static const int remap[3] { 2, 1, 3 }; c.setProperty ("value", remap[juce::jlimit (0, 2, v)], nullptr); }     // SEM / Ladder / Prophet -> circuits
                else if (id.startsWith ("s_slot") && id.endsWith ("Dst") && v >= 38 && v <= 40)                                                       // sub / poly-mod / derive ont change de numero
                    c.setProperty ("value", (int) MdSub + (v - 38), nullptr);
            }
    if (! state.hasProperty ("lfoV2"))                                     // ancienne plage 0.05..40 Hz : on convertit pour garder la meme vitesse
        for (auto c : state)
            if (c.hasProperty ("id"))
            {
                const auto id = c.getProperty ("id").toString();
                if (id.startsWith ("s_l") && id.endsWith ("Rate") && id.length() == 8)
                    c.setProperty ("value", (float) c.getProperty ("value", 0.4) * 0.7677f, nullptr);
            }
    apvts.replaceState (state);
}

void ZephyrAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto st = captureState();
    st.setProperty ("midiMap", midiMapToString(), nullptr);                // assignations MIDI : avec le projet, pas avec les presets
    if (auto xml = st.createXml()) copyXmlToBinary (*xml, dest);
}

void ZephyrAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            const auto vt = juce::ValueTree::fromXml (*xml);
            applyState (vt);
            if (vt.hasProperty ("midiMap")) midiMapFromString (vt.getProperty ("midiMap").toString());
            else if (vt.hasProperty ("macroCC"))                                // projets d'une ancienne version : CC des macros sauves a part
            {
                juce::StringArray t; t.addTokens (vt.getProperty ("macroCC").toString(), ",", ""); t.removeEmptyStrings();
                resetMidiMap();
                const bool oldDefault = vt.getProperty ("macroCC").toString().startsWith ("21,22,23,24,25,26,27,28");     // ancien reglage d'usine : on prend les CC 16..23
                for (int k = 0; k < 8 && k < t.size() && ! oldDefault; ++k)
                    if (const int i = mapIds.indexOf ("s_m" + juce::String (k + 1)); i >= 0) mapCc[(size_t) i].store (t[k].getIntValue());
            }
        }
}

void ZephyrAudioProcessor::setParamReal (const char* id, float real)
{
    if (auto* prm = apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (real));
}

#if ZEPHYR_SYNTH
int ZephyrAudioProcessor::modDstOfParam (const juce::String& id)
{
    struct E { const char* id; int dst; };
    static const E tab[] = {
        { "s_pw1", MdPw1 }, { "s_pw2", MdPw2 }, { "s_fm", MdFm }, { "s_ring", MdRing }, { "s_lvl2", MdMix }, { "s_lvl1", MdLvl1 }, { "s_noise", MdNoise },
        { "s_hpf", MdHpf }, { "s_hpfRes", MdHpfReso }, { "s_lpf", MdLpf }, { "s_lpfRes", MdReso }, { "s_fltEnv", MdFltEnv }, { "s_drive", MdDrive }, { "s_semMorph", MdMorph },
        { "s_fold", MdFold }, { "s_foldSym", MdFoldSym }, { "s_glide", MdGlide }, { "s_key", MdKeyTrack },
        { "s_aA", MdAmpA }, { "s_aD", MdAmpD }, { "s_aS", MdAmpS }, { "s_aR", MdAmpR }, { "s_fA", MdFltA }, { "s_fD", MdFltD }, { "s_fS", MdFltS }, { "s_fR", MdFltR },
        { "s_arpRate", MdArpRate }, { "s_arpGate", MdArpGate }, { "s_arpSwing", MdArpSwing }, { "s_arpChance", MdArpChance },
        { "s_l1Rate", MdLfo1Rate }, { "s_l2Rate", MdLfo2Rate }, { "s_l3Rate", MdLfo3Rate }, { "s_l4Rate", MdLfo4Rate },
        { "s_sub", MdSub }, { "s_pmEnv", MdPolyMod }, { "s_pmOsc", MdPolyMod }, { "s_drift", MdDrift },
        { "satDrive", MdSatDrive }, { "satTone", MdSatTone }, { "wideWidth", MdWideWidth }, { "wideSpread", MdWideSpread }, { "hpf", MdFxHpf }, { "lpf", MdFxLpf },
        { "phMix", MdPhMix }, { "phRate", MdPhRate }, { "phFb", MdPhFb }, { "ensDepth", MdEnsDepth }, { "ensMix", MdEnsMix }, { "ensSpeed", MdEnsSpeed },
        { "mix", MdFxMix }, { "compAmt", MdCompAmt }, { "low", MdEqLow }, { "mid", MdEqMid }, { "high", MdEqAir },
        { "dlyTime", MdDlyTime }, { "dlyFb", MdDlyFb }, { "dlyMix", MdDlyMix }, { "dlyTone", MdDlyTone }, { "dlyMod", MdDlyMod },
        { "revMix", MdRevMix }, { "revDecay", MdRevDecay }, { "revDamp", MdRevDamp }, { "revSize", MdRevSize }, { "revPre", MdRevPre }, { "revShimmer", MdRevShimmer }, { "revWidth", MdRevWidth },
        { "stutLen", MdStutLen }, { "stutDecay", MdStutDecay }, { "stutPitch", MdStutPitch }, { "stutMix", MdStutMix } };
    for (const auto& e : tab) if (id == e.id) return e.dst;
    return 0;
}

float ZephyrAudioProcessor::modScaleOfDst (int)          // 100 % = tour complet du potard (la matrice compense l'echelle native du moteur, cf. modNative)
{
    return 1.f;
}

int ZephyrAudioProcessor::knobMods (int dst, KnobMod out[2]) const
{
    int n = 0;
    if (dst <= 0) return 0;
    for (int i = 1; i <= kSlots && n < 2; ++i)
    {
        const juce::String q ("s_slot" + juce::String (i));
        const int d = juce::roundToInt (apvts.getRawParameterValue (q + "Dst")->load()), s = juce::roundToInt (apvts.getRawParameterValue (q + "Src")->load());
        if (d == dst && s > 0) out[n++] = { i, s, apvts.getRawParameterValue (q + "Amt")->load() };
    }
    return n;
}

int ZephyrAudioProcessor::assignKnobMod (int dst, int src)
{
    KnobMod cur[2]; const int n = knobMods (dst, cur);
    for (int k = 0; k < n; ++k) if (cur[k].src == src)                                // deja routee : on la reutilise (et on lui donne une quantite si elle etait a zero, sinon rien ne s'entend)
    {
        if (std::fabs (cur[k].amt) < 0.01f) setKnobModAmt (cur[k].slot, 0.5f);
        return cur[k].slot;
    }
    if (n >= 2) return -2;
    for (int i = 1; i <= kSlots; ++i)                                                 // premier slot libre
    {
        const juce::String q ("s_slot" + juce::String (i));
        if (juce::roundToInt (apvts.getRawParameterValue (q + "Src")->load()) == 0 && juce::roundToInt (apvts.getRawParameterValue (q + "Dst")->load()) == 0)
        {
            setParamReal ((q + "Src").toRawUTF8(), (float) src); setParamReal ((q + "Dst").toRawUTF8(), (float) dst); setParamReal ((q + "Amt").toRawUTF8(), 0.5f);
            return i;
        }
    }
    return -1;
}

void ZephyrAudioProcessor::setKnobModAmt (int slot, float amt) { setParamReal (("s_slot" + juce::String (slot) + "Amt").toRawUTF8(), juce::jlimit (-1.f, 1.f, amt)); }

void ZephyrAudioProcessor::clearKnobMod (int slot)
{
    const juce::String q ("s_slot" + juce::String (slot));
    setParamReal ((q + "Src").toRawUTF8(), 0.f); setParamReal ((q + "Dst").toRawUTF8(), 0.f); setParamReal ((q + "Amt").toRawUTF8(), 0.f);
}

float ZephyrAudioProcessor::modSourceLive (int src) const
{
    for (int i = 0; i < kMacros; ++i) if (macroSrc (i) == src) return apvts.getRawParameterValue ("s_m" + juce::String (i + 1))->load();
    if (src == MsWheel) return (float) wheelShown.load() / 127.f;
    for (int i = 0; i < kLfos; ++i) if (src == MsLfo1 + i) return synth.lfoLive (i);
    return -99.f;                                                              // inconnue : l'anneau reste fixe
}
#endif

// v2c : plus de patchbay. Zephyr FX : chaque source (LFO 1, LFO 2, S&H, suiveur d'enveloppe) a un menu de destination.
// Zephyr (synthe) : la matrice du synthe est le seul systeme de modulation, aucune source du rack n'est routee.
void ZephyrAudioProcessor::applyModRoutes()
{
    uint32_t mask[zephyr::kNumCv] {};
#if ! ZEPHYR_SYNTH
    static const char* ids[4] { "lfo1Dst", "lfo2Dst", "shDst", "envDst" };
    static const int src[4] { zephyr::SrcLfo1, zephyr::SrcLfo2, zephyr::SrcSh, zephyr::SrcEnv };
    for (int k = 0; k < 4; ++k)
    {
        const int d = kModDstCv[juce::jlimit (0, 15, juce::roundToInt (apvts.getRawParameterValue (ids[k])->load()))];
        if (d >= 0) mask[d] |= 1u << src[k];
    }
#endif
    for (int d = 0; d < zephyr::kNumCv; ++d) if (engine.cvIn[d].load() != mask[d]) engine.cvIn[d].store (mask[d]);
    for (int nd = 0; nd < zephyr::kNumNodes; ++nd) if (engine.audioIn[nd].load() != 0) engine.audioIn[nd].store (0);
}

void ZephyrAudioProcessor::resetFx()
{
    for (const auto& d : kDefs)
        if (juce::String (d.id) != "on")
            if (auto* prm = apvts.getParameter (d.id)) prm->setValueNotifyingHost (prm->getDefaultValue());
}

// presets du synthe : le rack ne garde que les modules qui servent (reglage non neutre ou destination de la matrice)
void ZephyrAudioProcessor::pruneRack()
{
#if ZEPHYR_SYNTH
    auto v = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    auto nz = [] (float x) { return std::fabs (x) > 1e-4f; };
    std::set<int> used;
    for (int i = 1; i <= kSlots; ++i)
    {
        const juce::String q ("s_slot" + juce::String (i));
        const int dst = juce::roundToInt (v ((q + "Dst").toRawUTF8())), src = juce::roundToInt (v ((q + "Src").toRawUTF8()));
        if (src <= 0 || dst <= 0) continue;
        if (dst == MdSatDrive || dst == MdSatTone) used.insert (NSat);
        if (dst == MdEqLow || dst == MdEqMid || dst == MdEqAir) used.insert (NColor);
        if (dst == MdFxHpf || dst == MdFxLpf || dst == MdFxPeak) used.insert (NFilter);
        if (dst == MdCompAmt) used.insert (NComp);
        if (dst == MdPhMix || dst == MdPhRate || dst == MdPhFb) used.insert (NPhaser);
        if (dst == MdEnsMix || dst == MdEnsDepth || dst == MdEnsSpeed) used.insert (NEnsemble);
        if (dst >= MdStutOn && dst <= MdStutMix) used.insert (NStut);
        if (dst == MdDlyTime || dst == MdDlyFb || dst == MdDlyMix || dst == MdDlyTone || dst == MdDlyMod) used.insert (NDelay);
        if (dst == MdRevMix || dst == MdRevDecay || dst == MdRevDamp || dst == MdRevSize || dst == MdRevPre || dst == MdRevShimmer || dst == MdRevWidth) used.insert (NVerb);
        if (dst == MdWideWidth || dst == MdWideSpread) used.insert (NWide);
    }
    if (nz (v ("satDrive")) || nz (v ("satTone") - 0.5f)) used.insert (NSat);
    if (nz (v ("low")) || nz (v ("mid")) || nz (v ("high"))) used.insert (NColor);
    if (nz (v ("hpf")) || nz (v ("lpf") - 1.f)) used.insert (NFilter);
    if (juce::roundToInt (v ("compType")) != 0) used.insert (NComp);
    if (nz (v ("phMix"))) used.insert (NPhaser);
    if (nz (v ("ensMix"))) used.insert (NEnsemble);
    if (v ("stutOn") > 0.5f) used.insert (NStut);
    if (nz (v ("dlyMix"))) used.insert (NDelay);
    if (nz (v ("revMix"))) used.insert (NVerb);
    if (nz (v ("wideWidth")) || nz (v ("wideSpread")) || nz (v ("wideBass"))) used.insert (NWide);
    std::vector<int> r;
    for (int nd : { NSat, NColor, NFilter, NComp, NPhaser, NEnsemble, NStut, NDelay, NVerb, NWide }) if (used.count (nd)) r.push_back (nd);
    engine.setRack (r);
#endif
}

void ZephyrAudioProcessor::resetModules()
{
    for (const auto& d : kDefs)
        if (juce::String (d.id) != "on")
            if (auto* prm = apvts.getParameter (d.id)) prm->setValueNotifyingHost (prm->getDefaultValue());
#if ZEPHYR_SYNTH
    for (const auto& d : synDefs())
        if (auto* prm = apvts.getParameter (d.id)) prm->setValueNotifyingHost (prm->getDefaultValue());
#endif
}
void ZephyrAudioProcessor::clearRack()     { engine.setRack ({}); }
void ZephyrAudioProcessor::defaultRack()   { engine.setDefaultRack(); }
void ZephyrAudioProcessor::clearPatchbay() { engine.clearPatch(); }

int ZephyrAudioProcessor::getNumFactoryPresets() { return kNumFactory; }
juce::String ZephyrAudioProcessor::getFactoryPresetName (int i) { return kFactory[juce::jlimit (0, kNumFactory - 1, i)].name; }
juce::String ZephyrAudioProcessor::getFactoryPresetCategory (int i) { return kFactory[juce::jlimit (0, kNumFactory - 1, i)].category; }
juce::StringArray ZephyrAudioProcessor::getCategories() { juce::StringArray a; for (auto* c : kCategories) a.add (c); return a; }

void ZephyrAudioProcessor::loadFactoryPreset (int i)
{
    i = juce::jlimit (0, kNumFactory - 1, i);
    resetModules();
#if ZEPHYR_SYNTH
    arpPat.reset();
#endif
    engine.clearPatch();
    engine.bypassMask.store (0);
    if (i == 0) engine.setRack ({ NEnsemble, NDelay, NVerb }); else engine.setDefaultRack();      // Init = ensemble + delay + reverb (mix a zero) ; tous les modules pour les autres presets
    kFactory[i].apply (*this);
    {   // v2 : recalage de niveau (dB) mesure contre la v1 (filtres circuit) - chord tenu, meme velocite
        static const std::map<juce::String, float> trimDb {
            { "Glass Cathedral", 1.8f }, { "Polaris Chords", 1.5f }, { "Cello Light", 2.f }, { "Wurli Gold", -1.8f }, { "Electric Grand", 1.2f },
            { "Crystal Bell", 2.4f }, { "Poly-Mod Bell", 2.3f }, { "Golden Brass", -3.f }, { "Prophecy Sync", 2.1f }, { "Satin Bass", -3.f },
            { "Moon Ladder", -3.6f }, { "Crystal Cascade", 2.3f }, { "Frozen Time", 2.1f }, { "Halo Choir", 2.f },
            { "Prism Sequence", 3.f }, { "Gossamer", 3.f } };
        float add = 0.f;
        if (const auto it = trimDb.find (kFactory[i].name); it != trimDb.end()) add += it->second;
#if ZEPHYR_SYNTH
        add += kSynthMakeupDb;                                                          // v2b : le bus du synthe garde 6 dB de reserve pour la polyphonie
        if (juce::String (kFactory[i].name) == "Opto Glow") add -= 3.5f;                 // le comp rattrape deja le niveau
        else if (juce::String (kFactory[i].name) == "Wide & Bright") add -= 2.f;
#endif
        if (add != 0.f) if (auto* o = apvts.getRawParameterValue ("output")) setParamReal ("output", o->load() + add);
    }
#if ZEPHYR_SYNTH
    if (i != 0) { applyMacroSet (*this, kFactory[i].category); pruneRack(); }
#endif
    presetName = kFactory[i].name;
    presetCategory = kFactory[i].category;
}

// CC MIDI -> parametres assignes (macros de l'Hydrasynth, ou tout potard appris par clic droit). Retourne true si le CC a ete consomme.
bool ZephyrAudioProcessor::handleCC (int cc, int value)
{
    if (cc < 0 || cc > 127) return false;
    const float v = (float) value / 127.f;
    const int ln = learnIdx.load();
    if (ln >= 0 && ln < mapIds.size() && cc != 0 && cc != 32 && cc != 64 && cc < 120)
    { mapCc[(size_t) ln].store (cc); learnIdx.store (-1); mapDirty.store (true); }
    const float prevHw = hwPos[cc];
    hwPos[cc] = v;
    bool used = false;
    const int n = mapIds.size();
    for (int i = 0; i < n; ++i)
        if (mapCc[(size_t) i].load() == cc) { used = true; applyMapped (i, v, prevHw); }
    return used;
}

// Reprise de valeur : l'Hydrasynth n'envoie sa position qu'en bougeant. SCALE : la valeur de depart du plugin est conservee et rejoint
// progressivement celle du potard (pas de saut, pas de course a la valeur) ; PICKUP : rien ne bouge tant que le potard n'a pas croise la valeur ; DIRECT : saute.
void ZephyrAudioProcessor::applyMapped (int i, float v, float prevHw)
{
    auto* prm = mapParams[(size_t) i];
    const float cur = prm->getValue();
    const int mode = takeoverMode.load();
    float out = v;
    if (prm->isDiscrete() || mode == 2) { anchored[i] = caught[i] = false; }
    else
    {
        const bool external = lastSetV[i] < 0.f || std::fabs (cur - lastSetV[i]) > 0.004f;      // valeur changee ailleurs (preset, souris, automation)
        if (mode == 1)
        {
            if (external) { caught[i] = false; lastIn[i] = prevHw >= 0.f ? prevHw : v; }
            if (! caught[i])
            {
                if (std::fabs (v - cur) < 0.02f || (lastIn[i] - cur) * (v - cur) <= 0.f) caught[i] = true;
                lastIn[i] = v;
                if (! caught[i]) return;
            }
        }
        else
        {
            if (external || ! anchored[i]) { anchored[i] = true; aCur[i] = cur; aHw[i] = prevHw >= 0.f ? prevHw : v; }
            if (v >= aHw[i]) out = aCur[i] + (v - aHw[i]) * (1.f - aCur[i]) / std::max (1.f - aHw[i], 1e-4f);
            else             out = aCur[i] - (aHw[i] - v) * aCur[i] / std::max (aHw[i], 1e-4f);
            out = juce::jlimit (0.f, 1.f, out);
        }
    }
    prm->setValueNotifyingHost (out);
    lastSetV[i] = prm->getValue();
}

// cadence propre de l'arpegiateur : libre (0,5 a 20 pas/s) ou calee sur le tempo (memes divisions que les LFO)
float ZephyrAudioProcessor::arpStepHz (double bpm) const
{
#if ZEPHYR_SYNTH
    const float tr = arpTriplet ? 1.5f : 1.f;                                       // triolets : 3 pas pour 2
    if (arpSyncDiv > 0 && arpSyncDiv < 14) return tr * (float) bpm / 60.f / kLfoDivBeats[arpSyncDiv];
    return tr * 0.5f * std::pow (40.f, arpRateKnob);
#else
    juce::ignoreUnused (bpm); return 1.f;
#endif
}

juce::File ZephyrAudioProcessor::getPresetDir() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("Foudre Audio").getChildFile (ZEPHYR_SYNTH ? "Zephyr" : "Zephyr FX").getChildFile ("Presets");
}

juce::StringArray ZephyrAudioProcessor::getUserPresetNames() const
{
    juce::StringArray names;
    for (const auto& f : getPresetDir().findChildFiles (juce::File::findFiles, false, "*.zph"))
        names.add (f.getFileNameWithoutExtension());
    names.sort (true);
    return names;
}

juce::String ZephyrAudioProcessor::getUserPresetCategory (const juce::String& name) const
{
    if (auto xml = juce::parseXML (getPresetDir().getChildFile (juce::File::createLegalFileName (name) + ".zph")))
        return xml->getStringAttribute ("category", "");
    return {};
}

bool ZephyrAudioProcessor::saveUserPreset (const juce::String& name, const juce::String& category)
{
    const auto clean = name.trim();
    if (clean.isEmpty()) return false;
    const auto dir = getPresetDir();
    if (! dir.createDirectory().wasOk()) return false;
    presetName = clean; presetCategory = category;
    if (auto xml = captureState().createXml())
        return xml->writeTo (dir.getChildFile (juce::File::createLegalFileName (clean) + ".zph"));
    return false;
}

bool ZephyrAudioProcessor::loadUserPreset (const juce::String& name)
{
    const auto file = getPresetDir().getChildFile (juce::File::createLegalFileName (name) + ".zph");
    if (auto xml = juce::parseXML (file))
        if (xml->hasTagName (apvts.state.getType()))
        {
            applyState (juce::ValueTree::fromXml (*xml));
            presetName = name;
            return true;
        }
    return false;
}

bool ZephyrAudioProcessor::deleteUserPreset (const juce::String& name)
{
    return getPresetDir().getChildFile (juce::File::createLegalFileName (name) + ".zph").deleteFile();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ZephyrAudioProcessor(); }
