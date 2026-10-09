#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DaveLookAndFeel.h"

using APVTS = juce::AudioProcessorValueTreeState;

/** Rotary knob with a name above and an editable value below. Double-click resets. */
class Knob : public juce::Component
{
public:
    Knob (APVTS& state, const juce::String& paramID, const juce::String& name, juce::Colour accent);
    void resized() override;

    juce::Slider slider;

private:
    juce::Label label;
    std::unique_ptr<APVTS::SliderAttachment> attachment;
};

/** On/off switch with a name above it. */
class Toggle : public juce::Component
{
public:
    Toggle (APVTS& state, const juce::String& paramID, const juce::String& name, const juce::String& buttonText, juce::Colour accent);
    void resized() override;

    juce::TextButton button;

private:
    juce::Label label;
    std::unique_ptr<APVTS::ButtonAttachment> attachment;
};

/** Drop-down list with a name above it. */
class Choice : public juce::Component
{
public:
    Choice (APVTS& state, const juce::String& paramID, const juce::String& name, juce::Colour accent);
    void resized() override;

    juce::ComboBox box;

private:
    juce::Label label;
    std::unique_ptr<APVTS::ComboBoxAttachment> attachment;
};

/** Rounded section with a coloured title. Children are placed on a cols x rows grid. */
class Panel : public juce::Component
{
public:
    Panel (const juce::String& title, juce::Colour accent, int columns, int rows = 2);

    void paint (juce::Graphics&) override;
    void resized() override;

    /** Adds a child at a grid cell (the panel does not own it). */
    void place (juce::Component& c, int col, int row, int colSpan = 1, int rowSpan = 1);

    int getColumns() const { return columns; }
    juce::Colour getAccent() const { return accent; }

private:
    struct Cell { juce::Component* comp; int col, row, colSpan, rowSpan; };
    juce::String title;
    juce::Colour accent;
    int columns, rows;
    std::vector<Cell> cells;
};

