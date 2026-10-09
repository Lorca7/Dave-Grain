#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "ui/DaveLookAndFeel.h"
#include "ui/Controls.h"
#include "ui/WaveformView.h"
#include "ui/PresetBar.h"

/** Everything inside the window, laid out at a fixed design size and scaled as a whole. */
class MainView : public juce::Component, private juce::Timer
{
public:
    static constexpr int designWidth = 1180, designHeight = 800;

    explicit MainView (DaveGrainProcessor&);
    ~MainView() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    WaveformView waveform;
    std::function<void()> onLoadSample;

private:
    void timerCallback() override;

    DaveGrainProcessor& processor;
    APVTS& state;

    PresetBar presetBar;
    juce::TextButton loadButton { "Load Sample" };

    Panel grainPanel, pitchPanel, windowPanel, ampPanel, filterPanel, lfo1Panel, lfo2Panel, fxPanel, outPanel;

    // Grain
    Knob position, spray, scan, size, sizeRand, density, jitter;
    Toggle freeze;
    // Pitch
    Knob pitch, fine, pitchRand, rootNote;
    Choice quantize;
    Toggle keyTrack;
    // Window
    Choice windowShape;
    Knob reverse, spread;
    // Amp
    Knob attack, decay, sustain, release, velocity;
    Toggle hold;
    // Filter
    Choice filterType;
    Knob cutoff, resonance;
    // LFOs
    Knob lfo1Rate, lfo1Depth, lfo2Rate, lfo2Depth;
    Choice lfo1Div, lfo1Shape, lfo1Target, lfo2Div, lfo2Shape, lfo2Target;
    Toggle lfo1Sync, lfo2Sync;
    // FX
    Knob delayTime, delayFb, delayMix, reverbSize, reverbDamp, reverbMix;
    Toggle delayPing;
    // Output
    Knob output;

    std::vector<std::unique_ptr<juce::Component>> decorations; // previews, meters, indicators

    juce::MidiKeyboardComponent keyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainView)
};

class DaveGrainEditor : public juce::AudioProcessorEditor, public juce::FileDragAndDropTarget
{
public:
    explicit DaveGrainEditor (DaveGrainProcessor&);
    ~DaveGrainEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray&, int, int) override;
    void fileDragExit (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray& files, int, int) override;

private:
    void loadFile (const juce::File& f);
    void showLoadMenu();

    DaveGrainProcessor& processor;
    DaveLookAndFeel lookAndFeel;
    MainView view;
    juce::TooltipWindow tooltips { this, 600 };
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DaveGrainEditor)
};
