#pragma once

#include <juce_dsp/juce_dsp.h>

/** State-variable filter applied to the summed grain cloud (cutoff can change every chunk). */
class CloudFilter
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset();
    /** type: 0 off, 1 LP, 2 BP, 3 HP. resonance 0..1 */
    void process (float* left, float* right, int numSamples, int type, float cutoffHz, float resonance);

private:
    juce::dsp::StateVariableTPTFilter<float> filter;
    double sampleRate = 44100.0;
    int lastType = -1;
};

/** Stereo feedback delay with optional ping-pong and damped repeats. */
class StereoDelay
{
public:
    void prepare (double sampleRate);
    void reset();
    void process (float* left, float* right, int numSamples, float timeMs, float feedback, float mix, bool pingPong);

private:
    juce::AudioBuffer<float> buffer;
    int writePos = 0;
    double sampleRate = 44100.0;
    float smoothedDelay = -1.0f;
    float dampL = 0.0f, dampR = 0.0f;
    float mixSmoothed = 0.0f;
};

class ReverbFx
{
public:
    void prepare (double sampleRate);
    void reset() { reverb.reset(); }
    void process (float* left, float* right, int numSamples, float size, float damping, float mix);

private:
    juce::Reverb reverb;
};
