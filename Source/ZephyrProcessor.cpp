#include "ZephyrProcessor.h"
#include "ZephyrEditor.h"

using namespace eo;

juce::StringArray ZephyrProcessor::choiceNames (int i)
{
    switch (i)
    {
        case p_mode:  return { "PIPE", "FILTER" };
        case p_lock:  return { "HARMONIC", "SCALE" };
        case p_scale: return { "Major", "Minor", "Dorian", "Pentatonic Minor", "Pentatonic Major", "Whole Tone", "Chromatic" };
        case p_root:  return { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        default:      return { "FEEDBACK", "WIND", "FOCUS", "BRIGHT", "DRIVE", "BREATH", "PITCH", "EXT INPUT" };   // cibles de la pression
    }
}

namespace
{
juce::String valueText (int i, float v)
{
    switch (i)
    {
        case p_out:      return (v > 0.05f ? "+" : "") + juce::String (v, 1) + " dB";
        case p_glide: case p_atk: case p_rel:
            return v >= 1000.f ? juce::String (v / 1000.f, 2) + " s" : juce::String ((int) std::round (v)) + " ms";
        case p_detune:   return juce::String (v, 1) + " cents";
        case p_bend:     return juce::String ((int) std::round (v)) + " semitones";
        case p_pcurve:   return v < 0.95f ? "soft" : (v > 1.05f ? "hard" : "linear");
        case p_p1a: case p_p2a: case p_p3a:
        { const int pc = (int) std::round (v * 100.f); return (pc > 0 ? "+" : "") + juce::String (pc) + " %"; }
        default:         return juce::String ((int) std::round (v * 100.f)) + " %";
    }
}
}

juce::AudioProcessorValueTreeState::ParameterLayout ZephyrProcessor::makeLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (int i = 0; i < kNumP; ++i)
    {
        const auto& d = paramDefs()[i];
        const juce::ParameterID pid { d.id, 1 };
        if (d.kind == 1)
            layout.add (std::make_unique<juce::AudioParameterBool> (pid, d.name, d.def > 0.5f));
        else if (d.kind == 2)
            layout.add (std::make_unique<juce::AudioParameterChoice> (pid, d.name, choiceNames (i), (int) d.def));
        else
        {
            juce::NormalisableRange<float> r (d.mn, d.mx);
            if (d.centre > d.mn && d.centre < d.mx) r.setSkewForCentre (d.centre);
            juce::AudioParameterFloatAttributes at;
            at = at.withStringFromValueFunction ([i] (float v, int) { return valueText (i, v); })
                   .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); });
            layout.add (std::make_unique<juce::AudioParameterFloat> (pid, d.name, r, d.def, at));
        }
    }
    return layout;
}

ZephyrProcessor::ZephyrProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                                       .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "ZEPHYR", makeLayout())
{
    for (int i = 0; i < kNumP; ++i) raw[i] = apvts.getRawParameterValue (paramDefs()[i].id);
}

bool ZephyrProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    if (l.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    const auto in = l.getChannelSet (true, 0);
    return in.isDisabled() || in == juce::AudioChannelSet::stereo();
}

void ZephyrProcessor::prepareToPlay (double sr, int block)
{
    synth.prepare ((float) sr);
    extBuf.setSize (2, juce::jmax (512, block), false, true, false);
}

void ZephyrProcessor::handle (const juce::MidiMessage& m)
{
    const int ch = m.getChannel() - 1;
    if (m.isNoteOn())                  synth.noteOn (ch, m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())            synth.noteOff (ch, m.getNoteNumber());
    else if (m.isAftertouch())         synth.polyPressure (ch, m.getNoteNumber(), (float) m.getAfterTouchValue() / 127.f);
    else if (m.isChannelPressure())    synth.channelPressure (ch, (float) m.getChannelPressureValue() / 127.f);
    else if (m.isPitchWheel())         synth.pitchBend (ch, (float) (m.getPitchWheelValue() - 8192) / 8192.f);
    else if (m.isController())         synth.controller (ch, m.getControllerNumber(), (float) m.getControllerValue() / 127.f);
    else if (m.isAllNotesOff() || m.isAllSoundOff()) synth.controller (ch, 123, 0.f);
}

