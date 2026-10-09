#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Every automatable parameter. IDs are stable: they are stored in Live sets and preset files,
// so never rename one — add new ones instead.
namespace ParamIDs
{
    // Grain cloud
    inline constexpr auto position    = "position";
    inline constexpr auto spray       = "spray";
    inline constexpr auto scan        = "scan";
    inline constexpr auto freeze      = "freeze";
    inline constexpr auto size        = "size";
    inline constexpr auto sizeRand    = "sizeRand";
    inline constexpr auto density     = "density";
    inline constexpr auto jitter      = "jitter";

    // Pitch
    inline constexpr auto pitch       = "pitch";
    inline constexpr auto fine        = "fine";
    inline constexpr auto pitchRand   = "pitchRand";
    inline constexpr auto quantize    = "quantize";
    inline constexpr auto keyTrack    = "keyTrack";
    inline constexpr auto rootNote    = "rootNote";

    // Grain window / shape
    inline constexpr auto window      = "window";
    inline constexpr auto reverse     = "reverse";
    inline constexpr auto spread      = "spread";

    // Amp
    inline constexpr auto attack      = "attack";
    inline constexpr auto decay       = "decay";
    inline constexpr auto sustain     = "sustain";
    inline constexpr auto release     = "release";
    inline constexpr auto velocity    = "velocity";
    inline constexpr auto hold        = "hold";

    // Filter
    inline constexpr auto filterType  = "filterType";
    inline constexpr auto cutoff      = "cutoff";
    inline constexpr auto resonance   = "resonance";

    // LFOs (prefix + suffix, e.g. "lfo1Rate")
    inline constexpr auto lfo1Rate    = "lfo1Rate";
    inline constexpr auto lfo1Shape   = "lfo1Shape";
    inline constexpr auto lfo1Target  = "lfo1Target";
    inline constexpr auto lfo1Depth   = "lfo1Depth";
    inline constexpr auto lfo1Sync    = "lfo1Sync";
    inline constexpr auto lfo1Div     = "lfo1Div";
    inline constexpr auto lfo2Rate    = "lfo2Rate";
    inline constexpr auto lfo2Shape   = "lfo2Shape";
    inline constexpr auto lfo2Target  = "lfo2Target";
    inline constexpr auto lfo2Depth   = "lfo2Depth";
    inline constexpr auto lfo2Sync    = "lfo2Sync";
    inline constexpr auto lfo2Div     = "lfo2Div";

    // FX + output
    inline constexpr auto delayTime   = "delayTime";
    inline constexpr auto delayFb     = "delayFeedback";
    inline constexpr auto delayMix    = "delayMix";
    inline constexpr auto delayPing   = "delayPingPong";
    inline constexpr auto reverbSize  = "reverbSize";
    inline constexpr auto reverbDamp  = "reverbDamp";
    inline constexpr auto reverbMix   = "reverbMix";
    inline constexpr auto output      = "output";
}

namespace Choices
{
    juce::StringArray windowShapes();
    juce::StringArray quantizeModes();
    juce::StringArray filterTypes();
    juce::StringArray lfoShapes();
    juce::StringArray lfoTargets();
    juce::StringArray lfoDivisions();

    /** Length of each lfoDivisions() entry in quarter notes. */
    double divisionInBeats (int index);
}

namespace LfoTarget
{
    enum { none = 0, position, spray, size, density, pitch, cutoff, spread, volume, numTargets };
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

/** Parameters the dice button is allowed to touch (keeps volume/envelopes sane). */
juce::StringArray randomisableParameterIDs();

/** One-line description of each parameter, shown as a tooltip. */
juce::String parameterHelp (const juce::String& paramID);
