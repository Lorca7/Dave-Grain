#include "Parameters.h"

namespace
{
    using APF = juce::AudioParameterFloat;
    using APC = juce::AudioParameterChoice;
    using APB = juce::AudioParameterBool;
    using API = juce::AudioParameterInt;

    juce::NormalisableRange<float> skewedRange (float min, float max, float centre, float interval = 0.0f)
    {
        juce::NormalisableRange<float> r (min, max, interval);
        r.setSkewForCentre (centre);
        return r;
    }

    juce::String percent (float v, int)         { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }
    juce::String signedPercent (float v, int)   { auto p = juce::roundToInt (v * 100.0f); return (p > 0 ? "+" : "") + juce::String (p) + " %"; }

    juce::String milliseconds (float v, int)
    {
        if (v >= 1000.0f) return juce::String (v / 1000.0f, 2) + " s";
        return juce::String (v, v < 100.0f ? 1 : 0) + " ms";
    }

    juce::String seconds (float v, int)
    {
        if (v < 1.0f) return juce::String (juce::roundToInt (v * 1000.0f)) + " ms";
        return juce::String (v, 2) + " s";
    }

    juce::String hertz (float v, int)
    {
        if (v >= 1000.0f) return juce::String (v / 1000.0f, 1) + " kHz";
        return juce::String (v, v < 10.0f ? 2 : (v < 100.0f ? 1 : 0)) + " Hz";
    }

    juce::String semitones (float v, int)
    {
        auto s = juce::String (v, std::abs (v - std::round (v)) < 0.005f ? 0 : 2);
        return (v > 0.0f ? "+" : "") + s + " st";
    }

    std::unique_ptr<APF> floatParam (const char* id, const char* name, juce::NormalisableRange<float> range, float def,
                                     std::function<juce::String (float, int)> toText, const char* label = "")
    {
        return std::make_unique<APF> (juce::ParameterID { id, 1 }, name, range, def,
                                      juce::AudioParameterFloatAttributes()
                                          .withLabel (label)
                                          .withStringFromValueFunction (std::move (toText)));
    }

    void addLfo (juce::AudioProcessorValueTreeState::ParameterLayout& layout, int index,
                 const char* rateID, const char* shapeID, const char* targetID,
                 const char* depthID, const char* syncID, const char* divID,
                 float defRate, int defShape)
    {
        const auto n = juce::String (index);
        auto name = [&] (const char* what) { return "LFO " + n + " " + what; };

        layout.add (std::make_unique<APF> (juce::ParameterID { rateID, 1 }, name ("Rate"),
                                           skewedRange (0.01f, 20.0f, 1.0f), defRate,
                                           juce::AudioParameterFloatAttributes().withStringFromValueFunction (hertz)));
        layout.add (std::make_unique<APC> (juce::ParameterID { shapeID, 1 }, name ("Shape"), Choices::lfoShapes(), defShape));
        layout.add (std::make_unique<APC> (juce::ParameterID { targetID, 1 }, name ("Target"), Choices::lfoTargets(), 0));
        layout.add (std::make_unique<APF> (juce::ParameterID { depthID, 1 }, name ("Depth"),
                                           juce::NormalisableRange<float> (-1.0f, 1.0f), 0.3f,
                                           juce::AudioParameterFloatAttributes().withStringFromValueFunction (signedPercent)));
        layout.add (std::make_unique<APB> (juce::ParameterID { syncID, 1 }, name ("Sync"), false));
        layout.add (std::make_unique<APC> (juce::ParameterID { divID, 1 }, name ("Division"), Choices::lfoDivisions(), 4));
    }
}

namespace Choices
{
    juce::StringArray windowShapes()  { return { "Hann", "Gaussian", "Triangle", "Tukey", "Percussive", "Swell" }; }
    juce::StringArray quantizeModes() { return { "Off", "Semitones", "Octaves", "Fifths", "Major", "Minor", "Pentatonic" }; }
    juce::StringArray filterTypes()   { return { "Off", "Low Pass", "Band Pass", "High Pass" }; }
    juce::StringArray lfoShapes()     { return { "Sine", "Triangle", "Saw", "Square", "Random", "Drift" }; }
    juce::StringArray lfoTargets()    { return { "None", "Position", "Spray", "Size", "Density", "Pitch", "Cutoff", "Spread", "Volume" }; }

    juce::StringArray lfoDivisions()
    {
        return { "4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/32",
                 "1/2T", "1/4T", "1/8T", "1/16T", "1/4D", "1/8D" };
    }