void ZephyrProcessor::processBlock (juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    const int n = buf.getNumSamples();
    // entree audio (sidechain) : copiee avant d'ecrire la sortie
    const float *eL = nullptr, *eR = nullptr;
    auto in = getBusBuffer (buf, true, 0);
    if (getBus (true, 0) != nullptr && getBus (true, 0)->isEnabled() && in.getNumChannels() >= 1)
    {
        if (extBuf.getNumSamples() < n) extBuf.setSize (2, n, false, true, false);
        extBuf.copyFrom (0, 0, in, 0, 0, n);
        extBuf.copyFrom (1, 0, in, in.getNumChannels() > 1 ? 1 : 0, 0, n);
        eL = extBuf.getReadPointer (0); eR = extBuf.getReadPointer (1);
    }
    for (int i = 0; i < kNumP; ++i) values[i] = raw[i]->load();
    synth.setParams (values);

    auto out = getBusBuffer (buf, false, 0);
    out.clear();
    float* L = out.getWritePointer (0);
    float* R = out.getNumChannels() > 1 ? out.getWritePointer (1) : out.getWritePointer (0);
    int pos = 0;
    for (const auto meta : midi)                                              // les evenements MIDI coupent le bloc a leur position
    {
        const int at = juce::jlimit (0, n, meta.samplePosition);
        if (at > pos)
        {
            synth.process (L + pos, R + pos, at - pos, eL != nullptr ? eL + pos : nullptr, eR != nullptr ? eR + pos : nullptr);
            pos = at;
        }
        handle (meta.getMessage());
    }
    if (pos < n) synth.process (L + pos, R + pos, n - pos, eL != nullptr ? eL + pos : nullptr, eR != nullptr ? eR + pos : nullptr);
    midi.clear();
}

juce::AudioProcessorEditor* ZephyrProcessor::createEditor() { return new ZephyrEditor (*this); }

// ---------------------------------------------------------------- etat et presets
juce::ValueTree ZephyrProcessor::captureState()
{
    auto s = apvts.copyState();
    s.setProperty ("presetName", presetName, nullptr);
    return s;
}

void ZephyrProcessor::applyState (const juce::ValueTree& s)
{
    presetName = s.getProperty ("presetName", "Init").toString();
    apvts.replaceState (s);
}

void ZephyrProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = captureState().createXml()) copyXmlToBinary (*xml, dest);
}

void ZephyrProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) applyState (juce::ValueTree::fromXml (*xml));
}

