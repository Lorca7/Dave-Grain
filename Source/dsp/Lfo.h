#pragma once

#include <juce_core/juce_core.h>
#include <cmath>

/** Control-rate LFO producing values in -1..1. */
class Lfo
{
public:
    enum Shape { sine = 0, triangle, saw, square, random, drift };

    void reset() { phase = 0.0; held = target = current = 0.0f; }

    /** Sets the phase directly (used when locked to the host transport). */
    void syncPhase (double newPhase)
    {
        newPhase -= std::floor (newPhase);
        if (newPhase < phase - 0.5) newCycle(); // wrapped
        phase = newPhase;
    }

    /** Advances by `seconds` at `hz` and returns the new value. */
    float advance (double hz, double seconds, int shape, bool freeRunning = true)
    {
        if (freeRunning)
        {
            phase += hz * seconds;
            if (phase >= 1.0) { phase -= std::floor (phase); newCycle(); }
        }

        const float p = (float) phase;
        switch (shape)
        {
            case sine:     return std::sin (p * juce::MathConstants<float>::twoPi);
            case triangle: return 1.0f - 4.0f * std::abs (p - 0.5f);
            case saw:      return 2.0f * p - 1.0f;
            case square:   return p < 0.5f ? 1.0f : -1.0f;
            case random:   return held;
            case drift:
            default:
            {
                // Smooth random wander: cosine-interpolate between successive random targets.
                const float t = 0.5f - 0.5f * std::cos (p * juce::MathConstants<float>::pi);
                return current + (target - current) * t;
            }
        }
    }

private:
    void newCycle()
    {
        held = rng.nextFloat() * 2.0f - 1.0f;
        current = target;
        target = rng.nextFloat() * 2.0f - 1.0f;
    }

    double phase = 0.0;
    float held = 0.0f, current = 0.0f, target = 0.0f;
    juce::Random rng;
};