    double divisionInBeats (int index)
    {
        static constexpr double beats[] = { 16.0, 8.0, 4.0, 2.0, 1.0, 0.5, 0.25, 0.125,
                                            4.0 / 3.0, 2.0 / 3.0, 1.0 / 3.0, 1.0 / 6.0, 1.5, 0.75 };
        return beats[juce::jlimit (0, (int) std::size (beats) - 1, index)];
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace ParamIDs;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // --- Grain cloud ---------------------------------------------------------------------------
    layout.add (floatParam (position, "Position", { 0.0f, 1.0f }, 0.25f, percent));
    layout.add (floatParam (spray, "Spray", skewedRange (0.0f, 1.0f, 0.15f), 0.04f, percent));
    layout.add (floatParam (scan, "Scan", { -2.0f, 2.0f }, 0.0f, signedPercent));
    layout.add (std::make_unique<APB> (juce::ParameterID { freeze, 1 }, "Freeze", false));
    layout.add (floatParam (size, "Grain Size", skewedRange (5.0f, 2000.0f, 150.0f), 120.0f, milliseconds));
    layout.add (floatParam (sizeRand, "Size Variation", { 0.0f, 1.0f }, 0.15f, percent));
    layout.add (floatParam (density, "Density", skewedRange (0.5f, 200.0f, 20.0f), 16.0f,
                            [] (float v, int) { return juce::String (v, v < 10.0f ? 1 : 0) + " /s"; }));
    layout.add (floatParam (jitter, "Jitter", { 0.0f, 1.0f }, 0.35f, percent));

    // --- Pitch ---------------------------------------------------------------------------------
    layout.add (floatParam (pitch, "Pitch", { -24.0f, 24.0f, 1.0f }, 0.0f, semitones));
    layout.add (floatParam (fine, "Fine Tune", { -100.0f, 100.0f }, 0.0f,
                            [] (float v, int) { return (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v)) + " ct"; }));
    layout.add (floatParam (pitchRand, "Pitch Random", skewedRange (0.0f, 24.0f, 3.0f), 0.0f,
                            [] (float v, int) { return juce::String (v, 2) + " st"; }));
    layout.add (std::make_unique<APC> (juce::ParameterID { quantize, 1 }, "Pitch Quantize", Choices::quantizeModes(), 0));
    layout.add (std::make_unique<APB> (juce::ParameterID { keyTrack, 1 }, "Key Track", true));
    layout.add (std::make_unique<API> (juce::ParameterID { rootNote, 1 }, "Root Note", 0, 127, 60,
                                       juce::AudioParameterIntAttributes().withStringFromValueFunction (
                                           [] (int v, int) { return juce::MidiMessage::getMidiNoteName (v, true, true, 3); })));

    // --- Window --------------------------------------------------------------------------------
    layout.add (std::make_unique<APC> (juce::ParameterID { window, 1 }, "Window Shape", Choices::windowShapes(), 0));
    layout.add (floatParam (reverse, "Reverse", { 0.0f, 1.0f }, 0.0f, percent));
    layout.add (floatParam (spread, "Stereo Spread", { 0.0f, 1.0f }, 0.5f, percent));

    // --- Amp -----------------------------------------------------------------------------------
    layout.add (floatParam (attack, "Attack", skewedRange (0.001f, 10.0f, 0.5f), 0.05f, seconds));
    layout.add (floatParam (decay, "Decay", skewedRange (0.001f, 10.0f, 0.8f), 0.6f, seconds));
    layout.add (floatParam (sustain, "Sustain", { 0.0f, 1.0f }, 1.0f, percent));
    layout.add (floatParam (release, "Release", skewedRange (0.005f, 20.0f, 1.5f), 1.2f, seconds));
    layout.add (floatParam (velocity, "Velocity Sens", { 0.0f, 1.0f }, 0.7f, percent));
    layout.add (std::make_unique<APB> (juce::ParameterID { hold, 1 }, "Hold", false));

    // --- Filter --------------------------------------------------------------------------------
    layout.add (std::make_unique<APC> (juce::ParameterID { filterType, 1 }, "Filter Type", Choices::filterTypes(), 1));
    layout.add (floatParam (cutoff, "Cutoff", skewedRange (20.0f, 20000.0f, 1000.0f), 18000.0f, hertz));
    layout.add (floatParam (resonance, "Resonance", { 0.0f, 1.0f }, 0.15f, percent));

    // --- LFOs ----------------------------------------------------------------------------------
    addLfo (layout, 1, lfo1Rate, lfo1Shape, lfo1Target, lfo1Depth, lfo1Sync, lfo1Div, 0.25f, 0);
    addLfo (layout, 2, lfo2Rate, lfo2Shape, lfo2Target, lfo2Depth, lfo2Sync, lfo2Div, 0.1f, 5);

