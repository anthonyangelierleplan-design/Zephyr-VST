// Inclus par PluginProcessor.cpp (dans un namespace anonyme) : definition des parametres et presets d'usine de Zephyr.
enum Kind { kFloat, kBool, kChoice };
struct PDef { const char* id; const char* name; float mn, mx, def; Kind kind; const char* choices; };

const char* const kSyncChoices = "Free|4 bar|2 bar|1 bar|1/2|1/4|1/8|1/16|1/32|1/2T|1/4T|1/8T|1/16T|1/4.|1/8.|1/16.";

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
};
constexpr int kNumDefs = (int) (sizeof (kDefs) / sizeof (kDefs[0]));

using namespace zephyr;
#if ZEPHYR_SYNTH
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
        F ("s_aA", "Amp A", 0, 1, 0.1f);  F ("s_aD", "Amp D", 0, 1, 0.4f);  F ("s_aS", "Amp S", 0, 1, 0.8f);  F ("s_aR", "Amp R", 0, 1, 0.45f);
        F ("s_fA", "Flt A", 0, 1, 0.1f);  F ("s_fD", "Flt D", 0, 1, 0.45f);  F ("s_fS", "Flt S", 0, 1, 0.2f);  F ("s_fR", "Flt R", 0, 1, 0.45f);
        F ("s_master", "Synth Master", 0, 1, 0.7f);
        juce::StringArray divs; for (auto* d : kLfoDivNames) divs.add (d);
        for (int i = 1; i <= kLfos; ++i)
        {
            const juce::String p ("s_l" + juce::String (i));
            F (p + "Rate", "LFO" + juce::String (i) + " Rate", 0, 1, 0.4f);  C (p + "Wave", "LFO" + juce::String (i) + " Wave", "Sine|Triangle|Saw|Square|S&H", 0);
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
        C ("s_arpMode", "Arp Direction", "Up|Down|Up-Down", 0);
        C ("s_arpOct", "Arp Range", "1 Oct|2 Oct|Full", 0);
        C ("s_ftype", "Filter Type", "SEM|Ladder|Prophet", 0);  F ("s_semMorph", "SEM Morph", 0, 1, 0);
        C ("s_assign", "Key Assign", "Poly|Mono|Unison|Share|Chord", 0);  B ("s_hold", "Hold", false);
        F ("s_uniDetune", "Unison Detune", 0, 1, 0.3f);  B ("s_chordLearn", "Chord Learn", false);
        for (int i = 5; i <= kMacros; ++i) F ("s_m" + juce::String (i), "Macro " + juce::String (i), 0, 1, 0);
        F ("s_arpRate", "Arp Rate", 0, 1, 0.55f);  C ("s_arpSync", "Arp Sync", divs.joinIntoString ("|"), 0);
        F ("s_sub", "Sub Level", 0, 1, 0);  F ("s_drift", "Drift", 0, 1, 0);
        F ("s_pmEnv", "Poly-Mod Filter Env", 0, 1, 0);  F ("s_pmOsc", "Poly-Mod VCO2", 0, 1, 0);
        B ("s_pmFreq", "Poly-Mod > Freq VCO1", false);  B ("s_pmPw", "Poly-Mod > PW VCO1", false);  B ("s_pmFilt", "Poly-Mod > Filter", false);
        F ("s_arpShift", "Arp Shift", 0, 1, 0);  F ("s_arpCluster", "Arp Cluster", 0, 1, 0);
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
void S (ZephyrAudioProcessor& p, std::initializer_list<KV> l) { for (auto& kv : l) p.setParamReal (kv.first, kv.second); }
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
