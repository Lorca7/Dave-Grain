#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "dsp/BuiltinSources.h"

namespace
{
    constexpr double maxSampleSeconds = 600.0; // 10 minutes

    inline float softClip (float x) noexcept
    {
        constexpr float knee = 0.9f;
        const float a = std::abs (x);
        if (a <= knee) return x;
        const float y = knee + (1.0f - knee) * std::tanh ((a - knee) / (1.0f - knee));
        return std::copysign (y, x);
    }
}

DaveGrainProcessor::DaveGrainProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "DaveGrain", createParameterLayout()),
      presets (*this)
{
    formatManager.registerBasicFormats();

    using namespace ParamIDs;
    auto raw = [this] (const juce::String& id) { auto* v = apvts.getRawParameterValue (id); jassert (v != nullptr); return v; };

    p.position = raw (position);   p.spray = raw (spray);       p.scan = raw (scan);         p.freeze = raw (freeze);
    p.size = raw (size);           p.sizeRand = raw (sizeRand); p.density = raw (density);   p.jitter = raw (jitter);
    p.pitch = raw (pitch);         p.fine = raw (fine);         p.pitchRand = raw (pitchRand); p.quantize = raw (quantize);
    p.keyTrack = raw (keyTrack);   p.rootNote = raw (rootNote);
    p.window = raw (window);       p.reverse = raw (reverse);   p.spread = raw (spread);
    p.attack = raw (attack);       p.decay = raw (decay);       p.sustain = raw (sustain);   p.release = raw (release);
    p.velocity = raw (velocity);   p.hold = raw (hold);
    p.filterType = raw (filterType); p.cutoff = raw (cutoff);   p.resonance = raw (resonance);
    p.lfoRate[0] = raw (lfo1Rate); p.lfoShape[0] = raw (lfo1Shape); p.lfoTarget[0] = raw (lfo1Target);
    p.lfoDepth[0] = raw (lfo1Depth); p.lfoSync[0] = raw (lfo1Sync); p.lfoDiv[0] = raw (lfo1Div);
    p.lfoRate[1] = raw (lfo2Rate); p.lfoShape[1] = raw (lfo2Shape); p.lfoTarget[1] = raw (lfo2Target);
    p.lfoDepth[1] = raw (lfo2Depth); p.lfoSync[1] = raw (lfo2Sync); p.lfoDiv[1] = raw (lfo2Div);
    p.delayTime = raw (delayTime); p.delayFb = raw (delayFb);   p.delayMix = raw (delayMix); p.delayPing = raw (delayPing);
    p.reverbSize = raw (reverbSize); p.reverbDamp = raw (reverbDamp); p.reverbMix = raw (reverbMix);
    p.output = raw (output);

    // Make a sound straight away, before anything is dropped in.
    loadBuiltinSample (BuiltinSources::names()[0]);
}

DaveGrainProcessor::~DaveGrainProcessor() = default;

// =============================================================================================
void DaveGrainProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    engine.prepare (sampleRate);
    filter.prepare (sampleRate, juce::jmax (samplesPerBlock, GrainEngine::maxChunk));
    delay.prepare (sampleRate);
    reverb.prepare (sampleRate);
    for (auto& l : lfos) l.reset();
    lastVolumeGain = 1.0f;
    lastOutputGain = juce::Decibels::decibelsToGain (p.output->load());
    keyboardState.reset();
}

void DaveGrainProcessor::releaseResources()
{
    engine.reset();
}

bool DaveGrainProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void DaveGrainProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
        engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())
        engine.noteOff (m.getNoteNumber());
    else if (m.isAllNotesOff() || m.isAllSoundOff())
        engine.allNotesOff (m.isAllSoundOff());
    else if (m.isSustainPedalOn())
        sustainPedal = true;
    else if (m.isSustainPedalOff())
        sustainPedal = false;
    else if (m.isPitchWheel())
        pitchBendSemis = (float) (m.getPitchWheelValue() - 8192) / 8192.0f * 2.0f;
}

