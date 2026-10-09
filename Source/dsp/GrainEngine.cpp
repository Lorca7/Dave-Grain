#include "GrainEngine.h"
#include <cmath>

namespace
{
    struct WindowTables
    {
        float data[GrainEngine::numWindowShapes][GrainEngine::windowTableSize + 1];

        WindowTables()
        {
            constexpr int n = GrainEngine::windowTableSize;
            const float pi = juce::MathConstants<float>::pi;

            for (int i = 0; i <= n; ++i)
            {
                const float x = (float) i / (float) n;

                // 0 Hann
                data[0][i] = 0.5f - 0.5f * std::cos (2.0f * pi * x);

                // 1 Gaussian (sigma = 0.15), lifted so the edges land exactly on zero
                const float edge = std::exp (-0.5f * (0.5f / 0.15f) * (0.5f / 0.15f));
                const float g = std::exp (-0.5f * ((x - 0.5f) / 0.15f) * ((x - 0.5f) / 0.15f));
                data[1][i] = juce::jmax (0.0f, (g - edge) / (1.0f - edge));

                // 2 Triangle
                data[2][i] = 1.0f - std::abs (2.0f * x - 1.0f);

                // 3 Tukey (50% taper): flat top, cosine edges
                constexpr float a = 0.5f;
                if (x < a * 0.5f)               data[3][i] = 0.5f * (1.0f - std::cos (2.0f * pi * x / a));
                else if (x > 1.0f - a * 0.5f)   data[3][i] = 0.5f * (1.0f - std::cos (2.0f * pi * (1.0f - x) / a));
                else                            data[3][i] = 1.0f;

                // 4 Percussive: very short attack, curved decay
                constexpr float att = 0.03f;
                const float perc = x < att ? std::sin (0.5f * pi * x / att)
                                           : std::pow (1.0f - (x - att) / (1.0f - att), 3.0f);
                data[4][i] = perc;

                // 5 Swell: reversed percussive (filled in below)
            }

            for (int i = 0; i <= n; ++i)
                data[5][i] = data[4][n - i];

            for (auto& table : data)
                table[0] = table[n] = 0.0f;
        }
    };

    const WindowTables& windowTables()
    {
        static const WindowTables tables;
        return tables;
    }

    /** 4-point Hermite interpolation of both channels, wrapping around the sample ends. */
    inline void hermiteStereo (const float* l, const float* r, int n, double pos, float& outL, float& outR) noexcept
    {
        const int i1 = (int) pos;
        const float t = (float) (pos - (double) i1);
        const int i0 = i1 == 0 ? n - 1 : i1 - 1;
        const int i2 = i1 + 1 >= n ? i1 + 1 - n : i1 + 1;
        const int i3 = i2 + 1 >= n ? i2 + 1 - n : i2 + 1;

        auto interp = [t] (float y0, float y1, float y2, float y3)
        {
            const float c1 = 0.5f * (y2 - y0);
            const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
            const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
            return ((c3 * t + c2) * t + c1) * t + y1;
        };

        outL = interp (l[i0], l[i1], l[i2], l[i3]);
        outR = interp (r[i0], r[i1], r[i2], r[i3]);
    }

    inline double wrapPosition (double p, double length) noexcept
    {
        if (p >= length || p < 0.0)
        {
            p = std::fmod (p, length);
            if (p < 0.0) p += length;
            if (p >= length) p = 0.0;
        }
        return p;
    }

    inline double wrap01 (double x) noexcept { return x - std::floor (x); }
}

GrainEngine::GrainEngine()
{
    windowTables(); // build tables up front, not on the audio thread
    for (auto& p : vizPlayhead) p.store (-1.0f);
}

void GrainEngine::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    for (auto& v : voices)
        v.adsr.setSampleRate (sampleRate);
    reset();
}

void GrainEngine::reset()
{
    for (auto& v : voices)
    {
        v.active = false;
        v.adsr.reset();
        for (auto& g : v.grains) g.active = false;
    }
    vizNumGrains.store (0);
    for (auto& p : vizPlayhead) p.store (-1.0f);
}

void GrainEngine::setEnvelope (const juce::ADSR::Parameters& p)
{
    if (juce::exactlyEqual (p.attack, envParams.attack) && juce::exactlyEqual (p.decay, envParams.decay)
        && juce::exactlyEqual (p.sustain, envParams.sustain) && juce::exactlyEqual (p.release, envParams.release))
        return;

    envParams = p;
    for (auto& v : voices)
        v.adsr.setParameters (envParams);
}

float GrainEngine::windowValue (int shape, float phase) noexcept
{
    const auto& t = windowTables().data[juce::jlimit (0, numWindowShapes - 1, shape)];
    const float x = juce::jlimit (0.0f, 1.0f, phase) * (float) windowTableSize;
    const int i = juce::jmin ((int) x, windowTableSize - 1);
    const float f = x - (float) i;
    return t[i] + f * (t[i + 1] - t[i]);
}

