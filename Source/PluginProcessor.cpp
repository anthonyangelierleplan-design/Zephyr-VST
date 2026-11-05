#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "SynthEditor.h"

namespace
{
#include "ParamsAndPresets.inc"
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
    arp.prepare ((float) sampleRate);
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
    s.hpf = f(); s.hpfRes = f(); s.lpf = f(); s.lpfRes = f(); s.fltEnv = f(); s.keytrack = f(); s.sat = f();
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
    s.sub = f(); s.drift = f(); s.pmEnv = f(); s.pmOsc = f();
    s.pmFreq = b() ? 1 : 0; s.pmPw = b() ? 1 : 0; s.pmFilt = b() ? 1 : 0;
    arpParams.shift = f(); arpParams.cluster = f();
    jassert (i == rawSyn.size());
    return s;
}
#endif

void ZephyrAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int nc = juce::jmin (2, buffer.getNumChannels());
    const int total = buffer.getNumSamples();
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, total);
    if (nc == 0 || total == 0) return;

    // tempo et position d'Ableton (synchro des LFO, du S&H, du phaser et du delay)
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
        synth.setParams (readSynth());
        float* sl = buffer.getWritePointer (0);
        float* sr = nc > 1 ? buffer.getWritePointer (1) : buffer.getWritePointer (0);

        // ---- evenements du bloc : MIDI direct, ou arpegiateur (qui genere ses propres notes)
        evs.clear();
        arp.setParams (arpParams);
        const bool arpOn = arpParams.on != 0;
        if (arpParams.on != lastArpOn) { lastArpOn = arpParams.on; synth.allNotesOff(); arp.allOff(); }
        if (arpParams.assign != lastAssign) { lastAssign = arpParams.assign; arp.resetVoices(); }         // changer de mode ne coupe pas l'arpege : les touches restent
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
            if (m.isNoteOn())
            {
                if (chordLearn && learnN < 6) learnNotes[learnN++] = m.getNoteNumber();
                if (chordLearn) continue;                                    // en apprentissage, les touches ne jouent pas
                if (arpOn) arp.noteOn (m.getNoteNumber(), m.getFloatVelocity()); else evs.push_back ({ pos, 0, m.getNoteNumber(), m.getFloatVelocity() });
            }
            else if (m.isNoteOff())      { if (chordLearn) continue; if (arpOn) arp.noteOff (m.getNoteNumber()); else evs.push_back ({ pos, 1, m.getNoteNumber(), 0.f }); }
            else if (m.isAllNotesOff() || m.isAllSoundOff()) { arp.allOff(); evs.push_back ({ pos, 2, 0, 0.f }); }
            else if (m.isPitchWheel())   evs.push_back ({ pos, 3, 0, (float) (m.getPitchWheelValue() - 8192) / 8192.f * 2.f });
            else if (m.isAftertouch())   evs.push_back ({ pos, 5, m.getNoteNumber(), (float) m.getAfterTouchValue() / 127.f });
            else if (m.isChannelPressure()) evs.push_back ({ pos, 6, 0, (float) m.getChannelPressureValue() / 127.f });
            else if (m.isController() && m.getControllerNumber() != 1 && handleMacroCC (m.getControllerNumber(), m.getControllerValue())) {}
            else if (m.isController() && m.getControllerNumber() == 1) evs.push_back ({ pos, 4, 0, (float) m.getControllerValue() / 127.f });
        }
        {
            zephyr::Arp::Ev ae[256];
            const int ac = arp.process (total, arpStepHz (bpm), ae, 256);
            for (int k = 0; k < ac; ++k) evs.push_back ({ juce::jlimit (0, total - 1, ae[k].pos), ae[k].slot >= 0 ? (ae[k].on ? 10 : 11) : (ae[k].on ? (ae[k].stack ? 8 : 7) : 9), ae[k].note, ae[k].vel, ae[k].slot });
        }
        std::stable_sort (evs.begin(), evs.end(), [] (const MidiEv& a, const MidiEv& b) { return a.pos < b.pos; });

        int pos = 0;
        auto render = [&] (int upTo)
        {
            if (upTo > pos) { synth.process (sl + pos, sr + pos, upTo - pos); pos = upTo; }
        };
        for (const auto& e : evs)
        {
            render (e.pos);
            switch (e.type)
            {
                case 0: synth.noteOn (e.a, e.v); break;
                case 1: synth.noteOff (e.a); break;
                case 2: synth.allNotesOff(); break;
                case 3: synth.setBend (e.v); break;
                case 7: synth.noteOnRaw (e.a, e.v); break;
                case 8: synth.noteOnStack (e.a, e.v); break;
                case 9: synth.noteOffRaw (e.a); break;
                case 10: synth.noteOnSlot (e.a, e.v, e.b); break;
                case 11: synth.noteOffSlot (e.b, e.a); break;
                case 5: synth.setPolyAT (e.a, e.v); break;
                case 6: synth.setChanAT (e.v); break;
                default: synth.setWheel (e.v); break;
            }
        }
        render (total);
        engine.setExtMod (synth.fxMod());
    }