GrainSettings DaveGrainProcessor::readSettings() const
{
    GrainSettings s;
    s.position     = p.position->load();
    s.spray        = p.spray->load();
    s.scan         = p.scan->load();
    s.freeze       = p.freeze->load() > 0.5f;
    s.sizeMs       = p.size->load();
    s.sizeRand     = p.sizeRand->load();
    s.density      = p.density->load();
    s.jitter       = p.jitter->load();
    s.pitch        = p.pitch->load() + p.fine->load() * 0.01f + pitchBendSemis;
    s.pitchRand    = p.pitchRand->load();
    s.quantize     = (int) p.quantize->load();
    s.keyTrack     = p.keyTrack->load() > 0.5f;
    s.rootNote     = (int) p.rootNote->load();
    s.window       = (int) p.window->load();
    s.reverse      = p.reverse->load();
    s.spread       = p.spread->load();
    s.velocitySens = p.velocity->load();
    return s;
}

void DaveGrainProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    // On-screen keyboard (standalone / editor) merges into the host MIDI stream.
    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);

    // Grab the current sample without ever blocking the audio thread.
    {
        const juce::SpinLock::ScopedTryLockType lock (sampleLock);
        if (lock.isLocked() && audioThreadSample != sample)
            audioThreadSample = sample; // held notes carry on with the new sample
    }
    const SampleData* src = audioThreadSample.get();

    engine.setEnvelope ({ p.attack->load(), p.decay->load(), p.sustain->load(), p.release->load() });
    engine.setHold (p.hold->load() > 0.5f || sustainPedal);

    // Host tempo for synced LFOs
    double bpm = 120.0, ppq = 0.0;
    bool transportRunning = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition()) ppq = *q;
            transportRunning = pos->getIsPlaying() && pos->getPpqPosition().hasValue();
        }

    const GrainSettings base = readSettings();
    const int filterType = (int) p.filterType->load();
    const float baseCutoff = p.cutoff->load();
    const float resonance = p.resonance->load();

    float* left = buffer.getWritePointer (0);
    float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    float monoScratch[GrainEngine::maxChunk];

    auto midiIt = midi.cbegin();
    int pos = 0;
    float lfoValues[2] = { 0.0f, 0.0f };

    while (pos < numSamples)
    {
        int nextEvent = numSamples;
        for (; midiIt != midi.cend(); ++midiIt)
        {
            const auto meta = *midiIt;
            if (meta.samplePosition > pos) { nextEvent = juce::jmin (meta.samplePosition, numSamples); break; }
            handleMidi (meta.getMessage());
        }
        engine.setHold (p.hold->load() > 0.5f || sustainPedal);

        const int chunk = juce::jmin (GrainEngine::maxChunk, nextEvent - pos);
        const double chunkSeconds = chunk / currentSampleRate;

        // ---- modulation ----
        GrainSettings s = base;
        float cutoff = baseCutoff, spreadMod = 0.0f, volume = 1.0f;

        for (int i = 0; i < 2; ++i)
        {
            const int target = (int) p.lfoTarget[i]->load();
            const bool synced = p.lfoSync[i]->load() > 0.5f;
            const double beats = Choices::divisionInBeats ((int) p.lfoDiv[i]->load());
            const int shape = (int) p.lfoShape[i]->load();

            float v;
            if (synced && transportRunning)
            {
                lfos[i].syncPhase ((ppq + pos * bpm / 60.0 / currentSampleRate) / beats);
                v = lfos[i].advance (0.0, 0.0, shape, false);
            }
            else
            {
                const double hz = synced ? bpm / 60.0 / beats : (double) p.lfoRate[i]->load();
                v = lfos[i].advance (hz, chunkSeconds, shape);
            }
            lfoValues[i] = v;

            const float m = v * p.lfoDepth[i]->load();
            switch (target)
            {
                case LfoTarget::position: s.position = s.position + 0.5f * m; s.position -= std::floor (s.position); break;
                case LfoTarget::spray:    s.spray    = juce::jlimit (0.0f, 1.0f, s.spray + 0.5f * m); break;
                case LfoTarget::size:     s.sizeMs   = juce::jlimit (2.0f, 4000.0f, s.sizeMs * std::exp2 (3.0f * m)); break;
                case LfoTarget::density:  s.density  = juce::jlimit (0.2f, 400.0f, s.density * std::exp2 (3.0f * m)); break;
                case LfoTarget::pitch:    s.pitch   += 12.0f * m; break;
                case LfoTarget::cutoff:   cutoff     = baseCutoff * std::exp2 (5.0f * m); break;
                case LfoTarget::spread:   spreadMod += m; break;
                case LfoTarget::volume:   volume    *= juce::jmax (0.0f, 1.0f - std::abs (p.lfoDepth[i]->load()) * 0.5f + 0.5f * m); break;
                default: break;
            }
        }
        s.spread = juce::jlimit (0.0f, 1.0f, s.spread + spreadMod);

        // ---- render ----
        float* l = left + pos;
        float* r = right != nullptr ? right + pos : monoScratch;
        if (right == nullptr) std::fill (monoScratch, monoScratch + chunk, 0.0f);

        engine.renderChunk (l, r, chunk, s, src);
        filter.process (l, r, chunk, filterType, cutoff, resonance);

        // volume (LFO) with a per-chunk ramp to avoid zipper noise
        const float step = (volume - lastVolumeGain) / (float) chunk;
        for (int i = 0; i < chunk; ++i)
        {
            const float g = lastVolumeGain + step * (float) (i + 1);
            l[i] *= g;
            r[i] *= g;
        }
        lastVolumeGain = volume;

        if (right == nullptr)
            for (int i = 0; i < chunk; ++i) l[i] = 0.5f * (l[i] + r[i]);

        pos += chunk;
    }

    for (; midiIt != midi.cend(); ++midiIt)
        handleMidi ((*midiIt).getMessage());

    lfoDisplay[0].store (lfoValues[0]);
    lfoDisplay[1].store (lfoValues[1]);

    // ---- FX + output (stereo only; mono layouts skip the stereo effects) ----
    if (right != nullptr)
    {
        delay.process (left, right, numSamples, p.delayTime->load(), p.delayFb->load(),
                       p.delayMix->load(), p.delayPing->load() > 0.5f);
        reverb.process (left, right, numSamples, p.reverbSize->load(), p.reverbDamp->load(), p.reverbMix->load());
    }

    const float outGain = juce::Decibels::decibelsToGain (p.output->load(), -100.0f);
    float peakL = 0.0f, peakR = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        const float g = lastOutputGain + (outGain - lastOutputGain) * (float) (i + 1) / (float) numSamples;
        left[i] = softClip (left[i] * g);
        peakL = juce::jmax (peakL, std::abs (left[i]));
        if (right != nullptr)
        {
            right[i] = softClip (right[i] * g);
            peakR = juce::jmax (peakR, std::abs (right[i]));
        }
    }
    lastOutputGain = outGain;

    meterLeft.store (juce::jmax (peakL, meterLeft.load() * 0.85f));
    meterRight.store (juce::jmax (right != nullptr ? peakR : peakL, meterRight.load() * 0.85f));

    engine.publishVisualisation (src);
}