void GrainEngine::startVoice (Voice& v, int note, float velocity)
{
    v.active = true;
    v.note = note;
    v.keyDown = true;
    v.sustainedByHold = false;
    v.velocityGain = velocity;
    v.startOrder = ++voiceCounter;
    v.scanOffset = 0.0;
    v.samplesToNextGrain = 0.0;
    for (auto& g : v.grains) g.active = false;
    v.adsr.setParameters (envParams);
    v.adsr.reset();
    v.adsr.noteOn();
}

void GrainEngine::noteOn (int note, float velocity)
{
    Voice* target = nullptr;

    for (auto& v : voices)
        if (! v.active) { target = &v; break; }

    if (target == nullptr) // steal: prefer the oldest released voice, else the oldest voice
    {
        for (auto& v : voices)
            if (! v.keyDown && ! v.sustainedByHold && (target == nullptr || v.startOrder < target->startOrder))
                target = &v;

        if (target == nullptr)
            for (auto& v : voices)
                if (target == nullptr || v.startOrder < target->startOrder)
                    target = &v;
    }

    startVoice (*target, note, velocity);
}

void GrainEngine::releaseVoice (Voice& v)
{
    v.keyDown = false;
    v.sustainedByHold = false;
    v.adsr.noteOff();
}

void GrainEngine::noteOff (int note)
{
    for (auto& v : voices)
    {
        if (v.active && v.keyDown && v.note == note)
        {
            if (hold) { v.keyDown = false; v.sustainedByHold = true; }
            else      releaseVoice (v);
        }
    }
}

void GrainEngine::setHold (bool shouldHold)
{
    if (hold == shouldHold) return;
    hold = shouldHold;

    if (! hold)
        for (auto& v : voices)
            if (v.active && v.sustainedByHold)
                releaseVoice (v);
}

void GrainEngine::allNotesOff (bool immediately)
{
    for (auto& v : voices)
    {
        if (! v.active) continue;
        if (immediately) { v.active = false; v.adsr.reset(); for (auto& g : v.grains) g.active = false; }
        else releaseVoice (v);
    }
}

int GrainEngine::getNumActiveVoices() const noexcept
{
    int n = 0;
    for (auto& v : voices) n += v.active ? 1 : 0;
    return n;
}

float GrainEngine::quantizeSemitones (float semis, int mode) const
{
    if (mode <= 0) return semis;
    if (mode == 1) return std::round (semis);

    static const std::vector<int> scales[] = {
        { 0 },                       // octaves
        { 0, 7 },                    // fifths
        { 0, 2, 4, 5, 7, 9, 11 },    // major
        { 0, 2, 3, 5, 7, 8, 10 },    // minor
        { 0, 3, 5, 7, 10 },          // minor pentatonic
    };
    const auto& scale = scales[juce::jlimit (0, 4, mode - 2)];

    const float octave = std::floor (semis / 12.0f);
    float best = 0.0f, bestDist = 1.0e9f;
    for (float o = octave - 1.0f; o <= octave + 1.0f; o += 1.0f)
        for (int degree : scale)
        {
            const float candidate = o * 12.0f + (float) degree;
            const float d = std::abs (candidate - semis);
            if (d < bestDist) { bestDist = d; best = candidate; }
        }
    return best;
}

double GrainEngine::nextGrainInterval (const GrainSettings& s)
{
    const double base = sampleRate / juce::jmax (0.05, (double) s.density);
    // Blend between a fixed clock (synchronous) and exponential spacing (asynchronous / Poisson).
    const double r = juce::jmax (1.0e-6, 1.0 - (double) random.nextFloat());
    const double async = -std::log (r);
    const double factor = (1.0 - s.jitter) + s.jitter * async;
    return juce::jmax (1.0, base * factor);
}

void GrainEngine::spawnGrain (Voice& v, int delay, const GrainSettings& s, const SampleData& sample)
{
    Grain* g = nullptr;
    for (auto& candidate : v.grains)
        if (! candidate.active) { g = &candidate; break; }
    if (g == nullptr) return; // pool full: skip this grain

    const double length = (double) sample.getNumSamples();
    auto bipolar = [this] { return random.nextFloat() * 2.0f - 1.0f; };

    // Position: base + scan playhead + spray
    const double centre = wrap01 ((double) s.position + v.scanOffset);
    const double pos = wrap01 (centre + (double) (s.spray * 0.5f * bipolar()));

    // Size (randomised in the log domain, +/- 2 octaves at 100%)
    const float sizeMs = juce::jlimit (2.0f, 4000.0f, s.sizeMs * std::exp2 (2.0f * s.sizeRand * bipolar()));
    const int grainLength = juce::jmax (8, (int) (sizeMs * 0.001f * (float) sampleRate));

    // Pitch
    float semis = s.pitch;
    if (s.keyTrack) semis += (float) (v.note - s.rootNote);
    if (s.pitchRand > 0.0f) semis += quantizeSemitones (s.pitchRand * bipolar(), s.quantize);
    double ratio = std::exp2 ((double) semis / 12.0) * sample.sampleRate / sampleRate;
    ratio = juce::jlimit (1.0 / 64.0, 64.0, ratio);
    if (random.nextFloat() < s.reverse) ratio = -ratio;

    // Pan (equal power) and overlap compensation so dense clouds don't explode in level
    const float pan = juce::jlimit (-1.0f, 1.0f, s.spread * bipolar());
    const float angle = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
    const float overlap = juce::jmax (1.0f, s.density * s.sizeMs * 0.001f);
    const float gain = 0.9f / std::sqrt (overlap);

    g->active = true;
    g->delay = delay;
    g->readPos = pos * length;
    g->increment = ratio;
    g->length = grainLength;
    g->age = 0;
    g->gainL = std::cos (angle) * gain * juce::MathConstants<float>::sqrt2;
    g->gainR = std::sin (angle) * gain * juce::MathConstants<float>::sqrt2;
    g->window = s.window;
}

