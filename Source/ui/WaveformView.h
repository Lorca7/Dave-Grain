#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../dsp/SampleData.h"

class DaveGrainProcessor;

/**
    Shows the loaded sample, the grain position/spray region, each voice's scan playhead and
    every live grain. Click/drag sets Position; the mouse wheel changes Spray.
*/
class WaveformView : public juce::Component, private juce::Timer
{
public:
    explicit WaveformView (DaveGrainProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMove (const juce::MouseEvent&) override;

    void setDropHighlight (bool shouldHighlight);
    void showMessage (const juce::String& text);

private:
    void timerCallback() override;
    void rebuildPeaks();
    juce::Rectangle<float> waveArea() const;
    void setPositionFromX (float x);

    DaveGrainProcessor& processor;
    SamplePtr sample;
    int seenVersion = -1;
    std::vector<std::pair<float, float>> peaks; // min/max per pixel column
    bool dropHighlight = false;
    bool dragging = false;
    juce::String message;
    int messageTicks = 0;
    float phase = 0.0f;
};