namespace
{
struct Setting { int id; float v; };
struct Factory { const char* name; std::vector<Setting> s; };

const std::vector<Factory>& factory()
{
    static const std::vector<Factory> f {
        { "Init", {} },
        { "Breath of Glass", {
            { p_fb, .32f }, { p_wind, .15f }, { p_range, .5f }, { p_focus, .6f }, { p_bright, .35f }, { p_drive, .15f }, { p_breath, .6f },
            { p_p1a, .7f }, { p_p2a, .55f }, { p_rvSize, .8f }, { p_rvMix, .45f }, { p_atk, 1400.f }, { p_rel, 4500.f } } },
        { "Whirly Pipe", {
            { p_fb, .4f }, { p_range, .85f }, { p_focus, .55f }, { p_stick, .6f }, { p_snap, 1.f }, { p_glide, 70.f }, { p_breath, .5f },
            { p_p1t, 0 }, { p_p1a, .5f }, { p_p2t, 1 }, { p_p2a, 1.f }, { p_rvSize, .5f }, { p_rvMix, .25f }, { p_atk, 300.f }, { p_rel, 900.f } } },
        { "Pentatonic Larsen", {
            { p_lock, 1 }, { p_scale, 3 }, { p_fb, .38f }, { p_range, .6f }, { p_focus, .7f }, { p_snap, .8f }, { p_glide, 220.f }, { p_drive, .4f },
            { p_p1a, .6f }, { p_p2t, 1 }, { p_p2a, .9f }, { p_rvSize, .7f }, { p_rvMix, .4f } } },
        { "Filter Choir", {
            { p_mode, 1 }, { p_lock, 1 }, { p_scale, 2 }, { p_fb, .4f }, { p_range, .4f }, { p_focus, .85f }, { p_bright, .25f }, { p_snap, .6f }, { p_glide, 350.f },
            { p_p1a, .7f }, { p_p2a, .8f }, { p_detune, 14.f }, { p_spread, .9f }, { p_rvSize, .8f }, { p_rvMix, .4f }, { p_atk, 900.f }, { p_rel, 3500.f } } },
        { "Cathedral Feedback", {
            { p_fb, .5f }, { p_wind, .2f }, { p_range, .7f }, { p_drive, .7f }, { p_breath, .6f }, { p_bright, .55f }, { p_focus, .4f },
            { p_p1a, .5f }, { p_p2a, .7f }, { p_p3t, 4 }, { p_p3a, .4f }, { p_rvSize, .95f }, { p_rvMix, .55f }, { p_atk, 2200.f }, { p_rel, 6500.f } } },
        { "Drone Bloom", {
            { p_hold, 1 }, { p_fb, .58f }, { p_wind, .3f }, { p_range, .6f }, { p_stick, .7f }, { p_snap, .9f }, { p_glide, 400.f }, { p_drive, .35f },
            { p_p1a, .3f }, { p_p2a, .7f }, { p_atk, 3000.f }, { p_rel, 7000.f }, { p_rvSize, .9f }, { p_rvMix, .5f } } },
        { "Resonator (Audio In)", {
            { p_fb, .3f }, { p_ext, .8f }, { p_focus, .5f }, { p_breath, .1f }, { p_bright, .6f }, { p_p1a, .5f }, { p_p2a, .3f },
            { p_atk, 80.f }, { p_rel, 1200.f }, { p_rvMix, .25f } } },
    };
    return f;
}
}

int ZephyrProcessor::getNumFactoryPresets() { return (int) factory().size(); }
juce::String ZephyrProcessor::getFactoryPresetName (int i) { return factory()[(size_t) juce::jlimit (0, getNumFactoryPresets() - 1, i)].name; }

void ZephyrProcessor::loadFactoryPreset (int i)
{
    const auto& f = factory()[(size_t) juce::jlimit (0, getNumFactoryPresets() - 1, i)];
    for (int k = 0; k < kNumP; ++k)
        if (auto* prm = apvts.getParameter (paramDefs()[k].id)) prm->setValueNotifyingHost (prm->getDefaultValue());
    for (const auto& s : f.s)
        if (auto* prm = apvts.getParameter (paramDefs()[s.id].id)) prm->setValueNotifyingHost (prm->convertTo0to1 (s.v));
    presetName = f.name;
}

juce::File ZephyrProcessor::getPresetDir() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("Foudre Audio").getChildFile ("Zephyr").getChildFile ("Presets");
}

juce::StringArray ZephyrProcessor::getUserPresetNames() const
{
    juce::StringArray names;
    for (const auto& f : getPresetDir().findChildFiles (juce::File::findFiles, false, "*.zephyr"))
        names.add (f.getFileNameWithoutExtension());
    names.sort (true);
    return names;
}

bool ZephyrProcessor::saveUserPreset (const juce::String& name)
{
    const auto clean = name.trim();
    if (clean.isEmpty()) return false;
    const auto dir = getPresetDir();
    if (! dir.createDirectory().wasOk()) return false;
    presetName = clean;
    if (auto xml = captureState().createXml())
        return xml->writeTo (dir.getChildFile (juce::File::createLegalFileName (clean) + ".zephyr"));
    return false;
}

bool ZephyrProcessor::loadUserPreset (const juce::String& name)
{
    const auto file = getPresetDir().getChildFile (juce::File::createLegalFileName (name) + ".zephyr");
    if (auto xml = juce::parseXML (file))
        if (xml->hasTagName (apvts.state.getType()))
        {
            applyState (juce::ValueTree::fromXml (*xml));
            presetName = name;
            return true;
        }
    return false;
}

bool ZephyrProcessor::deleteUserPreset (const juce::String& name)
{
    return getPresetDir().getChildFile (juce::File::createLegalFileName (name) + ".zephyr").deleteFile();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ZephyrProcessor(); }