    // --- FX + output ---------------------------------------------------------------------------
    layout.add (floatParam (delayTime, "Delay Time", skewedRange (10.0f, 2000.0f, 400.0f), 375.0f, milliseconds));
    layout.add (floatParam (delayFb, "Delay Feedback", { 0.0f, 0.95f }, 0.35f, percent));
    layout.add (floatParam (delayMix, "Delay Mix", { 0.0f, 1.0f }, 0.0f, percent));
    layout.add (std::make_unique<APB> (juce::ParameterID { delayPing, 1 }, "Delay Ping-Pong", true));
    layout.add (floatParam (reverbSize, "Reverb Size", { 0.0f, 1.0f }, 0.7f, percent));
    layout.add (floatParam (reverbDamp, "Reverb Damping", { 0.0f, 1.0f }, 0.4f, percent));
    layout.add (floatParam (reverbMix, "Reverb Mix", { 0.0f, 1.0f }, 0.25f, percent));
    layout.add (floatParam (output, "Output", { -36.0f, 12.0f }, -3.0f,
                            [] (float v, int) { return juce::String (v, 1) + " dB"; }, "dB"));

    return layout;
}

juce::StringArray randomisableParameterIDs()
{
    using namespace ParamIDs;
    return { position, spray, scan, size, sizeRand, density, jitter,
             pitchRand, quantize, window, reverse, spread,
             cutoff, resonance,
             lfo1Rate, lfo1Shape, lfo1Target, lfo1Depth,
             lfo2Rate, lfo2Shape, lfo2Target, lfo2Depth,
             delayTime, delayFb, delayMix, reverbSize, reverbDamp, reverbMix };
}

juce::String parameterHelp (const juce::String& id)
{
    using namespace ParamIDs;
    static const std::map<juce::String, juce::String> help {
        { position,   "Where in the sample grains are taken from. You can also click or drag on the waveform." },
        { spray,      "Randomises each grain's start point around the position. Small = focused, large = smeared cloud." },
        { scan,       "Moves the playhead through the sample while a note is held. 100% = original speed, negative = backwards." },
        { freeze,     "Stops the scan playhead so the sound hangs on one moment of the sample." },
        { size,       "Length of each grain. Short grains = buzzy/textural, long grains = smooth and recognisable." },
        { sizeRand,   "Randomises the length of each grain." },
        { density,    "How many grains start per second (per voice)." },
        { jitter,     "Timing randomness. 0% = evenly spaced (synchronous, can sound pitched), 100% = random (asynchronous cloud)." },
        { pitch,      "Transposes every grain in semitones." },
        { fine,       "Fine tuning in cents." },
        { pitchRand,  "Gives each grain a random transposition up to this many semitones up or down." },
        { quantize,   "Snaps the random pitch of each grain to a musical scale." },
        { keyTrack,   "When on, MIDI notes transpose the sample relative to the Root Note." },
        { rootNote,   "The MIDI note that plays the sample at its original pitch." },
        { window,     "The volume envelope of each grain. Hann/Gaussian = smooth, Tukey = fuller, Percussive/Swell = rhythmic." },
        { reverse,    "Probability that a grain plays backwards." },
        { spread,     "Random left/right placement of each grain." },
        { attack,     "Note fade-in time." },
        { decay,      "Time to fall to the sustain level." },
        { sustain,    "Level held while the key is down." },
        { release,    "Fade-out time after the key is released." },
        { velocity,   "How much MIDI velocity affects loudness." },
        { hold,       "Latches notes so they keep playing after you let go — great for drones. Turn off to release." },
        { filterType, "Filter applied to the grain cloud." },
        { cutoff,     "Filter cutoff frequency." },
        { resonance,  "Filter emphasis around the cutoff." },
        { lfo1Rate,   "LFO speed in Hz." },               { lfo2Rate,   "LFO speed in Hz." },
        { lfo1Shape,  "LFO waveform. Drift is a smooth random wander." }, { lfo2Shape, "LFO waveform. Drift is a smooth random wander." },
        { lfo1Target, "Which parameter this LFO moves." }, { lfo2Target, "Which parameter this LFO moves." },
        { lfo1Depth,  "How far the LFO moves its target (negative inverts)." }, { lfo2Depth, "How far the LFO moves its target (negative inverts)." },
        { lfo1Sync,   "Lock the LFO to the host tempo." }, { lfo2Sync, "Lock the LFO to the host tempo." },
        { lfo1Div,    "Tempo-synced LFO cycle length." },  { lfo2Div,  "Tempo-synced LFO cycle length." },
        { delayTime,  "Echo time." },
        { delayFb,    "Number of repeats." },
        { delayMix,   "Echo level." },
        { delayPing,  "Bounce echoes between left and right." },
        { reverbSize, "Size of the reverb space." },
        { reverbDamp, "Darkens the reverb tail." },
        { reverbMix,  "Reverb level." },
        { output,     "Master output level." },
    };

    auto it = help.find (id);
    return it != help.end() ? it->second : juce::String();
}