void GrainEngine::renderChunk (float* left, float* right, int numSamples, const GrainSettings& s, const SampleData* sample)
{
    jassert (numSamples <= maxChunk);
    const auto& tables = windowTables();

    for (size_t vi = 0; vi < voices.size(); ++vi)
    {
        auto& v = voices[vi];
        if (! v.active) continue;

        if (sample == nullptr || sample->getNumSamples() < 4)
        {
            // Nothing to play: keep the envelope running so voices still end normally.
            for (int i = 0; i < numSamples; ++i) v.adsr.getNextSample();
            if (! v.adsr.isActive()) v.active = false;
            continue;
        }

        const int n = sample->getNumSamples();
        const double length = (double) n;
        const float* srcL = sample->buffer.getReadPointer (0);
        const float* srcR = sample->buffer.getReadPointer (1);

        // Schedule grains that start inside this chunk
        while (v.samplesToNextGrain < (double) numSamples)
        {
            spawnGrain (v, juce::jmax (0, (int) v.samplesToNextGrain), s, *sample);
            v.samplesToNextGrain += nextGrainInterval (s);
        }
        v.samplesToNextGrain -= (double) numSamples;

        std::fill (voiceL, voiceL + numSamples, 0.0f);
        std::fill (voiceR, voiceR + numSamples, 0.0f);

        for (auto& g : v.grains)
        {
            if (! g.active) continue;

            const float* win = tables.data[juce::jlimit (0, numWindowShapes - 1, g.window)];
            const float phaseScale = (float) windowTableSize / (float) g.length;

            for (int i = g.delay; i < numSamples; ++i)
            {
                if (g.age >= g.length) { g.active = false; break; }

                const float wx = (float) g.age * phaseScale;
                const int wi = juce::jmin ((int) wx, windowTableSize - 1);
                const float w = win[wi] + (wx - (float) wi) * (win[wi + 1] - win[wi]);

                g.readPos = wrapPosition (g.readPos, length);
                float sl, sr;
                hermiteStereo (srcL, srcR, n, g.readPos, sl, sr);
                voiceL[i] += sl * w * g.gainL;
                voiceR[i] += sr * w * g.gainR;

                g.readPos += g.increment;
                ++g.age;
            }
            g.delay = 0;
            if (g.age >= g.length) g.active = false;
        }

        const float velGain = (1.0f - s.velocitySens) + s.velocitySens * v.velocityGain;
        for (int i = 0; i < numSamples; ++i)
        {
            const float e = v.adsr.getNextSample() * velGain;
            left[i]  += voiceL[i] * e;
            right[i] += voiceR[i] * e;
        }

        if (! s.freeze)
            v.scanOffset = wrap01 (v.scanOffset + (double) s.scan * (double) numSamples
                                                     * (sample->sampleRate / sampleRate) / length);

        vizPlayhead[vi].store ((float) wrap01 ((double) s.position + v.scanOffset), std::memory_order_relaxed);

        if (! v.adsr.isActive())
        {
            v.active = false;
            for (auto& g : v.grains) g.active = false;
        }
    }
}

void GrainEngine::publishVisualisation (const SampleData* sample)
{
    int count = 0;

    if (sample != nullptr && sample->getNumSamples() > 0)
    {
        const double length = (double) sample->getNumSamples();

        for (auto& v : voices)
        {
            if (! v.active) continue;
            for (auto& g : v.grains)
            {
                if (! g.active || count >= maxVizGrains) continue;
                const float w = windowValue (g.window, (float) g.age / (float) g.length);
                vizGrainPos[(size_t) count].store ((float) (wrapPosition (g.readPos, length) / length), std::memory_order_relaxed);
                vizGrainAmp[(size_t) count].store (w * v.velocityGain, std::memory_order_relaxed);
                const float gl = g.gainL, gr = g.gainR;
                vizGrainPan[(size_t) count].store ((gr - gl) / juce::jmax (1.0e-6f, gl + gr), std::memory_order_relaxed);
                ++count;
            }
        }
    }

    vizNumGrains.store (count, std::memory_order_release);

    for (size_t i = 0; i < voices.size(); ++i)
        if (! voices[i].active)
            vizPlayhead[i].store (-1.0f, std::memory_order_relaxed);
}