#endif
    engine.setParams (readParams());
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
    state.setProperty ("patch", juce::String (engine.serializePatch()), nullptr);
    state.setProperty ("rack", juce::String (engine.serializeRack()), nullptr);
    state.setProperty ("presetName", presetName, nullptr);
    state.setProperty ("category", presetCategory, nullptr);
    state.setProperty ("hdr", headerScope.load(), nullptr);
    { juce::String c; for (int k = 0; k < chordN; ++k) c << chordIv[k] << ","; state.setProperty ("chord", c, nullptr); }
    { juce::String c; for (int k = 0; k < 8; ++k) c << macroCC[k].load() << ","; state.setProperty ("macroCC", c, nullptr); }
    return state;
}

void ZephyrAudioProcessor::applyState (juce::ValueTree state)
{
    engine.loadPatch (state.getProperty ("patch", "").toString().toStdString());
    const auto rk = state.getProperty ("rack", "").toString();
    if (rk.isNotEmpty()) engine.loadRack (rk.toStdString()); else engine.setDefaultRack();
    presetName = state.getProperty ("presetName", "Init").toString();
    presetCategory = state.getProperty ("category", "").toString();
    headerScope.store ((int) state.getProperty ("hdr", 0));
    {
        juce::StringArray t; t.addTokens (state.getProperty ("macroCC", "").toString(), ",", ""); t.removeEmptyStrings();
        bool oldDefault = t.size() == 8;                              // anciennes valeurs par defaut (21..28) : on passe aux CC des macros de l'Hydrasynth
        for (int k = 0; k < t.size() && k < 8; ++k) oldDefault = oldDefault && t[k].getIntValue() == 21 + k;
        for (int k = 0; k < 8; ++k) macroCC[k].store (k < t.size() && ! oldDefault ? t[k].getIntValue() : 16 + k);
    }
    {
        const auto cs = state.getProperty ("chord", "").toString();
        if (cs.isNotEmpty()) { juce::StringArray t; t.addTokens (cs, ",", ""); t.removeEmptyStrings(); chordN = 0; for (auto& x : t) if (chordN < 6) chordIv[chordN++] = x.getIntValue(); }
        else { const int d[4] { 4, 7, 11, 0 }; chordN = 3; for (int k = 0; k < 3; ++k) chordIv[k] = d[k]; }
    }
    apvts.replaceState (state);
}

void ZephyrAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = captureState().createXml()) copyXmlToBinary (*xml, dest);
}

void ZephyrAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            applyState (juce::ValueTree::fromXml (*xml));
}

void ZephyrAudioProcessor::setParamReal (const char* id, float real)
{
    if (auto* prm = apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (real));
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
    engine.clearPatch();
    if (i == 0) engine.setRack ({}); else engine.setDefaultRack();      // Init = rack vide ; les autres presets utilisent tous les modules
    kFactory[i].apply (*this);
#if ZEPHYR_SYNTH
    if (i != 0) applyMacroSet (*this, kFactory[i].category);
#endif
    presetName = kFactory[i].name;
    presetCategory = kFactory[i].category;
}

// CC MIDI -> macros (ex. macros de l'Hydrasynth). Retourne true si le CC a ete consomme.
bool ZephyrAudioProcessor::handleMacroCC (int cc, int value)
{
#if ZEPHYR_SYNTH
    const int ln = learnMacro.load();
    if (ln >= 0 && ln < 8 && cc != 1 && cc < 120) { macroCC[ln].store (cc); learnMacro.store (-1); }
    for (int k = 0; k < 8; ++k)
        if (macroCC[k].load() == cc)
        {
            if (auto* prm = apvts.getParameter ("s_m" + juce::String (k + 1))) prm->setValueNotifyingHost ((float) value / 127.f);
            return true;
        }
#else
    juce::ignoreUnused (cc, value);
#endif
    return false;
}

// cadence propre de l'arpegiateur : libre (0,5 a 20 pas/s) ou calee sur le tempo (memes divisions que les LFO)
float ZephyrAudioProcessor::arpStepHz (double bpm) const
{
#if ZEPHYR_SYNTH
    if (arpSyncDiv > 0 && arpSyncDiv < 14) return (float) bpm / 60.f / kLfoDivBeats[arpSyncDiv];
    return 0.5f * std::pow (40.f, arpRateKnob);
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