// =============================================================================================
juce::AudioProcessorEditor* DaveGrainProcessor::createEditor()
{
    return new DaveGrainEditor (*this);
}

// =============================================================================================
bool DaveGrainProcessor::isSupportedAudioFile (const juce::String& path)
{
    const juce::File f (path);
    return f.hasFileExtension ("wav;wave;aif;aiff;aifc;flac;ogg;mp3;caf;m4a");
}

void DaveGrainProcessor::setSample (SamplePtr newSample)
{
    {
        const juce::SpinLock::ScopedLockType lock (sampleLock);
        if (sample != nullptr) retiredSamples.push_back (sample);
        sample = std::move (newSample);
    }

    // Free any retired sample the audio thread has let go of.
    retiredSamples.erase (std::remove_if (retiredSamples.begin(), retiredSamples.end(),
                                          [] (const SamplePtr& s) { return s.use_count() <= 1; }),
                          retiredSamples.end());

    missingSample.clear();
    ++sampleVersion;
}

bool DaveGrainProcessor::loadSampleFile (const juce::File& file, juce::String& error)
{
    if (! file.existsAsFile()) { error = "File not found: " + file.getFullPathName(); return false; }

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader == nullptr) { error = "Unsupported or unreadable audio file: " + file.getFileName(); return false; }

    const auto maxLength = (juce::int64) (maxSampleSeconds * reader->sampleRate);
    const int length = (int) juce::jmin (reader->lengthInSamples, maxLength);
    if (length < 64) { error = "That file is too short to granulate."; return false; }

    auto data = std::make_shared<SampleData>();
    data->buffer.setSize (2, length);
    data->sampleRate = reader->sampleRate > 0 ? reader->sampleRate : 44100.0;
    data->name = file.getFileName();
    data->source = file.getFullPathName();

    if (reader->numChannels >= 2)
    {
        reader->read (&data->buffer, 0, length, 0, true, true);
    }
    else
    {
        reader->read (&data->buffer, 0, length, 0, true, false);
        data->buffer.copyFrom (1, 0, data->buffer, 0, 0, length);
    }

    setSample (std::move (data));
    return true;
}

