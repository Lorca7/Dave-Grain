#include "BuiltinSources.h"
#include <cmath>

namespace
{
    constexpr double rate = 44100.0;
    constexpr float twoPi = juce::MathConstants<float>::twoPi;

    double midiToHz (double note) { return 440.0 * std::exp2 ((note - 69.0) / 12.0); }

    std::shared_ptr<SampleData> makeEmpty (const juce::String& name, double seconds)
    {
        auto s = std::make_shared<SampleData>();
        s->buffer.setSize (2, (int) (seconds * rate));
        s->buffer.clear();
        s->sampleRate = rate;
        s->name = name;
        s->source = BuiltinSources::prefix + name;
        return s;
    }

    void normalise (juce::AudioBuffer<float>& b, float peak = 0.89f)
    {
        const float m = b.getMagnitude (0, b.getNumSamples());
        if (m > 0.0f) b.applyGain (peak / m);
    }

    // A slowly morphing major-9 chord of soft additive tones.
    SamplePtr glassPad()
    {
        auto s = makeEmpty ("Glass Pad", 8.0);
        const int n = s->buffer.getNumSamples();
        auto* l = s->buffer.getWritePointer (0);
        auto* r = s->buffer.getWritePointer (1);

        const double notes[] = { 48, 55, 59, 62, 64, 71 };
        juce::Random rng (7);

        for (size_t k = 0; k < std::size (notes); ++k)
        {
            const double f0 = midiToHz (notes[k]);
            const float pan = (float) k / (float) (std::size (notes) - 1);
            for (int h = 1; h <= 7; ++h)
            {
                const double f = f0 * h * (1.0 + 0.0007 * (h - 1)); // slight stretch = glassy
                if (f > 16000.0) break;
                const double inc = f / rate;
                const float amp = 1.0f / std::pow ((float) h, 1.4f);
                const float lfoRate = 0.05f + 0.25f * rng.nextFloat();
                const float lfoPhase = rng.nextFloat() * twoPi;
                double phase = rng.nextDouble();
                for (int i = 0; i < n; ++i)
                {
                    const float t = (float) i / (float) rate;
                    const float morph = 0.55f + 0.45f * std::sin (lfoPhase + twoPi * lfoRate * t * (float) h * 0.5f);
                    const float v = amp * morph * std::sin (twoPi * (float) phase);
                    l[i] += v * (1.0f - 0.6f * pan);
                    r[i] += v * (0.4f + 0.6f * pan);
                    phase += inc;
                    if (phase >= 1.0) phase -= 1.0;
                }
            }
        }

        normalise (s->buffer);
        return s;
    }

    // Inharmonic struck bells scattered over time.
    SamplePtr bellCloud()
    {
        auto s = makeEmpty ("Bell Cloud", 8.0);
        const int n = s->buffer.getNumSamples();
        auto* l = s->buffer.getWritePointer (0);
        auto* r = s->buffer.getWritePointer (1);

        const double ratios[] = { 0.56, 0.92, 1.19, 1.71, 2.0, 2.74, 3.0, 3.76, 4.07 };
        const double pitches[] = { 60, 67, 72, 74, 79, 64, 71, 76, 83, 69, 62, 77 };
        juce::Random rng (11);

        for (size_t b = 0; b < std::size (pitches); ++b)
        {
            const int start = (int) b * (n / (int) std::size (pitches)) + rng.nextInt (4000);
            const double f0 = midiToHz (pitches[b]);
            const float pan = rng.nextFloat();
            for (size_t p = 0; p < std::size (ratios); ++p)
            {
                const double f = f0 * ratios[p];
                if (f > 15000.0) continue;
                const float decay = 0.6f + 2.5f / (float) (p + 1);
                const float amp = 0.5f / (float) (p + 1);
                const double inc = f / rate;
                double phase = 0.0;
                const int len = juce::jmin (n - start, (int) (decay * 4.0f * rate));
                for (int i = 0; i < len; ++i)
                {
                    const float t = (float) i / (float) rate;
                    const float env = std::exp (-t / decay) * juce::jmin (1.0f, t * 400.0f);
                    const float v = amp * env * std::sin (twoPi * (float) phase);
                    l[start + i] += v * (1.0f - pan);
                    r[start + i] += v * pan;
                    phase += inc;
                    if (phase >= 1.0) phase -= 1.0;
                }
            }
        }

        normalise (s->buffer);
        return s;
    }

    // Breathy filtered noise with sweeping resonances.
    SamplePtr windTexture()
    {
        auto s = makeEmpty ("Wind Texture", 8.0);
        const int n = s->buffer.getNumSamples();
        juce::Random rng (3);

        for (int ch = 0; ch < 2; ++ch)
        {
            auto* d = s->buffer.getWritePointer (ch);
            float low = 0.0f, band = 0.0f;
            for (int i = 0; i < n; ++i)
            {
                const float t = (float) i / (float) rate;
                const float sweep = 0.5f + 0.5f * std::sin (twoPi * (0.11f + 0.03f * (float) ch) * t);
                const float fc = 180.0f + 2400.0f * sweep * sweep;
                const float f = 2.0f * std::sin (juce::MathConstants<float>::pi * fc / (float) rate);
                const float q = 0.08f;
                const float in = rng.nextFloat() * 2.0f - 1.0f;
                low += f * band;
                const float high = in - low - q * band;
                band += f * high;
                const float swell = 0.4f + 0.6f * std::pow (0.5f + 0.5f * std::sin (twoPi * 0.17f * t + (float) ch), 2.0f);
                d[i] = band * swell;
            }
        }

        normalise (s->buffer);
        return s;
    }
}

namespace BuiltinSources
{
    juce::StringArray names() { return { "Glass Pad", "Bell Cloud", "Wind Texture" }; }

    SamplePtr create (const juce::String& name)
    {
        if (name == "Glass Pad")    return glassPad();
        if (name == "Bell Cloud")   return bellCloud();
        if (name == "Wind Texture") return windTexture();
        return nullptr;
    }
}
