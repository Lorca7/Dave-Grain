#include "Effects.h"

// ---------------------------------------------------------------------------------------------
void CloudFilter::prepare (double sr, int maxBlock)
{
    sampleRate = sr;
    filter.prepare ({ sr, (juce::uint32) juce::jmax (1, maxBlock), 2 });
    reset();
}

void CloudFilter::reset()
{
    filter.reset();
}

void CloudFilter::process (float* left, float* right, int numSamples, int type, float cutoffHz, float resonance)
{
    if (type <= 0) { lastType = 0; return; }

    if (type != lastType)
    {
        filter.setType (type == 1 ? juce::dsp::StateVariableTPTFilterType::lowpass
                      : type == 2 ? juce::dsp::StateVariableTPTFilterType::bandpass
                                  : juce::dsp::StateVariableTPTFilterType::highpass);
        if (lastType <= 0) filter.reset();
        lastType = type;
    }

    filter.setCutoffFrequency (juce::jlimit (20.0f, (float) (sampleRate * 0.45), cutoffHz));
    filter.setResonance (0.5f + resonance * resonance * 9.5f);

    for (int i = 0; i < numSamples; ++i)
    {
        left[i]  = filter.processSample (0, left[i]);
        right[i] = filter.processSample (1, right[i]);
    }
}

// ---------------------------------------------------------------------------------------------
void StereoDelay::prepare (double sr)
{
    sampleRate = sr;
    buffer.setSize (2, (int) (sr * 2.2) + 4);
    reset();
}

void StereoDelay::reset()
{
    buffer.clear();
    writePos = 0;
    smoothedDelay = -1.0f;
    dampL = dampR = 0.0f;
}

void StereoDelay::process (float* left, float* right, int numSamples, float timeMs, float feedback, float mix, bool pingPong)
{
    if (mix <= 0.0001f && mixSmoothed <= 0.0001f)
    {
        mixSmoothed = 0.0f;
        // Keep the line flowing (silently) so turning the mix up doesn't replay stale audio.
        for (int i = 0; i < numSamples; ++i)
        {
            buffer.setSample (0, writePos, 0.0f);
            buffer.setSample (1, writePos, 0.0f);
            if (++writePos >= buffer.getNumSamples()) writePos = 0;
        }
        return;
    }

    const int size = buffer.getNumSamples();
    float* bl = buffer.getWritePointer (0);
    float* br = buffer.getWritePointer (1);

    const float targetDelay = juce::jlimit (1.0f, (float) size - 3.0f, timeMs * 0.001f * (float) sampleRate);
    if (smoothedDelay < 0.0f) smoothedDelay = targetDelay;

    const float dampCoef = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 5500.0f / (float) sampleRate);

    for (int i = 0; i < numSamples; ++i)
    {
        smoothedDelay += 0.0005f * (targetDelay - smoothedDelay);
        mixSmoothed   += 0.002f * (mix - mixSmoothed);

        float readPos = (float) writePos - smoothedDelay;
        if (readPos < 0.0f) readPos += (float) size;
        const int r0 = (int) readPos;
        const int r1 = r0 + 1 >= size ? 0 : r0 + 1;
        const float f = readPos - (float) r0;
        const float dl = bl[r0] + f * (bl[r1] - bl[r0]);
        const float dr = br[r0] + f * (br[r1] - br[r0]);

        dampL += dampCoef * (dl - dampL);
        dampR += dampCoef * (dr - dampR);

        const float inL = left[i], inR = right[i];
        if (pingPong)
        {
            const float mono = 0.5f * (inL + inR);
            bl[writePos] = mono + dampR * feedback;
            br[writePos] = dampL * feedback;
        }
        else
        {
            bl[writePos] = inL + dampL * feedback;
            br[writePos] = inR + dampR * feedback;
        }

        left[i]  = inL + dl * mixSmoothed;
        right[i] = inR + dr * mixSmoothed;

        if (++writePos >= size) writePos = 0;
    }
}

// ---------------------------------------------------------------------------------------------
void ReverbFx::prepare (double sr)
{
    reverb.setSampleRate (sr);
    reverb.reset();
}

void ReverbFx::process (float* left, float* right, int numSamples, float size, float damping, float mix)
{
    juce::Reverb::Parameters p;
    p.roomSize = size;
    p.damping = damping;
    p.wetLevel = mix;
    p.dryLevel = 1.0f - 0.5f * mix;
    p.width = 1.0f;
    reverb.setParameters (p); // juce::Reverb smooths these internally
    reverb.processStereo (left, right, numSamples);
}
