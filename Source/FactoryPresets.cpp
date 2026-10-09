#include "FactoryPresets.h"
#include "Parameters.h"

// Choice indices, for readability below
namespace
{
    enum Window { hann, gaussian, triangle, tukey, percussive, swell };
    enum Quant  { qOff, qSemis, qOctaves, qFifths, qMajor, qMinor, qPenta };
    enum Filter { fOff, fLP, fBP, fHP };
    enum Shape  { sSine, sTri, sSaw, sSquare, sRandom, sDrift };
    enum Div    { d4bars, d2bars, d1bar, dHalf, dQuarter, dEighth, dSixteenth, dThirtySecond };
}

const std::vector<FactoryPreset>& getFactoryPresets()
{
    using namespace ParamIDs;

    static const std::vector<FactoryPreset> presets {
        { "Init", "Basics", "Glass Pad", {} },

        { "Glass Cathedral", "Pads", "Glass Pad", {
            { size, 380 }, { sizeRand, 0.3f }, { density, 26 }, { spray, 0.12f }, { jitter, 0.7f },
            { window, gaussian }, { spread, 0.85f }, { pitchRand, 0.12f },
            { attack, 1.2f }, { release, 4.0f },
            { lfo1Target, LfoTarget::position }, { lfo1Shape, sDrift }, { lfo1Rate, 0.08f }, { lfo1Depth, 0.25f },
            { reverbMix, 0.45f }, { reverbSize, 0.88f } } },

        { "Frozen Moment", "Pads", "Glass Pad", {
            { freeze, 1 }, { position, 0.4f }, { size, 620 }, { density, 30 }, { spray, 0.02f },
            { jitter, 0.8f }, { window, tukey }, { spread, 0.7f }, { attack, 0.6f }, { release, 3.0f },
            { reverbMix, 0.35f } } },

        { "Time Stretch", "Playback", "Bell Cloud", {
            { position, 0.0f }, { scan, 0.35f }, { size, 90 }, { density, 32 }, { spray, 0.004f },
            { jitter, 0.25f }, { window, hann }, { spread, 0.3f }, { attack, 0.01f }, { release, 1.0f } } },

        { "Reverse Tape", "Playback", "Glass Pad", {
            { scan, -0.5f }, { reverse, 1.0f }, { size, 260 }, { density, 14 }, { spray, 0.03f },
            { jitter, 0.4f }, { pitch, -12 }, { fine, -8 },
            { filterType, fLP }, { cutoff, 3500 }, { resonance, 0.1f },
            { lfo1Target, LfoTarget::pitch }, { lfo1Shape, sSine }, { lfo1Rate, 0.4f }, { lfo1Depth, 0.02f },
            { delayMix, 0.2f }, { delayTime, 480 }, { delayFb, 0.45f } } },

        { "Shimmer Octaves", "Pads", "Glass Pad", {
            { size, 300 }, { density, 30 }, { spray, 0.08f }, { jitter, 0.75f },
            { pitchRand, 12 }, { quantize, qOctaves }, { window, gaussian }, { spread, 1.0f },
            { attack, 0.8f }, { release, 3.5f },
            { delayMix, 0.2f }, { delayTime, 600 }, { reverbMix, 0.5f }, { reverbSize, 0.9f } } },

        { "Pentatonic Rain", "Melodic", "Bell Cloud", {
            { size, 70 }, { density, 18 }, { spray, 0.35f }, { jitter, 1.0f },
            { pitchRand, 12 }, { quantize, qPenta }, { window, percussive }, { spread, 1.0f },
            { attack, 0.005f }, { release, 2.0f },
            { delayMix, 0.3f }, { delayTime, 333 }, { delayFb, 0.5f }, { reverbMix, 0.4f } } },

        { "Fifth Choir", "Melodic", "Glass Pad", {
            { size, 420 }, { density, 20 }, { spray, 0.15f }, { jitter, 0.6f },
            { pitchRand, 7 }, { quantize, qFifths }, { window, hann }, { spread, 0.8f },
            { filterType, fLP }, { cutoff, 2400 }, { resonance, 0.3f },
            { lfo1Target, LfoTarget::cutoff }, { lfo1Shape, sSine }, { lfo1Rate, 0.15f }, { lfo1Depth, 0.4f },
            { attack, 1.5f }, { release, 3.0f }, { reverbMix, 0.4f } } },

        { "Granular Stutter", "Rhythmic", "Bell Cloud", {
            { size, 110 }, { density, 8 }, { jitter, 0.0f }, { spray, 0.0f }, { window, percussive },
            { lfo1Target, LfoTarget::position }, { lfo1Shape, sRandom }, { lfo1Sync, 1 }, { lfo1Div, dSixteenth }, { lfo1Depth, 0.5f },
            { lfo2Target, LfoTarget::pitch }, { lfo2Shape, sSquare }, { lfo2Sync, 1 }, { lfo2Div, dQuarter }, { lfo2Depth, 1.0f },
            { attack, 0.002f }, { release, 0.3f }, { delayMix, 0.25f }, { delayTime, 250 } } },

        { "Tremolo Shards", "Rhythmic", "Glass Pad", {
            { size, 80 }, { density, 40 }, { spray, 0.2f }, { jitter, 0.5f }, { window, triangle },
            { spread, 0.9f }, { pitchRand, 0.2f },
            { lfo1Target, LfoTarget::volume }, { lfo1Shape, sSquare }, { lfo1Sync, 1 }, { lfo1Div, dEighth }, { lfo1Depth, 0.9f },
            { reverbMix, 0.3f } } },

        { "Buzz Swarm", "Textures", "Glass Pad", {
            { size, 12 }, { density, 120 }, { jitter, 0.0f }, { spray, 0.05f }, { window, triangle },
            { filterType, fBP }, { cutoff, 1200 }, { resonance, 0.5f },
            { lfo1Target, LfoTarget::cutoff }, { lfo1Shape, sTri }, { lfo1Rate, 0.3f }, { lfo1Depth, 0.5f },
            { lfo2Target, LfoTarget::density }, { lfo2Shape, sDrift }, { lfo2Rate, 0.2f }, { lfo2Depth, 0.3f } } },

        { "Dust Cloud", "Textures", "Wind Texture", {
            { size, 22 }, { density, 70 }, { jitter, 1.0f }, { spray, 0.6f }, { spread, 1.0f },
            { window, percussive }, { reverse, 0.5f },
            { filterType, fHP }, { cutoff, 700 }, { resonance, 0.2f },
            { reverbMix, 0.45f }, { reverbSize, 0.8f } } },

        { "Drifting Memory", "Textures", "Glass Pad", {
            { scan, 0.1f }, { size, 240 }, { density, 18 }, { spray, 0.1f }, { jitter, 0.6f },
            { pitchRand, 0.3f }, { window, gaussian }, { spread, 0.7f },
            { filterType, fLP }, { cutoff, 4200 },
            { lfo1Target, LfoTarget::position }, { lfo1Shape, sDrift }, { lfo1Rate, 0.05f }, { lfo1Depth, 0.4f },
            { lfo2Target, LfoTarget::spread }, { lfo2Shape, sSine }, { lfo2Rate, 0.12f }, { lfo2Depth, 0.4f },
            { delayMix, 0.25f }, { delayTime, 700 }, { delayFb, 0.55f }, { reverbMix, 0.4f } } },

        { "Swell Machine", "Textures", "Wind Texture", {
            { size, 800 }, { density, 6 }, { jitter, 0.5f }, { spray, 0.2f }, { window, swell },
            { reverse, 0.3f }, { spread, 0.8f }, { attack, 2.0f }, { release, 4.0f },
            { reverbMix, 0.35f } } },

        { "Sub Drone", "Bass", "Glass Pad", {
            { pitch, -12 }, { size, 900 }, { density, 10 }, { jitter, 0.7f }, { spray, 0.05f },
            { window, tukey }, { spread, 0.3f },
            { filterType, fLP }, { cutoff, 650 }, { resonance, 0.35f },
            { lfo1Target, LfoTarget::cutoff }, { lfo1Shape, sDrift }, { lfo1Rate, 0.1f }, { lfo1Depth, 0.3f },
            { attack, 0.4f }, { release, 2.5f }, { reverbMix, 0.15f } } },

        { "Glitch Scatter", "Experimental", "Bell Cloud", {
            { size, 40 }, { density, 25 }, { spray, 1.0f }, { jitter, 0.9f }, { reverse, 0.5f },
            { pitchRand, 24 }, { quantize, qSemis }, { window, percussive }, { spread, 1.0f },
            { delayMix, 0.35f }, { delayTime, 250 }, { delayFb, 0.6f }, { delayPing, 1 } } },

        { "Wind Chimes", "Melodic", "Bell Cloud", {
            { scan, 0.25f }, { size, 200 }, { pitch, 12 }, { spray, 0.3f }, { density, 12 },
            { jitter, 0.9f }, { pitchRand, 5 }, { quantize, qMajor }, { window, hann }, { spread, 1.0f },
            { reverbMix, 0.6f }, { reverbSize, 0.9f } } },
    };

    return presets;
}
