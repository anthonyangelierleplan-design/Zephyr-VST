#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Engine.h"
#ifndef ZEPHYR_SYNTH
 #define ZEPHYR_SYNTH 0
#endif
#include "Synth.h"
#include "Arp.h"

class ZephyrAudioProcessor : public juce::AudioProcessor, private juce::AsyncUpdater
{
public:
    ZephyrAudioProcessor();
    ~ZephyrAudioProcessor() override { cancelPendingUpdate(); }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    #if ZEPHYR_SYNTH
    const juce::String getName() const override { return "Zephyr"; }
    bool acceptsMidi() const override  { return true; }
#else
    const juce::String getName() const override { return "Zephyr FX"; }
    bool acceptsMidi() const override  { return false; }
#endif
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void handleAsyncUpdate() override { const int i = pendingProgram.exchange (-1); if (i >= 0) { loadFactoryPreset (i); presetSerial.fetch_add (1); } }

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- presets et remises a zero (thread de l'interface)
    static int getNumFactoryPresets();
    static juce::String getFactoryPresetName (int index);
    void loadFactoryPreset (int index);
    juce::File getPresetDir() const;
    juce::StringArray getUserPresetNames() const;
    static juce::String getFactoryPresetCategory (int index);
    static juce::StringArray getCategories();
    juce::String getUserPresetCategory (const juce::String& name) const;
    bool saveUserPreset (const juce::String& name, const juce::String& category);
    bool loadUserPreset (const juce::String& name);
    bool deleteUserPreset (const juce::String& name);
    void resetModules();
    void resetFx();             // v2c : remet les effets seuls a leur valeur par defaut (le synthe n'est pas touche)
    void applyModRoutes();      // v2c : routage des sources de modulation du rack (plus de patchbay)
    void pruneRack();           // v2c : presets du synthe : rack limite aux modules utilises        // remet tous les reglages par defaut (rack et patchbay intacts)
    void clearRack();           // vide le rack : INPUT est relie directement a OUTPUT
    void defaultRack();         // remet les modules dans l'ordre d'origine
    void clearPatchbay();       // retire tous les cables
    void setParamReal (const char* id, float realValue);
#if ZEPHYR_SYNTH
    // ---- anneaux de modulation : un potard = une destination de la matrice ; 2 sources au maximum par potard
    static int modDstOfParam (const juce::String& paramId);                  // 0 : ce potard n'a pas de destination de matrice
    static float modScaleOfDst (int dst);                                    // part de la plage du potard couverte par une quantite de 100 % (pour l'anneau)
    struct KnobMod { int slot, src; float amt; };
    int  knobMods (int dst, KnobMod out[2]) const;                           // sources deja routees vers cette destination (2 max)
    int  assignKnobMod (int dst, int src);                                   // cree le routage (ou le reutilise) ; renvoie le slot, -1 si plus de place ; -2 si deja 2 sources
    void setKnobModAmt (int slot, float amt);
    void clearKnobMod (int slot);
    float modSourceLive (int src) const;                                     // valeur courante de la source pour l'affichage (macros, molette), -1 si inconnue
#endif

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;  // clavier a l'ecran (version instrument)
    // ---- MIDI learn universel : n'importe quel potard peut etre pilote par un CC (clic droit -> assigner). Les macros 1..8 sont sur CC 16..23 par defaut (Hydrasynth).
    //      La configuration est celle du controleur, pas du son : elle est sauvee avec le projet et dans un fichier global, jamais dans les presets.
    static constexpr int kMaxMap = 1024;
    int  ccOfParam (const juce::String& id) const { const int i = mapIds.indexOf (id); return i < 0 ? -1 : mapCc[(size_t) i].load(); }
    bool isLearning (const juce::String& id) const { const int i = mapIds.indexOf (id); return i >= 0 && i == learnIdx.load(); }
    int  mapIndex (const juce::String& id) const { return mapIds.indexOf (id); }
    int  ccAt (int i) const { return i < 0 ? -1 : mapCc[(size_t) i].load(); }
    bool learningAt (int i) const { return i >= 0 && i == learnIdx.load(); }
    void startMidiLearn (const juce::String& id) { learnIdx.store (mapIds.indexOf (id)); }
    void cancelMidiLearn() { learnIdx.store (-1); }
    void clearMidiAssign (const juce::String& id);
    void defaultMidiAssign (const juce::String& id);          // CC d'origine (macros : 16..23), sinon rien
    int  defaultCcOf (const juce::String& id) const;          // -1 si pas de CC d'origine
    void resetMidiMap();
    juce::String midiMapToString() const;
    void midiMapFromString (const juce::String&);
    void markMidiMapDirty() { mapDirty.store (true); }
    void flushMidiMapIfDirty();                                // ecrit le fichier global si l'assignation a change
    std::atomic<int> takeoverMode { 0 };                       // reprise de valeur : 0 SCALE (converge sans saut), 1 PICKUP (attend de croiser la valeur), 2 DIRECT (saute)
    zephyr::Synth synth;                     // utilise seulement par la version instrument (ZEPHYR_SYNTH)
    std::atomic<int> lastMidiMsg { 0 };     // dernier message MIDI recu (affichage) : kind<<16 | numero<<8 | valeur ; kind 1 note, 2 CC, 3 pitch, 4 aftertouch
    std::atomic<int> pendingProgram { -1 };     // Program Change recu -> preset d'usine (charge sur le thread de l'interface)
    std::atomic<int> presetSerial { 0 };        // incremente quand un preset est charge par MIDI (l'editeur se rafraichit)
    // reglages globaux (fichier + projet, jamais dans les presets) : panneau SETTINGS de l'en-tete
    std::atomic<int> quality { 1 };             // 0 performance, 1 qualite, 2 high fidelity
    std::atomic<int> ccSmooth { 3 };            // lissage des reglages : 0 off, 1 5 ms, 2 10 ms, 3 20 ms, 4 40 ms, 5 80 ms
    std::atomic<int> atCurve { 0 };             // courbe de l'aftertouch : 0 lineaire, 1 exponentielle, 2 logarithmique, 3 en S
    std::atomic<int> atSpeed { 2 };             // reactivite de l'aftertouch : 0 instantanee ... 5 tres lente
    void settingChanged() { mapDirty.store (true); }
    float atShape (float x) const;
    float blockMs = 10.f, smV[16] {}; bool smInit = false;
    float bendRange = 2.f;                      // etendue du pitch bend en demi-tons
    bool pedal = false, keyDown[128] = {}, pedalHeld[128] = {};
    std::atomic<int> wheelShown { 0 };      // valeur de la molette (CC1) 0..127 pour l'affichage
    std::atomic<int> arpStepShown { -1 };   // pas courant de l'arpegiateur (affichage)
    zephyr::ArpPattern arpPat;               // motif de l'arpegiateur / sequenceur (pas + pistes de modulation), edite par l'interface
    zephyr::Arp arp;                         // arpegiateur (version instrument)
    zephyr::Engine engine;                   // rack + patchbay vivent ici ; l'editeur les lit et les modifie
    juce::String presetName { "Init" }, presetCategory;

    // oscilloscope de l'en-tete : tampon circulaire alimente par la sortie finale (lecture sans verrou par l'interface)
    static constexpr int kScope = 8192;
    std::atomic<int> headerScope { 0 };                // 0 : lampe, 1 : oscilloscope (clic sur la lampe)
    void copyScope (float* dst, int n) const
    {
        const int w = scopeW.load (std::memory_order_acquire);
        for (int i = 0; i < n; ++i) dst[i] = scopeBuf[(w - n + i) & (kScope - 1)];
    }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    zephyr::Params readParams() const;
    zephyr::SynthParams readSynth();
    zephyr::ArpParams arpParams;
    struct MidiEv { int pos; int type; int a; float v; };       // type : 0 note on, 1 note off, 2 all off, 3 bend, 4 molette, 5 aftertouch poly, 6 aftertouch canal, 7/8/9 notes brutes de l'arpegiateur (simple / pile unison / off)
    std::vector<MidiEv> evs;
    int lastArpOn = 0, lastAssign = 0, chordN = 3, chordIv[6] { 4, 7, 11, 0, 0, 0 }, learnN = 0, learnNotes[6] {};
    bool chordLearn = false, lastLearn = false, seqRec = false, lastRec = false;
    bool handleCC (int cc, int value);
    void applyMapped (int i, float v, float prevHw);
    juce::StringArray mapIds;                                   // identifiants de tous les parametres assignables
    std::vector<juce::RangedAudioParameter*> mapParams;
    std::atomic<int> mapCc[kMaxMap];
    std::atomic<int> learnIdx { -1 };
    std::atomic<bool> mapDirty { false };
    float hwPos[128];                                           // derniere position connue de chaque potard du clavier (thread audio)
    float lastSetV[kMaxMap], aCur[kMaxMap], aHw[kMaxMap], lastIn[kMaxMap];
    bool  caught[kMaxMap], anchored[kMaxMap];
    juce::File midiMapFile() const;
    float arpStepHz (double bpm) const;
    float arpRateKnob = 0.55f; int arpSyncDiv = 0; bool arpTriplet = false;
    std::vector<std::atomic<float>*> rawSyn;
    juce::ValueTree captureState();
    void applyState (juce::ValueTree state);

    std::vector<std::atomic<float>*> raw;
    float scopeBuf[kScope] {};
    std::atomic<int> scopeW { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZephyrAudioProcessor)
};
