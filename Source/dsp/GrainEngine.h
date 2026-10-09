#pragma once

#include "SampleData.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>

/** Grain-cloud settings, already modulated, resolved once per control chunk. */
struct GrainSettings
{
    float position    = 0.25f; // 0..1 of sample
    float spray       = 0.0f;  // 0..1 of sample
    float scan        = 0.0f;  // playback speed of the scan playhead (1 = realtime)
    bool  freeze      = false;
    float sizeMs      = 100.0f;
    float sizeRand    = 0.0f;
    float density     = 10.0f; // grains / second / voice
    float jitter      = 0.0f;  // 0 = synchronous, 1 = asynchronous
    float pitch       = 0.0f;  // semitones (incl. fine tune and LFO)
    float pitchRand   = 0.0f;  // +/- semitones
    int   quantize    = 0;
    bool  keyTrack    = true;
    int   rootNote    = 60;
    int   window      = 0;
    float reverse     = 0.0f;  // probability
    float spread      = 0.0f;  // stereo spread 0..1
    float velocitySens = 0.7f;
};

/**
    Polyphonic granular engine. Each MIDI voice owns a scan playhead and a pool of grains
    that read from a shared sample with cubic interpolation and a selectable window.
*/
class GrainEngine
{
public:
    static constexpr int maxVoices = 8;
    static constexpr int maxGrainsPerVoice = 96;
    static constexpr int maxChunk = 64;
    static constexpr int numWindowShapes = 6;
    static constexpr int windowTableSize = 1024;

    GrainEngine();

    void prepare (double sampleRate);
    void reset();

    void setEnvelope (const juce::ADSR::Parameters& p);
    void noteOn (int note, float velocity);
    void noteOff (int note);
    void setHold (bool shouldHold);
    void allNotesOff (bool immediately);

    /** Adds `numSamples` (<= maxChunk) of output into left/right. */
    void renderChunk (float* left, float* right, int numSamples, const GrainSettings& s, const SampleData* sample);

    /** Snapshot of active grains/playheads for the UI (written by the audio thread, lock-free). */
    void publishVisualisation (const SampleData* sample);

    int getNumActiveVoices() const noexcept;

    static float windowValue (int shape, float phase) noexcept;

    // ---- visualisation (read from the message thread) ----
    static constexpr int maxVizGrains = 256;
    std::atomic<int> vizNumGrains { 0 };
    std::array<std::atomic<float>, maxVizGrains> vizGrainPos {};   // 0..1
    std::array<std::atomic<float>, maxVizGrains> vizGrainAmp {};   // 0..1
    std::array<std::atomic<float>, maxVizGrains> vizGrainPan {};   // -1..1
    std::array<std::atomic<float>, maxVoices> vizPlayhead {};      // 0..1, or -1 if idle

private:
    struct Grain
    {
        bool   active = false;
        int    delay = 0;      // samples before the grain starts within the current chunk
        double readPos = 0.0;  // in source samples
        double increment = 1.0;
        int    length = 1;
        int    age = 0;
        float  gainL = 0.0f, gainR = 0.0f;
        int    window = 0;
    };

    struct Voice
    {
        bool   active = false;
        int    note = -1;
        float  velocityGain = 1.0f;
        bool   keyDown = false;
        bool   sustainedByHold = false;
        uint64_t startOrder = 0;
        double scanOffset = 0.0;        // normalised, added to position
        double samplesToNextGrain = 0.0;
        juce::ADSR adsr;
        std::array<Grain, maxGrainsPerVoice> grains;
    };

    void startVoice (Voice& v, int note, float velocity);
    void releaseVoice (Voice& v);
    void spawnGrain (Voice& v, int delay, const GrainSettings& s, const SampleData& sample);
    double nextGrainInterval (const GrainSettings& s);
    float quantizeSemitones (float semis, int mode) const;

    double sampleRate = 44100.0;
    std::array<Voice, maxVoices> voices;
    juce::ADSR::Parameters envParams;
    bool hold = false;
    uint64_t voiceCounter = 0;
    juce::Random random;

    float voiceL[maxChunk] {}, voiceR[maxChunk] {};
};
