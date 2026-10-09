#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>

/** An immutable, loaded source sample. Always stereo (mono files are duplicated). */
struct SampleData
{
    juce::AudioBuffer<float> buffer;
    double sampleRate = 44100.0;
    juce::String name;   // shown in the UI
    juce::String source; // absolute file path, or "builtin:<Name>"

    int getNumSamples() const noexcept { return buffer.getNumSamples(); }
    double getLengthSeconds() const noexcept { return buffer.getNumSamples() / sampleRate; }
};

using SamplePtr = std::shared_ptr<const SampleData>;