bool DaveGrainProcessor::loadBuiltinSample (const juce::String& name)
{
    if (auto current = getSample(); current != nullptr && current->source == BuiltinSources::prefix + name)
        return true;

    if (auto s = BuiltinSources::create (name))
    {
        setSample (std::move (s));
        return true;
    }
    return false;
}

bool DaveGrainProcessor::loadSampleSource (const juce::String& source)
{
    if (source.isEmpty()) return false;

    if (source.startsWith (BuiltinSources::prefix))
        return loadBuiltinSample (source.fromFirstOccurrenceOf (BuiltinSources::prefix, false, false));

    juce::String error;
    if (loadSampleFile (juce::File (source), error))
        return true;

    missingSample = source;
    ++sampleVersion;
    return false;
}

SamplePtr DaveGrainProcessor::getSample() const
{
    const juce::SpinLock::ScopedLockType lock (sampleLock);
    return sample;
}

bool DaveGrainProcessor::hasUserSample() const
{
    auto s = getSample();
    return s != nullptr && ! s->source.startsWith (BuiltinSources::prefix);
}

// =============================================================================================
juce::ValueTree DaveGrainProcessor::createStateTree()
{
    auto state = apvts.copyState();
    auto s = getSample();
    state.setProperty ("sampleSource", s != nullptr ? s->source : missingSample, nullptr);
    state.setProperty ("presetName", presets.getCurrentName(), nullptr);
    state.setProperty ("keepSample", keepSampleOnPresetLoad.load(), nullptr);
    state.setProperty ("uiScale", uiScale, nullptr);
    state.setProperty ("version", JucePlugin_VersionString, nullptr);
    return state;
}

void DaveGrainProcessor::restoreStateTree (const juce::ValueTree& tree, bool includeSample)
{
    if (! tree.hasType (apvts.state.getType())) return;

    apvts.replaceState (tree.createCopy());

    if (includeSample)
        loadSampleSource (tree.getProperty ("sampleSource").toString());
}

void DaveGrainProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = createStateTree().createXml())
        copyXmlToBinary (*xml, destData);
}

void DaveGrainProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr) return;

    auto tree = juce::ValueTree::fromXml (*xml);
    if (! tree.hasType (apvts.state.getType())) return;

    keepSampleOnPresetLoad.store ((bool) tree.getProperty ("keepSample", false));
    uiScale = (float) tree.getProperty ("uiScale", 1.0f);
    restoreStateTree (tree, true);
    presets.setCurrentName (tree.getProperty ("presetName", "Init").toString());
}

// =============================================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DaveGrainProcessor();
}
