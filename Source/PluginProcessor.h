#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include "Parameters.h"
#include "PresetManager.h"
#include "dsp/GrainEngine.h"
#include "dsp/Effects.h"
#include "dsp/Lfo.h"

class DaveGrainProcessor : public juce::AudioProcessor
{
public:
    DaveGrainProcessor();
    ~DaveGrainProcessor() override;

    // ---- AudioProcessor ----
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Dave Grain"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- Samples (message thread) ----
    static bool isSupportedAudioFile (const juce::String& path);
    bool loadSampleFile (const juce::File& file, juce::String& errorMessage);
    bool loadBuiltinSample (const juce::String& name);
    bool loadSampleSource (const juce::String& source); // file path or "builtin:Name"
    SamplePtr getSample() const;
    bool hasUserSample() const;
    int getSampleVersion() const noexcept { return sampleVersion.load(); }
    juce::String getMissingSample() const { return missingSample; }

    // ---- Whole-instrument state (used by host state + presets) ----
    juce::ValueTree createStateTree();
    void restoreStateTree (const juce::ValueTree& tree, bool includeSample);

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    GrainEngine engine;
    PresetManager presets;

    std::atomic<float> meterLeft { 0.0f }, meterRight { 0.0f };
    std::atomic<float> lfoDisplay[2] { 0.0f, 0.0f };
    std::atomic<bool>  keepSampleOnPresetLoad { false };

    float uiScale = 1.0f;

private:
    void setSample (SamplePtr newSample);
    void handleMidi (const juce::MidiMessage& m);
    GrainSettings readSettings() const;

    juce::AudioFormatManager formatManager;

    mutable juce::SpinLock sampleLock;
    SamplePtr sample;              // guarded by sampleLock
    SamplePtr audioThreadSample;   // only touched by the audio thread
    std::vector<SamplePtr> retiredSamples; // freed on the message thread, never the audio thread
    std::atomic<int> sampleVersion { 0 };
    juce::String missingSample;

    Lfo lfos[2];
    CloudFilter filter;
    StereoDelay delay;
    ReverbFx reverb;
    float lastVolumeGain = 1.0f;
    float lastOutputGain = 1.0f;
    float pitchBendSemis = 0.0f;
    bool sustainPedal = false;
    double currentSampleRate = 44100.0;

    // cached raw parameter pointers
    struct P
    {
        std::atomic<float>* position; std::atomic<float>* spray; std::atomic<float>* scan; std::atomic<float>* freeze;
        std::atomic<float>* size; std::atomic<float>* sizeRand; std::atomic<float>* density; std::atomic<float>* jitter;
        std::atomic<float>* pitch; std::atomic<float>* fine; std::atomic<float>* pitchRand; std::atomic<float>* quantize;
        std::atomic<float>* keyTrack; std::atomic<float>* rootNote;
        std::atomic<float>* window; std::atomic<float>* reverse; std::atomic<float>* spread;
        std::atomic<float>* attack; std::atomic<float>* decay; std::atomic<float>* sustain; std::atomic<float>* release;
        std::atomic<float>* velocity; std::atomic<float>* hold;
        std::atomic<float>* filterType; std::atomic<float>* cutoff; std::atomic<float>* resonance;
        std::atomic<float>* lfoRate[2]; std::atomic<float>* lfoShape[2]; std::atomic<float>* lfoTarget[2];
        std::atomic<float>* lfoDepth[2]; std::atomic<float>* lfoSync[2]; std::atomic<float>* lfoDiv[2];
        std::atomic<float>* delayTime; std::atomic<float>* delayFb; std::atomic<float>* delayMix; std::atomic<float>* delayPing;
        std::atomic<float>* reverbSize; std::atomic<float>* reverbDamp; std::atomic<float>* reverbMix;
        std::atomic<float>* output;
    } p {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DaveGrainProcessor)
};
